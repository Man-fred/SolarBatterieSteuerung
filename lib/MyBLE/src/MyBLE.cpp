#include "MyBLE.h"
extern MyClass my;
//MyBLE clientCB;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        
 **  from Example: class MyClientCallback : public BLEClientCallbacks                 */

  void MyBLE::onConnect(BLEClient *pclient)
  {
    ble_connected = true;
    Serial.println(" BLE connect ");
    //    - delay: 500ms
    // statt ble_set_time(); jetzt:
    ble_command(0x02, 0x00);
    //    - delay: 500ms
    ble_set_time();
  }

  void MyBLE::onDisconnect(BLEClient *pclient)
  {
    ble_connected = false;
    ble_initialized = false;
    Serial.println("BLE disconnect");
  }
  // ***************** New - Security handled here ********************
  /* ****** Note: these are the same return values as defaults ********
  uint32_t MyBLE::onPassKeyRequest()
  {
    Serial.println("Client PassKeyRequest");
    return 123456;
  }
  bool MyBLE::onConfirmPIN(uint32_t pass_key)
  {
    Serial.print("The passkey YES/NO number: ");
    Serial.println(pass_key);
    return true;
  }
  void MyBLE::onAuthenticationComplete(ble_gap_conn_desc desc)
  {
    Serial.println("Starting BLE work!");
  }
  */

MyBLE::MyBLE()
{
  pClient = BLEDevice::createClient();
  ble_ok = true;
}

String MyBLE::ble_json()
{
  String sType(reinterpret_cast<char*>(vType));
  String sID(reinterpret_cast<char*>(vID));
  String sMac(reinterpret_cast<char*>(vMac));

  return "{\"is_valid\":" + (String)(ble_ok ? "true" : "false") +
         ",\"connected\":" + (String)ble_connected +
         ",\"ts_last_04\":" + (String)ts_last_04 +
         ",\"mac\":\"" + sMac +
         "\",\"id\":\"" + sID +
         "\",\"type\":\"" + sType +
         "\",\"ts_last_0F\":" + (String)ts_last_0F +
         ",\"t1\":" + (String)bct1 +
         ",\"t2\":" + (String)bct2 +
         ",\"soccalc\":" + (String)bcsoccalc +
         ",\"cmin\":" + (String)bccmin +
         ",\"cmax\":" + (String)bccmax +
         ",\"cv\":" + (String)bccv +
         ",\"cellV\":[" + (String)bccellV[0] + "," + (String)bccellV[1] + "," + (String)bccellV[2] + "," + (String)bccellV[3] + "]" +
         ",\"ts_last_03\":" + (String)ts_last_03 +
         ",\"soc\":" + (String)bcsoc +
         ",\"disCharge\":" + (String)disCharge +
         ",\"pvPower1\":" + (String)pvPower1 +
         ",\"pvPower2\":" + (String)pvPower2 +
         ",\"powerOut1\":" + (String)powerOut1 +
         ",\"powerOut2\":" + (String)powerOut2 +
         ",\"batCapacity\":" + (String)batCapacity +
         ",\"batRemain\":" + (String)batRemain +
         ",\"dev_version\":" + (String)dev_version +
         ",\"dod_level\":" + (String)dod_level + "}";
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */

  /*** Only a reference to the advertised device is passed now
    void onResult(BLEAdvertisedDevice advertisedDevice) { **/
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice->toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice->getAddress().toString() == "e8:8d:a6:56:9c:ad")
    {
      BLEDevice::getScan()->stop();
      Serial.println(advertisedDevice->toString().c_str());
      /*******************************************************************
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
      *******************************************************************/
      //myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
      //doScan = false;

    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

void MyBLE::ble_write(const char *uuidServ, const char *uuidChar, std::vector<unsigned char> rdat1)
{
  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;
  // NimBLERemoteDescriptor *pDsc = nullptr;

  int rlen = rdat1.size();
  rdat1.at(1) = rlen + 1;
  int rxor = 0;
  for (int i = 0; i < rlen; i++)
  {
    rxor = rxor ^ rdat1[i];
  }
  rdat1.push_back(rxor);

  pSvc = pClient->getService(uuidServ);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(uuidChar);

    if (pChr)
    {
      pChr->writeValue(rdat1);
      Serial.print(" Wrote ");
      Serial.print(pChr->getUUID().toString().c_str());
      Serial.print(":");
      Serial.print(rdat1[3]);
      if (pChr->canRead())
      {
        Serial.print(" New value ");
        Serial.print(pChr->readValue().c_str());
      }
    }
  }
}

