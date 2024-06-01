#include "MyClass.h"

#ifdef ESP32
  #include <WiFi.h>
  #define EspTaskWdtInit 
  #define EspTaskWdtReset 
  #define system_get_free_heap_size esp_get_free_heap_size
  #include <HTTPUpdate.h>
  #include <WebServer.h>
  #define ESPhttpUpdate httpUpdate
  #define dRead(pin) digitalRead(pin)
  #define dPinModeInputPullup(pin) pinMode(pin, INPUT_PULLUP); // turn on a 100K pullup internally
#else
  #include <ESP8266WiFi.h>
  #define EspTaskWdtInit 
  #define EspTaskWdtReset ESP.wdtEnable(0);
  #include <ESP8266httpUpdate.h>
  #include <ESP8266WebServer.h>
  #define WebServer ESP8266WebServer
  #include <ESP8266HTTPClient.h>
  // #include <ESP8266HTTPUpdateServer>
  #define dRead(pin) digitalRead(pin)
  #define dPinModeInputPullup(pin) pinMode(pin, INPUT_PULLUP); // turn on a 100K pullup internally
#endif
#include "StreamString.h"

#include <EEPROM.h>
#include <FS.h>
#define FORMAT_LITTLEFS_IF_FAILED true
#define SPIFFS LittleFS
#include <LittleFS.h>
#define DBG_OUTPUT_PORT Serial
#include <TimeLib.h>             //<Time.h> http://www.arduino.cc/playground/Code/Time
#include <WiFiUdp.h>
#define TIMEZONE 1
#define SUMMERTIME 1

#include <ArduinoJson.h>
JsonDocument doc;

extern String mVersionNr;
extern String mVersionVariante;

#ifndef ESP32
  FSInfo fs_info;
#endif

const char UpdateSuccessResponse[] PROGMEM = R"=====(
  <META http-equiv='refresh' content='15;URL=/'>Update Success! Rebooting...
)=====";

extern specialStruct special[];


  WebServer server;

class MyClass {
  #define LOGINLAENGE 32
  int z;
  int version;
  byte t[4] = { 0, 0, 0, 0 };
  byte tToggle[4] = { 0, 0, 0, 0 };
  unsigned int tTime[4] = { 0, 0, 0, 0 };
  byte command[3] = { 0, 0, 0 };

  unsigned long timeNow;
  unsigned long timeLast = 0, time15Last = 0, time60Last = 0;
  byte inSetup = 3;  //Rückwärtszaehler 3 - 2 - 1 (setup2) - 0 (setup fertig)
  boolean sec15 = 0;
  boolean sec60 = 0;
  int timeout = 0; //WLan-Connect;
  //unsigned long timeLast = 0, time15Last = 0, time60Last = 0;
  byte timeStart = 0;
  boolean start_ap = 0;
  boolean self_ap = 0; // Accesspoint Modus aus
  boolean self_station = 0;

  #define COOKIE_MAX 10
  #define COOKIE_ADMINISTRATOR 1
  #define COOKIE_BENUTZER 2
  int UserCookie[COOKIE_MAX];  // = [0,0,0,0,0,0,0,0,0,0];
  int UserNext = 0;
  int UserCurrent = -1;
  //int httpCode = 0;

  byte authenticated = 0;
  const char *headerKeys[6] = { "User-Agent", "Set-Cookie", "Cookie", "Date", "Content-Type", "Connection" };
  size_t headerKeysCount = 6;
  File fsUploadFile;
  EspClass esp;
  String _updaterError;

  IPAddress timeServerIP;
  #define NTP_PACKET_SIZE 48
  byte packetBuffer[ NTP_PACKET_SIZE];
  WiFiUDP udp;
  boolean sommerzeit = false;
  unsigned long NTPTime = 0;

  byte NTPok = 0;
  byte IOok = 0;
  byte DISPLAYok = 0;

public:
  byte RTCok = 0;

  // Diese Daten müssen angepasst werden
  char ssid[32] = "BCSS-R\0";
  char passwort[64] = "0742340475201820\0";
  char AdminName[LOGINLAENGE] = "admin\0";
  char AdminPasswort[LOGINLAENGE] = "\0";
  char UserName[LOGINLAENGE] = "user\0";
  char UserPasswort[LOGINLAENGE] = "\0";
  int UserStatus[COOKIE_MAX];  // = [0,0,0,0,0,0,0,0,0,0];
  char UpdateServer[LOGINLAENGE];// = "192.168.178.60\0";
  char timeserver[LOGINLAENGE];//;// = "time.nist.gov\0";
  char name_dev[LOGINLAENGE]; //= "Nulleinspeisung";
  
