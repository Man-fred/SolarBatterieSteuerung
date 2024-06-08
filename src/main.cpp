#include "MyClass.h"
#include "MyBLE.h"

#include "index.htm.h"
#include "app.css.h"
//#include "app.js.h"

#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
JsonDocument doc;

extern AsyncWebServer server;
extern char *display1;
extern char *display2;

const char *ssid = "";
const char *password = "";

byte T[4] = {17, 5, 18, 18};
timerNames timerBle[3] = {timerBle1, timerBle2, timerBle3};

// Inspiriert von Selbstbau-PV/Selbstbau-PV-Hoymiles-nulleinspeisung-mit-OpenDTU-und-Shelly3EM
// https://github.com/Selbstbau-PV/Selbstbau-PV-Hoymiles-nulleinspeisung-mit-OpenDTU-und-Shelly3EM
//
// openDTU-api: https://www.opendtu.solar/firmware/web_api/

String mVersionNr = "V00-05-02.";
String mVersionVariante = "nul.";

enum specialVal
{
  valType,      // Typ des Geräts
  valLfd,       // Position in Gerät
  valInSum,     // Leitung
  valInDev,     // Gerätenr
  valRes,       // reservierte Leistung
  valMin,       // Mindestleistung
  valLimit,     // Limit in %
  valTypeDevice // Gerät zu Type
};
#define valMax valRes

enum specialType
{
  Leitung,
  DTU,
  Wechselrichter,
  Strommeter,
  Battery
};
// options in select
enum specialMeters
{
  specialMetersCfos,
  specialMetersEM3,
  specialMeters25
};
enum specialInverters
{
  specialInverterSolar,
  specialInverterBatterie,
  specialInvertergemischt
};

const char *PARAM_MESSAGE = "message";

struct myConfigStruct
{
  byte version0;
  char ssid[32];
  char pass[64];
  byte version1;
};
myConfigStruct myConfig;

MyClass my(T, 4);
//MyBLE ble;

boolean monitorJson = 0;
WiFiClient wifiClient;
int httpCode = 0;

class nulleinspeisung_t
{
public:
  byte dtus;
  byte inverters;
  byte meters;
  byte batteries;
  byte sums;
};
nulleinspeisung_t nulleinspeisung;

class dtu_t
{
public:
  byte sp;
  char name[SPECIAL_NAME];
  char version[20];
  // char address[SPECIAL_ADDRESS];
  byte inverters;
  float ist;
  float max;
  int limit;
  boolean is_avail;
  unsigned long ts_now;

  void get()
  {
    strncpy(version, doc["generic"]["version"], 19);
    ts_now = doc["ts_now"];
    ist = 0;
    inverters = doc["inverter"].size();
    for (int i = 0; i < inverters; i++)
    {
      float cur_pwr = doc["inverter"][i]["cur_pwr"];
      ist += cur_pwr;
    }
    is_avail = true;
  }

  /*  openDTU!
      if setpoint != altes_limit:
          print(f'Setze Inverterlimit von {round(altes_limit, 1)} W auf {round(setpoint, 1)} W... ', end='')
          # Neues Limit setzen
          try:
              r = requests.post(
                  url = f'http://{dtu_ip}/api/limit/config',
                  data = f'data={{"serial":"{serial}", "limit_type":0, "limit_value":{setpoint}}}',
                  auth = HTTPBasicAuth(dtu_nutzer, dtu_passwort),
                  headers = {'Content-Type': 'application/x-www-form-urlencoded'}
              )
              print(f'Konfiguration gesendet ({r.json()["type"]})')
          except:
              print('Fehler beim Senden der Konfiguration')
              */

  String json()
  {
    return "{\"version\":\"" + (String)version + "\",\"ts_now\":" + (String)ts_now +
           ",\"is_avail\":" + (is_avail ? "true" : "false") +
           ",\"inverters\":" + (String)inverters +
           ",\"sum\":" + (String)my.globals.special[sp].val[valInSum] +
           ",\"ist\":" + (String)ist +
           ",\"max\":" + (String)max +
           ",\"limit\":" + (String)limit + "}";
  }
};
dtu_t dtu[SPECIAL_DTU];

class inverter_t
{
public:
  byte sp;
  byte s_dtu;
  boolean enabled;
  boolean is_avail;
  unsigned long ts_last_success;
  float P_AC;
  float P_AC_Soll;
  float MaxPower;
  int power_limit_set;
  int power_limit_read;
  int power_limit_ack;