void MyBLE::ble_command(int ble_cmd, int ble_cmd_parm)
{
  //            - ble_client.ble_write:
  //                #service_uuid: 'ff00'
  //                #characteristic_uuid: 'ff01'
  //                service_uuid: 0000ff00-0000-1000-8000-00805f9b34fb
  //                characteristic_uuid: 0000ff01-0000-1000-8000-00805f9b34fb
  //                value: !lambda |-
  std::vector<unsigned char> rdat1{0x73, 0x06, 0x23, (unsigned char)ble_cmd};
  if (ble_cmd == 0x0C)
  {
    rdat1.push_back((uint8_t)((ble_cmd_parm >> 0) & 0xFF));
    rdat1.push_back((uint8_t)((ble_cmd_parm >> 8) & 0xFF));
  }
  else
  {
    rdat1.push_back((unsigned char)ble_cmd_parm);
  }
  command_possible = false;
  ble_write("0000ff00-0000-1000-8000-00805f9b34fb", "0000ff01-0000-1000-8000-00805f9b34fb", rdat1);
}
void MyBLE::ble_command(int ble_cmd, String ble_cmd_parm)
{
  std::vector<unsigned char> rdat1{0x73, 0x06, 0x23, (unsigned char)ble_cmd};

  if (ble_cmd == 0x24)
  {
    rdat1.push_back(0xaa);
  }

  for (auto b : ble_cmd_parm)
  {
    rdat1.push_back((unsigned char)b);
  }
  command_possible = false;
  ble_write("0000ff00-0000-1000-8000-00805f9b34fb", "0000ff01-0000-1000-8000-00805f9b34fb", rdat1);
}

// Loop
void MyBLE::BLE_loop()
{ 
  if (loop_nr == 99) return;

  unsigned long loop = millis() - loop_start;
  if (command_possible)
  {
    if (!ble_initialized)
    {
      // eigentlich callback onConnect()
      //    - delay: 500ms
      if (loop_nr == 0 && loop > 500)
      {
        Serial.printf(" BLE to %s ok, RSSI %i", device, pClient->getRssi());
        // statt ble_set_time(); jetzt:
        ble_command(0x02, 0x00);
        // erwarte keine Antwort
        command_possible = true;
        loop_nr++;
      }
      if (loop_nr == 1 && loop > 1000)
      {
        Serial.printf(" (i1)");
        //    - delay: 500ms
        ble_set_time();
        // erwarte keine Antwort
        command_possible = true;
        loop_nr++;
      }

      if (loop_nr == 2 && loop > 1500)
      {
        Serial.printf(" (i2)");
        // Obtain a reference to the service we are after in the remote BLE server.
        if (infoX("ff00", "ff02", [=](BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify)
        { ble_notify_parse(pBLERemoteCharacteristic, x, data_len,isNotify); } ))
          Serial.print("ff02 ok");
        loop_nr++;
      }
      if (loop_nr == 3 && loop > 2000)
      {
        Serial.printf(" (i3)");
        if (infoX("ff00", "ff06", [=](BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify)
          { ble_notify_parse(pBLERemoteCharacteristic, x, data_len,isNotify); } ))
          Serial.print("ff06 ok");
        loop_nr++;
      }
      if (loop_nr == 4 && loop > 2500)
      {
        Serial.printf(" (i4)");
        ble_command(0x04, 0x01);
        loop_nr++;
      }
      if (loop_nr == 5 && loop > 3000)
      {
        if (vMac[0] != 0)
        { // deviceinfo ok (id(txt_A03_1).state != "" (mac))
          loop_nr = 0;
          ble_initialized = true;
        }
        else
        { // loop 4 wiederholen
          loop_nr = 4;
        }
      }
    }
    else
    {
      if (loop_nr == 0)
      {
        ble_command(0x03, 0x01);
        loop_nr++;
      }
      if (loop_nr == 1 && loop > 500 /* && dev_version > 130 */) // id(sensor_device_version_1).state
      {
        ble_command(0x30, 0x01);
        loop_nr++;
      }
      if (loop_nr == 2 && loop > 1000 /* && dev_version > 130 */) // id(sensor_device_version_1).state
      {
        ble_command(0x0F, 0x01);
        loop_nr = 99;
      }
    }
  }
}

