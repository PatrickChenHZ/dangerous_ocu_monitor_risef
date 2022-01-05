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
int submenuid = 0; //0 indicate main menu, else match pageid
int submenupageid = 1;
bool shortpressed = false;

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
char allowedzonerating[50];
bool configured = false;

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
  //reset all page and subpage
  pageid=1;
  submenuid = 0; //0 indicate main menu, else match pageid
  submenupageid = 1;

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
  tft.drawString("1.1",0, 20);
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

void emergency(String type){
    String message;
    String display;
    Serial.println("Emergency Triggered");
    //making sure mqtt is working
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    if(type == "sos"){
      message = "shortcut";
      display = "general";
    }
    else if(type == "med"){
      message = "medical";
      display = "medical";
    }
    else if(type == "lab"){
      message = "lab rating";
      display = "lab";
    }
    else if(type == "hzd"){
      message = "hazard rating";
      display = "hazard";
    }
    else{
      message = "shortcut";
      display = "general";
    }
    String msgpub = "emergency>" + zone + ">" + message;
    char msgchar[msgpub.length()+1];
    msgpub.toCharArray(msgchar,msgpub.length()+1);
    client.publish("clients/wb/wb1upstream1", msgchar);
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("SOS",0, 10);
    tft.drawString("HELP",0, 27);
    tft.drawString("REQUESTED",0, 44);
    tft.drawString(display,0, 61);
    //need to be changed to dismiss message or cancel request, no delay allowed
    settimeout(10);
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

void requesthelpsub(int subid){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Request",0, 10);
  tft.drawString("Help",0, 27);
  tft.drawString("From",0, 44);
  if(subid == 1){
    tft.drawString("Medical",0, 78);
    page3buffer = "med";
  }
  else if(subid == 2){
    tft.drawString("Lab",0, 78);
    page3buffer = "lab";
  }
  else if(subid == 3){
    tft.drawString("Hazard",0, 78);
    page3buffer = "hzd";
  }
  settimeout(15);
}

void requesthelp(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Request",0, 10);
  tft.drawString("Help",0, 27);
  tft.drawString("Hold",0, 61);
  tft.drawString("To",0, 78);
  tft.drawString("Select",0, 95);
  settimeout(12);
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

//only for mqtt profile fetching
void reconnect2(){
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("WB1",mqttuser,mqttpass)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("clients/wb/wb1config");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void mqttgetprofile(char* topic, byte* message, unsigned int length) {
  //double interlock, avoid multiple runs
  if(!configured){
    String messageTemp;
    String time[6];
    int fragment = 1;
    int peri = 0;
    int zoneni = 0;
    int zoneri = 0;
    int timei = 0;


    //sample data: username > time(year,month,day,hh,mm,ss) > permitted zone rating(split using ,)> zone names(split using ,) array > corresponding zone rating(split using ,) array
    if (String(topic) == "clients/wb/wb1config") {
      Serial.println("Profile received ");
      for (int i = 0; i < length; i++) {
        messageTemp += (char)message[i];
        if(message[i] == '>'){
          fragment++;
          continue;
        }
        if(fragment == 1){
          user += (char)message[i];
        }
        if(fragment == 2){
          if(message[i] == ','){
            timei ++;
            continue;
          }
          time[timei] += (char)message[i];
          //(year,month,day,hour,min,sec)
          rtc.setDateTime(time[0].toInt(), time[1].toInt(), time[2].toInt(), time[3].toInt(), time[4].toInt(), time[5].toInt());
        }
        if(fragment == 3){
          if(message[i] == ','){
            peri ++;
            continue;
          }
          permitted[peri] = (char)message[i];
        }
        if(fragment == 4){
          if(message[i] == ','){
            zoneni ++;
            continue;
          }
          zoneid[zoneni] += (char)message[i];
        }
        if(fragment == 5){
          if(message[i] == ','){
            zoneri ++;
            continue;
          }
          allowedzonerating[zoneri] = (char)message[i];
        }
      }
    }
    Serial.println("Name: " + user);
    Serial.print("Time set to: ");
    Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
    Serial.println("permitted zones");
    for(int l = 0; l < 50; l++)
    {
    Serial.print(permitted[l]);
    Serial.print(',');
    }
    Serial.println("");
    Serial.println("all zone zoneids");
    for(int l = 0; l < 50; l++)
    {
    Serial.print(zoneid[l]);
    Serial.print(',');
    }
    Serial.println("");
    Serial.println("all zone zone ratings");
    for(int l = 0; l < 50; l++)
    {
    Serial.print(allowedzonerating[l]);
    Serial.print(',');
    }
    Serial.println("");
    configured = true;
  }
}

void getprofile(){
  //configure callback function for profile reception
  Serial.println("Awaiting Profile");
  client.setCallback(mqttgetprofile);
  while(!configured){
    if (!client.connected()) {
      reconnect2();
    }
    //if there is message this will process it
    client.loop();
    //send request(send uuid)
    client.publish("clients/wb/wb1profilereq", hwuuid);
    delay(1500);
  }
  //reset mqtt client
  client.unsubscribe("clients/wb/wb1config");
  //client.subscribe("clients/wb/wb1");
  delay(100);
  client.loop();
  client.disconnect();
  client.setCallback(callback);
}


void setup() {
  Serial.begin(115200);
  pinMode(LCD_BL,OUTPUT);
  digitalWrite(LCD_BL,HIGH);
  // TFT

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  //WIFI
  setup_wifi();
  //MQTT
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);
  //rtc
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000); //I2C frequency
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
  tft.drawString("BOOTED",  0, tft.height() / 2  + 60);
  delay(3000); //stablize


  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Await",  0, tft.height() / 2 - 20);
  tft.drawString("User",  0, tft.height() / 2  + 20);
  tft.drawString("Profile",  0, tft.height() / 2  + 60);

  getprofile();

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

  //try to keep this at end of loop
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
