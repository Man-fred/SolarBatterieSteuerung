#ifndef MY_CLASS_H
#define MY_CLASS_H

#define VERSION 3

enum timerNames {timerWlanOffline,timerNtpRefresh,timerDisplay,timerDtuRefresh,timerWhenOnline,timerBle1,timerBle2,timerBle3};
enum timerStates {timerDisabled, timerEnabled, timerIfOnline};
#define TIMER_MAX 8

#include "Arduino.h"
typedef std::function<void (void)> cb_timerset;

#ifdef ESP32
  // byte Taster[4] = {32,12,27,25}; // A4-A7 ! noch zu prüfen
  #define mySDA  25 
  #define mySCL  27
  #include <WiFi.h>
  #include <AsyncTCP.h>

  #define EspTaskWdtInit
  #define EspTaskWdtReset
  #define system_get_free_heap_size esp_get_free_heap_size
  #include <Update.h>
  #define ESPhttpUpdate httpUpdate
  #define dRead(pin) digitalRead(pin)
  #define dPinModeInputPullup(pin) pinMode(pin, INPUT_PULLUP); // turn on a 45K pullup internally
#else
  // byte Taster[4] = {D1, D2, D3, D4}; // ! noch zu setzen
  byte T[4] = {D1, D2, D3, D4};
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

//#include <ESPAsyncWebServer.h>
#include <ESPAsyncWebSrv.h>

#include "upload.htm.h"
const size_t MAX_FILESIZE = 1024 * 1024 * 15;

constexpr const char *MIMETYPE_JSON = "application/json";
constexpr const char *MIMETYPE_HTML = "text/html";
constexpr const char *MIMETYPE_TEXT = "text/plain";

// serialize/deserialize a scruct:  p-ranav/alpaca
#include <EEPROM.h>
#define EEPROM_RESERVIERUNG 4096
#define EEPROM_VERSION 2

#include <FS.h>
#define FORMAT_LITTLEFS_IF_FAILED true
#include <LittleFS.h>
#define DBG_OUTPUT_PORT Serial
#include <TimeLib.h> //<Time.h> http://www.arduino.cc/playground/Code/Time
#include <WiFiUdp.h>
#define TIMEZONE 1
#define SUMMERTIME 1

extern String mVersionNr;
extern String mVersionVariante;

#ifndef ESP32
FSInfo fs_info;
#endif

#define SPECIAL_DTU 1
#define SPECIAL_SUM 2
#define SPECIAL_INVERTER 6
#define SPECIAL_METER 6
#define SPECIAL_BATTERY 2
#define SPECIAL_MAX (SPECIAL_DTU + SPECIAL_SUM + SPECIAL_INVERTER + SPECIAL_METER + SPECIAL_BATTERY)
#define SPECIAL_NAME 32
//#define SPECIAL_ADDRESS 80
#define SPECIAL_VAL_MAX 8

const char UpdateSuccessResponse[] PROGMEM = R"=====(
  <META http-equiv='refresh' content='15;URL=/'>Update Success! Rebooting...
)=====";

class MyClass
{
  #define LOGINLAENGE 32
  int version;
  byte t[4] = {0, 0, 0, 0};
  byte tToggle[4] = {0, 0, 0, 0};
  unsigned int tTime[4] = {0, 0, 0, 0};
  byte command[3] = {0, 0, 0};
  struct specialStruct
  {
    char name[SPECIAL_NAME];
    char address[SPECIAL_NAME];
    char devuser[SPECIAL_NAME];
    char devpass[SPECIAL_NAME];
    int val[SPECIAL_VAL_MAX];
  };

  struct timerStruct
  {
    unsigned long interval;
    timerStates state;
    unsigned long last;
    //void (*callBack)();
    cb_timerset callBack;
  } timers[TIMER_MAX];

  unsigned long timeLast = 0, time15Last = 0, time60Last = 0;
  void timerLoop();

  byte inSetup = 3; // Rückwärtszaehler 3 - 2 - 1 (setup2) - 0 (setup fertig)
  boolean sec15 = 0;
  boolean sec60 = 0;
  enum networkStates {networkBoot, networkNotConfigured, networkApMode, networkConnecting, networkOnLine, networkOffLine};
  unsigned long networkTimeout = 0; // WLan-Connect;
  unsigned long networkTimeoutLoop = 0; // WLan-Connect;
  networkStates networkState = networkBoot;
  boolean start_ap = 0;
  boolean self_ap = 0; // Accesspoint Modus aus
  boolean self_station = 0;
  boolean httpMissed = true;
  void WlanLoop();
  void WlanStation();
  void WlanAP();

  enum ntpStates {ntpBoot, ntpConnecting, ntpOnline, ntpOffline};
  unsigned long ntpTimeout = 0; // WLan-Connect;
  unsigned long ntpTimeoutLoop = 0; // WLan-Connect;
  ntpStates ntpState = ntpBoot;
  void ntpLoop();

#define COOKIE_MAX 10
#define COOKIE_ADMINISTRATOR 1
#define COOKIE_BENUTZER 2
  int UserCookie[COOKIE_MAX]; // = [0,0,0,0,0,0,0,0,0,0];
  int UserStatus[COOKIE_MAX]; // = [0,0,0,0,0,0,0,0,0,0];
  int UserNext = 0;
  int UserCurrent = -1;
  // int httpCode = 0;

  byte authenticated = 0;
  const char *headerKeys[6] = {"User-Agent", "Set-Cookie", "Cookie", "Date", "Content-Type", "Connection"};
  size_t headerKeysCount = 6;
  File fsUploadFile;
  EspClass esp;
  String _updaterError;

