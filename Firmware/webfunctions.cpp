
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "timecore.h"
#include "datastore.h"
#include "NTP_Client.h"
#include "led.h"

#include "webfunctions.h"
#include "websocket_if.h"

extern NTP_Client NTPC;
extern Timecore timec;
extern void sendData(String data);
extern WebServer * server;
extern TaskHandle_t MQTTTaskHandle;

/**************************************************************************************************
*    Function      : response_settings
*    Description   : Sends the timesettings as json 
*    Input         : non
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void response_settings(){

char strbuffer[129];
String response="";  

  DynamicJsonDocument root(350);
  memset(strbuffer,0,129);
  datum_t d = timec.GetLocalTimeDate();
  snprintf(strbuffer,64,"%02d:%02d:%02d",d.hour,d.minute,d.second);
  
  root["time"] = strbuffer;
 
  memset(strbuffer,0,129);
  snprintf(strbuffer,64,"%04d-%02d-%02d",d.year,d.month,d.day);
  root["date"] = strbuffer;

  memset(strbuffer,0,129);
  snprintf(strbuffer,129,"%s",NTPC.GetServerName());
  root["ntpname"] = strbuffer;
  root["tzidx"] = (int32_t)timec.GetTimeZone();
  root["ntpena"] = NTPC.GetNTPSyncEna();
  root["ntp_update_span"]=NTPC.GetSyncInterval();
  root["zoneoverride"]=timec.GetTimeZoneManual();;
  root["gmtoffset"]=timec.GetGMT_Offset();;
  root["dlsdis"]=!timec.GetAutomacitDLS();
  root["dlsmanena"]=timec.GetManualDLSEna();
  uint32_t idx = timec.GetDLS_Offset();
  root["dlsmanidx"]=idx;
  serializeJson(root,response);
  sendData(response);
}


/**************************************************************************************************
*    Function      : settime_update
*    Description   : Parses POST for new local time
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void settime_update( ){ /* needs to process date and time */
  datum_t d;
  d.year=2000;
  d.month=1;
  d.day=1;
  d.hour=0;
  d.minute=0;
  d.second=0;

  bool time_found=false;
  bool date_found=false;
  
  if( ! server->hasArg("date") || server->arg("date") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
  } else {
   
    Serial.printf("found date: %s\n\r",server->arg("date").c_str());
    uint8_t d_len = server->arg("date").length();
    Serial.printf("datelen: %i\n\r",d_len);
    if(server->arg("date").length()!=10){
      Serial.println("date len failed");
    } else {   
      String year=server->arg("date").substring(0,4);
      String month=server->arg("date").substring(5,7);
      String day=server->arg("date").substring(8,10);
      d.year = year.toInt();
      d.month = month.toInt();
      d.day = day.toInt();
      date_found=true;
    }   
  }

  if( ! server->hasArg("time") || server->arg("time") == NULL ) { // If the POST request doesn't have username and password data
    
  } else {
    if(server->arg("time").length()!=8){
      Serial.println("time len failed");
    } else {
    
      String hour=server->arg("time").substring(0,2);
      String minute=server->arg("time").substring(3,5);
      String second=server->arg("time").substring(6,8);
      d.hour = hour.toInt();
      d.minute = minute.toInt();
      d.second = second.toInt();     
      time_found=true;
    }
     
  } 
  if( (time_found==true) && ( date_found==true) ){
    Serial.printf("Date: %i, %i, %i ", d.year , d.month, d.day );
    Serial.printf("Time: %i, %i, %i ", d.hour , d.minute, d.second );
    timec.SetLocalTime(d);
  }
  
  server->send(200);   
 
 }