  byte Taster[4];
  byte TasterSize = 4;
  #ifdef ARDUINO_ESP8266_NODEMCU
    const byte board = 1;
    String mVersionBoard = "nodemcu";
  #elif ARDUINO_ESP8266_WEMOS_D1MINI
    const byte board = 2;
    String mVersionBoard = "d1_mini";
  #elif ESP32
    const byte board = 3;
    String mVersionBoard = "esp32";
  #else
    const byte board = 4;
    String mVersionBoard = "unknown";
  #endif
  char chipId[13];


  MyClass() {
    TasterSize = 0;
    version = 0;
  }
  MyClass(byte arr[], byte size) { 
    // Process the elements of the array 
    for (int i = 0; i < size; ++i) { 
      Taster[i] = arr[i]; 
    } 
    TasterSize = size; 
    version = 0;
  } 



  void getT() {
    for (byte k = 0; k < 4; k++) {
      // durch Pullup ist Standard 1, gedrückt 0
      byte tTemp = !dRead(Taster[k]);
      // TasterTemp Standard 0, gedrückt 1
      if (tTemp != t[k] && millis() - tTime[k] > 250) {
        // Prellen abfangen
        //Serial.print(k);
        tTime[k] = millis();
        t[k] = tTemp;
        if (tTemp) {
          tToggle[k] = !tToggle[k];
          //String logText = "Taster " + String(k + 1);
          //Relais_Schalten(k, val[k], logText);
        }
      }
    }
    if (inSetup > 1) {
      command[0] = t[0] && t[1];
      command[1] = t[0] && t[2];
      command[2] = t[0] && t[3];
    } else {
      command[0] = 0;
      command[1] = 0;
      command[2] = 0;
    }
  }
  void clearT() {
    t[0] = 0;
    t[1] = 0;
    t[2] = 0;
    t[3] = 0;
    command[0] = 0;
    command[1] = 0;
    command[2] = 0;
  }
  byte getCommand(byte nr) {
    return command[nr];
  }
  
