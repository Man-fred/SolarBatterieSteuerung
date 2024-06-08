#include "MyClass.h"
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
// #include <TimeLib.h>

#include "ArduinoOTA.h"
#include <Update.h>

U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/mySCL, /* data=*/mySDA, /* reset=*/U8X8_PIN_NONE); // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED

AsyncWebServer server(80);

void freeHeap(byte nr){
  Serial.printf("Freies RAM (%i) = %s", nr, String(system_get_free_heap_size()));
}



char *display0 = (char *)malloc(17);
char *display1 = (char *)malloc(17);
char *display2 = (char *)malloc(17);
char *display3 = (char *)malloc(17);

void gRefreshDisplayTime()
{ // Uhrzeit Datum
  // u8x8.setFont(u8x8_font_chroma48medium8_r);
  // u8x8.setInverseFont(0);
  snprintf(display0, 16, "%02i:%02i:%02i %02i.%02i._   ", hour(), minute(), second(), day(), month());
  u8x8.drawString(0, 0, display0);
  // u8x8.drawString(0, 1, display1);
  // u8x8.drawString(0, 2, display2);
  // u8x8.drawString(0, 3, display3);
  u8x8.refreshDisplay(); // only required for SSD1606/7
}
void gRefreshDisplayIP()
{ // Uhrzeit Datum
  //  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 3, display3);
  u8x8.refreshDisplay(); // only required for SSD1606/7
}

MyClass::MyClass()
{
  // TasterSize = 0;
  version = 0;
  // AsyncWebSocket ws("/ws");
  // AsyncEventSource events("/events");
}
MyClass::MyClass(byte arr[], byte size)
{
  // Process the elements of the array
  for (int i = 0; i < size; ++i)
  {
    globals.Taster[i] = arr[i];
  }
  // TasterSize = size;
  MyClass();
}
byte MyClass::inStart()
{
  return inSetup;
}
void MyClass::clearStart()
{
  inSetup = 0;
}
boolean MyClass::inSec15()
{
  if (sec15)
  {
    sec15 = false;
    return true;
  }
  return false;
}
boolean MyClass::inSec60()
{
  if (sec60)
  {
    sec60 = false;
    return true;
  }
  return false;
}
boolean MyClass::moreDiff(unsigned long ts, unsigned long diff)
{
  return (ts + diff < timeNow);
}
void MyClass::timerSet(byte nr, timerStates state, unsigned long period, cb_timerset callBack)
{
  if (nr < TIMER_MAX)
  {
    timers[nr].last = now();
    timers[nr].interval = period;
    timers[nr].state = state;
    timers[nr].callBack = callBack;
  }
}

void MyClass::timer__Set(byte nr, timerStates state, unsigned long period, void (*callBack)(void))
{
  if (nr < TIMER_MAX)
  {
    timers[nr].last = now();
    timers[nr].interval = period;
    timers[nr].state = state;
    timers[nr].callBack = callBack;
  }
}
void MyClass::timerLoop()
{
  unsigned long jetzt = now();
  if (timeNow < jetzt)
  {
    timeNow = jetzt;
    for (byte nr = 0; nr < TIMER_MAX; nr++)
    {
      if ((timers[nr].state == timerEnabled || (timers[nr].state == timerIfOnline && networkState == networkOnLine)) && (timers[nr].last + timers[nr].interval <= jetzt))
      {
        timers[nr].last = jetzt;
        timers[nr].callBack();
        if (timers[nr].interval == 0)
          timers[nr].state = timerDisabled;
      }
    }
  }
}