  void setInverter()
  {
    enabled = doc["enabled"];
    ts_last_success = doc["ts_last_success"];
    is_avail = !my.moreDiff(ts_last_success, 60);
    //Serial.printf("I %i a: %i last: %i now: %i\n", sp, is_avail, ts_last_success, my.timeNow);
    power_limit_read = doc["power_limit_read"];
    if (power_limit_read > 100)
      power_limit_read = 100 * power_limit_read / 65535;
    if (power_limit_set == 0 || my.moreDiff(ts_last_success, 60))
      power_limit_set = power_limit_read;
    power_limit_ack = doc["power_limit_ack"];
    if (is_avail)
    {
      MaxPower = doc["ch"][0][11];
      P_AC = doc["ch"][0][2];
      P_AC_Soll = MaxPower * (float)power_limit_read / 100.0;
    }
    else{
      MaxPower = 0;
      P_AC = 0;
      P_AC_Soll = 0;
    }
  }
  boolean belongsToDtu(int nr)
  {
    return (my.globals.special[sp].val[valInDev] == nr);
  }
  void read(int nr)
  {
    s_dtu = nr;
    String request = (String) "/api/inverter/id/" + (String)my.globals.special[sp].val[valLfd] + "\0";
    HttpClient http = HttpClient(wifiClient, my.globals.special[nr].address, 80);
    httpCode = http.get(request);         // Send the request
    httpCode = http.responseStatusCode(); //  read the status code and body of the response
    String payload = http.responseBody();

    if (httpCode == 200)
    {
      http.stop(); // Close connection
      if (monitorJson)
      {
        Serial.println(payload); // Print request response payload
      }
      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, payload);

      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("dtu-inverter failed: "));
        Serial.println(error.f_str());
        is_avail = false;
      }
      else
      {
        setInverter();
      }
    }
    else
    {
      is_avail = false;
      Serial.println(request);
      Serial.print("Response Code:"); // 200 is OK
      Serial.println(httpCode);       // Print HTTP return code
    }
  }
  String json()
  {
    return "{\"name\":\"" + (String)my.globals.special[sp].name + "\",\"is_avail\":" + (is_avail ? "true" : "false") + ",\"enabled\":" + (enabled ? "true" : "false") +
           ",\"ts_last_success\":" + (String)ts_last_success +
           ",\"sum\":" + (String)my.globals.special[sp].val[valInSum] +
           ",\"P_AC\":" + (String)P_AC +
           ",\"MaxPower\":" + (String)MaxPower +
           ",\"max\":" + (String)my.globals.special[sp].val[valMax] +
           ",\"power_limit_set\":" + (String)power_limit_set +
           ",\"power_limit_read\":" + (String)power_limit_read +
           ",\"power_limit_ack\":" + (String)power_limit_ack + "}";
  }
};
inverter_t inverter[SPECIAL_INVERTER];

class meter_t
{
public:
  byte sp;
  char name[SPECIAL_NAME];
  byte lfdNr;
  boolean is_valid;
  byte sum;
  unsigned long timestamp;
  float power;
  float V;
  float I;

  String json()
  {
    return "{\"name\":\"" + (String)my.globals.special[sp].name + "\",\"is_valid\":" + (is_valid ? "true" : "false") +
           ",\"ts_last_success\":" + (String)timestamp +
           ",\"sum\":" + (String)my.globals.special[sp].val[valInSum] +
           ",\"power\":" + (String)power +
           ",\"I\":" + (String)I +
           ",\"V\":" + (String)V + "}";
  }
};
meter_t meter[SPECIAL_METER];

class battery_t
{
public:
  byte sp;
  MyBLE ble;
  boolean is_valid;
  float state;
  float power;
  float V;
  float I;

  String json()
  {
    return "{\"name\":\"" + (String)my.globals.special[sp].name + "\",\"is_valid\":" + (is_valid ? "true" : "false") +
           ",\"sum\":" + (String)my.globals.special[sp].val[valInSum] +
           ",\"max\":" + (String)my.globals.special[sp].val[valMax] +
           ",\"ble\":" + ble.ble_json() + "}";
  }
};
battery_t battery[SPECIAL_BATTERY];

class sum_t
{
public:
  // Pointer auf special
  byte sp;
  char name[SPECIAL_NAME];
  boolean is_valid;
  unsigned long timestamp;
  boolean onBattery;
  float powerIst;
  float powerSolar;
  float powerBattery;
  float powerMax;
  float powerInstall;
  float powerAvail;
  float powerSollBattery;
  float powerMaxBattery;
  float powerSoll;
  float powerUsed;
  float powerLimitP;
  float powerLimitW;

  void init()
  {
    onBattery = false;
    powerIst = 0;
    powerSolar = 0;
    powerBattery = 0;
    powerMax = 0;
    powerInstall = 0;
    powerAvail = 0;
    powerSollBattery = 0;
    powerMaxBattery = 0;
  }

