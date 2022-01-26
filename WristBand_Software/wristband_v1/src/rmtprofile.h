// to obtain remote profile fro server

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
