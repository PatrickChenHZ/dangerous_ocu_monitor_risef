void zonehandler(){
  int zoneindex = 0;
  for(int c = 0; c<50; c++){
    if(zoneid[c] == zone){
      zoneindex = c;
    }
  }
  zonerating = zoneidrating[zoneindex];
  if(zonerating == 'a'){
    notification("green",zone,"zone");
  }
  else if(zonerating == 'b'){
    notification("red",zone,"zone");
  }
  else{
    notification("black",zone,"zone");
  }
  //check if permitted
  for(int d = 0; d<50; d++){
    if(permitted[d] == zonerating){
      //recover/set stats once matching rating is found
      notpermittedentry = false;
      //end this for loop now
      d=50;
      continue;
    }
    //when it reach 49 it means it never satisfied before as loop is still running
    //when 49 is also not found as permitted, set flag
    //avoid repeated trigger
    if(d == 49 && permitted[d] != zonerating && !notpermittedentry){
      notpermittedentry = true;
      lastnotify = millis() - 15000;
      //publish message to server
      String msgpub2 = "Not Permitted Zone Entry: " + zone + " > User: " + user;
      char msgchar2[msgpub2.length()+1];
      msgpub2.toCharArray(msgchar2,msgpub2.length()+1);
      client.publish("clients/wb/wb1upstream1", msgchar2);
    }
  }
}