  String json()
  {
    String ret;
    ret =  "{\"name\":\"";
    ret += (String)my.globals.special[sp].name;
    ret += "\",\"is_valid\":" ;
    ret += (is_valid ? "true" : "false");
    ret += ",\"ts_last_success\":";
    ret += (String)timestamp;
    ret += ",\"minimum\":";
    ret += (String)my.globals.special[sp].val[valMin];
    ret += ",\"reserve\":";
    ret += (String)my.globals.special[sp].val[valRes] ;
    ret += ",\"powerIst\":";
    ret += (String)powerIst;
    ret += ",\"solar\":";
    ret += (String)powerSolar;
    ret += ",\"battery\":";
    ret += (String)powerBattery;
    ret += ",\"max\":";
    ret += (String)powerMax;
    ret +=  ",\"avail\":";
    ret += (String)powerAvail;
    ret += ",\"install\":";
    ret +=  (String)(onBattery ? powerMaxBattery : powerInstall);
    ret += ",\"soll\":";
    ret += (String)powerSoll;
    ret += ",\"sollBattery\":";
    ret += (String)powerSollBattery;
    ret += ",\"powerUsed\":";
    ret += (String)powerUsed;
    ret += ",\"limit_watt\":";
    ret += (String)powerLimitW;
    ret += ",\"limit_i0\":";
    ret += (String)inverter[0].power_limit_set;
    ret += ",\"limit_i1\":";
    ret += (String)inverter[1].power_limit_set;
    ret += ",\"limit_i2\":";
    ret += (String)inverter[2].power_limit_set;
    ret += ",\"limit_i3\":";
    ret += (String)inverter[3].power_limit_set;
    ret += ",\"limit_i4\":";
    ret += (String)inverter[4].power_limit_set;
    ret += ",\"limit_i5\":";
    ret += (String)inverter[5].power_limit_set;
    ret += ",\"powerLimit\":";
    ret += (String)(powerLimitP*100.0);
    ret += "}";
    return ret;
  }
};
sum_t sum[SPECIAL_SUM];

void ValueJson(AsyncWebServerRequest *request)
{ // Wird ausgefuehrt wenn "http://<ip address>/value.json" aufgerufen wurde
  AsyncResponseStream *response = request->beginResponseStream(MIMETYPE_JSON);
  //Serial.printf("Freies RAM (%i) = %s", 6, String(system_get_free_heap_size()));

  String temp;
  String type;
  temp.reserve(1024);
  temp="{";
  //Serial.printf("Freies RAM (%i) = %s", 6, String(system_get_free_heap_size()));
  if (byte nr = my.is_user(request, response))
  {
    //const char* PARAM_MESSAGE
    //if (request->hasParam("type")) {
      type = "";//request->getParam("type")->value();
      if (type == "" || type == "d")
      {
        temp += "\"dtus\":";
        temp += (String)nulleinspeisung.dtus;
        temp += ",\"dtu\":[";
        for (int i = 0; i < nulleinspeisung.dtus; i++)
        {
          if (i > 0)
            temp += ",";
          temp += dtu[i].json();
        }
        temp += "]";
      }
      if (type == "") temp += ",";
      if (type == "" || type == "i")
      {
        temp += "\"inverters\":";
        temp += (String)nulleinspeisung.inverters;
        temp += ",\"inverter\":[";
        for (int i = 0; i < nulleinspeisung.inverters; i++)
        {
          if (i > 0)
            temp += ",";
          temp += inverter[i].json();
        }
        temp += "]";
      }
      if (type == "") temp += ",";
      if (type == "" || type == "m")
      {
        temp += "\"meters\":";
        temp += (String)nulleinspeisung.meters;
        temp += ",\"meter\":[";
        for (int i = 0; i < nulleinspeisung.meters; i++)
        {
          if (i > 0)
            temp += ",";
          temp += meter[i].json();
        }
        temp += "]";
      }
      if (type == "") temp += ",";
      if (type == "" || type == "b")
      {
        temp += "\"batteries\":";
        temp += (String)nulleinspeisung.batteries;
        temp += ",\"battery\":[";
        for (int i = 0; i < nulleinspeisung.batteries; i++)
        {
          if (i > 0)
            temp += ",";
          temp += battery[i].json();
        }
        temp += "]";
      }
      if (type == "") temp += ",";
      if (type == "" || type == "s")
      {
        temp += "\"sums\":";
        temp += (String)nulleinspeisung.sums;
        temp += ",\"sum\":[";
        for (int i = 0; i < nulleinspeisung.sums; i++)
        {
          if (i > 0)
            temp += ",";
          temp += sum[i].json();
        }
        //"{\"state\":\"" + stateRaw(nr) + "\"}";
        temp += "]";
      }
      if (type == "") temp += ",";
      
    //}
    //else
    if (type == "")
    {
      temp += "\"state\":\"";
      temp += my.stateRaw(nr);
      temp += "\"";
    }
  }
  temp += "}";
  response->addHeader("Server", "ESP Async Web Server");
  response->print(temp);
  request->send(response); // Antwort an Internet Browser
  temp.clear();
  Serial.printf("Freies RAM (%i) = %s", 7, String(system_get_free_heap_size()));
}

void refreshRead();

void http_serve()
{
  server.on("/", HTTP_GET, [=](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_htm); });
  server.on("/index.htm", HTTP_GET, [=](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_htm); });
  server.on("/app.css", HTTP_GET, [=](AsyncWebServerRequest *request)
            { request->send(200, "text/css", app_css); });
  //server.on("/app.js", HTTP_GET, [=](AsyncWebServerRequest *request)
  //          { Serial.printf("app.js: %i", strlen(app_js)); request->send(200, "application/javascript", app_js); });

  server.on("/config.json", [=](AsyncWebServerRequest *request)
            { my.configJson(request); });
  server.on("/values.json", [=](AsyncWebServerRequest *request)
            { ValueJson(request); });
  server.on("/state.json", [=](AsyncWebServerRequest *request)
            { my.stateTxt(request); });
  server.on("/summe0.json", [=](AsyncWebServerRequest *request)
            { my.sendString(request, sum[0].json(), ".json"); });

  /*ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);*/
  my.httpStart();
}
void http_get(AsyncWebServerRequest *request)
{
  String message;
  if (request->hasParam(PARAM_MESSAGE))
  {
    message = request->getParam(PARAM_MESSAGE)->value();
  }
  else
  {
    message = "No message sent";
  }
  request->send(200, "text/plain", "Hello, GET: " + message);
}
void http_post(AsyncWebServerRequest *request)
{
  String message;
  if (request->hasParam(PARAM_MESSAGE, true))
  {
    message = request->getParam(PARAM_MESSAGE, true)->value();
  }
  else
  {
    message = "No message sent";
  }
  request->send(200, "text/plain", "Hello, POST: " + message);
}

