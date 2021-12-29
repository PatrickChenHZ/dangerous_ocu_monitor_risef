#include <Arduino.h>
#include <pcf8563.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "sensor.h"
#include "esp_adc_cal.h"

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "myicons.h"

//  git clone -b development https://github.com/tzapu/WiFiManager.git
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

// #define FACTORY_HW_TEST     //! Test RTC and WiFi scan when enabled
// #define ARDUINO_OTA_UPDATE      //! Enable this line OTA update

/*
#ifdef ARDUINO_OTA_UPDATE
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif
*/

#define TP_PIN_PIN          33
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define IMU_INT_PIN         38
#define RTC_INT_PIN         34
#define BATT_ADC_PIN        35
#define VBUS_PIN            36
#define TP_PWR_PIN          25
#define LED_PIN             4
#define CHARGE_PIN          32


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
PCF8563_Class rtc;
WiFiManager wifiManager;


//Self
String zone = "Unknown";
char zonerating = 's';

//Static
String user = "Patr C.";
char permitted = 's';

//BLE beacon related
BLEAdvertising *pAdvertising;
String product_url = "null";

//mqtt
const char* mqtt_server = "192.168.123.16";
long lastMsg = 0;
char msg[50];
int value = 0;
const char* mqttuser = "admin";
const char* mqttpass = "admin123";

WiFiClient espClient;
PubSubClient client(espClient);

//wifi
const char* ssid = "hub";
const char* password = "20040317";


void homescreen(){
  //text
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString(user,0, 10);
  tft.drawString(zone,0, 50);
  //icons border
  tft.drawRect(0,120,40,40,TFT_WHITE);
  tft.drawRect(40,120,40,40,TFT_WHITE);
  //icons
  tft.drawXBitmap(2,122,syncalr,36,36,TFT_WHITE,TFT_BLACK);
  tft.drawXBitmap(42,122,personalr,36,36,TFT_WHITE,TFT_BLACK);
}

void info(){

}

void notification(String background,String info,String type){
  if(background == "black"){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
  }
  else if(background == "red"){
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_BLACK);
  }
  else if(background == "green"){
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  }
  else if(background == "white"){
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
  }
  if(type == "zone"){
    //reserved long zone name spitter
    tft.setTextSize(2);
    tft.drawString("Zone",0, 10);
    tft.drawString("Change",0, 27);
    tft.drawString("To",0, 44);
    tft.drawString(zone,0,69);
    //reserve another line for long zone name, minium distance for size 2 is 15
  }
  delay(5000);
  homescreen();
}

void sleep(){
  tft.writecommand(ST7735_DISPOFF);
  tft.writecommand(ST7735_SLPIN);
  delay(100); //for command to finish
}

void wake(){
  //wakes up the LCD only
  //tft.writecommand(ST7735_SLPOUT)
  //tft.writecommand(ST7735_DISPON);
  tft.init();
  delay(150); //wait for voltage to stablize
}

void deepsleep(){

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "clients/wb/wb1") {
    Serial.print("Changing zone to ");
    Serial.println(messageTemp);
    zone = messageTemp;
    //TODO utilize objective coding and use server to determine color
    if(messageTemp == "HUB1"){
      notification("green",zone,"zone");
    }
    else if(messageTemp == "HUB2"){
      notification("red",zone,"zone");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("WB1",mqttuser,mqttpass)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("clients/wb/wb1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // TFT
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Wristband v1.0-b BOOTED",  0, tft.height() / 2 - 20);
  tft.drawString("BLE beacon broadcasting",  0, tft.height() / 2  + 20);

  Serial.println("BOOTED");

  homescreen();

  //WIFI
  setup_wifi();
  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // start BLE beacon related
  char beacon_data[36];
  uint16_t beaconUUID = 0xFFAA;   // UUID for Eddystone Service
  int url_length;
  int count;

  // Create BLE device
  BLEDevice::init("Band1");

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

  Serial.println("BLE all done.");
  delay(5000);

  // end BLE Beacon related
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


}