void MyClass::ntpLoop()
{
  switch (ntpState)
  {
  case ntpBoot:
    NTPok = false;
    NTPTime = 0;
    GetNTPStart();
    break;
  case ntpConnecting:
    if (millis() - ntpTimeoutLoop > 500)
    {
      ntpTimeoutLoop = millis();
      Serial.print("n");
      // if (WiFi.waitForConnectResult() != WL_ble_connected)
      if (GetNTPLoop())
      {
        GetNTPEnd();
      }
      else if (millis() - ntpTimeout > 10000)
      {
        ntpState = ntpOffline;
      }
    }
    break;
  case ntpOnline:
    // Zeit Update alle 24Stunden
    if (millis() - ntpTimeout > 86400000ul)
    {
      GetNTPStart();
    }
    break;
  case ntpOffline:
    if (millis() - ntpTimeout > 3600000ul && !RTCok)
    { // Zeit Update jede Stunde wenn offline
      GetNTPStart();
    }
    break;
  }
}
boolean MyClass::summertime(int year, byte month, byte day, byte hour, byte tzHours)
{
  if (month < 3 || month > 10)
    return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10)
    return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if ((month == 3 && ((hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)))) || (month == 10 && ((hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))))
    return true;
  else
    return false;
}
void MyClass::GetNTPStart(void)
{
  ntpState = ntpConnecting;
  ntpTimeout = millis();
  ntpTimeoutLoop = networkTimeout;

  udp.begin(2390);
  strcpy(globals.timeserver, "192.168.178.1");
  // WiFi.hostByName(globals.timeserver, timeServerIP);
  // sendNTPpacket(timeServerIP);
  Serial.print("sending NTP packet to ");
  Serial.println(globals.timeserver);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request https://www.rfc-editor.org/rfc/rfc5905#page-16
  // see URL for details on the packets https://www.meinberg.de/german/info/ntp-packet.htm
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(globals.timeserver, 123); // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
boolean MyClass::GetNTPLoop(void)
{
  int cb = udp.parsePacket();
  return (cb >= NTP_PACKET_SIZE);
}
unsigned long MyClass::GetNTPEnd(void)
{
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE);
  // the timestamp starts at byte 40 of the received packet and is four bytes,
  //  or two words, long. First, esxtract the two words:
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  // local time from UTC
  unsigned long ntp_time = epoch; //////////////////////// + TIMEZONE * 3600;
  Serial.print("Unix local time = ");
  Serial.println(ntp_time);
  if (SUMMERTIME && summertime(year(ntp_time), month(ntp_time), day(ntp_time), hour(ntp_time), TIMEZONE))
  {
    ///////////////ntp_time += 3600;
    sommerzeit = true;
  }
  else
  {
    sommerzeit = false;
  }
  NTPok = true;
  NTPTime = ntp_time;
  if (abs((int)(NTPTime - now())) > 1)
  {
    // LogSchreibenNow("falsche Zeit");
    // Serial.println( Temp );
    setTime(NTPTime);
    // LogSchreiben("NTP: Zeit gesetzt");
    // Serial.println( Temp );
  }
  ntpState = ntpOnline;
  return ntp_time;
}

void MyClass::getT(boolean verbose)
{
  for (byte k = 0; k < 4; k++)
  {
    // durch Pullup ist Standard 1, gedrückt 0
    if (globals.Taster[k] < 0xff)
    {
      byte tTemp = dRead(globals.Taster[k]);
      // TasterTemp Standard 0, gedrückt 1
      if (tTemp != t[k]) // && millis() - tTime[k] > 250)
      {
        // Prellen abfangen
        if (verbose)
        {
          Serial.printf("Key: %i %i (%i %i %i %i)\n", k, t[k], t[0], t[1], t[2], t[3]);
        }
        tTime[k] = millis();
        t[k] = tTemp;
        if (tTemp)
        {
          tToggle[k] = !tToggle[k];
          // String logText = "Taster " + String(k + 1);
          // Relais_Schalten(k, val[k], logText);
        }
        if (verbose)
        {
          Serial.printf("tTg: %i %i (%i %i %i %i)\n", k, t[k], tToggle[0], tToggle[1], tToggle[2], tToggle[3]);
        }
      }
    }
  }
  if (inSetup > 1)
  {
    command[0] = t[0] && t[1];
    command[1] = t[0] && t[2];
    command[2] = t[0] && t[3];
  }
  else
  {
    command[0] = 0;
    command[1] = 0;
    command[2] = 0;
  }
}
void MyClass::clearT()
{
  t[0] = 0;
  t[1] = 0;
  t[2] = 0;
  t[3] = 0;
  command[0] = 0;
  command[1] = 0;
  command[2] = 0;
}
byte MyClass::getCommand(byte nr)
{
  return command[nr];
}

void MyClass::WlanLoop()
{
  switch (networkState)
  {
  case networkBoot:
    if (globals.ssid[0] != 0)
    {
      WlanStation();
      // WlanAP();
    }
    else
    {
      WlanAP();
    }
    break;
  case networkNotConfigured:
    break;
  case networkApMode:
    break;
  case networkConnecting:
    if (millis() - networkTimeoutLoop > 500)
    {
      networkTimeoutLoop = millis();
      Serial.print("O");
      // if (WiFi.waitForConnectResult() != WL_ble_connected)
      if (WiFi.localIP())
      {
        networkState = networkOnLine;
        Serial.print("WL_Status ");
        Serial.println(WiFi.status());
        WiFi.printDiag(Serial);
        Serial.print("IP Adresse: ");
        Serial.println(WiFi.localIP());
        snprintf(display3, 16, "%s  ", WiFi.localIP().toString().c_str());
        gRefreshDisplayIP();
      }
      else if (millis() - networkTimeout > 10000)
      {
        networkState = networkOffLine;
      }
    }
    break;
  case networkOnLine:
    ntpLoop();
    break;
  case networkOffLine:
    WlanAP();
    break;
  }
}
void MyClass::WlanStation()
{
  networkState = networkConnecting;
  networkTimeout = millis();
  networkTimeoutLoop = networkTimeout;
  Serial.print("Verbinde mit ");
  Serial.println(globals.ssid);
#ifndef ESP32
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.setOutputPower(2.5);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(globals.ssid, globals.passwort);
  // Serial.print("Status ");
  // Serial.println(WiFi.status());
}
void MyClass::WlanAP()
{
  networkState = networkApMode;
  IPAddress apIP(192, 168, 168, 30); // wenn in AP-Mode
  snprintf(display1, 17, "%s", "SSID WebSchalter");
  snprintf(display2, 17, "%s", "PASS tiramisu   ");
  snprintf(display3, 17, "%s", "192.168.168.30  ");
  gRefreshDisplayIP();
  Serial.println("Starte in Access Point modus");
  Serial.println("IP http://192.168.168.30");
  Serial.print("SSID: WebSchalter, Passwort: tiramisu");
  WiFi.mode(WIFI_AP); // access point modus
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("WebSchalter", "tiramisu"); // Name des Wi-Fi netzes
  // dnsServer.start(DNS_PORT, "*", apIP);
}

