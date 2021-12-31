#include <Arduino.h>
#include <pcf8563.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "sensor.h"
#include "esp_adc_cal.h"

#include "myicons.h"
#include "ble_eddystone.h"

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
#define LCD_BL              27


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
PCF8563_Class rtc;
WiFiManager wifiManager;


//Self
String zone = "N/A";
char zonerating = 's';
unsigned long timeout = 0;
bool timeoutbol = true;
uint8_t hh, mm, ss ;
int pageid = 1;
bool pressed = false;


bool slept= false;
String chour,cminute;

//Static
String user = "Patr C.";
char permitted = 's';

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


void settimeout(int period){
  //warning millis function overflow after 50 days back to 1
  //parameter period is in unit of second
  //Serial.println(millis());
  timeout = millis() + period * 1000;
  //Serial.print("timeout at:");
  //Serial.println(timeout);
  timeoutbol = false; //set timeoutbool=true to cancel timeout
}

void setupRTC()
{
  rtc.begin(Wire);
  //Check if the RTC clock matches, if not, use compile time
  //rtc.check();
  RTC_Date datetime = rtc.getDateTime();
  hh = datetime.hour;
  mm = datetime.minute;
  ss = datetime.second;
  rtc.setDateTime(2021, 12, 30, 14, 45, 3);
}

void clockscreen(){
  RTC_Date datetime = rtc.getDateTime();
  hh = datetime.hour;
  mm = datetime.minute;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.drawNumber(hh,20, tft.width()-22);
  tft.drawNumber(mm,20, tft.width()+22);
  settimeout(10);
}

void wake(){
  //wakes up the LCD only
  digitalWrite(LCD_BL,HIGH);
  tft.writecommand(ST7735_SLPOUT);
  delay(100);
  tft.writecommand(ST7735_DISPON);
  tft.init();
  delay(150); //wait for voltage to stablize
}

void sleep(){
  tft.writecommand(ST7735_DISPOFF);
  tft.writecommand(ST7735_SLPIN);
  delay(100); //for command to finish
  digitalWrite(LCD_BL,LOW);
  pageid=1;
}

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
  settimeout(10);

}

void infoscreen(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Version",0, 10);
  tft.drawString("1.0-d",0, 20);
  tft.drawString("Battery",0, 30);
  tft.drawString("NULL",0, 40);
  tft.drawString("User ID",0, 50);
  tft.drawString("01",0, 60);
  tft.drawString("BLE ID",0, 70);
  tft.drawString("d8a01d5b4636",0, 80);
  settimeout(10);
}

void notification(String background,String info,String type){
  if(slept){
    wake();
    slept = false;
    Serial.println("Wake up by event");
  }
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
    if(messageTemp == "Wshop1"){
      notification("green",zone,"zone");
    }
    else if(messageTemp == "Lab1"){
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
  pinMode(LCD_BL,OUTPUT);
  digitalWrite(LCD_BL,HIGH);
  // TFT

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Wristband v1.0",  0, tft.height() / 2 - 20);
  tft.drawString("BLE OK",  0, tft.height() / 2  + 20);
  tft.drawString("BOOTED",  0, tft.height() / 2  + 60);
;

  //WIFI
  setup_wifi();
  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //rtc
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000); //I2C frequency
  //touch pad
  pinMode(TP_PIN_PIN, INPUT);
  //! Must be set to pull-up output mode in order to wake up in deep sleep mode
  pinMode(TP_PWR_PIN, PULLUP);


  setupRTC();

  startble();
  delay(3000); //stablize

  homescreen();

  Serial.println("BOOTED");

}

void loop() {
  if(!timeoutbol && millis() >= timeout && !slept){
    sleep();
    slept = true;
    timeoutbol = true;
    Serial.println("shallow sleep start");
  }

  if(digitalRead(TP_PIN_PIN) == HIGH && slept){
    Serial.println("Wake up attempt");
    wake();
    slept = false;
    homescreen();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(digitalRead(TP_PIN_PIN) == HIGH && !slept){
    pageid++;
    delay(400);
    pressed = true; //indicate action required by the code
    Serial.print("Change Page:");
    Serial.println(pageid);
  }

  if(pressed){
    //if statement to avoid repeated testing and triggering
    switch(pageid){
      case 1:
        //when initial wake up it won't trigger this,this is for loop back only
        timeoutbol = true;
        homescreen();
        pressed = false;
        break;  //idk if necessary, maybe is
      case 2:
        timeoutbol = true;
        clockscreen();
        pressed = false;
        break;
      case 3:
        timeoutbol = true;
        pageid = 0; //page maxed out
        infoscreen();
        pressed = false;
        break;
    }
  }

}
