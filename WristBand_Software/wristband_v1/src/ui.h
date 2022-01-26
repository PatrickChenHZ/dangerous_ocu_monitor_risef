// to handle almost all user interface

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