// Check if header is present and correct
byte MyClass::newCookie(AsyncWebServerRequest *request, AsyncResponseStream *response, int level)
{
  if (++UserNext >= COOKIE_MAX)
    UserNext = 0;
  UserCookie[UserNext] = random(1000000, 9999999);
  UserStatus[UserNext] = level;
  UserCurrent = UserNext;
  response->addHeader("Set-Cookie", "esp=" + String(UserCookie[UserNext]) + "; SameSite=Strict");
  Serial.println("Login " + String(UserNext) + String(UserStatus[UserNext]));
  return UserNext;
}
byte MyClass::is_auth(AsyncWebServerRequest *request, AsyncResponseStream *response)
{
  UserCurrent = 0;
  if (request->hasHeader("Cookie"))
  {
    // Serial.print("Cookie: ");
    String cookie = request->header("Cookie");
    // Serial.print(cookie);
    //  for (int i=0;i<14;i++){Serial.print(cookie.charAt(i));}
    int p = cookie.indexOf("esp=");
    if (p != -1)
    {
      // Serial.print(" test-");Serial.print(p);Serial.print(":");
      String cookieEsp = cookie.substring(p + 4, 7 + 4);
      // Serial.print(cookieEsp);
      int testuser = cookieEsp.toInt();
      // Serial.print(" int-");Serial.print(testuser);
      if (testuser > 0)
      {
        for (int k = 0; k < 10; k++)
        {
          // Serial.print(" ? ");
          // Serial.print(UserCookie[k]);
          if (UserCookie[k] == testuser)
          {
            UserCurrent = k;
            // Serial.print(" user ");
            // Serial.print(UserCurrent);
            // Serial.println(" successful");
            return k;
          }
        }
        Serial.println(" unbekannt");
      }
    }
  }
  // Serial.println("Authentification No Cookie");
  if (!globals.AdminPasswort[0])
  { // Wenn kein Benutzerpasswort angegeben wurde
    // Serial.println("Login nicht nötig");
    return newCookie(request, response, COOKIE_ADMINISTRATOR);
  }
  else if (request->authenticate(&globals.AdminName[0], &globals.AdminPasswort[0]))
  {
    return newCookie(request, response, COOKIE_ADMINISTRATOR);
  }
  else if (globals.UserPasswort[0] && request->authenticate(&globals.UserName[0], &globals.UserPasswort[0]))
  {
    // Serial.println("Login User");
    return newCookie(request, response, COOKIE_BENUTZER);
  }
  else
  {
    request->requestAuthentication();
    return 0;
  }
  freeHeap(3);
}
byte MyClass::is_admin(AsyncWebServerRequest *request, AsyncResponseStream *response)
{
  byte user = is_auth(request, response);
  if (UserStatus[user] == COOKIE_ADMINISTRATOR)
  {
    return (user);
  }
  else
  {
    return 0;
  }
}
byte MyClass::is_user(AsyncWebServerRequest *request, AsyncResponseStream *response)
{
  byte user = is_auth(request, response);
  return (user);
}

unsigned int hexToDec(String hexString)
{
  unsigned int decValue = 0;
  int nextInt;

  for (unsigned int i = 0; i < hexString.length(); i++)
  {

    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57)
      nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70)
      nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102)
      nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);

    decValue = (decValue * 16) + nextInt;
  }

  return decValue;
}

// Read and Write any data structure or variable to EEPROM
int MyClass::SchreibeEeprom(int pos, char *zeichen, int laenge)
{
  for (int i = 0; i < laenge; i++)
  {
    EEPROM.write(pos + i, *zeichen);
    zeichen++;
  }
  posEeprom = pos + laenge;
  return posEeprom;
}
int MyClass::LeseEeprom(int pos, char *zeichen, int laenge)
{
  for (int i = 0; i < laenge; i++)
  {
    *zeichen = EEPROM.read(pos + i);
    zeichen++;
  }
  posEeprom = pos + laenge;
  return posEeprom;
}
void MyClass::LeseEeprom(int *daten)
{
  char *my_s_bytes = reinterpret_cast<char *>(daten);
  int laenge = sizeof(daten);
  LeseEeprom(posEeprom, my_s_bytes, laenge); // Serial.print((int) *daten);
}
void MyClass::LeseEeprom(char *daten, int laenge)
{
  char a;
  do
  {
    a = EEPROM.read(posEeprom); // if(a < 0xf) Serial.print(" 0"); else Serial.print(" "); Serial.print(a,HEX);
    *daten = a;
    daten++;
    posEeprom++;
    laenge--;
  } while (laenge > 0);
  // Serial.print(z);Serial.println("-");
}