void dtu_read(byte nr)
{
  // Specify request destination
  // openDTU (String)dtu_ip+"/livedata/status/inverters"
  String request = "/api/index";
  HttpClient http = HttpClient(wifiClient, my.globals.special[dtu[nr].sp].address, 80);
  http.beginRequest();
  http.get(request); // Send the request
  if(my.globals.special[meter[nr].sp].devuser[0] > 0){
    http.sendBasicAuth(my.globals.special[meter[nr].sp].devuser, my.globals.special[meter[nr].sp].devpass);
  }
  http.endRequest();
  httpCode = http.responseStatusCode(); //  read the status code and body of the response
  String payload = http.responseBody();
  if (httpCode == 200)
  {
    http.stop(); // Close connection
    // Serial.print("Response Code:"); //200 is OK
    // Serial.println(httpCode);   //Print HTTP return code
    if (monitorJson)
    {
      Serial.println("Request DTU:");
      Serial.println(payload); // Print request response payload
    }
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, payload);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("dtu-index failed: "));
      Serial.println(error.f_str());
      return;
    }
    /*  opendtu:
    reachable   = r['inverters'][0]['reachable'] # Ist DTU erreichbar?
    producing   = int(r['inverters'][0]['producing']) # Produziert der Wechselrichter etwas?
    altes_limit = int(r['inverters'][0]['limit_absolute']) # Altes Limit
    power_dc    = r['inverters'][0]['AC']['0']['Power DC']['v']  # Lieferung DC vom Panel
    power       = r['inverters'][0]['AC']['0']['Power']['v'] # Abgabe BKW AC in Watt
    */
    dtu[nr].get();
    dtu[nr].max = 0;
    dtu[nr].ist = 0;
    for (int i = 0; i < nulleinspeisung.inverters; i++)
    {
      if (inverter[i].belongsToDtu(nr))
      {
        inverter[i].read(dtu[nr].sp);
        dtu[nr].max += my.globals.special[inverter[i].sp].val[valMin];
        dtu[nr].ist += inverter[i].P_AC;
      }
    }
  }
  else
  {
    dtu[nr].is_avail = false;
    Serial.println(request);
    Serial.print("Response Code:"); // 200 is OK
    Serial.println(httpCode);       // Print HTTP return code
  }
}
void shelly_em3_read(byte nr)
{
  // em3
  char host[50] = "http://192.168.178.127/emeter/0\0";
  HttpClient http = HttpClient(wifiClient, my.globals.special[meter[nr].sp].address, 80);
  String request = "/emeter/0";
  for (int i = 0; i < 3; i++)
  {
    httpCode = http.get(request);         // Send the request
    httpCode = http.responseStatusCode(); //  read the status code and body of the response
    String payload = http.responseBody();
    if (httpCode == 200)
    {
      // host[30] = 48+i;
      // Serial.println(host);
      // http.begin(wifiClient, host);     //Specify request destination
      // httpCode = http.GET();            //Send the request
      // payload = http.getString();    //Get the response payload from server
      // http.end();  //Close connection
      Serial.println(payload); // Print request response payload
      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, payload);

      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("meter failed: "));
        Serial.println(error.f_str());
      }
      else
      {
        meter[i].is_valid = doc["meters"][i]["is_valid"];
        meter[i].timestamp = doc["meters"][i]["timestamp"];
        meter[i].power = doc["meters"][i]["power"];
      }
    }
  }
  http.stop(); // Close connection
  /*
    try:
        # Nimmt Daten von der Shelly 3EM Rest-API und übersetzt sie in ein json-Format
        phase_a     = requests.get(f'http://{shelly_ip}/emeter/0', headers={'Content-Type': 'application/json'}).json()['power']
        phase_b     = requests.get(f'http://{shelly_ip}/emeter/1', headers={'Content-Type': 'application/json'}).json()['power']
        phase_c     = requests.get(f'http://{shelly_ip}/emeter/2', headers={'Content-Type': 'application/json'}).json()['power']
        grid_sum    = phase_a + phase_b + phase_c # Aktueller Bezug - rechnet alle Phasen zusammen
        */
}

