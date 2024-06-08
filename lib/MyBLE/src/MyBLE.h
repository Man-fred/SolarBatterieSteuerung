// Grundlage ist 
// https://github.com/noone2k/hm2500pub/blob/master/config/bc2500-ble-idf.yaml
// Vergleich bis commit a957b7b
#ifndef MY_BLE_H
#define MY_BLE_H

#include "Arduino.h"
#include "NimBLEDevice.h"
#include "MyClass.h"

class MyBLE : public BLEClientCallbacks
{
  unsigned char vType[8]; // Batterietyp
  unsigned char vID[32];  // Seriennummer
  unsigned char vMac[16]; // MAC
  int pvPower1 = 0;
  int pvPower2 = 0;
  int batRemain = 0;    //            id(sensor_bat_remain_1).publish_state(batRemain / 10);
  int disCharge = 0;    //            id(sensor_discharge_treshold_1).publish_state(disCharge);
  int batCapacity = 0;  //            id(sensor_bat_capacity_1).publish_state(batCapacity);
  int powerOut1 = 0;           // id(sensor_power_out_1_1).publish_state(powerOut1);
  int powerOut2 = 0;           // id(sensor_power_out_1_2).publish_state(powerOut2);
  byte dev_version = 0; //            id(sensor_device_version_1).publish_state(dev_version / 100);
  byte dod_level = 0;   //            id(sensor_dod_1).publish_state(dod_level);
  boolean pv_active_1 = false;
  boolean pv_active_2 = false;
  boolean pv_transparent_1 = false;
  boolean pv_transparent_2 = false;
  boolean pv2_passthrough = false;
  boolean powerout_1 = false;
  boolean powerout_2 = false;
  boolean power_active_1 = false;
  boolean power_active_2 = false;
  enum txt_region
  {
    EU,
    China,
    nonEU
  };
  txt_region txt_region_1 = EU;

  int bcsoc = 0;
  float bcsoccalc = 0;
  int bct1 = 0;
  int bct2 = 0;
  float bccv = 0.0;
  float bccmin = std::numeric_limits<float>::max();
  float bccmax = std::numeric_limits<float>::min();
  float bclowlimit = 3.0;  // low voltage limit
  float bchighlimit = 3.5; // high voltage limit

  float bcct = 0.0;
  float bccellV[14];

  unsigned int vSignal;
  unsigned char vSSID[32];
  unsigned char vFC41D_FW[16];

  BLEClient *pClient;
  BLERemoteCharacteristic *pRemoteCharacteristic;
  BLEAdvertisedDevice *myDevice;
  BLEScan *pBLEScan;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        **
 **  from Example: class MyClientCallback : public BLEClientCallbacks
*/
  void onConnect(BLEClient *pclient);
  void onDisconnect(BLEClient *pclient);
  //uint32_t onPassKeyRequest();
  //bool onConfirmPIN(uint32_t pass_key);
  //void onAuthenticationComplete(ble_gap_conn_desc desc);

  //void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
  bool connectToServer();
  void ble_write(const char *uuidServ, const char *uuidChar, std::vector<unsigned char> rdat1);
  boolean infoX(const char *uuidServ, const char *uuidChar, notify_callback notifyCallback);
  void ble_command(int ble_cmd, int ble_cmd_parm);
  void ble_command(int ble_cmd, String ble_cmd_parm);
  void ble_set_time();
  void ble_notify_dump(String info, uint8_t *x, size_t data_len);
  void ble_notify_parse(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify);
  void ble_notify_parse_test(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *x, size_t data_len, bool isNotify);

  void ble_set_dod(int dod);
  void ble_set_discharge_treshold(int discharge);
  void ble_passthrough(boolean passthrough);
  void ble_powerout(boolean powerout_1, boolean powerout_2);
  void ble_reboot(boolean passthrough);
  void ble_factory_settings(boolean passthrough);

  char *device;
  int ts_last_04=0;
  int ts_last_03=0;
  int ts_last_30=0;
  int ts_last_0F=0;
  boolean command_possible = true;
  unsigned long loop_start=0;
  unsigned long loop_millis=0;
  byte loop_nr = 0;
  boolean innerLoop(byte nr);


public:
  boolean ble_connected = false;
  boolean ble_initialized = false;
  boolean ble_ok = false;
  boolean doScan = true;
  //boolean doConnect = false;
  // 

  MyBLE();
  String ble_json();
  static void BLE_init();
  void BLE_setup(timerNames nr, char *mac);
  void BLE_timer();
  void BLE_loop();
};

#endif