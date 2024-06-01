#ifndef MY_CLASS_H
#define MY_CLASS_H

#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

#include <EEPROM.h>

#include <ArduinoJson.h>
JsonDocument doc;

#define TIMEZONE 1
#define SUMMERTIME 1
#include <TimeLib.h>             //<Time.h> http://www.arduino.cc/playground/Code/Time
#include <WiFiUdp.h>


class MyClass {
  String getContentType(String filename);
  public:

  MyClass() {
    //TasterSize = 0;
    //version = 0;
  }

  void sendProgmem(AsyncWebServerRequest *request, PGM_P content, String type){
    request->send(200, type, content);
  }
  void sendString(AsyncWebServerRequest *request, String response, String type){
    //request->sendHeader("Cache-Control", "no-cache");
    request->send(200, getContentType(type), response); // Antwort an Internet Browser senden
  }

};

MyClass my;

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

#endif