void shelly_25_read(byte nr)
{
  Serial.println("Request Shelly 2.5:");

  String request = "/status";
  HttpClient http = HttpClient(wifiClient, my.globals.special[meter[nr].sp].address, 80);
  http.beginRequest();
  http.get(request); // Send the request
  if(my.globals.special[meter[nr].sp].devuser[0] > 0){
    http.sendBasicAuth(my.globals.special[meter[nr].sp].devuser, my.globals.special[meter[nr].sp].devpass);
  }
  http.endRequest();
  httpCode = http.responseStatusCode(); //  read the status code and body of the response
  String payload = http.responseBody();
  if (httpCode == 200)
  {
    http.stop(); // Close connection
    // Serial.print("Response Code:"); //200 is OK
    // Serial.println(httpCode);   //Print HTTP return code
    if (monitorJson)
    {
      Serial.println("Request Shelly 2.5:");
      Serial.println(payload); // Print request response payload
    }
    DeserializationError error = deserializeJson(doc, payload);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("meter failed: "));
      Serial.println(error.f_str());
    }
    else
    {

      for (int i = 0; i < nulleinspeisung.meters; i++)
      {
        if (my.globals.special[meter[i].sp].val[valInDev] == my.globals.special[meter[nr].sp].val[valInDev])
        {
          byte lfdNr = my.globals.special[meter[i].sp].val[valLfd];
          meter[i].is_valid = doc["meters"][lfdNr]["is_valid"];
          meter[i].timestamp = doc["meters"][lfdNr]["timestamp"];
          meter[i].power = doc["meters"][lfdNr]["power"];
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < nulleinspeisung.meters; i++)
    {
      if (my.globals.special[meter[i].sp].val[valInDev] == my.globals.special[meter[nr].sp].val[valInDev])
      {
        meter[i].is_valid = false;
      }
    }
    Serial.println(request);
    Serial.print("Response Code:"); // 200 is OK
    Serial.println(httpCode);       // Print HTTP return code
  }
}