boolean MyBLE::infoX(const char *uuidServ, const char *uuidChar, notify_callback notifyCallback)
{
  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;
  // NimBLERemoteDescriptor *pDsc = nullptr;

  pSvc = pClient->getService(uuidServ);
  if (pSvc)
  { /** make sure it's not null */
    pChr = pSvc->getCharacteristic(uuidChar);

    if (pChr)
    { /** make sure it's not null */
      if (pChr->canRead())
      {
        Serial.print(pChr->getUUID().toString().c_str());
        Serial.print(" Read: ");
        Serial.print(pChr->readValue().c_str());
      }

      if (pChr->canWrite())
      {
        Serial.print(" Write: ");
      }
      if (pChr->canBroadcast())
      {
        Serial.print(" broadcast ");
      }
      if (pChr->canWriteNoResponse())
      {
        Serial.print(" writeNoResponse ");
      }

      /** registerForNotify() has been deprecated and replaced with subscribe() / unsubscribe().
       *  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
       *  Unsubscribe parameter defaults are: response=false.
       */
      if (pChr->canNotify())
      {
        if (!pChr->subscribe(true, notifyCallback))
        { // ble_notify_parse_test
          /** Disconnect if subscribe failed */
          // pClient->disconnect();
          return false;
        }
        else
        {
          Serial.print(" Notify: ");
        }
      }
      else if (pChr->canIndicate())
      {
        /** Send false as first argument to subscribe to indications instead of notifications */
        if (!pChr->subscribe(false, notifyCallback))
        {
          /** Disconnect if subscribe failed */
          // pClient->disconnect();
          return false;
        }
        else
        {
          Serial.print(" Indicate: ");
        }
      }
    }
  }
  else
  {
    Serial.println("DEAD service not found.");
  }
  return true;
}

void MyBLE::ble_set_time()
{
  std::vector<unsigned char> rdat1{0x73, 0x0d, 0x23, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00};
  auto time = now();
  rdat1.at(4) = year(time) - 1900;
  rdat1.at(5) = month(time);
  rdat1.at(6) = day(time);
  rdat1.at(7) = hour(time);
  rdat1.at(8) = minute(time);
  rdat1.at(9) = second(time);
  int rlen = rdat1.size();
  rdat1.at(1) = rlen + 1;
  // #service_uuid: 'ff00'
  // #characteristic_uuid: 'ff01'
  ble_write("0000ff00-0000-1000-8000-00805f9b34fb", "0000ff01-0000-1000-8000-00805f9b34fb", rdat1);
}

void MyBLE::ble_set_dod(int dod)
{
  if (dod <= 100 && dod >= 10)
  {
    ble_command(0x0B, dod);
  }
}

void MyBLE::ble_set_discharge_treshold(int discharge)
{
  if (discharge <= 500 && discharge >= 1)
  {
    ble_command(0x0C, discharge);
  }
}

void MyBLE::ble_passthrough(boolean passthrough)
{
  ble_command(0x0D, (int)(passthrough ? 1 : 0));
}

void MyBLE::ble_powerout(boolean powerout_1, boolean powerout_2)
{
  int powerout = (powerout_1 ? 1 : 0) + (powerout_2 ? 2 : 0);
  ble_command(0x0E, powerout);
}

void MyBLE::ble_reboot(boolean passthrough)
{
  ble_command(0x25, (int)(passthrough ? 1 : 0));
}

void MyBLE::ble_factory_settings(boolean passthrough)
{
  ble_command(0x26, (int)(passthrough ? 1 : 0));
}

