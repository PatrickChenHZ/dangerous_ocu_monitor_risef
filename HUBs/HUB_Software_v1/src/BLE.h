bool reportDevice(BLEAdvertisedDevice advertisedDevice) {

	Serial.printf("\n\n");

	StaticJsonDocument<500> doc;

	String mac_address = advertisedDevice.getAddress().toString().c_str();
	mac_address.replace(":","");
	mac_address.toLowerCase();


	//Check scanned MAC Address against a list of allowed MAC Addresses

	if (allowedListCheck) {
		bool allowedListFound = false;
		for (uint32_t x = 0; x < allowedListNumberOfItems; x++) {
			if (mac_address == allowedList[x]) {
				allowedListFound = true;
			}
		}

		if (allowedListFound == false) {
			return false;
		}
	}
	// --------------


	//Serial.print("mac:\t");
	//Serial.println(mac_address);
	int rssi = advertisedDevice.getRSSI();
	float distance;

	doc["id"] = mac_address;
	doc["uuid"] = mac_address;
	doc["rssi"] = rssi;

	if (advertisedDevice.haveName()){
		String nameBLE = String(advertisedDevice.getName().c_str());
		Serial.print("Name: ");
		Serial.println(nameBLE);
		doc["name"] = nameBLE;

	} else {
		doc["name"] = "unknown";
		Serial.println("Device name unknown");
	}

	Serial.printf("\n\r");
	Serial.printf("Advertised Device: %s \n\r", advertisedDevice.toString().c_str());
	std::string strServiceData = advertisedDevice.getServiceData();
	 uint8_t cServiceData[100];
	 strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);

	 if (advertisedDevice.getServiceDataUUID().equals(BLEUUID(beaconUUID))==true) {  // found Eddystone UUID
		Serial.printf("is Eddystone: %d %s length %d\n", advertisedDevice.getServiceDataUUID().bitSize(), advertisedDevice.getServiceDataUUID().toString().c_str(),strServiceData.length());
	       // Update distance variable for Eddystone BLE devices
	    BLEBeacon oBeacon = BLEBeacon();
	    distance = calculateDistance(rssi, oBeacon.getSignalPower());
	    doc["distance"] = distance;


		if (cServiceData[0]==0x10) {
			 BLEEddystoneURL oBeacon = BLEEddystoneURL();
			 oBeacon.setData(strServiceData);
			 Serial.printf("Eddystone Frame Type (Eddystone-URL) ");
			 Serial.printf(oBeacon.getDecodedURL().c_str());
			 doc["url"] = oBeacon.getDecodedURL().c_str();

		} else if (cServiceData[0]==0x20) {
			 BLEEddystoneTLM oBeacon = BLEEddystoneTLM();
			 oBeacon.setData(strServiceData);
			 Serial.printf("Eddystone Frame Type (Unencrypted Eddystone-TLM) \n");
			 Serial.printf(oBeacon.toString().c_str());
		} else {
			Serial.println("service data");
			for (int i=0;i<strServiceData.length();i++) {
				Serial.printf("[%X]",cServiceData[i]);
			}
		}
		Serial.printf("\n");
	} else {
		if (advertisedDevice.haveManufacturerData()==true) {
			std::string strManufacturerData = advertisedDevice.getManufacturerData();


			uint8_t cManufacturerData[100];
			strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

			if (strManufacturerData.length()==25 && cManufacturerData[0] == 0x4C  && cManufacturerData[1] == 0x00 ) {
				BLEBeacon oBeacon = BLEBeacon();
				oBeacon.setData(strManufacturerData);

				String proximityUUID = getProximityUUIDString(oBeacon);

				distance = calculateDistance(rssi, oBeacon.getSignalPower());

				// Serial.print("RSSI: ");
				// Serial.print(rssi);
				// Serial.print("\ttxPower: ");
				// Serial.print(oBeacon.getSignalPower());
				// Serial.print("\tDistance: ");
				// Serial.println(distance);

				int major = ENDIAN_CHANGE_U16(oBeacon.getMajor());
				int minor = ENDIAN_CHANGE_U16(oBeacon.getMinor());

				doc["major"] = major;
				doc["minor"] = minor;

				doc["uuid"] = proximityUUID;
				doc["id"] = proximityUUID + "-" + String(major) + "-" + String(minor);
				doc["txPower"] = oBeacon.getSignalPower();
				doc["distance"] = distance;

			} else {

				if (advertisedDevice.haveTXPower()) {
					distance = calculateDistance(rssi, advertisedDevice.getTXPower());
					doc["txPower"] = advertisedDevice.getTXPower();
				} else {
					distance = calculateDistance(rssi, defaultTxPower);
				}

				doc["distance"] = distance;

				// Serial.printf("strManufacturerData: %d \n\r",strManufacturerData.length());
				// TODO: parse manufacturer data

			}
		 } else {

			if (advertisedDevice.haveTXPower()) {
				distance = calculateDistance(rssi, advertisedDevice.getTXPower());
				doc["txPower"] = advertisedDevice.getTXPower();
				doc["distance"] = distance;
			} else {
				distance = calculateDistance(rssi, defaultTxPower);
				doc["distance"] = distance;
			}

			Serial.printf("no Beacon Advertised ServiceDataUUID: %d %s \n\r", advertisedDevice.getServiceDataUUID().bitSize(), advertisedDevice.getServiceDataUUID().toString().c_str());
		 }
		}

		char JSONmessageBuffer[512];
		serializeJson(doc, JSONmessageBuffer);

		String publishTopic = String(channel) + "/" + room;

		if (mqttClient.connected()) {
			if (maxDistance == 0 || doc["distance"] < maxDistance) {
				if (mqttClient.publish((char *)publishTopic.c_str(), 0, 0, JSONmessageBuffer) == true) {

			    Serial.print("Success sending message to topic: "); Serial.println(publishTopic);
					return true;

			  } else {
			    Serial.print("Error sending message: ");
					Serial.println(publishTopic);
			    Serial.print("Message: ");
					Serial.println(JSONmessageBuffer);
					return false;
			  }
			} else {
				Serial.printf("%s exceeded distance threshold %.2f\n\r", mac_address.c_str(), distance);
				return false;
			}

		} else {

			Serial.println("MQTT disconnected.");
			if (xTimerIsTimerActive(mqttReconnectTimer) != pdFALSE) {
				TickType_t xRemainingTime = xTimerGetExpiryTime( mqttReconnectTimer ) - xTaskGetTickCount();
				Serial.print("Time remaining: ");
				Serial.println(xRemainingTime);
			} else {
				handleMqttDisconnect();
			}
		}
		return false;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

	void onResult(BLEAdvertisedDevice advertisedDevice) {

		digitalWrite(LED_BUILTIN, LED_ON);
		vTaskDelay(10 / portTICK_PERIOD_MS);
		digitalWrite(LED_BUILTIN, !LED_ON);

	}

};

void scanForDevices(void * parameter) {
	while(1) {
		if (!updateInProgress && WiFi.isConnected() && (millis() - last > (waitTime * 1000) || last == 0)) {

			Serial.print("Scanning...\t");
			BLEScanResults foundDevices = pBLEScan->start(scanTime);
			int devicesCount = foundDevices.getCount();
	    Serial.printf("Scan done! Devices found: %d\n\r",devicesCount);

			int devicesReported = 0;
			if (mqttClient.connected()) {
			  for (uint32_t i = 0; i < devicesCount; i++) {
					bool included = reportDevice(foundDevices.getDevice(i));
					if (included) {
						devicesReported++;
					}
				}
				sendTelemetry(devicesCount, devicesReported);
				pBLEScan->clearResults();
			} else {
				Serial.println("Cannot report; mqtt disconnected");
				if (xTimerIsTimerActive(mqttReconnectTimer) != pdFALSE) {
					TickType_t xRemainingTime = xTimerGetExpiryTime( mqttReconnectTimer ) - xTaskGetTickCount();
					Serial.print("Time remaining: ");
					Serial.println(xRemainingTime);
				} else {
					handleMqttDisconnect();
				}
			}
			last = millis();
	  }
	}
}