  void WlanStation() {
    start_ap = true;
    self_ap = false;
    if (start_ap){
      WlanAP();
    } else if (self_ap) {
      self_station = false;
    } else {
      if (!self_station) {
        Serial.print("Verbinde mit ");
        Serial.println(ssid);
        #ifndef ESP32
              WiFi.setPhyMode(WIFI_PHY_MODE_11G);
              WiFi.setOutputPower(2.5);
        #endif
        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);
        WiFi.begin(ssid, passwort);
        timeout = 0;
        Serial.print("Status ");
        Serial.println(WiFi.status());
        //while (WiFi.status() != WL_CONNECTED && !WiFi.localIP())
        while (!WiFi.localIP()) {
          delay(500);
          Serial.print("O");
          if (timeout++ > 10)  // Wenn Anmeldung nicht möglich
          {
            Serial.println("");
            Serial.println("Wlan verbindung fehlt");
            break;
          }
        }
        Serial.print("WL_Status ");
        Serial.println(WiFi.status());
        WiFi.printDiag(Serial);
      }
      //if (WiFi.status() == WL_CONNECTED)
      if (WiFi.localIP()) {
        self_station = true;
        Serial.println("");
        Serial.println("Mit Wlan verbunden");
        Serial.print("IP Adresse: ");
        Serial.println(WiFi.localIP());
        GetNTP();
      } else {
        self_station = false;
        NTPTime = 0;
        NTPok = false;
      }
    }
  }
  void WlanAP() {
    if (!self_ap) {
      IPAddress apIP(192, 168, 168, 30); // wenn in AP-Mode
      Serial.println("Starte in Access Point modus");
      Serial.println("IP http://192.168.168.30");
      Serial.print("SSID: WebSchalter, Passwort: tiramisu");
      WiFi.mode(WIFI_AP);  // access point modus
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP("WebSchalter", "tiramisu");  // Name des Wi-Fi netzes
      //dnsServer.start(DNS_PORT, "*", apIP);
      self_ap = true;
      self_station = false;
    }
  }

  void Einstellen() {
    Serial.printf("Start Einstellen: %i %i %i\n", self_ap, self_station, start_ap);
    //if (!self_ap) {
      Serial.println("e: Einstellen");
      WlanAP();
    //}
    Serial.printf("Ende Einstellen: %i %i %i\n", self_ap, self_station, start_ap);
  }
  String MyClass::getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".txt")) return "text/plain";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    else if (filename.endsWith(".manifest")) return "text/cache-manifest";
    return "text/plain";
  }

  bool handleFileRead(String path) {
    if (path.endsWith("/")) path += "index.htm";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
      if (SPIFFS.exists(pathWithGz))
        path += ".gz";
      File file = SPIFFS.open(path, "r");
      server.streamFile(file, contentType);  // size_t sent =
      file.close();
      return true;
    }
    return false;
  }

  void saveFile(String filename, PGM_P content){
    Serial.print(filename);
    if (SPIFFS.exists(filename)){
      Serial.println(" - vorhanden");
    } else {
      File file = SPIFFS.open(filename, "w");
      if(file){
        if(file.print( FPSTR(content) )){
            Serial.println(" - file written");
        } else {
            Serial.println(" - write failed");
        }
        file.close();
      } else {
        Serial.println(" - open failed");
      }
    }
  }
  void handleFileUpload() {
    if (is_admin()) {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        DBG_OUTPUT_PORT.print("handleFileUpload Start: ");
        DBG_OUTPUT_PORT.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");
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
  }
  void handleFileDelete() {
    if (authenticated == 2) {
      if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
      String path = server.arg(0);
      DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
      if (path == "/")
        return server.send(500, "text/plain", "BAD PATH");
      if (!SPIFFS.exists(path))
        return server.send(404, "text/plain", "FileNotFound");
      SPIFFS.remove(path);
      server.send(200, "text/plain", "");
      path = String();
    }
  }
  void handleFileList() {
    if (is_admin()) {
      if (!server.hasArg("dir")) {
        server.send(500, "text/plain", "BAD ARGS");
        return;
      }

      String path = server.arg("dir");
      DBG_OUTPUT_PORT.println("handleFileList: " + path);
      #ifdef ESP32
        File root = SPIFFS.open(path);
        path = String();

        String output = "[";
        if (!root) {
          Serial.println("- failed to open directory");
          return;
        }
        if (!root.isDirectory()) {
          Serial.println(" - not a directory");
          return;
        }

        File entry = root.openNextFile();
        while (entry) {
          if (output != "[") output += ',';
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
        Dir dir = SPIFFS.openDir(path);
        path = String();

        String output = "[";
        while (dir.next()) {
          File entry = dir.openFile("r");
          if (output != "[") output += ',';
          bool isDir = false;
          output += "{\"type\":\"";
          output += (isDir) ? "dir" : "file";
          output += "\",\"name\":\"";
          output += String(entry.name());  //.substring(1);
          output += "\",\"size\":\"";
          output += (isDir) ? "" : String(entry.size());
          output += "\"}";
          entry.close();
        }
      #endif
      output += "]";
      sendString(output, ".json");
    }
  }
  void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, "w");  //FILE_WRITE);
    if (!file) {
      Serial.println("- failed to open file for writing");
      return;
    }
    if (file.print(message)) {
      Serial.println("- file written");
    } else {
      Serial.println("- write failed");
    }
    file.close();
  }

  void resetPassword(){
    AdminPasswort[0] = 0;
    UserPasswort[0] = 0;
  }
  //Check if header is present and correct
  byte newCookie(int level) {
    if (++UserNext >= COOKIE_MAX) UserNext = 0;
    UserCookie[UserNext] = random(1000000, 9999999);
    UserStatus[UserNext] = level;
    server.sendHeader("Set-Cookie", "esp=" + String(UserCookie[UserNext])+"; SameSite=Strict"); //SameSite=Strict
    UserCurrent = UserNext;
    Serial.println("Login " + String(UserNext) + String(UserStatus[UserNext]));
    return UserNext;
  }
  
  byte is_auth() {
    UserCurrent = 0;
    //AdminPasswort[0] = 0;
    //Serial.println(server.client().remoteIP().toString()+":"+String(server.client().remotePort()) );
    //Serial.println(server.hostHeader());
    //for (int i=0; i <server.headers(); i++){
    //  Serial.print(server.headerName(i));Serial.print(":");Serial.println(server.header(i));
    //}
    if (server.hasHeader("Cookie")) {
      //Serial.print("Cookie: ");
      String cookie = server.header("Cookie");
      //Serial.print(cookie);
      //for (int i=0;i<14;i++){Serial.print(cookie.charAt(i));}
      int p = cookie.indexOf("esp=");
      if (p != -1) {
        //Serial.print(" test-");Serial.print(p);Serial.print(":");
        String cookieEsp = cookie.substring(p+4, 7+4);
        //Serial.print(cookieEsp);
        int testuser = cookieEsp.toInt();
        //Serial.print(" int-");Serial.print(testuser);
        if (testuser > 0) {
          for (int k = 0; k < 10; k++) {
            //Serial.print(" ? ");
            //Serial.print(UserCookie[k]);
            if (UserCookie[k] == testuser) {
              UserCurrent = k;
              //Serial.print(" user ");
              //Serial.print(UserCurrent);
              //Serial.println(" successful");
              return k;
            }
          }
          //Serial.println(" failed");
        }
      }
    }
    //Serial.println("Authentification No Cookie");
    if (!AdminPasswort[0]) {  // Wenn Benutzerpasswort angegeben wurde
      //Serial.println("Login nicht nötig");
      return newCookie(COOKIE_ADMINISTRATOR);
    } else if (server.authenticate(&AdminName[0], &AdminPasswort[0])) {
      return newCookie(COOKIE_ADMINISTRATOR);
    } else if (UserPasswort[0] && server.authenticate(&UserName[0], &UserPasswort[0])) {
      //Serial.println("Login User");
      return newCookie(COOKIE_BENUTZER);
    } else {
      server.requestAuthentication();
      return 0;
    }
  }
  boolean is_admin() {
    byte user = is_auth();
    return (user > 0 && UserStatus[user] == COOKIE_ADMINISTRATOR);
  }
  boolean is_user() {
    byte user = is_auth();
    return (user > 0);
  }

  unsigned int hexToDec(String hexString) {
    unsigned int decValue = 0;
    int nextInt;

    for (unsigned int i = 0; i < hexString.length(); i++) {

      nextInt = int(hexString.charAt(i));
      if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
      if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
      if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
      nextInt = constrain(nextInt, 0, 15);

      decValue = (decValue * 16) + nextInt;
    }

    return decValue;
  }
  
  void LeseEeprom(int *daten){
    char* my_s_bytes = reinterpret_cast<char*>(daten);
    int laenge = sizeof(daten);
    LeseEeprom(my_s_bytes, laenge);//Serial.print((int) *daten);
  }
  void LeseEeprom(char *daten, int laenge) {
    char a;
    do {
      a = EEPROM.read(z);//if(a < 0xf) Serial.print(" 0"); else Serial.print(" "); Serial.print(a,HEX);
      *daten = a;
      daten++;
      z++;
      laenge--;
    } while (laenge > 0);
    //Serial.print(z);Serial.println("-");
  }

  void SchreibeEeprom(char *daten, int laenge) {
    do {
      EEPROM.write(z, *daten);
      daten++;
      z++;
      laenge--;
    } while (laenge > 0);
  }
  void SchreibeEeprom(int daten){
    char* my_s_bytes = reinterpret_cast<char*>(&daten);
    int laenge = sizeof(daten);
    SchreibeEeprom(my_s_bytes, laenge);
  }
 
   // 0x55 zufällig gewählt, synchron gesetzt in SchreibeEepromCheck() und LeseEepromCheck()
  void SchreibeEepromCheck() {
    EEPROM.write(z, 0x55);
    Serial.print("Schreibe EEPROM-Pos ");
    Serial.println(z);
    if (z+4 > EEPROM_RESERVIERUNG){
      Serial.println(F("Config überschreitet die EEPROM Größe"));
    }
  }
  bool LeseEepromCheck() {
    int check = 0;
    check = EEPROM.read(z);
    Serial.print("Lese EEPROM-Pos ");
    Serial.println(z);
    return (check == 0x55);
  }
  void configLoad() {
    int versionEeprom0 = 0;
    int versionEeprom1 = 0;
    EEPROM.begin(EEPROM_RESERVIERUNG);
    z = 0;
    LeseEeprom(&versionEeprom0);Serial.print(F("versionEeprom0 "));Serial.println(versionEeprom0);
    LeseEeprom(ssid, 32);//Serial.println(ssid);
    LeseEeprom(passwort, 64);//Serial.println(passwort);
    LeseEeprom(AdminName, LOGINLAENGE);//Serial.println(AdminName);
    LeseEeprom(AdminPasswort, LOGINLAENGE);//Serial.println(AdminPasswort);
    LeseEeprom(UserName, LOGINLAENGE);//Serial.println(UserName);
    LeseEeprom(UserPasswort, LOGINLAENGE);//Serial.println(UserPasswort);
    LeseEeprom(UpdateServer, LOGINLAENGE);//Serial.println(UpdateServer);
    LeseEeprom(timeserver, LOGINLAENGE);//Serial.println(timeserver);
    LeseEeprom(name_dev, LOGINLAENGE);//Serial.println(name_dev);
    for (int s = 0; s < SPECIAL_MAX; s++) {
      if (s<11 || versionEeprom0 > 4){
        LeseEeprom(special[s].name, SPECIAL_NAME);
        LeseEeprom(special[s].address, SPECIAL_ADDRESS);
        for (int k = 0; k < SPECIAL_VAL_MAX; k++) {
          LeseEeprom(&(special[s].val[k]));
        }
      }
    }
    LeseEeprom(&versionEeprom1);//Serial.println(versionEeprom);
    int check = LeseEepromCheck();
    EEPROM.end();
    start_ap = true;
    if (versionEeprom0 < EEPROM_VERSION){
      Serial.print(versionEeprom0); Serial.print(check); Serial.println(F(" EEPROM veraltet!"));
      return;
    }
    if (versionEeprom0 != versionEeprom1 || versionEeprom0 > EEPROM_VERSION || !check)
    { // Lese-Fehler, alles initialisieren. Eventuell außer ssid / passwort, da sonst der Zugang zum eventuell laufenden System zerstört wird
      Serial.println(F("EEPROM unvollständig gelesen!"));
      // 
      // ssid[0] = 0;
      // passwort[0] = 0;
      AdminPasswort[0] = 0;
      strcpy(AdminName, "admin");
      UserPasswort[0] = 0;
      strcpy(UserName, "user");
      UpdateServer[0] = 0;
      strcpy(timeserver, "time.nist.gov");
      strcpy(name_dev, "Wifi 4-fach Timer");
      start_ap = true;
    }
  }
  void ConfigSave() {  // Wird ausgeuehrt bei "http://<ip address>/setup.json"
    if (is_admin()) {
      server.arg("ssid").toCharArray(ssid, server.arg("ssid").length() + 1);
      String temp1 = server.arg("passwort");
      if (temp1[2] != '*') {
        temp1.toCharArray(passwort, temp1.length() + 1);
      }
      server.arg("AdminName").toCharArray(AdminName, server.arg("AdminName").length() + 1);
      server.arg("AdminPasswort").toCharArray(AdminPasswort, server.arg("AdminPasswort").length() + 1);
      server.arg("UserName").toCharArray(UserName, server.arg("UserName").length() + 1);
      server.arg("UserPasswort").toCharArray(UserPasswort, server.arg("UserPasswort").length() + 1);
      server.arg("UpdateServer").toCharArray(UpdateServer, server.arg("UpdateServer").length() + 1);
      server.arg("timeserver").toCharArray(timeserver, server.arg("timeserver").length() + 1);
      server.arg("name_dev").toCharArray(name_dev, server.arg("name_dev").length() + 1);
      int versionEeprom = EEPROM_VERSION;
      z = 0;
      EEPROM.begin(EEPROM_RESERVIERUNG);
      SchreibeEeprom(versionEeprom);
      SchreibeEeprom(ssid, sizeof(ssid));
      SchreibeEeprom(passwort, sizeof(passwort));
      SchreibeEeprom(AdminName, sizeof(AdminName));
      SchreibeEeprom(AdminPasswort, sizeof(AdminPasswort));
      SchreibeEeprom(UserName, sizeof(UserName));
      SchreibeEeprom(UserPasswort, sizeof(UserPasswort));
      SchreibeEeprom(UpdateServer, sizeof(UpdateServer));
      SchreibeEeprom(timeserver, sizeof(timeserver));
      SchreibeEeprom(name_dev, sizeof(name_dev));
      // ab versionEeprom = 2
      for (int s = 0; s < SPECIAL_MAX; s++) {
        server.arg("name_r" + String(s)).toCharArray(special[s].name, SPECIAL_NAME);
        SchreibeEeprom(special[s].name, SPECIAL_NAME);
        server.arg("address" + String(s)).toCharArray(special[s].address, SPECIAL_ADDRESS);
        SchreibeEeprom(special[s].address, SPECIAL_ADDRESS);
        for (int k = 0; k < SPECIAL_VAL_MAX; k++) {
          special[s].val[k] = server.arg("setup" + String(s)+"_"+String(k)).toInt();
          SchreibeEeprom(special[s].val[k]);
        }
      }
      SchreibeEeprom(versionEeprom);
      SchreibeEepromCheck();
      EEPROM.commit();
      EEPROM.end();
    }
    server.sendHeader("Location", "/index.htm#page3");
    server.send(303, "text/html", "");  // Antwort an Internet Browser
  }

  void configJson(){      // Wird ausgeuehrt wenn "http://<ip address>/" aufgerufen wurde
    String temp = "";              // Sternchen einfügen bei Passwortanzeige
    if (is_admin()) {
      if (ssid[0] == 255) ssid[0] = 0;            // Wenn speicher leer alles auf 0
      if (passwort[0] == 255) passwort[0] = 0;
      if (AdminPasswort[0] == 255) AdminPasswort[0] = 0;


      temp += "{\"ssid1\":\"" + String(ssid) + "\",";
      temp +=  "\"pass1\":\"" + String(passwort) + "\",";
      temp +=  "\"name2\":\"" + String(AdminName) + "\",";
      temp +=  "\"pass2\":\"" + String(AdminPasswort) + "\",";
      temp +=  "\"name3\":\"" + String(UserName) + "\",";
      temp +=  "\"pass3\":\"" + String(UserPasswort) + "\",";
      temp +=  "\"timeserver\":\"" + String(timeserver) + "\",";
      temp +=  "\"update\":\"" + String(UpdateServer) + "\",";
      temp +=  "\"name_dev\":\"" + String(name_dev) + "\",\"special\":[";
      for (int s = 0; s < SPECIAL_MAX; s++) {
        if (s>0)
          temp += ",";
        temp +=  "{\"name\":\"" + String(special[s].name)+"\",\"address\":\""+String(special[s].address)+"\"";
        for (int k = 0; k < SPECIAL_VAL_MAX; k++) {
          temp +=  ",\"setup" + String(k) + "\":" + String(special[s].val[k]);
        }
        temp += "}";
      }
      temp +=  "]}";
      
    } else {
      temp = "{}";
    }
    sendString(temp, ".json");
  }

  String stateRaw(byte nr){
    String Antwort = (String)now()+";";//PrintDate(now()) + " " + PrintTime(now()) + ";";
    // Version + ChipId
    Antwort += mVersionNr + mVersionVariante + mVersionBoard+";";
    Antwort += String(chipId)+";";
    Antwort += (UserStatus[nr] == COOKIE_ADMINISTRATOR ? "Administrator" : "Eingeschränkt");
    Antwort += ";" + String(NTPok) + ";" + String(RTCok) + ";" + String(IOok) + ";" + String(DISPLAYok);
    Antwort += ";" + String(name_dev);
    Antwort += ";" + String(SPECIAL_MAX)+";";
    for (int s = 0; s < SPECIAL_MAX; s++) {
      Antwort += String(special[s].name) + ";";
      for (int k = 0; k < SPECIAL_VAL_MAX; k++) {
        Antwort += String(special[s].val[k]) + ";";
      }
    }
    return Antwort;
  }
  void stateTxt(){
    if (byte nr = is_user()) {
      sendString(stateRaw(nr), ".txt");
    }
  }
  String stateJson(){
    String Antwort = "{\"state\":\"";
    if (byte nr = is_user()) {
      Antwort += stateRaw(nr);
    }
    Antwort += "\"}";
    return Antwort;
  }
  void httpStart() {
    /*
    const char *serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";

    // Behandlung der Ereignisse anschliessen
    // Seiten aus dem SPIFFS-Speicher werden bei onNotFoud() ausgewertet
    server.on("/list", HTTP_GET, [&]() { handleFileList(); });
    server.on("/delete", HTTP_GET, [&]() {handleFileDelete();});
    server.on("/setup.json", [&]() { ConfigSave(); }); 
    server.on("/upload", HTTP_POST, [=]() { if (authenticated == 2) {
          server.send(200, "text/html", serverIndex);
        }
      },
      [&]() { handleFileUpload();});
    server.on("/upload", HTTP_GET, [=]() {
      if (authenticated == 2) {
        server.sendHeader("Connection", "close");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/html", serverIndex);
      }
    });
    server.on("/update_client", HTTP_POST, [&](){ handleUpdate();}, [&](){ handleUpdate2();});
*/
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
    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([=]() {
      if (authenticated == 2) {
        if (!handleFileRead(server.uri())) {
          String notfound = "File Not Found\r\n\r\n";
          notfound += server.uri();
          notfound += (server.method() == HTTP_GET) ? " GET " : " POST ";
          notfound += server.args();

          for (byte i = 0; i < server.args(); i++) {
            notfound += " " + server.argName(i) + ": " + server.arg(i) + "\r\n";
          }
          server.send(404, "text/plain", notfound);
        }
      }
    });
    */
    server.collectHeaders(headerKeys, headerKeysCount);
    server.begin();  // Starte den Server
    Serial.println("HTTP Server gestartet");
  }
  void sendProgmem(PGM_P content, String type){

    server.send(200, getContentType(type), content); // Antwort an Internet Browser senden
  }
  void sendString(String response, String type){
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, getContentType(type), response); // Antwort an Internet Browser senden
  }
  void _setUpdaterError() {
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    this->_updaterError = str.c_str();
  }
  void handleUpdate(){
    if(is_admin()){
      if (Update.hasError()) {
        server.send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
      } else {
        server.client().setNoDelay(true);
        server.send_P(200, PSTR("text/html"), UpdateSuccessResponse);
        delay(100);
        server.client().stop();
        ESP.restart();
      }
    }
  }
  void handleUpdate2(){
      // handler for the file upload, get's the sketch bytes, and writes
      // them through the Update object
    if(is_admin()){
      HTTPUpload& upload = server.upload();

      if(upload.status == UPLOAD_FILE_START) {
        _updaterError.clear();
        Serial.printf("Authenticated Update\n");
        Serial.printf("Update: %s\n", upload.filename.c_str());
          
        if (upload.name == "filesystem") {
        } else if (upload.name == "firmware") {
          Serial.printf("Processing firmware ...\n");
          
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace, U_FLASH)) { //start with max available size
            this->_setUpdaterError();
          }
        } 
      } else if(upload.status == UPLOAD_FILE_WRITE && !_updaterError.length()){
        Serial.printf(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          this->_setUpdaterError();
        }
      } else if(upload.status == UPLOAD_FILE_END && !_updaterError.length()){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          this->_setUpdaterError();
        }
      } else if(upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        Serial.println("Update was aborted");
      }
      delay(0);     
    }
  }

  void setup(){
    for(int i=0; i<4; i++) {
      dPinModeInputPullup(Taster[i]);
    }
    Serial.println("dPinModeInputPullup");
    if(!LittleFS.begin()){
      Serial.println("LittleFS Mount Failed");
    }
    Serial.println("LittleFS");
    #ifdef ESP32
      
      snprintf(chipId, 13, "%llX", ESP.getEfuseMac());
      uint32_t chipId = 0;
      for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
      Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
      Serial.printf("This chip has %d cores\n", ESP.getChipCores());
      Serial.print("Chip ID: "); Serial.println(chipId);
      
    #else 
      snprintf(chipId, 13, "%u", esp.getChipId());
    #endif

    //configLoad();
  }
  void setup2(){
    Serial.println("Setup2()");
    WlanStation();
    // ENDE Stationmodus / Access Point modus Auswahl
    # ifdef DISPLAY_I2C_ADDRESS
        if (DISPLAYok) {
          oledStatus();
        }
    # endif
    //printUser();
    Serial.println("Freies RAM = " + String(system_get_free_heap_size()));
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
  void loop(){
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
};

#endif