  IPAddress timeServerIP;
#define NTP_PACKET_SIZE 48
  byte packetBuffer[NTP_PACKET_SIZE];
  WiFiUDP udp;
  boolean sommerzeit = false;
  unsigned long NTPTime = 0;

  byte NTPok = 0;
  byte IOok = 0;
  byte DISPLAYok = 0;
  void refreshDisplayTime();
  void refreshDisplayIP();
  String getContentType(AsyncWebServerRequest *request, String filename);

public:
  static boolean monitorJson;

  void refreshDisplaySolar();
  //AsyncWebSocket ws("");
  //AsyncEventSource events("");

  unsigned long timeNow;
  byte RTCok = 0;

  void timer__Set(byte nr, timerStates state, unsigned long period, void (*callBack)(void));
  void timerSet(byte nr, timerStates state, unsigned long period, cb_timerset callBack);
  int posEeprom=0;
  int posEepromGlobals=0;

  // Diese Daten müssen angepasst werden
  struct MyClassConfig {
    byte version0;
    char ssid[32];
    char passwort[64];
    byte version2;
    char AdminName[LOGINLAENGE] = "admin\0";
    char AdminPasswort[LOGINLAENGE] = "\0";
    char UserName[LOGINLAENGE] = "user\0";
    char UserPasswort[LOGINLAENGE] = "\0";
    char UpdateServer[LOGINLAENGE]; // = "192.168.178.60\0";
    char timeserver[LOGINLAENGE];   //;// = "time.nist.gov\0";
    char name_dev[LOGINLAENGE];     //= "Nulleinspeisung";
    byte Taster[4]={17,5,18,19}; // 0 ist gültiger GPIO
    specialStruct special[SPECIAL_MAX];
    byte version3;
  } globals;
  //byte TasterSize = 4;
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

  MyClass();
  MyClass(byte arr[], byte size);
 
  boolean second();
  byte inStart();
  void clearStart();
  boolean inSec15();
  boolean inSec60();
  boolean moreDiff(unsigned long ts, unsigned long diff);
  /**
     NTP senden und empfangen

  // European Daylight Savings Time calculation by "jurs" for German Arduino Forum
  // input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
  // return value: returns true during Daylight Saving Time, false otherwise
  */
  boolean summertime(int year, byte month, byte day, byte hour, byte tzHours);
  void GetNTPStart(void);
  boolean GetNTPLoop(void);
  unsigned long GetNTPEnd(void);

  void getT(boolean verbose);
  void clearT();
  byte getCommand(byte nr);

  void Einstellen();
  void WlanChangeToStation();

  bool handleFileRead(AsyncWebServerRequest *request);
  void saveFile(String filename, PGM_P content);
  void handleFileUpload(AsyncWebServerRequest *request);
  void handleFileDelete(AsyncWebServerRequest *request);
  void handleFileList(AsyncWebServerRequest *request);
  void writeFile(fs::FS &fs, const char *path, const char *message);
  void handleUpdate1(AsyncWebServerRequest *request);
  void handleUpdate2(AsyncWebServerRequest *request);

  void resetPassword()
  {
    globals.AdminPasswort[0] = 0;
    globals.UserPasswort[0] = 0;
  }

  // Check if header is present and correct
  byte newCookie(AsyncWebServerRequest *request, AsyncResponseStream *response, int level);
  byte is_auth(AsyncWebServerRequest *request, AsyncResponseStream *response);
  byte is_admin(AsyncWebServerRequest *request, AsyncResponseStream *response);
  byte is_user(AsyncWebServerRequest *request, AsyncResponseStream *response);

  unsigned int hexToDec(String hexString);

  int LeseEeprom(int pos, char *zeichen, int laenge);
  int SchreibeEeprom(int pos, char *zeichen, int laenge);

  void LeseEeprom(int *daten);
  void LeseEeprom(char *daten, int laenge);
  void SchreibeEeprom(int daten);
  // 0x55 zufällig gewählt, synchron gesetzt in SchreibeEepromCheck() und LeseEepromCheck()
  void SchreibeEepromCheck();
  bool LeseEepromCheck();
  
  void configLoad();
  void ConfigSave(AsyncWebServerRequest *request);
  void configJson(AsyncWebServerRequest *request);
  String stateRaw(byte nr);
  void stateTxt(AsyncWebServerRequest *request);

  void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
// ab hier fertig
  boolean httpPossible();
  void httpStart();
  void sendProgmem(AsyncWebServerRequest *request, PGM_P content, String type);
  void sendString(AsyncWebServerRequest *request, String result, String type);
  /*
  void _setUpdaterError()
  {
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    this->_updaterError = str.c_str();
  }
    //Send OTA events to the browser
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();
  */
  /*
  void handleUpdate(AsyncWebServerRequest *request){
    if(is_admin(request)){
      if (Update.hasError()) {
        request->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
      } else {
        request->client().setNoDelay(true);
        request->send_P(200, PSTR("text/html"), UpdateSuccessResponse);
        delay(100);
        request->client().stop();
        ESP.restart();
      }
    }
  }
  void handleUpdate2(AsyncWebServerRequest *request){
      // handler for the file upload, get's the sketch bytes, and writes
      // them through the Update object
    if(is_admin(request)){
      HTTPUpload& upload = request->upload();

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
  */
  void setup();
  void setupEnd();
  void loop();
};

#endif