void MyClass::SchreibeEeprom(int daten)
{
  char *my_s_bytes = reinterpret_cast<char *>(&daten);
  int laenge = sizeof(daten);
  SchreibeEeprom(posEeprom, my_s_bytes, laenge);
}

// 0x55 zufällig gewählt, synchron gesetzt in SchreibeEepromCheck() und LeseEepromCheck()
void MyClass::SchreibeEepromCheck()
{
  EEPROM.write(posEeprom, 0x55);
  Serial.print("Schreibe EEPROM-Pos ");
  Serial.println();
  if (posEeprom + 4 > EEPROM_RESERVIERUNG)
  {
    Serial.println(F("Config überschreitet die EEPROM Größe"));
  }
}
bool MyClass::LeseEepromCheck()
{
  int check = 0;
  check = EEPROM.read(posEeprom);
  Serial.print("Lese EEPROM-Pos ");
  Serial.println(posEeprom);
  return (check == 0x55);
}

void MyClass::Einstellen()
{
  Serial.printf("Start Einstellen: %i %i %i\n", self_ap, self_station, start_ap);
  // if (!self_ap) {
  Serial.println("e: Einstellen");
  WlanAP();
  //}
  Serial.printf("Ende Einstellen: %i %i %i\n", self_ap, self_station, start_ap);
}

void MyClass::WlanChangeToStation()
{
  WlanStation();
}

void MyClass::configLoad()
{
  posEeprom = LeseEeprom(posEeprom, reinterpret_cast<char *>(&globals), sizeof(globals));
}
void MyClass::ConfigSave(AsyncWebServerRequest *request)
{ // Wird ausgeuehrt bei "http://<ip address>/setup.json"
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_HTML);
  response->setCode(302);
  if (byte user = is_admin(request, response))
  {
    request->arg("ssid").toCharArray(globals.ssid, request->arg("ssid").length() + 1);
    String temp1 = request->arg("passwort");
    if (temp1[2] != '*')
    {
      temp1.toCharArray(globals.passwort, temp1.length() + 1);
    }
    request->arg("AdminName").toCharArray(globals.AdminName, request->arg("AdminName").length() + 1);
    request->arg("AdminPasswort").toCharArray(globals.AdminPasswort, request->arg("AdminPasswort").length() + 1);
    request->arg("UserName").toCharArray(globals.UserName, request->arg("UserName").length() + 1);
    request->arg("UserPasswort").toCharArray(globals.UserPasswort, request->arg("UserPasswort").length() + 1);
    request->arg("UpdateServer").toCharArray(globals.UpdateServer, request->arg("UpdateServer").length() + 1);
    request->arg("timeserver").toCharArray(globals.timeserver, request->arg("timeserver").length() + 1);
    request->arg("name_dev").toCharArray(globals.name_dev, request->arg("name_dev").length() + 1);
    for (int s = 0; s < SPECIAL_MAX; s++)
    {
      request->arg("name_r" + String(s)).toCharArray(globals.special[s].name, SPECIAL_NAME);
      request->arg("address" + String(s)).toCharArray(globals.special[s].address, SPECIAL_NAME);
      request->arg("devuser" + String(s)).toCharArray(globals.special[s].devuser, SPECIAL_NAME);
      request->arg("devpass" + String(s)).toCharArray(globals.special[s].devpass, SPECIAL_NAME);
      for (int k = 0; k < SPECIAL_VAL_MAX; k++)
      {
        globals.special[s].val[k] = request->arg("setup" + String(s) + "_" + String(k)).toInt();
      }
    }
    EEPROM.begin(EEPROM_RESERVIERUNG);
    globals.version0 = VERSION;
    globals.version2 = 2;
    globals.version3 = VERSION;
    posEepromGlobals = SchreibeEeprom(0, reinterpret_cast<char *>(&globals), sizeof(globals));
    EEPROM.commit();
    EEPROM.end();
  }

  // response = request->beginResponse_P(303, MIMETYPE_HTML, "");
  response->addHeader("Server", "ESP Async Web Server");
  response->addHeader("Location", "/index.htm#page3");
  request->send(response); // Antwort an Internet Browser
}

