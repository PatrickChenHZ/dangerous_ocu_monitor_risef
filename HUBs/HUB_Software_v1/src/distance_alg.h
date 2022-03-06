String getProximityUUIDString(BLEBeacon beacon) {
  std::string serviceData = beacon.getProximityUUID().toString().c_str();
  int serviceDataLength = serviceData.length();
  String returnedString = "";
  int i = serviceDataLength;
  while (i > 0)
  {
    if (serviceData[i-1] == '-') {
      i--;
    }
    char a = serviceData[i-1];
    char b = serviceData[i-2];
    returnedString += b;
    returnedString += a;

    i -= 2;
  }

  return returnedString;
}

float calculateDistance(int rssi, int txPower) {

	float distFl;

  if (rssi == 0) {
      return -1.0;
  }

  if (!txPower) {
      // somewhat reasonable default value
      txPower = defaultTxPower;
  }

	if (txPower > 0) {
		txPower = txPower * -1;
	}

  const float ratio = rssi * 1.0 / txPower;
  if (ratio < 1.0) {
      distFl = pow(ratio, 10);
  } else {
      distFl = (0.89976) * pow(ratio, 7.7095) + 0.111;
  }

	return round(distFl * 100) / 100;

}
