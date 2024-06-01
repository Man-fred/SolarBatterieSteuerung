#include <Arduino.h>
#include "MyClass.h"

  boolean MyClass::second() {
    timeNow = (unsigned long)now();
    if (timeNow > timeLast)  // Ausführung 1 mal je delta Sekunden
    {
      if (timeNow > time15Last + 15) {
        time15Last = timeNow;
        sec15 = true;
      }
      if (timeNow > time60Last + 60) {
        time60Last = timeNow;
        sec60 = true;
      }
      timeLast = timeNow;
      if (inSetup > 1)
        inSetup--;
      return true;
    }
    return false;
  }

  byte MyClass::inStart() {
    return inSetup;
  }
  void MyClass::clearStart() {
    inSetup = 0;
  }
  boolean MyClass::inSec15() {
    if (sec15) {
      sec15 = false;
      return true;
    }
    return false;
  }
  boolean MyClass::inSec60() {
    if (sec60) {
      sec60 = false;
      return true;
    }
    return false;
  }
  boolean MyClass::moreDiff(unsigned long ts, unsigned long diff){
    return (ts + diff > timeNow);
  }
/**
   NTP senden und empfangen

// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
*/
boolean MyClass::summertime(int year, byte month, byte day, byte hour, byte tzHours){
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if ((month == 3 && ((hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)))) || (month == 10 && ((hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))))
    return true;
  else
    return false;
}

unsigned long MyClass::GetNTP(void) {
  unsigned long ntp_time = 0;
  NTPok = false;

  udp.begin(2390);
  WiFi.hostByName(timeserver, timeServerIP);
  //sendNTPpacket(timeServerIP);
  Serial.print("sending NTP packet to ");Serial.println(timeserver);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request https://www.rfc-editor.org/rfc/rfc5905#page-16
  // see URL for details on the packets https://www.meinberg.de/german/info/ntp-packet.htm
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(timeserver, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

  int cb = udp.parsePacket();
  delay(500);
  int timout1 = 0;
  while (cb  < NTP_PACKET_SIZE)
  {
    timout1++;
    if  (timout1 > 20) return 0;                  // 10s Timout
    cb = udp.parsePacket();
    delay(500);
  }
  Serial.print("packet received, length=");
  Serial.println(cb);
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE);
  //the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:
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
  ntp_time = epoch + TIMEZONE * 3600;
  Serial.print("Unix local time = ");
  Serial.println(ntp_time);
  if (SUMMERTIME && summertime(year(ntp_time), month(ntp_time), day(ntp_time),  hour(ntp_time), TIMEZONE)) {
    ntp_time += 3600;
    sommerzeit = true;
  } else {
    sommerzeit = false;
  }
  NTPok = true;
  NTPTime = ntp_time;
  if(abs((int)(NTPTime - now())) > 1) {
    //LogSchreibenNow("falsche Zeit");
    //Serial.println( Temp );
    setTime(NTPTime);
    //LogSchreiben("NTP: Zeit gesetzt");
    //Serial.println( Temp );
  }

  return ntp_time;
}