void cfos_read_channel(byte nr, byte channel)
{
  JsonObject devices_1 = doc["devices"][channel];
  // strncpy(meter[nr+0].name, devices_1["name"],19);
  // strncpy(meter[nr+1].name, devices_1["name"],19);
  // strncpy(meter[nr+2].name, devices_1["name"],19);
  meter[nr + 0].is_valid = devices_1["device_enabled"];
  // meter[nr+1].is_valid = devices_1["device_enabled"];
  // meter[nr+2].is_valid = devices_1["device_enabled"];
  meter[nr + 0].I = devices_1["current_l1"]; //+devices_1["current_l2"]+devices_1["current_l3"];
  meter[nr + 0].V = devices_1["voltage_l1"];
  // meter[nr+1].I = devices_1["current_l2"];
  // meter[nr+1].V = devices_1["voltage_l2"];
  // meter[nr+2].I = devices_1["current_l3"];
  // meter[nr+2].V = devices_1["voltage_l3"];
  meter[nr + 0].power = devices_1["power_w"];
  // meter[nr+1].power = 0;
  // meter[nr+2].power = 0;
}
void cfos_read(byte nr)
{
  // cFos-api
  String request = "/cnf?cmd=get_dev_info";
  HttpClient http = HttpClient(wifiClient, my.globals.special[meter[nr].sp].address, 80);
  http.beginRequest();
  http.get(request); // Send the request
  if(my.globals.special[meter[nr].sp].devuser[0] > 0){
    http.sendBasicAuth(my.globals.special[meter[nr].sp].devuser, my.globals.special[meter[nr].sp].devpass);
  }
  http.endRequest();
  httpCode = http.responseStatusCode(); //  read the status code and body of the response
  String payload = http.responseBody();
  if (httpCode == 200)
  {
    http.stop(); // Close connection
                 // Serial.print("Response Code:"); //200 is OK
    // Serial.println(httpCode);   //Print HTTP return code
    if (monitorJson)
    {
      Serial.println("Request cFos:");
      Serial.println(payload); // Print request response payload
    }
    DeserializationError error = deserializeJson(doc, payload);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("cfos failed: "));
      Serial.println(error.f_str());
    }
    else
    {
      // JsonArray devices = doc["devices"];
      // JsonObject params = doc["params"];
      for (int i = 0; i < nulleinspeisung.meters; i++)
      {
        if (my.globals.special[meter[i].sp].val[valInDev] == my.globals.special[meter[nr].sp].val[valInDev])
        {
          byte lfdNr = my.globals.special[meter[i].sp].val[valLfd];
          meter[i].is_valid = doc["devices"][lfdNr]["device_enabled"];
          meter[i].timestamp = doc["params"]["time"];
          meter[i].power = doc["devices"][lfdNr]["power_w"];
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < nulleinspeisung.meters; i++)
    {
      if (my.globals.special[meter[i].sp].val[valInDev] == my.globals.special[meter[nr].sp].val[valInDev])
      {
        meter[i].is_valid = false;
      }
    }
    Serial.println(request);
    Serial.print("Response Code:"); // 200 is OK
    Serial.println(httpCode);       // Print HTTP return code
  }
}
void calculate()
{
  for (int s = 0; s < nulleinspeisung.sums; s++)
  {
    sum[s].init();
    sum[s].timestamp = now();
    sum[s].is_valid = true;
    byte inSum = my.globals.special[sum[s].sp].val[valInSum];
    for (int i = 0; i < nulleinspeisung.inverters; i++)
    {
      if (my.globals.special[inverter[i].sp].val[valInSum] == inSum)
      {
        if (sum[s].timestamp > inverter[i].ts_last_success)
          sum[s].timestamp = inverter[i].ts_last_success;
        sum[s].powerIst += inverter[i].P_AC;
        if (my.globals.special[inverter[i].sp].val[valTypeDevice] == 0)
        { // Solar
          sum[s].powerSolar += inverter[i].P_AC;
        }
        else
        { // Battery-Inverter
          sum[s].powerBattery += inverter[i].P_AC;
        }
        // Inverter kann vielleicht Leistung erhöhen
        if (inverter[i].P_AC >= inverter[i].P_AC_Soll)
          sum[s].powerAvail += my.globals.special[inverter[i].sp].val[valMax];
        sum[s].powerMax += inverter[i].P_AC_Soll;
        sum[s].powerInstall += my.globals.special[inverter[i].sp].val[valMax];
      }
    }
    sum[s].powerUsed = sum[s].powerIst;
    for (int i = 0; i < nulleinspeisung.meters; i++)
    {
      if (my.globals.special[meter[i].sp].val[valInSum] == inSum)
      {
        if (sum[s].timestamp > meter[i].timestamp)
          sum[s].timestamp = meter[i].timestamp;
        sum[s].powerUsed += meter[i].power;
      }
    }
    for (int i = 0; i < nulleinspeisung.batteries; i++)
    { // ToDo !!! zur Zeit fester Abgabewert
      if (my.globals.special[battery[i].sp].val[valInSum] == inSum)
      {
        sum[s].powerSollBattery += my.globals.special[battery[i].sp].val[valMin];
        if (my.globals.special[battery[i].sp].val[valMax] > 0)
          sum[s].powerMaxBattery += my.globals.special[battery[i].sp].val[valMax];
      }
    }
    float reserve = my.globals.special[sum[s].sp].val[valRes];

    // ToDo: onBattery besser berechnen?
    if (sum[s].powerSolar < 20)
    { // && sum[s].powerSolar < sum[s].powerIst) {//onBatterie
      sum[s].onBattery = true;
      sum[s].powerSoll = sum[s].powerSollBattery;
    }
    else
    { // ohne Batterie
      sum[s].powerSoll = sum[s].powerUsed + reserve;
      if (sum[s].powerSoll < my.globals.special[sum[s].sp].val[valMin])
        sum[s].powerSoll = my.globals.special[sum[s].sp].val[valMin];
    }

    float factor = 0;
    if (sum[s].onBattery)
    {
      if (sum[s].powerMaxBattery > sum[s].powerSoll)
        factor = sum[s].powerSoll / sum[s].powerMaxBattery;
      else
        factor = 1;
    }
    else
    {
      if (sum[s].powerInstall > sum[s].powerSoll)
        factor = sum[s].powerSoll / sum[s].powerInstall;
      else
        factor = 1;
    }
    //Serial.printf("Limit %i alt/neu %f %f\n", s, sum[s].powerLimitP, factor);
    if (sum[s].powerLimitP < factor || sum[s].powerLimitP > factor + 0.05)
    { // Limit anpassen
      sum[s].powerLimitW = 0;
      for (int i = 0; i < nulleinspeisung.inverters; i++)
      {
        if (my.globals.special[inverter[i].sp].val[valInSum] == inSum)
        {
          if (/*sum[s].is_valid &&*/ inverter[i].is_avail){
            Serial.printf("  WR %i ist/soll %i %i\n", i, inverter[i].power_limit_read, inverter[i].power_limit_set);
          }
          if (/*sum[s].is_valid &&*/ inverter[i].is_avail && inverter[i].power_limit_read == inverter[i].power_limit_set)
          { // nur aktualisieren, wenn die letzte Änderung erfolgreich zurückgemeldet wurde, sonst ist ahoy bzw. WR überlastet
            float newLimit = (float)inverter[i].power_limit_read / (float)100.0;
            if (sum[s].onBattery){
              if(my.globals.special[inverter[i].sp].val[valTypeDevice] == specialInverterSolar)
              { // kein Solarertrag, jetzt vorbereiten für nächsten Tag
                newLimit = 1;
              }
              else{
                newLimit = factor;
              }
            }
            else
            {
              newLimit += ((sum[s].powerSoll - sum[s].powerIst) / sum[s].powerMax);
            }

            int intLimit = (int)(newLimit * (float)100.0);
            sum[s].powerLimitW += newLimit * (float)my.globals.special[inverter[i].sp].val[valMax];

            Serial.printf(" - WR %i alt/neu %i %i (%f)\n", i, inverter[i].power_limit_read, intLimit, newLimit);

            if (intLimit < 20)
              intLimit = 20;
            if (intLimit > 100)
              intLimit = 100;
            if ((inverter[i].power_limit_read < intLimit) || (inverter[i].power_limit_read > intLimit + 5))
            {
              String anfrage = "{\"id\":" + (String)my.globals.special[inverter[i].sp].val[valLfd] + 
                ",\"token\":\"*\",\"cmd\":\"limit_nonpersistent_relative\",\"val\":\"" + (String)intLimit + "\"}";
              Serial.printf("Steuerung ");
              Serial.print(s);
              Serial.print(" Limit: ");
              Serial.print(intLimit);
              Serial.print(", Inverter Limit: ");
              Serial.println(inverter[i].power_limit_read);
              Serial.println(anfrage);
              inverter[i].power_limit_set = intLimit;
              // wenn "unscharf":          return;
              // ahoyDTU:
              String request = "/api/ctrl";
              HttpClient http = HttpClient(wifiClient, my.globals.special[inverter[i].s_dtu].address, 80);
              httpCode = http.post(request, MIMETYPE_JSON, anfrage); // Send the request
              httpCode = http.responseStatusCode();                  //  read the status code and body of the response
              String payload = http.responseBody();

              if (httpCode == 200)
              {
                http.stop(); // Close connection

                Serial.print(httpCode); // Print HTTP return code
                Serial.print(": ");
                Serial.println(payload); // Print request response payload
                DeserializationError error = deserializeJson(doc, payload);

                // Test if parsing succeeds.
                if (error)
                {
                  Serial.print(F("setLimit failed: "));
                  Serial.println(error.f_str());
                  return;
                }
              }
            }
          }
        }
      }
      sum[s].powerLimitP = factor;
    }
  }
  snprintf(display1, 16, "%7.0f %7.0f_", sum[0].powerSoll, sum[0].powerIst);
  snprintf(display2, 16, "%7.2f %7.0f_", (sum[0].powerLimitP), sum[0].powerLimitW);
  my.refreshDisplaySolar();
}

