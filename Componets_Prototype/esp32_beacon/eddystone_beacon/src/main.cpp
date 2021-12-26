#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
//#include "esp_sleep.h"
#include "Arduino.h"


/******************** BLE ************************************/
BLEAdvertising *pAdvertising;
String product_url = "null";


void setup() {
  char beacon_data[36];
  uint16_t beaconUUID = 0xFFAA;   // UUID for Eddystone Service
  int url_length;
  int count;

  // Create BLE device
  BLEDevice::init("Beacon1");

  // Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();

  pAdvertising = pServer->getAdvertising();

  //setBeacon();
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setFlags(0x06);    // GENERAL_DISK_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  oAdvertisementData.setCompleteServices(BLEUUID(beaconUUID));

  //beacon_data[0] = 0x20;    // Eddystone Frame Type (Unencrypted Eddystone - TLM)
  beacon_data[0] = 0x02;      // Length
  beacon_data[1] = 0x01;      //
  beacon_data[2] = 0x06;      //
  beacon_data[3] = 0x03;      // Length
  beacon_data[4] = 0x03;      // Flag - Complete list of 16-bit Service UUIDs data type value
  beacon_data[5] = 0xAA;      // 16bit Eddystone UUID
  beacon_data[6] = 0xFE;      // ...

  url_length = product_url.length();
  beacon_data[7] = url_length+6;      // Length
  beacon_data[8] = 0x16;      // Frame Type - Service Data
  beacon_data[9] = 0xAA;      // Eddystone
  beacon_data[10] = 0xFE;      //
  beacon_data[11] = 0x10;      // Frame Type - URL
  beacon_data[12] = 0x00;      // Tx power 4dBm?
  beacon_data[13] = 0x03;      // URL Scheme Prefix - https://
  for(count=0; count<url_length; count++) {
    beacon_data[14+count] = product_url.charAt(count);
  }
  //beacon_data[14+count] = 0xFF;

#ifdef DEBUG
  Serial.print("Beacon Data: ");
  for(count=0; count<url_length+15; count++) {
    if(beacon_data[count] < 16){
      Serial.print('0');
    }
    Serial.print(beacon_data[count], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif

  oAdvertisementData.setServiceData(BLEUUID(beaconUUID), std::string(beacon_data, url_length+14));
#ifdef DEBUG
  Serial.println("Service Data set!");
#endif

  pAdvertising->setScanResponseData(oAdvertisementData);
#ifdef DEBUG
  Serial.println("Scan response set!");
#endif
  // Start advertising
  pAdvertising->start();
#ifdef DEBUG
  Serial.println("Advertising started...");
#endif

  delay(5000);
}

void loop() {

}