/**************************************************************************************************
*    Function      : ntp_settings_update
*    Description   : Parses POST for new ntp settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void ntp_settings_update( ){ /* needs to process NTP_ON, NTPServerName and NTP_UPDTAE_SPAN */

  if( ! server->hasArg("NTP_ON") || server->arg("NTP_ON") == NULL ) { // If the POST request doesn't have username and password data
    NTPC.SetNTPSyncEna(false);  
  } else {
    NTPC.SetNTPSyncEna(true);  
  }

  if( ! server->hasArg("NTPServerName") || server->arg("NTPServerName") == NULL ) { // If the POST request doesn't have username and password data
      
  } else {
     NTPC.SetServerName( server->arg("NTPServerName") );
  }

  if( ! server->hasArg("ntp_update_delta") || server->arg("ntp_update_delta") == NULL ) { // If the POST request doesn't have username and password data
     
  } else {
    NTPC.SetSyncInterval( server->arg("ntp_update_delta").toInt() );
  }
  NTPC.SaveSettings();
  server->send(200);   
  
 }

/**************************************************************************************************
*    Function      : timezone_update
*    Description   : Parses POST for new timezone settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void timezone_update( ){ /*needs to handel timezoneid */
  if( ! server->hasArg("timezoneid") || server->arg("timezoneid") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
  } else {
   
    Serial.printf("New TimeZoneID: %s\n\r",server->arg("timezoneid").c_str());
    uint32_t timezoneid = server->arg("timezoneid").toInt();
    timec.SetTimeZone( (TIMEZONES_NAMES_t)timezoneid );   
  }
  timec.SaveConfig();
  server->send(200);    

 }

 
/**************************************************************************************************
*    Function      : timezone_overrides_update
*    Description   : Parses POST for new timzone overrides
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
 void timezone_overrides_update( ){ /* needs to handle DLSOverrid,  ManualDLS, dls_offset, ZONE_OVERRRIDE and GMT_OFFSET */

  bool DLSOverrid=false;
  bool ManualDLS = false;
  bool ZONE_OVERRRIDE = false;
  int32_t gmt_offset = 0;
  DLTS_OFFSET_t dls_offsetidx = DLST_OFFSET_0;
  if( ! server->hasArg("dlsdis") || server->arg("dlsdis") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    DLSOverrid=true;  
  }

  if( ! server->hasArg("dlsmanena") || server->arg("dlsmanena") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    ManualDLS=true;  
  }

  if( ! server->hasArg("ZONE_OVERRRIDE") || server->arg("ZONE_OVERRRIDE") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    ZONE_OVERRRIDE=true;  
  }

  if( ! server->hasArg("gmtoffset") || server->arg("gmtoffset") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    gmt_offset = server->arg("gmtoffset").toInt();
  }

  if( ! server->hasArg("dlsmanidx") || server->arg("dlsmanidx") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    dls_offsetidx = (DLTS_OFFSET_t) server->arg("dlsmanidx").toInt();
  }
  timec.SetGMT_Offset(gmt_offset);
  timec.SetDLS_Offset( (DLTS_OFFSET_t)(dls_offsetidx) );
  timec.SetAutomaticDLS(!DLSOverrid);
  timec.SetManualDLSEna(ManualDLS);
  timec.SetTimeZoneManual(ZONE_OVERRRIDE);

 
  timec.SaveConfig();
  server->send(200);    

  
 }