void getSystem()
{
  nulleinspeisung.sums = 0;
  nulleinspeisung.dtus = 0;
  nulleinspeisung.inverters = 0;
  nulleinspeisung.meters = 0;
  nulleinspeisung.batteries = 0;
  for (int s = 0; s < SPECIAL_MAX; s++)
  {
    if (my.globals.special[s].val[0] == Leitung && nulleinspeisung.sums < SPECIAL_SUM)
    {
      byte p = my.globals.special[s].val[valInSum];
      if (p >= nulleinspeisung.sums)
        nulleinspeisung.sums = p + 1;
      sum[p].sp = s;
      sum[p].is_valid = true;
    }
    if (my.globals.special[s].val[0] == DTU && nulleinspeisung.dtus < SPECIAL_DTU)
    {
      dtu[nulleinspeisung.dtus].sp = s;
      nulleinspeisung.dtus++;
    }
    if (my.globals.special[s].val[0] == Wechselrichter && nulleinspeisung.inverters < SPECIAL_INVERTER)
    {
      inverter[nulleinspeisung.inverters].sp = s;
      nulleinspeisung.inverters++;
    }
    if (my.globals.special[s].val[0] == Strommeter && nulleinspeisung.meters < SPECIAL_METER)
    {
      meter[nulleinspeisung.meters].sp = s;
      nulleinspeisung.meters++;
    }
    if (my.globals.special[s].val[0] == Battery && nulleinspeisung.batteries < SPECIAL_BATTERY)
    {
      battery[nulleinspeisung.batteries].sp = s;
      nulleinspeisung.batteries++;
    }
  }
}

void readInput()
{
  char inSerial = 0;
  if (Serial.available() > 0)
  {
    char inSerial = Serial.read();
    Serial.printf("Input: %c\n", inSerial);
    if (inSerial == 'e')
    {
      Serial.println("Ie");
      my.Einstellen();
    }
    else if (inSerial == 's')
    {
      Serial.println("Is");
      my.WlanChangeToStation();
    }
    else if (inSerial == 'f')
    {
      Serial.println("Please wait 30 secs for SPIFFS to be formatted");
      LittleFS.format();
      Serial.println("Spiffs formatted");
// See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8194#sthash.mj02URAZ.dpuf
#ifndef ESP32
      SPIFFS.info(fs_info);
      Serial.println("totalBytes " + String(fs_info.totalBytes));
#endif
    }
    else if (inSerial == 'd')
    {
      for (byte d = 0; d < 1; d++)
      { // nulleinspeisung.dtus
        dtu_read(d);
      }
    }
    else if (inSerial == 'r')
    {
      my.configLoad();
    }
    else if (inSerial == 'p')
    {
      my.resetPassword();
    }
    else if (inSerial == 'm')
    {
      monitorJson = !monitorJson;
      //MyClass::monitorJson = monitorJson;
    }
    else if (inSerial == 'i')
    {
      Serial.println("");
      Serial.println("Mit Wlan verbunden");
      Serial.print("IP Adresse: ");
      Serial.println(WiFi.localIP());
      // Serial.println("Zeit: " + PrintDate(now()) + " " + PrintTime(now()));
      // printUser();
    }
  }
  my.getT(true);
  if (my.getCommand(0))
  {
    Serial.println("C0");
    my.clearStart();
    // my.Einstellen();
  }
  else if (my.getCommand(1))
  {
    Serial.println("C1");
    return;
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    LittleFS.format();
    Serial.println("Spiffs formatted");
// See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8194#sthash.mj02URAZ.dpuf
#ifndef ESP32
    SPIFFS.info(fs_info);
    Serial.println("totalBytes " + String(fs_info.totalBytes));
#endif
  }
  else if (my.getCommand(2))
  {
    Serial.println("");
    Serial.println("Mit Wlan verbunden");
    Serial.print("IP Adresse: ");
    Serial.println(WiFi.localIP());
    // Serial.println("Zeit: " + PrintDate(now()) + " " + PrintTime(now()));
    // printUser();
  }
}