void MyBLE::ble_notify_dump(String info, uint8_t *x, size_t data_len)
{
  if (true) // MyClass::monitorJson)
  {
    Serial.printf("ble_notify %s", info);
    for (int i = 0; i < data_len; i++)
    {
      Serial.printf(" %02x", char(x[i]));
    }
    Serial.println("");
    /*
    for (int i = 0; i < data_len; i++)
    {
      int d1 = x[i];
      Serial.printf("%x \t %i \t %c\n", d1, d1, char(d1));
    }
    Serial.println("");
    */
  }
}

void MyBLE::ble_notify_parse(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify)
{
  // loop_nr 0: 0x04 einmalig                 //Data: runtimeInfo
  //        1: 0x03 periodisch               // Data: deviceInfo
  //        2: 0x30 periodisch unklar, fix ->unnötig?
  //        3: 0x0F periodisch temp und zellspannungen
  
  command_possible=true; 
  char delimiter = '_';
  uint8_t count = 0;
  uint8_t count11 = 0;

  for (int i = 0; i < data_len; i++)
  {
    if (x[i] == delimiter)
    {
      if (i < 11)
        count11++;
      count++;
    }
  }

  if ((count == 16) || (count11 == 3))
  {
    ts_last_0F=now();
    // Serial.println("main Data: cmd 0x0F");
    int pos = 0;
    int found = -1;
    std::string xstr((char *)x, data_len); // copy values from vector into string xstr, deep copy

    xstr = xstr + delimiter;      // append delimiter to xstr
    found = xstr.find(delimiter); // search for position of the first delimiter
    bccv = 0;
    bccmin = std::numeric_limits<float>::max();
    bccmax = std::numeric_limits<float>::min();
    while (found != -1) // loop until no more delimiter found
    {
      if (pos == 0)
        bcsoc = atoi(xstr.substr(0, found).c_str()); // pos 0 don't care
      if (pos == 1)
        bct1 = atoi(xstr.substr(0, found).c_str()); // pos 1 get int value of temperature sensor 1
      if (pos == 2)
        bct2 = atoi(xstr.substr(0, found).c_str()); // pos 2 get int value of temperature sensor 2
      if ((pos >= 3) && (pos <= 16))                // pos 3-16 parse pos for the 14 cell voltages
      {
        bcct = atof(xstr.substr(0, found).c_str()); // get float value of pos x
        bccellV[pos - 3] = bcct;
        // ESP_LOGD("cell voltage", bcct.c_str());
        bccv += bcct; // add actual value to var bccv
        if (bcct > bccmax)
          bccmax = bcct; // check for higher value as stored in bccmax
        if (bcct < bccmin)
          bccmin = bcct; // check for lower value as stored in bccmin
      }
      xstr.erase(xstr.begin(), xstr.begin() + found + 1); // remove parsed string part
      found = xstr.find(delimiter);                       // find next delimiter
      pos++;                                              // increment pos
    }
    /* calculate bcsoc from cell voltages
         cell empty = 3.0 Volt  =  0% bcsoc
         cell full  = 3.5 Volt  = 100% bcsoc
    */
    // float bcsoccalc =  (bccv/14000 - 3.0) * 200;

    bcsoccalc = 100 * ((bccv / 14000) - bchighlimit) / (bchighlimit - bclowlimit) + 100; // equation of line with two points (0,bclowlimit) (100,bchighlimit)
    ESP_LOGD("cellVoltage", "bcsoc: %i, temp1: %i, temp2: %i", bcsoc, bct1, bct2);
    ESP_LOGD("cellVoltage", "cell01: %.f, cell 02: %.f, cell 03: %.f, cell 04: %.f", bccellV[0], bccellV[1], bccellV[2], bccellV[3]);
    ESP_LOGD("cellVoltage", "cell05: %.f, cell 06: %.f, cell 07: %.f, cell 08: %.f", bccellV[4], bccellV[5], bccellV[6], bccellV[7]);
    ESP_LOGD("cellVoltage", "cell09: %.f, cell 10: %.f, cell 11: %.f, cell 12: %.f", bccellV[8], bccellV[9], bccellV[10], bccellV[11]);
    ESP_LOGD("cellVoltage", "cell13: %.f, cell 14: %.f", bccellV[12], bccellV[13]);
    /*
     id(bcsoc).publish_state(bcsoc);                                    // bcsoc from device (%)
     id(bcsoccalc).publish_state(bcsoccalc);                            // bcsoc calculated from cell voltages (%)
     id(bctemp1).publish_state(bct1);                                   // Temperature 1 (°C)
     id(bctemp2).publish_state(bct2);                                   // Temperature 2 (°C)
     id(bccvsum).publish_state(bccv/1000);                              // sum of cellvoltages = battery Voltage(V)
     id(bccvmin).publish_state(bccmin/1000);                            // lowest cellvoltage (V)
     id(bccvmax).publish_state(bccmax/1000);                            // highest cellvoltage (V)
     id(bccvdiff).publish_state((bccmax-bccmin)/1000);
     id(bccvavg).publish_state(bccv/14000);                             // avarage cellvoltage (V)
     */
  }
  else if (x[3] == 0x03)
  {
    ts_last_03=now();
    // sensor
    //  pv_level 1 und 2
    /*
      [6][7]  PV-Eingangsleistung 1 (2Byte)
      [8][9]  PV-Eingangsleistung 2 (2Byte)
              x[Y] | x[Z] << 8;
    */
    pvPower1 = x[6] | x[7] << 8;
    pvPower2 = x[8] | x[9] << 8;

    // Batterie Stand in %
    /*
      [10][11]  Verbleibende Batteriekapazität in Prozent (2Byte)
                x[Y] | x[Z] << 8;
    */
    batRemain = x[10] | x[11] << 8;
    // Entladen bei weniger als ??? Watt PV Eingang
    /*
      [19][20]  Entladeschwelle(2Byte)
                x[Y] | x[Z] << 8;
    */
    disCharge = x[19] | x[20] << 8;
    // Füllstand des Akkus in Wh
    /*
      [22][23]  Gesamtkapazität der Batterie (1Byte)
                x[Y] | x[Z] << 8;
    */
    batCapacity = x[22] | x[23] << 8;
    // Ausgangsleistung in Watt
    /*
      [24][25]  Ausgangsleistung 1(1Byte)
      [26][27]  Ausgangsleistung 2(1Byte)
                x[Y] | x[Z] << 8;
    */
    powerOut1 = x[24] | x[25] << 8;
    powerOut2 = x[26] | x[27] << 8;
    // Geräte Version ( Firmware ? )
    /*
      [12]  B2500 Geräteversion (1Byte)
            0-255 ( ~ anzeige als /100 )
    */
    dev_version = x[12];
    //
    /*
      [18]  Dod (1Byte)
            0-100 Prozentualer Anteil der Entladeleistung an der Nennleistung
    */
    dod_level = x[18];
    // binary sensor / bool
    // pv 1 und 2 in
    /*
        [x4]  PV IN 1 Zustand (1Byte)
        [x5]  PV IN 2 Zustand (1Byte)
              0x00 （off）
              0x01 （Aufladung）
              0x02 （transparent für Wechselrichter）
    */
    if (x[4] == 0x00)
    {
      pv_active_1 = false;
      pv_transparent_1 = false;
    }
    if (x[4] == 0x01)
    {
      pv_active_1 = true;
      pv_transparent_1 = false;
    }
    if (x[4] == 0x02)
    {
      pv_active_1 = true;
      pv_transparent_1 = true;
    }
    if (x[5] == 0x00)
    {
      pv_active_2 = false;
      pv_transparent_2 = false;
    }
    if (x[5] == 0x01)
    {
      pv_active_2 = true;
      pv_transparent_2 = false;
    }
    if (x[5] == 0x02)
    {
      pv_active_2 = true;
      pv_transparent_2 = true;
    }
    // pv 2 durchleiten
    /*
        [13]  Einstellung des Ladevorgangs (1Byte)
              0x00 （PV1 Aufladung PV2 Durchleitung）
              0x01 （Volles Laden und Entladen）
    */
    pv2_passthrough = (x[13] != 0x00); //{ id(switch_pv2_passthrough_1).turn_on(); }
                                       // RESERVED ( wifi / mqtt )
                                       /*
                                           [15]  Reserve(1Byte)
                                                 0x00 wifi funktioniert nicht
                                                 0x01 wifi ok, mqtt nicht verbunden
                                                 0x02 wifi ok, mqtt connect ok
                                                 ??? 0x03 wifi ok, mqtt1 connect ok, mqtt2 connect ok
                                                 maybe wifi / mqtt
                                                 00 = false / false
                                                 01 = true / false
                                                 02 = false / true
                                                 03 = true / true
                                                 -------
                                                 first part means not wifi ble_connected ?!?!?
                                                 00 = ??? / mqtt not ble_connected
                                                 01 = ??? / mqtt not ble_connected
                                                 02 = ??? / mqtt ble_connected
                                                 03 = ??? / mqtt ble_connected
                                       // wifi and mqtt, 03 maybe webserver
                                         if( x[15] == 0x00 ) { id(bool_wifi_ok_1).publish_state(false); id(bool_mqtt1_ok_1).publish_state(false); }
                                         if( x[15] == 0x01 ) { id(bool_wifi_ok_1).publish_state(true);  id(bool_mqtt1_ok_1).publish_state(false);}
                                         if( x[15] == 0x02 ) { id(bool_wifi_ok_1).publish_state(false);  id(bool_mqtt1_ok_1).publish_state(true); }
                                         if( x[15] == 0x03 ) { id(bool_wifi_ok_1).publish_state(true);  id(bool_mqtt1_ok_1).publish_state(true); }
                                       */
                                       // power 1 und 2 enabled/disabled
                                       /*
                                           [14]  Entlade-Modus / Enabled (1Byte)
                                                 0x00 OUT1&OUT2 Sperren
                                                 0x01 nur OUT1 Freigabe
                                                 0x02 nur OUT2 Freigabe
                                                 0x03 OUT1&OUT2 Freigabe
                                       */

    if (x[14] == 0x00)
    {
      powerout_1 = false;
      powerout_2 = false;
    }
    if (x[14] == 0x01)
    {
      powerout_1 = true;
      powerout_2 = false;
    }
    if (x[14] == 0x02)
    {
      powerout_1 = false;
      powerout_2 = true;
    }
    if (x[14] == 0x03)
    {
      powerout_1 = true;
      powerout_2 = true;
    }
    // power 1 und 2 active
    /*
        [16]  Ausgang Port 1 Status (1Byte)
        [17]  Ausgang Port 2 Status (1Byte)
              0x00（Aus）
              0x01（Entladung）
    */
    power_active_1 = (x[16] == 0x01); // { id(bool_power_active_1_1).publish_state(true); }
    power_active_2 = (x[17] == 0x01); // { id(bool_power_active_1_2).publish_state(true); }
    // zusatzakku 1 und 2
    /*
        [28]  Ist Netzgerät 1 angeschlossen (1Byte)
        [29]  Ist Netzgerät 2 angeschlossen (1Byte)
              0x00（Kein Akkupack angeschlossen）
              0x01（Verbinden Sie das Netzteil）
      if( x[28] == 0x00 ) { id(bool_extern_ble_connected_1_1).publish_state(false);}
      if( x[28] == 0x01 ) { id(bool_extern_ble_connected_1_1).publish_state(true); }
      if( x[29] == 0x00 ) { id(bool_extern_ble_connected_1_2).publish_state(false);}
      if( x[29] == 0x01 ) { id(bool_extern_ble_connected_1_2).publish_state(true); }

      auto call_21 = id(txt_scene_1).make_call();
      if( x[21] == 0x00 ) { call_21.set_value("Tag"); }
      if( x[21] == 0x01 ) { call_21.set_value("Nacht"); }
      if( x[21] == 0x02 ) { call_21.set_value("Morgens/Abends"); }
      call_21.perform();
    */

    /*
    auto call_30 = id(txt_region_1).make_call();
    if( x[30] == 0x00 ) { call_30.set_value("EU"); }
    if( x[30] == 0x01 ) { call_30.set_value("China"); }
    if( x[30] == 0x02 ) { call_30.set_value("non-EU"); }
    */
    txt_region_1 = (txt_region)x[30];
  }
  else if (x[3] == 0x04)
  {
    ts_last_04=now();
    ESP_LOGD("main", "Data: deviceInfo ");
    for (int i = 9; i < 14; i++)
    {
      vType[i - 9] = x[i];
    }
    vType[5] = 0x00;

    for (int i = 18; i < 42; i++)
    {
      vID[i - 18] = x[i];
    }
    vID[24] = 0x00;

    for (int i = 47; i < 59; i++)
    {
      vMac[i - 47] = x[i];
    }
    vMac[12] = 0x00;
    Serial.printf("deviceInfo %i: %s [%s] %s\n", data_len, vType, vMac, vID);
    /*
    id(txt_A01_1).publish_state(sType);
    id(txt_A02_1).publish_state(sID);
    id(txt_A03_1).publish_state(sMac);
    */
  }
  // get wifi info - "admin mode" only
  else if (x[3] == 0x08)
  {
    unsigned char vSSID[32];
    for (int i = 4; i < data_len - 1; i++)
    {
      vSSID[i - 4] = x[i];
    }
    vSSID[data_len - 5] = 0x00;
    Serial.printf("deviceInfo %i: %s\n", data_len, vSSID);
    // std::string sSSID(reinterpret_cast<char*>(vSSID));
    // id(txt_A11_1).publish_state(sSSID);
    ble_notify_dump("data 0x08", x, data_len);
  }
  else if (x[3] == 0x30)
  {
    ts_last_30=now();
    ble_notify_dump("data 0x30", x, data_len);

    uint8_t rxor = 0;
    for (int i = 0; i < data_len; i++)
    {
      if (i + 1 == data_len)
        Serial.printf("%i - %i", rxor, x[i]);
      else
        rxor = rxor ^ x[i];
    }
    /*
    //if( rxor != id(cmd30_xor_last_1) ) {
      ESP_LOGD("data 30 - raw" , "Device %i",ble_device_nr);
      ESP_LOGD("data 30 - raw" , "0x%.2x 0x%.2x 0x%.2x 0x%.2x", x[0], x[1], x[2], x[3]);
      for(int i=4;i<data_len-1;i++) {
        ESP_LOGD("data 30 - raw" , "0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x", x[i], x[i+1], x[i+2], x[i+3], x[i+4],x[i+5],x[i+6],x[i+7],x[i+8]);
        i += 8;
      }
      //ESP_LOGD("data 30 - raw" , " ");
      id(cmd30_xor_last_1) = rxor;
    //}
    */
  }
  /*
  else if (x[3] == 0x30) {
    ESP_LOGD("main", "Data: cmd 0x30");
    for(int i=0;i<data_len;i++) {
      int d1 = x[i];
      ESP_LOGD("data 30" , "%x \t %i \t %c" , d1, d1, char(d1));
    }
  }
  */
  // debug ???
  else if (x[3] == 0x01)
  {
    ble_notify_dump("data 0x01", x, data_len);
  }
  else if (x[3] == 0x81)
  {
    ble_notify_dump("data 0x81", x, data_len);
  }
  else
  {
    ble_notify_dump("unknown", x, data_len);
  }
}

