#include <Arduino.h>
#include <WiFi.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include <AsyncTCP.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "BLEBeacon.h"
#include "BLEEddystoneTLM.h"
#include "BLEEddystoneURL.h"
#include "configs.h"



static const int scanTime = singleScanTime;
static const int waitTime = scanInterval;
static const uint16_t beaconUUID = 0xFEAA;
#ifdef TxDefault
static const int defaultTxPower = TxDefault;
#else
static const int defaultTxPower = -72;
#endif
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

WiFiClient espClient;
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
bool updateInProgress = false;
String localIp;
byte retryAttempts = 0;
unsigned long last = 0;
BLEScan* pBLEScan;
TaskHandle_t BLEScan;

//Global Vars to hold data
float microparticle;
String rfid_id = "";

#include "distance_alg.h"

/*
void reportSensorValues() {
	char temp[8]
	char humidity[8];

	dtostrf(getTemp(), 0, 1, temp); 						// convert float to string with one decimal place precision
	dtostrf(getHumidity(), 0, 1, humidity);			// convert float to string with one decimal place precision

	if (mqttClient.publish(tempTopic, 0, 0, temp) == true) {
		Serial.printf("Temperature %s sent\t", temp);
	}

	if (mqttClient.publish(humidityTopic, 0, 0, humidity) == true) {
		Serial.printf("Humidity %s sent\n\r", humidity);
	}
}
*/

#include "mqtt_status_telemetry.h"
#include "connections.h"
#include "BLE.h"

#include "sensors/gp2y1051.h"
#include "sensors/R200.h"

void setup() {

  Serial.begin(115200);
	Serial2.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LED_ON);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);

  mqttClient.setServer(mqttHost, mqttPort);
	mqttClient.setWill(availabilityTopic, 0, 1, "DISCONNECTED");
	mqttClient.setKeepAlive(60);

  connectToWifi();


  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(activeScan);
	pBLEScan->setInterval(bleScanInterval);
	pBLEScan->setWindow(bleScanWindow);

//default set to core1, changed to core0 to avoid fight with main loop
	xTaskCreatePinnedToCore(
		scanForDevices,
		"BLE Scan",
		4096,
		pBLEScan,
		1,
		&BLEScan,
		0);
	req_multi_read();

}

void loop() {
	r200_parse();
	TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
	TIMERG0.wdt_feed=1;
	TIMERG0.wdt_wprotect=0;
}
