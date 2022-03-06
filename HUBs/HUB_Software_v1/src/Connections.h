void connectToWifi() {
  Serial.println("Connecting to WiFi...");
	WiFi.begin(ssid, password);
	WiFi.setHostname(hostname);
}

bool handleWifiDisconnect() {
	if (WiFi.isConnected()) {
		Serial.println("WiFi appears to be connected. Not retrying.");
		return true;
	}
	if (retryAttempts > 10) {
		Serial.println("Too many retries. Restarting");
		ESP.restart();
	} else {
		retryAttempts++;
	}
	if (mqttClient.connected()) {
		mqttClient.disconnect();
	}
	if (xTimerIsTimerActive(mqttReconnectTimer) != pdFALSE) {
		xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
	}

	if (xTimerReset(wifiReconnectTimer, 0) == pdFAIL) {
		Serial.println("failed to restart");
		xTimerStart(wifiReconnectTimer, 0);
		return false;
	} else {
		Serial.println("restarted");
		return true;
	}

}

void connectToMqtt() {
  Serial.println("Connecting to MQTT");
	if (WiFi.isConnected() && !updateInProgress) {
		mqttClient.setCredentials(mqttUser, mqttPassword);
		mqttClient.setClientId(hostname);
	  mqttClient.connect();
	} else {
		Serial.println("Cannot reconnect MQTT - WiFi error");
		handleWifiDisconnect();
	}
}

bool handleMqttDisconnect() {
	if (updateInProgress) {
		Serial.println("Not retrying MQTT connection - OTA update in progress");
		return true;
	}
	if (retryAttempts > 10) {
		Serial.println("Too many retries. Restarting");
		ESP.restart();
	} else {
		retryAttempts++;
	}
	if (WiFi.isConnected() && !updateInProgress) {
		Serial.println("Starting MQTT reconnect timer");
    if (xTimerReset(mqttReconnectTimer, 0) == pdFAIL) {
			Serial.println("failed to restart");
			xTimerStart(mqttReconnectTimer, 0);
		} else {
			Serial.println("restarted");
		}
  } else {
		Serial.print("Disconnected from WiFi; starting WiFi reconnect timiler\t");
		handleWifiDisconnect();
	}
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %x\n\r", event);

		switch(event) {
	    case SYSTEM_EVENT_STA_GOT_IP:
					digitalWrite(LED_BUILTIN, !LED_ON);
	        Serial.print("IP address: \t");
	        Serial.println(WiFi.localIP());
					localIp = WiFi.localIP().toString().c_str();
					Serial.print("Hostname: \t");
					Serial.println(WiFi.getHostname());
	        connectToMqtt();
					if (xTimerIsTimerActive(wifiReconnectTimer) != pdFALSE) {
						Serial.println("Stopping wifi reconnect timer");
						xTimerStop(wifiReconnectTimer, 0);
					}
					retryAttempts = 0;
	        break;
	    case SYSTEM_EVENT_STA_DISCONNECTED:
					digitalWrite(LED_BUILTIN, LED_ON);
	        Serial.println("WiFi lost connection, resetting timer\t");
					handleWifiDisconnect();
					break;
			case SYSTEM_EVENT_WIFI_READY:
					Serial.println("Wifi Ready");
					handleWifiDisconnect();
					break;
			case SYSTEM_EVENT_STA_START:
					Serial.println("STA Start");
					tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname);
					if (xTimerIsTimerActive(wifiReconnectTimer) != pdFALSE) {
						TickType_t xRemainingTime = xTimerGetExpiryTime( wifiReconnectTimer ) - xTaskGetTickCount();
						Serial.print("WiFi Time remaining: ");
						Serial.println(xRemainingTime);
					} else {
						Serial.println("WiFi Timer is inactive; resetting\t");
						handleWifiDisconnect();
					}
					break;
			case SYSTEM_EVENT_STA_STOP:
					Serial.println("STA Stop");
					handleWifiDisconnect();
					break;

    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
	retryAttempts = 0;

	if (mqttClient.publish(availabilityTopic, 0, 1, "CONNECTED") == true) {
		Serial.print("Success sending message to topic:\t");
		Serial.println(availabilityTopic);
	} else {
		Serial.println("Error sending message");
	}

	sendTelemetry();

}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  handleMqttDisconnect();
}