String MyClass::getContentType(AsyncWebServerRequest *request, String filename)
{
  if (request->hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".json"))
    return "application/json";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".txt"))
    return "text/plain";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".manifest"))
    return "text/cache-manifest";
  return "text/plain";
}
bool MyClass::handleFileRead(AsyncWebServerRequest *request)
{
  String path = request->url();
  if (path.endsWith("/"))
    path += "index.htm";
  String contentType = getContentType(request, path);
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
  {
    if (LittleFS.exists(pathWithGz))
      path += ".gz";
    request->send(LittleFS, path);
    return true;
  }
  return false;
}

void MyClass::saveFile(String filename, PGM_P content)
{
  Serial.print(filename);
  if (LittleFS.exists(filename))
  {
    Serial.println(" - vorhanden");
  }
  else
  {
    File file = LittleFS.open(filename, "w");
    if (file)
    {
      if (file.print(FPSTR(content)))
      {
        Serial.println(" - file written");
      }
      else
      {
        Serial.println(" - write failed");
      }
      file.close();
    }
    else
    {
      Serial.println(" - open failed");
    }
  }
}
void MyClass::handleFileUpload(AsyncWebServerRequest *request)
{
  /*
  if (is_admin(request)) {
    HTTPUpload &upload = request->upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if (!filename.startsWith("/")) filename = "/" + filename;
      DBG_OUTPUT_PORT.print("handleFileUpload Start: ");
      DBG_OUTPUT_PORT.println(filename);
      fsUploadFile = LittleFS.open(filename, "w");
      filename = String();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (fsUploadFile) {
        fsUploadFile.write(upload.buf, upload.currentSize);
        DBG_OUTPUT_PORT.print("handleFileUpload Data : ");
        DBG_OUTPUT_PORT.println(upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (fsUploadFile) {
        fsUploadFile.close();
        DBG_OUTPUT_PORT.print("handleFileUpload End  : ");
        DBG_OUTPUT_PORT.println(upload.totalSize);
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      DBG_OUTPUT_PORT.println("handleFileUpload Aborted");
    }
  }
  */
}
void MyClass::handleFileDelete(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_JSON);
  if (is_admin(request, response))
  {
    if (request->args() == 0)
    {
      response->print("BAD ARGS");
    }
    else
    {
      String path = request->arg((size_t)0);
      DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
      if (path == "/")
        response->print("BAD PATH");
      else if (!LittleFS.exists(path))
        response->print("FileNotFound");
      else
      {
        LittleFS.remove(path);
        response->print(path);
      }
    }
  }
  request->send(response);
}
void MyClass::handleFileList(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_JSON);
  if (is_admin(request, response))
  {
    if (!request->hasArg("dir"))
    {
      request->send(500, "text/plain", "BAD ARGS");
      return;
    }

    String path = request->arg("dir");
    DBG_OUTPUT_PORT.println("handleFileList: " + path);
#ifdef ESP32
    File root = LittleFS.open(path);
    path = String();

    String output = "[";
    if (!root)
    {
      Serial.println("- failed to open directory");
      return;
    }
    if (!root.isDirectory())
    {
      Serial.println(" - not a directory");
      return;
    }

    File entry = root.openNextFile();
    while (entry)
    {
      if (output != "[")
        output += ',';
      bool isDir = entry.isDirectory();
      output += "{\"type\":\"";
      output += (isDir) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(entry.name());
      output += "\",\"size\":\"";
      output += (isDir) ? "" : String(entry.size());
      output += "\"}";
      entry = root.openNextFile();
    }
#else
    Dir dir = LittleFS.openDir(path);
    path = String();

    String output = "[";
    while (dir.next())
    {
      File entry = dir.openFile("r");
      if (output != "[")
        output += ',';
      bool isDir = false;
      output += "{\"type\":\"";
      output += (isDir) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(entry.name()); //.substring(1);
      output += "\",\"size\":\"";
      output += (isDir) ? "" : String(entry.size());
      output += "\"}";
      entry.close();
    }
#endif
    output += "]";
    response->print(output);
    request->send(response);
  }
}

void MyClass::writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w"); // FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
  file.close();
}

