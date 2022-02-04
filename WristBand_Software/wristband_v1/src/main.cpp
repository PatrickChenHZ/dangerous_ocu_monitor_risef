#include <Arduino.h>
#include <pcf8563.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "sensor.h"
#include "esp_adc_cal.h"

#include "icons.h"
#include "ble_eddystone.h"
#include "imu_sensor.h"

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
TaskHandle_t Mqttthread;
extern MPU9250 IMU;


//Self

//current
String zone = "N/A";
char zonerating = 'n';
//end current
unsigned long timeout = 0;
bool timeoutbol = true;
uint8_t hh, mm, ss ;
uint8_t month,date,year;
uint32_t weekday;
String weekdaystr;
String dateoutput;
int pageid = 1;
int submenuid = 0; //0 indicate main menu, else match pageid
int submenupageid = 1;
bool shortpressed = false;
unsigned long lastnotify = 0;
unsigned long lastmqtt = 0;

long buttonTimer = 0;
long pressedtime = 0;
long shortpressbound = 900; //milisec
long longPressTimebound = 3000; // milisec
long verylongPressTime = 5500; //milisec

boolean buttonActive = false;
boolean longPressActive = false;
boolean verylongpressactive = false;
boolean pressed = false;
boolean insubmenu = false;
boolean mqttflag = false;
boolean notpermittedentry = false;

String page3buffer = "";

bool slept= false;
String chour,cminute;

bool validprofile = false;

//Debug Statics
//String user = "Patr C.";
//char permitted = 's';

//profile
String user = "";
char permitted[50];
//assumed 50 zones, there is no freaking vector in arduino
String zoneid[50];
char zoneidrating[50];
bool configured = false;
bool fall = false;

//Device ID
const char* hwuuid = "d8a01d5b4636";

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
  //reset all page and subpage
  pageid=1;
  submenuid = 0; //0 indicate main menu, else match pageid
  submenupageid = 1;

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

void deepsleep(){

}

void softreset(){
  ESP.restart();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    //watch dog, if wifi won't connect within 1min soft reset
    if(millis() >= 60000){
      Serial.println("WIFI failed in 60 Sec, reboot in 10sec");
      delay(10000);
      softreset();
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//include ui file, arduino code is sequence sensitive, do not change location
//supposedbly this can be imagined as insert content of the code into here then compile
#include "ui.h"

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
    mqttflag = true;
  }

  if(String(topic) == "clients/wb/wb1"){
    if(messageTemp == "softreset"){
      softreset();
    }
  }
}

void longpresshandler(){
  if(pageid == 3 && submenuid == 3){
    //already in submenu and try to select a page
    emergency(page3buffer);
  }
  else if(pageid == 3){
    //just enter submenu
    submenuid = 3;
    requesthelpsub(1);
  }
}

//probably can be trimmed
void shortpresshandler(){
  if(submenuid == 0){
    pressed = true;
    if(pressed && !slept){
      //if statement to avoid repeated testing and triggering
      pageid++;
      switch(pageid){
        case 1:
          //when initial wake up it won't trigger this case,this is for loop back only
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
          requesthelp();
          pressed = false;
          break;
        case 4:
          timeoutbol = true;
          pageid = 0; //page maxed out
          infoscreen();
          pressed = false;
          break;
      }
    }
  }
  else if(submenuid == 3){
    pressed = true;
    if(pressed && !slept){
      //if statement to avoid repeated testing and triggering
      submenupageid++;
      switch(submenupageid){
        case 1:
          //when initial wake up it won't trigger this case,this is for loop back only
          timeoutbol = true;
          requesthelpsub(submenupageid);
          pressed = false;
          break;  //idk if necessary, maybe is
        case 2:
          timeoutbol = true;
          requesthelpsub(submenupageid);
          pressed = false;
          break;
        case 3:
          timeoutbol = true;
          requesthelpsub(submenupageid);
          submenupageid = 0; //page maxed out
          pressed = false;
          break;
      }
    }
  }
}

void pubstr(String message,const char* topic){
  char msgchar[message.length()+1];
  message.toCharArray(msgchar,message.length()+1);
  client.publish(topic, msgchar);
}

