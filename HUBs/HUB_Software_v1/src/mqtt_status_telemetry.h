bool sendTelemetry(int deviceCount = -1, int reportCount = -1) {
	StaticJsonDocument<256> tele;
	tele["room"] = room;
	tele["ip"] = localIp;
	tele["hostname"] = WiFi.getHostname();
	tele["scan_dur"] = scanTime;
	tele["wait_dur"] = waitTime;
	tele["max_dist"] = maxDistance;

	if (deviceCount > -1) {
		Serial.printf("devices_discovered: %d\n\r",deviceCount);
    tele["disc_ct"] = deviceCount;
	}

	if (reportCount > -1) {
		Serial.printf("devices_reported: %d\n\r",reportCount);
    tele["rept_ct"] = reportCount;
	}

	char teleMessageBuffer[258];
	serializeJson(tele, teleMessageBuffer);

	//call post sensor data functions here

	if (mqttClient.publish(telemetryTopic, 0, 0, teleMessageBuffer) == true) {
		Serial.println("Telemetry sent");
		return true;
	} else {
		Serial.println("Error sending telemetry");
		return false;
	}

}