boolean MyClass::httpPossible()
{
  return (httpMissed && (networkState == networkOnLine || networkState == networkApMode));
}
void MyClass::httpStart()
{
  const char *serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";

  // Behandlung der Ereignisse anschliessen
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_TEXT);
    /*if (byte nr = is_user(request, response))
    {
      response->print(stateRaw(nr));
    }*/
    response->addHeader("Server", "ESP Async Web Server");
    response->addHeader("Cache-Control", "no-cache");
    response->print("Booting ...");
    request->send(response); // Antwort an Internet Browser
    ESP.restart();
  });

  // Seiten aus dem LittleFS-Speicher werden bei onNotFoud() ausgewertet
  server.on("/list", HTTP_GET, [=](AsyncWebServerRequest *request)
            { handleFileList(request); });

  server.on("/delete", HTTP_GET, [=](AsyncWebServerRequest *request)
            { handleFileDelete(request); });
  server.on("/setup.json", [=](AsyncWebServerRequest *request)
            { ConfigSave(request); });
  server.on("/minimal", HTTP_GET, [=](AsyncWebServerRequest *request)
            {
    if (true) {
      AsyncWebServerResponse *response = request->beginResponse_P(200, MIMETYPE_HTML, serverIndex);//, upload_htm_len);
      response->addHeader("Server", "ESP Async Web Server");
      response->addHeader("Connection", "keep-alive");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    } });
  server.on("/upload", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
            {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization, FileSize");
        response->addHeader("Access-Control-Allow-Credentials", "true");
        request->send(response); });
  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        /*if (request->authenticate(HTTP_USERNAME, HTTP_PASSWORD))
            request->send(200);
        else {
            request->send(401);
            request->client()->close();
        }*/
        request->send(200); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        /*if (!request->authenticate(HTTP_USERNAME, HTTP_PASSWORD)) {
            request->send(401);
            request->client()->close();
            return;
        }*/

        // https://javascript.info/formdata

        static unsigned long startTimer;
        if (!index)
        {
          startTimer = millis();
          const char *FILESIZE_HEADER{"FileSize"};
          const char *ContentLength_HEADER{"Content-Length"};
          int filesize = 0;
          Serial.printf("UPLOAD: Receiving: '%s'\n", filename.c_str());

          if (request->hasHeader(FILESIZE_HEADER))
          {
            filesize = request->header(FILESIZE_HEADER).toInt();
          }
          else if (request->hasHeader(ContentLength_HEADER))
          {
            filesize = request->header(ContentLength_HEADER).toInt();
          }
          if (filesize == 0)
          {
            request->send(400, MIMETYPE_HTML, "No filesize header present!");
            request->client()->close();
            Serial.printf("UPLOAD: Aborted upload because missing filesize header.\n");
            return;
          }

          Serial.printf("UPLOAD: fileSize: %s\n", request->header(FILESIZE_HEADER));

          if (filesize >= MAX_FILESIZE)
          {
            request->send(400, MIMETYPE_HTML,
                          "Too large. (" + request->header(FILESIZE_HEADER) +
                              ") Max size is " + (String)MAX_FILESIZE + ".");

            request->client()->close();
            Serial.printf("UPLOAD: Aborted upload because filesize limit.\n");
            return;
          }
          request->_tempFile = LittleFS.open("/" + filename, "w");
        }

        // Store or do something with the data...
        if (len)
        {
          request->_tempFile.write(data, len);
          Serial.printf("file: '%s' received %i bytes\ttotal: %i\n", filename.c_str(), len, index + len);
        }

        if (final)
        {
          request->_tempFile.close();
          Serial.printf("UPLOAD: Done. Received %i bytes in %.2fs which is %.2f kB/s.\n",
                        index + len,
                        (millis() - startTimer) / 1000.0,
                        1.0 * (index + len) / (millis() - startTimer));
        }
      });
  // called when the url is not defined here
  // use it to load content from LittleFS
  server.onNotFound([=](AsyncWebServerRequest *request)
                    {
    AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_JSON);
      if (is_user(request, response)) {
        if (!handleFileRead(request)) {
          String notfound = "File Not Found\r\n\r\n";
          notfound += request->url();
          notfound += (request->method() == HTTP_GET) ? " GET " : " POST ";
          notfound += request->args();

          for (byte i = 0; i < request->args(); i++) {
            notfound += " " + request->argName(i) + ": " + request->arg(i) + "\r\n";
          }
          request->send(404, "text/plain", notfound);
        }
      } });
  server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
                      {
    Serial.printf("onFileUpload called, index: %d  len: %d  final: %d\n", index, len, final);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(0 == index) {
      Serial.printf("onFileUpload start: %s\n", filename.c_str());
      if (filename.endsWith(".bin"))
      {
        if(!Update.begin(maxSketchSpace)){ Serial.println("Update begin failure!"); }
#ifndef ESP32
          Update.runAsync(true);
#endif
      }
      else
      {
        request->_tempFile = LittleFS.open("/" + filename, "w");
      }
    }
    if (len) {
      // stream the incoming chunk to the opened file
      if (filename.endsWith(".bin"))
      {
        if(Update.write(data, len) != len){
            Update.printError(Serial);
        } else { Serial.printf("Write: %d bytes\n", len); }
      }
      else {
        request->_tempFile.write(data, len);
      }
    }

    if(final) {
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
      if (filename.endsWith(".bin"))
      {
        if (Update.end(true)) {
          Serial.println("Update succesful!");
        } else {
          Update.printError(Serial);   
        }
      }
      else
      {
        request->_tempFile.close();
      }
      request->send(200, MIMETYPE_JSON,"{\"result\":\"ok\"}");
    } });

  yield;
  server.begin();
  httpMissed = false;
  Serial.println("HTTP Server gestartet");
  yield;

  /*

  server.on("/schalte.php", Ereignis_Schalte);      // Aktion bei index.html
  server.on("/zustand.php", Ereignis_Zustand);      // Ajax-Antwort bei index.html

  server.on("/timer.json", Ereignis_Timer_JSON);    // json-Daten bei timer.html
  server.on("/settimer.php", Ereignis_NeueTimer);   // Formulardaten für neu/anpassen bei timer.html
  server.on("/delete.php", Ereignis_DeleteTimer);   // Formulardaten für löschen bei timer.html
  server.on("/laden.html", Ereignis_Timer_Laden);   // Timer neu aus Speicher laden und sortieren

  server.on("/deletelog.php", Ereignis_DeleteLog);  // löscht ohne Rückfrage die Datei log.txt

  server.on("/update.php", updateVersion);          // Update OTA

  //list directory


  server.on("/upload.json", HTTP_POST, []() {
    if (is_authentified()) {
      server.send(200, "text/json", "{\"ok\":1}");
    }
  }, handleFileUpload);
  */
  // server.collectHeaders(headerKeys, headerKeysCount);
  // server.begin();  // Starte den Server
}
void MyClass::configJson(AsyncWebServerRequest *request)
{                   // Wird ausgeuehrt wenn "http://<ip address>/" aufgerufen wurde
  String temp = ""; // Sternchen einfügen bei Passwortanzeige
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_JSON);
  if (is_admin(request, response))
  {
    temp += "{\"ssid1\":\"" + String(globals.ssid) + "\",";
    temp += "\"pass1\":\"" + String(globals.passwort) + "\",";
    temp += "\"name2\":\"" + String(globals.AdminName) + "\",";
    temp += "\"pass2\":\"" + String(globals.AdminPasswort) + "\",";
    temp += "\"name3\":\"" + String(globals.UserName) + "\",";
    temp += "\"pass3\":\"" + String(globals.UserPasswort) + "\",";
    temp += "\"timeserver\":\"" + String(globals.timeserver) + "\",";
    temp += "\"update\":\"" + String(globals.UpdateServer) + "\",";
    temp += "\"name_dev\":\"" + String(globals.name_dev) + "\",\"special\":[";
    for (int s = 0; s < SPECIAL_MAX; s++)
    {
      if (s > 0)
        temp += ",";
      temp += "{\"name\":\"" + String(globals.special[s].name) +
              "\",\"address\":\"" + String(globals.special[s].address) +
              "\",\"devuser\":\"" + String(globals.special[s].devuser) +
              "\",\"devpass\":\"" + String(globals.special[s].devpass) + "\"";
      for (int k = 0; k < SPECIAL_VAL_MAX; k++)
      {
        temp += ",\"setup" + String(k) + "\":" + String(globals.special[s].val[k]);
      }
      temp += "}";
    }
    temp += "]}";
  }
  else
  {
    temp = "{\"userstate\":\"not logged in\"}";
  }
  response->print(temp);
  request->send(response);
}
String MyClass::stateRaw(byte nr)
{
  String Antwort = (String)now() + ";"; // PrintDate(now()) + " " + PrintTime(now()) + ";";
  // Version + ChipId
  Antwort += mVersionNr;
  Antwort += mVersionVariante;
  Antwort += mVersionBoard;
  Antwort += ";";
  Antwort += String(chipId);
  Antwort += ";";
  Antwort += (UserStatus[nr] == COOKIE_ADMINISTRATOR ? "Administrator" : "Eingeschränkt");
  Antwort += ";";
  Antwort += String(NTPok);
  Antwort += ";";
  Antwort += String(RTCok);
  Antwort += ";";
  Antwort += String(IOok);
  Antwort += ";";
  Antwort += String(DISPLAYok);
  Antwort += ";";
  Antwort += String(globals.name_dev);
  /*
  Antwort += ";" + String(SPECIAL_MAX) + ";";
  for (int s = 0; s < SPECIAL_MAX; s++)
  {
    Antwort += String(globals.special[s].name) + ";";
    for (int k = 0; k < SPECIAL_VAL_MAX; k++)
    {
      Antwort += String(globals.special[s].val[k]) + ";";
    }
  }
  */
  return Antwort;
}
void MyClass::stateTxt(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_TEXT);
  if (byte nr = is_user(request, response))
  {
    response->print(stateRaw(nr));
  }
  response->addHeader("Server", "ESP Async Web Server");
  response->addHeader("Cache-Control", "no-cache");
  request->send(response); // Antwort an Internet Browser
}