/**************************************************************************************************
*    Function      : update_display_sleepmode
*    Description   : Parses POST for new sleeptime 
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void ledactivespan_send( void ){

  const size_t capacity = 2*JSON_ARRAY_SIZE(3) + 6*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
  String response ="";
  lightactivespan_t spandata = LED_GetLEDActiveSpan();
  
  DynamicJsonDocument root(capacity);  
  JsonObject spanstart = root.createNestedObject("spanstart");
  spanstart["hour"] = spandata.start.hour;
  spanstart["minute"] = spandata.start.minute;
  spanstart["second"] = spandata.start.second;
  
  JsonObject spanend = root.createNestedObject("spanend");
  spanend["hour"] = spandata.end.hour;
  spanend["minute"] = spandata.end.minute;
  spanend["second"] = spandata.end.second;
  root["ena"] = spandata.ena;

  serializeJson(root,response);
  sendData(response);
}




/**************************************************************************************************
*    Function      : update_ledactivespan
*    Description   : Parses POST for new sleeptime 
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_ledactivespan(){
  lightactivespan_t span;
  span.ena=false;
 
  Serial.println("Update Sleep Time");
  
  if( ! server->hasArg("enable") || server->arg("enable") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
    if(server->arg("enable") == "true"){
      span.ena = true;
    } else {
      span.ena = false;
    }
    
  }
  /* we also need the times for the mode */
  if( ! server->hasArg("silent_start") || server->arg("silent_start") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
    if(server->arg("silent_start").length()!=8){
      Serial.println("silent_start len failed");
    } else {
    
      String hour=server->arg("silent_start").substring(0,2);
      String minute=server->arg("silent_start").substring(3,5);
      String second=server->arg("silent_start").substring(6,8);
      span.start.hour = hour.toInt();
      span.start.minute = minute.toInt();
      span.start.second = second.toInt();
     
  
    }
    
  }

  if( ! server->hasArg("silent_end") || server->arg("silent_end") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
     if(server->arg("silent_end").length()!=8){
      Serial.println("silent_end len failed");
    } else {
    
      String hour=server->arg("silent_end").substring(0,2);
      String minute=server->arg("silent_end").substring(3,5);
      String second=server->arg("silent_start").substring(6,8);
      span.end.hour = hour.toInt();
      span.end.minute = minute.toInt();
      span.end.second = second.toInt();
     
    }
  }

/* we need to setup something for the led to use this values */
  LED_SetLEDActiveSpan(span);
  server->send(200);    
}



/**************************************************************************************************
*    Function      : update_notes
*    Description   : Parses POST for new notes
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_notes(){
  char data[501]={0,};
  if( ! server->hasArg("notes") || server->arg("notes") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Notes: %s\n\r",server->arg("notes").c_str());
    /* direct commit */
    uint32_t str_size = server->arg("notes").length();
    if(str_size<501){
      strncpy((char*)data,server->arg("notes").c_str(),501);
      eepwrite_notes((uint8_t*)data,501);
    } else {
      Serial.println("Note > 512 char");
    }
  }

  server->send(200);    
}

/**************************************************************************************************
*    Function      : read_notes
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : Retunrs the notes as plain text
**************************************************************************************************/ 
void read_notes(){
  char data[501]={0,};
  eepread_notes((uint8_t*)data,501);
  sendData(data);    
}


/**************************************************************************************************
*    Function      : led_update
*    Description   : Parses POST for new led settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void led_update( ){ /* sets the new PWM Value */
  if( ! server->hasArg("channel0") || server->arg("channel0") == NULL ) { 
    /* we are missong something here */
  } else {
   
    uint32_t value = server->arg("channel0").toInt();
    if(value > UINT16_MAX ){
      value = UINT16_MAX;
    }
    LED_SetValue( LED_CH0, value );
    
  }

  if( ! server->hasArg("channel1") || server->arg("channel1") == NULL ) { 
    /* we are missong something here */
  } else {
   
     uint32_t value = server->arg("channel1").toInt();
     if(value > UINT16_MAX ){
      value = UINT16_MAX;
     }
     LED_SetValue( LED_CH1, value );
    
  }

  if( ! server->hasArg("channel2") || server->arg("channel2") == NULL ) { 
    /* we are missong something here */
  } else {
   
     uint32_t value = server->arg("channel2").toInt();
     if(value > UINT16_MAX ){
      value = UINT16_MAX;
     }
     LED_SetValue( LED_CH2, value );
    
  }

  if( ! server->hasArg("channel3") || server->arg("channel3") == NULL ) { 
    /* we are missong something here */
  } else {
   
     uint32_t value = server->arg("channel3").toInt();
     if(value > UINT16_MAX ){
      value = UINT16_MAX;
     }
     LED_SetValue( LED_CH3, value );
    
  }

  if( ! server->hasArg("intensity") || server->arg("intensity") == NULL ) { 
    /* we are missong something here */
  } else {
   
     uint32_t value = server->arg("intensity").toInt();
     if(value > UINT16_MAX ){
      value = UINT16_MAX;
     }
     LED_SetIntensity( value );
    
  }

    
  ws_sendledvalues();
  server->send(200);    

 }