void setup()
{
  my.setup();
  EEPROM.begin(EEPROM_RESERVIERUNG);
  Serial.print("EEPROM-SZE: ");
  Serial.println(EEPROM.length());

  // const char *headerKeys[6] = { "User-Agent", "Set-Cookie", "Cookie", "Date", "Content-Type", "Connection" };
  // size_t headerKeysCount = 6;
  my.posEepromGlobals = my.LeseEeprom(0, reinterpret_cast<char *>(&my.globals), sizeof(my.globals));
  Serial.printf("my.globals read: %i %i %s %s %i\n", my.posEeprom, my.globals.version0, my.globals.ssid, my.globals.passwort, my.globals.version2);

  // Status des EEPROMS testen
  byte eepromVersion = (my.globals.version0 == 0 || my.globals.version0 == 0xFF) ? 0 : my.globals.version0;
  if (eepromVersion > 0)
  {
    if (my.globals.version0 == VERSION && my.globals.version3 == VERSION)
      eepromVersion = VERSION;
    else if (my.globals.version0 >= 2 && my.globals.version2 == 2)
      eepromVersion = 2;
  }

  if (eepromVersion < VERSION)
  {
    my.globals.version0 = VERSION;
    Serial.println("my.globals failed");
    if (eepromVersion == 0)
    {
      strncpy(my.globals.ssid, "", 1);
      strncpy(my.globals.passwort, "", 1);
      my.globals.version2 = 2;
    }
    if (eepromVersion < 3)
    {
      strncpy(my.globals.AdminName, "admin", LOGINLAENGE);
      strncpy(my.globals.AdminPasswort, "\0", LOGINLAENGE);
      strncpy(my.globals.UserName, "user\0", LOGINLAENGE);
      strncpy(my.globals.UserPasswort, "\0", LOGINLAENGE);
      strncpy(my.globals.UpdateServer, "", LOGINLAENGE);            // = "192.168.178.60\0";
      strncpy(my.globals.timeserver, "time.nist.gov", LOGINLAENGE); //;// = "time.nist.gov\0";
      strncpy(my.globals.name_dev, "Nulleinspeisung", LOGINLAENGE);
      my.globals.Taster[0] = 0xff; // 0 ist gültiger GPIO
      my.globals.Taster[1] = 0xff; // 0 ist gültiger GPIO
      my.globals.Taster[2] = 0xff; // 0 ist gültiger GPIO
      my.globals.Taster[3] = 0xff; // 0 ist gültiger GPIO
      for (int i = 0; i < SPECIAL_MAX; i++)
      {
        my.globals.special[i].name[0] = 0;
        my.globals.special[i].address[0] = 0;
        my.globals.special[i].devuser[0] = 0;
        my.globals.special[i].devpass[0] = 0;
        for (int j = 0; j < SPECIAL_VAL_MAX; j++)
        {
          my.globals.special[i].val[j] = 0;
        }
      }
      my.globals.version3 = 3;
    }
  }
  EEPROM.end();
  yield;
  my.timerSet(timerDtuRefresh, timerIfOnline, 15, refreshRead);
  getSystem();
  if (nulleinspeisung.batteries > 0)
  {
    MyBLE::BLE_init();
    for (int i = 0; i < nulleinspeisung.batteries; i++)
    {
      battery[i].ble.BLE_setup(timerBle[i], my.globals.special[battery[i].sp].address);
    }
  }
  my.setupEnd();
}

void refreshRead()
{
  //Serial.printf("RefreshRead DTU: %i, Meter: %i, WR: %i, Batt: %i, Ltg: %i\n", nulleinspeisung.dtus, nulleinspeisung.meters, nulleinspeisung.inverters, nulleinspeisung.batteries, nulleinspeisung.sums);
  for (byte d = 0; d < nulleinspeisung.dtus; d++)
  {
    dtu_read(d);
  }
  for (byte m = 0; m < nulleinspeisung.meters; m++)
  {
    if (my.globals.special[meter[m].sp].val[valTypeDevice] == 0 && my.globals.special[meter[m].sp].address[0] != 0)
      cfos_read(m);
    if (my.globals.special[meter[m].sp].val[valTypeDevice] == 2 && my.globals.special[meter[m].sp].address[0] != 0)
      shelly_25_read(m);
    // später: meter[m].read();
  }

  calculate();
}

void loop()
{
  if (my.httpPossible())
  {
    http_serve();
  }

  // ElegantOTA.loop();
  readInput();
  my.loop();
  
  for (int i = 0; i < nulleinspeisung.batteries; i++)
  {
    battery[i].ble.BLE_loop();
  }
}