void MyClass::sendProgmem(AsyncWebServerRequest *request, PGM_P content, String type)
{

  request->send(200, getContentType(request, type), content); // Antwort an Internet Browser senden
}
void MyClass::sendString(AsyncWebServerRequest *request, String result, String type)
{
  AsyncWebServerResponse *response = request->beginResponse(200, type, result);
  response->addHeader("Server", "ESP Async Web Server");
  response->addHeader("Cache-Control", "no-cache");
  request->send(response); // Antwort an Internet Browser
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if (info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    }
    else
    {
      // message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void MyClass::refreshDisplayTime()
{ // Uhrzeit Datum
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setInverseFont(0);
  snprintf(display0, 17, "%02i:%02i:%02i %02i.%02i.", hour(), minute(), second(), day(), month());
  u8x8.drawString(0, 0, display0);
  u8x8.refreshDisplay(); // only required for SSD1606/7
}
void MyClass::refreshDisplaySolar()
{
  u8x8.drawString(0, 1, display1);
  u8x8.drawString(0, 2, display2);
  u8x8.refreshDisplay(); // only required for SSD1606/7
}
void MyClass::refreshDisplayIP()
{
  u8x8.setInverseFont(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 3, display3);
  u8x8.refreshDisplay(); // only required for SSD1606/7
}

void MyClass::setup()
{
  Serial.begin(115200);
  Serial.println("Serial begin");

  for (int i = 0; i < 4; i++)
  {
    if (globals.Taster[i] < 0xff)
      dPinModeInputPullup(globals.Taster[i]);
  }
  Serial.printf("dPinModeInputPullup %i %i %i %i", globals.Taster[0], globals.Taster[1], globals.Taster[2], globals.Taster[3]);
  if (!LittleFS.begin(true, ""))
  {
    Serial.println("LittleFS Mount Failed");
  }
  Serial.println("LittleFS");
#ifdef ESP32

  snprintf(chipId, 13, "%llX", ESP.getEfuseMac());
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);

#else
  snprintf(chipId, 13, "%u", esp.getChipId());
#endif

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setPowerSave(0);
  timerSet(timerDisplay, timerEnabled, 1, gRefreshDisplayTime);
  // configLoad();
}

void MyClass::setupEnd()
{
  Serial.println("SetupEnd()");
// ENDE Stationmodus / Access Point modus Auswahl
#ifdef DISPLAY_I2C_ADDRESS
  if (DISPLAYok)
  {
    oledStatus();
  }
#endif
  // printUser();
  freeHeap(1);
  clearStart();
  WlanLoop();

 freeHeap(2);

  /*
  #ifdef RTC_I2C_ADDRESS
    if (RTCok) {
      RTC.begin();
      RTCTime = RTC.now().unixtime();
      setTime(RTCTime);
      Serial.println(PrintDate(now()) + "   " + PrintTime (now()) + "   RTC gestartet");
    } else {
      Serial.println("Unable to sync with the RTC");
    }
  #endif
  */

  /*
  if (!AP) {
    z = 0;
    //LeseEeprom(ssid, sizeof(ssid));        // EEPROM lesen
    if (ssid[0] == 255 || ssid[0] == '\0') // Wenn EEPROM leer oder SSID nicht gesetzt ist dann:
    {
      //for (i = 0; i < 6; i++) EEPROM.write(i, '\0');
      //EEPROM.commit();
      Einstellen();
    }
    else {
      // Wenn ssid angegeben dann in Stationmodus mit Router verbinden
      getEeprom();
      Zeit_Einstellen();

      Temp = PrintDate(now()) + "   " + PrintTime (now()) + "   Server gestartet";
      LogSchreiben(Temp);

      // gespeicherte Timer laden und Reihenfolge setzen, hier mit Argument neustart=true
      Timer_Laden(true);
    }
  } else {
    Zeit_Einstellen();
  }
  */
}

void MyClass::loop()
{
  WlanLoop();
  timerLoop();

  /*
  if ( (NTPTime + 86400) < jetzt || (NTPTime == 0 && !RTCok)) { //Zeit Update alle 24Stunden oder wenn gar keine Uhrzeit vorhanden ist
    WlanStation();
    Zeit_Einstellen();
    jetzt = now();
  }
  if (ZeitTempMin + 60 < jetzt) { // Ausführung 1 mal je Minute
    ZeitTempMin = jetzt;
  }
  if (ZeitTempStd + 3600 < jetzt) { // Ausführung 1 mal je Stunde
    if (sommerzeitTest()) {
      Zeit_Einstellen();
      jetzt = now();
    }
    ZeitTempStd = jetzt;
  }
  if (ZeitTempTag + 86400 < jetzt) { // Ausführung 1 mal je Tag
    ZeitTempTag = jetzt;
  }
  ZeitTemp = jetzt;
  */
  //  in Setup
  /*
  if (self_ap) {
    //  AP                 schnelles Blinken 1 / 1 sek
    statusLedBlink(2, 2);
  } else if (!WLANok) {
    //  !WLANok         länger an als aus 5 / 1 sek
    statusLedBlink(5, 2);
  } else if ((NTPTime + 86400) < jetzt) {
    //  NTP ungültig       länger aus als an 1 / 5 sek
    statusLedBlink(2, 5);
  } else {
    //  normale Funktion "Station":  !AP, WLANok, !inSetup, NTPTime -> ruhig Blinken 5 / 5 sek
    statusLedBlink(5, 5);
  }
  */
}