/**************************************************************************************************
*    Function      : led_status
*    Description   : Process GET for led settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void led_status( ){ 

  const size_t capacity = JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(3);
  String response ="";
DynamicJsonDocument root(capacity);

  root["intensity"] = LED_GetIntensity();
  
  JsonArray led_value = root.createNestedArray("led_value");
  for(uint32_t i=0;i<LED_CNT;i++){
    uint16_t value = LED_GetValue((LEDCH_t)i);
    led_value.add(value);  
  }
  serializeJson(root,response);
  sendData(response);
}


void mqttsettings_update( ){
 
  mqttsettings_t Data = eepread_mqttsettings();
  
  if( ! server->hasArg("MQTT_USER") || server->arg("MQTT_USER") == NULL ) { 
    /* we are missong something here */
  } else { 
    String value = server->arg("MQTT_USER");
    strncpy(Data.mqttusername, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_PASS") || server->arg("MQTT_PASS") == NULL ) { 
    /* we are missong something here */
  } else { 
    String value = server->arg("MQTT_PASS");
    strncpy(Data.mqttpassword, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_SERVER") || server->arg("MQTT_SERVER") == NULL ) { 
    /* we are missong something here */
  } else { 
    String value = server->arg("MQTT_SERVER");
    strncpy(Data.mqttservename, value.c_str(),128);
  }

  if( ! server->hasArg("MQTT_HOST") || server->arg("MQTT_HOST") == NULL ) { 
    /* we are missong something here */
  } else { 
    String value = server->arg("MQTT_HOST");
    strncpy(Data.mqtthostname, value.c_str(),64);
  }

  if( ! server->hasArg("MQTT_PORT") || server->arg("MQTT_PORT") == NULL ) { 
    /* we are missong something here */
  } else { 
    int32_t value = server->arg("MQTT_PORT").toInt();
    if( (value>=0) && ( value<=UINT16_MAX ) ){
      Data.mqttserverport = value;
    }
  }
  
  if( ! server->hasArg("MQTT_TOPIC") || server->arg("MQTT_TOPIC") == NULL ) { 
    /* we are missong something here */
  } else { 
    String value = server->arg("MQTT_TOPIC");
    strncpy(Data.mqtttopic, value.c_str(),500);
  }

  if( ! server->hasArg("MQTT_ENA") || server->arg("MQTT_ENA") == NULL ) { 
    /* we are missing something here */
  } else { 
    bool value = false;
    if(server->arg("MQTT_ENA")=="true"){
      value = true;
    }
    Data.enable = value;
  }
  
  /* write data to the eeprom */
  eepwrite_mqttsettings(Data);
  
  xTaskNotify( MQTTTaskHandle, 0x01, eSetBits );
  server->send(200); 

}


/*
  char mqttservename[129];
  uint16_t mqttserverport;
  char mqttusername[129];
  char mqttpassword[129];
  char mqtttopic[501];
  char mqtthostname[65];
 */
void read_mqttsetting(){
   String response ="";
  mqttsettings_t Data = eepread_mqttsettings();
  DynamicJsonDocument root(2000);
  

  root["mqttena"]= (bool)(Data.enable);
  root["mqttserver"] = String(Data.mqttservename);
  root["mqtthost"] = String(Data.mqtthostname);
  root["mqttport"] = Data.mqttserverport;
  root["mqttuser"] = String(Data.mqttusername);
  root["mqtttopic"] = String(Data.mqtttopic);
  if(Data.mqttpassword[0]!=0){
    root["mqttpass"] = "********";
  } else {
    root["mqttpass"] ="";
  }

  serializeJson(root,response);
  sendData(response);

}