void MyBLE::ble_notify_parse_test(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(data_len);
  ble_notify_dump("Test", x, data_len);
}

void MyBLE::BLE_init()
{
  Serial.println(F("Starting BLE Device"));
  
  BLEDevice::init("");
}

void MyBLE::BLE_setup(timerNames nr, char *mac)
{
  Serial.printf("Starting BLE Client %i: %s", nr, mac);
  device = mac;
  pClient = BLEDevice::createClient();
  //  pClient->setClientCallbacks(clientCB , false);
  /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
   *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
   *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
   *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
   */
  pClient->setConnectionParams(12,12,0,51);
  /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
  pClient->setConnectTimeout(5);

  my.timerSet(nr, timerIfOnline, 10, [=]() { BLE_timer(); });
  
  // pBLEScan->start(5, false);
} // End of setup.

void MyBLE::BLE_timer()
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // ble_connected we set the ble_connected flag to be true.
  Serial.print("\nBLE_timer");
  if (pClient->isConnected())
  {
    loop_nr = 0;
    loop_start = millis();
    command_possible = true;
  }
  else
  { // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    ble_initialized = false;
    BLEAddress address(device);
    ble_connected = pClient->connect(address);
  
    if (ble_connected)
    {
      loop_nr = 0;
      loop_start = millis();
      command_possible = true;
    }
  }
} // End of timer start