//HAHA    void * unused    is necessary
void mqtt_loop(void * unused){
  //try to keep this at end of loop
  for(;;){
    if (!client.connected()) {
      reconnect();
    }
    //keep mqtt fetch time at around 1 sec to avoid overload
    if(lastmqtt + 1000 <= millis()){
      client.loop();
      lastmqtt = millis();
    }
  }
}

//handle obtain remote profile allocate
#include "rmtprofile.h"
//function definiations for condition triggered action
#include "handler.h"

//#include "imu_sensor.h"

void setup() {
  Serial.begin(115200);
  pinMode(LCD_BL,OUTPUT);
  digitalWrite(LCD_BL,HIGH);

  //wire
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  // TFT
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  //WIFI
  setup_wifi();
  //IMU
  setupMPU9250();
  //MQTT
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);
  //rtc
  //Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  //Wire.setClock(400000); //I2C frequency
  //touch pad
  pinMode(TP_PIN_PIN, INPUT);
  //! Must be set to pull-up output mode in order to wake up in deep sleep mode
  pinMode(TP_PWR_PIN, PULLUP);


  setupRTC();


  startble();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Wristband v1.1",  0, tft.height() / 2 - 20);
  tft.drawString("BLE OK",  0, tft.height() / 2  + 20);
  tft.drawString("Ready",  0, tft.height() / 2  + 60);
  delay(3000); //stablize


  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Await",  0, tft.height() / 2 - 20);
  tft.drawString("User",  0, tft.height() / 2  + 20);
  tft.drawString("Profile",  0, tft.height() / 2  + 60);

  getprofile();

  homescreen();

  Serial.println("BOOT Sequence Complete");

  //create multi thread task
  /*
  xTaskCreatePinnedToCore(
      mqtt_loop, // Function to implement the task
      "MQTT_Loop", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      2,  // Priority of the task
      &Mqttthread,  //Task handle.
      1); // Core where the task should run
*/

}

#include "fall_detection.h"

void loop() {
  //warning 50ms delay included in this loop
  fall_det();

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

  //potential issue, if due to mqtt reconnect the loop did not cameback here on time, it would be an issue, need to consider multitasking
  if (digitalRead(TP_PIN_PIN) == HIGH) {
    //indicates button press just started
		if(!buttonActive){
      buttonTimer = millis();
      buttonActive = true;
    }

    //first trigger long press action, if button is still not released and went above time limit trigger emergency
    if(millis()-buttonTimer >= shortpressbound){

      //trigger long press action, also avoid repeated trigger
      if(!longPressActive){
        Serial.println("long press triggered");
        longPressActive = true;
        longpresshandler();
      }
      //if it continue to satisfy this
      if(millis()-buttonTimer >= verylongPressTime){
        //avoid repeated trigger
        if(!verylongpressactive){
          //trigger emergency
          //Serial.println("Very long press triggered");
          verylongpressactive = true;
          emergency("sos");
        }
      }
    }
	} else {
    //this section handles short press which is always switch page
    //reset everything when button is released
		if(buttonActive){
      //indicates end of button press
      buttonActive = false;
      //Serial.println("Released");
      //finish up
      if(!longPressActive){
        //trigger short press
        //page selector
        shortpresshandler();
      }
      else{
        //already acted for long press so recover
        longPressActive = false;
        if(verylongpressactive){
          verylongpressactive = false;
        }
      }
    }
	}

  //check if fall flag is set
  if (fall==true){
    Serial.println("FALL DETECTED");
    notification("red"," ","fall");
    fall=false;
    }


  //check if callback function is triggered, income data need to be processed and admitted
  if(mqttflag){
    zonehandler();
    mqttflag = false;
  }
  if(notpermittedentry){
    notpermittedzone();
  }

  //mqtt main
  if (!client.connected()) {
    reconnect();
  }
  //keep mqtt fetch time at around 1 sec to avoid overload
  if(lastmqtt + 1000 <= millis()){
    client.loop();
    lastmqtt = millis();
  }

}
