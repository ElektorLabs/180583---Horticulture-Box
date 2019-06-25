
/* 
 * This is the arduino sketch for 180583 ( horticulture box )
 * to compile the code you need to include some libarys
 * Time by Michael Margolis ( Arduino )
 * Arduino JSON 6.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * WebSockets by Markus Sattler
 * NTP client libary from https://github.com/gmag11/NtpClient/
 * 
 * Hardware used: ESP32-WROVER
 * 
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>

#include <TimeLib.h>
#include <Ticker.h>
#include <PubSubClient.h>                   // MQTT-Library

#include "ArduinoJson.h"
#include "NTP_Client.h"
#include "timecore.h"
#include "datastore.h"
#include "websocket_if.h"
#include "led.h"




Timecore timec;
NTP_Client NTPC;

Ticker TimeKeeper;
Ticker Fader;
Ticker LedBlink;
/* 63 Char max and 17 missign for the mac */
TaskHandle_t Task1;
TaskHandle_t MQTTTaskHandle;
void toggleWiFiLed( void );
/**************************************************************************************************
 *    Function      : setup
 *    Description   : Get all components in ready state
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void setup()
{
  wifi_mode_t mode;
  uint8_t macAddr[6];
  char stringBuffer[33]; 
  /* First we setup the serial console with 115k2 8N1 */
  Serial.begin (115200);
  Serial.println("Firmware build __DATE__ __TIME__");
  /* The next is to initilaize the datastore, here the eeprom emulation */
  datastoresetup();
  /* This is for the flash file system to access the webcontent */
  SPIFFS.begin();
  /* Set the LED output */
  Serial.printf("Setup LED");
  LED_Setup();
  /* And here also the "Terminal" to log messages, ugly but working */
  Serial.println(F("Booting..."));
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);

  digitalWrite(25, HIGH);
  digitalWrite(26,HIGH);
  digitalWrite(27,HIGH);
  for(uint32_t i=0;i<2500;i++){
    if(digitalRead(0)==LOW){
      Serial.println(F("Erase EEPROM"));
      erase_eeprom();  
      break;
    }
    delay(1);
  }
  
  digitalWrite(25, LOW);
  digitalWrite(26,LOW);
  digitalWrite(27,LOW);
  
  Serial.println(F("Init WiFi"));   
  initWiFi();
  if( ESP_OK==esp_wifi_get_mode(&mode) ){
    if(mode == WIFI_MODE_STA){
       digitalWrite(25, HIGH);
    } else {
      LedBlink.attach_ms(500,toggleWiFiLed);
    }

  } else {
    /* This is a problem */
  }

   /* We read the Config from flash */
  Serial.println(F("Read Timecore Config"));
  timecoreconf_t cfg = read_timecoreconf();
  timec.SetConfig(cfg);
  /* The NTP is running in its own task */
   xTaskCreatePinnedToCore(
      NTP_Task,       /* Function to implement the task */
      "NTP_Task",  /* Name of the task */
      10000,          /* Stack size in words */
      NULL,           /* Task input parameter */
      1,              /* Priority of the task */
      NULL,           /* Task handle. */
      1); 
          
   xTaskCreatePinnedToCore(
   MQTT_Task,
   "MQTT_Task",
   10000,
   NULL,
   1,
   &MQTTTaskHandle,
   1);
  /* We now start the Websocket part */
  ws_service_begin();
  /* Now we start with the config for the Timekeeping and sync */
  TimeKeeper.attach_ms(1000, _1SecondTick);
  Fader.attach_ms(100, _100msTick);

  /* We here now attach the OTA part */

   // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  WiFi.softAPmacAddress(macAddr);
  snprintf(stringBuffer,32,"Elektor HC-LED %02x:%02x:%02x",macAddr[3],macAddr[4],macAddr[5]);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname((const char*)stringBuffer);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Display specific mod to show four digit Password
      Serial.println("Start updating " + type);
      digitalWrite(26,HIGH);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      digitalWrite(26,LOW);
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      digitalWrite(26,HIGH);
    })
    .onError([](ota_error_t error) {
      digitalWrite(26,LOW);
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();


}

void toggleWiFiLed( void ){
  digitalWrite(25,!digitalRead(25));
}


/**************************************************************************************************
 *    Function      : _1SecondTick
 *    Description   : Runs all fnctions inside once a second
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void _100msTick( void ){
     LED_Tick();
}

/**************************************************************************************************
 *    Function      : _1SecondTick
 *    Description   : Runs all fnctions inside once a second
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void _1SecondTick( void ){
     timec.RTC_Tick();  
     LED_Tick();
}



/**************************************************************************************************
 *    Function      : loop
 *    Description   : Superloop
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void loop()
{  
  NetworkTask();
  ws_task();
  LED_Task();
  ArduinoOTA.handle();
}

void NTP_Task( void * param){
  Serial.println(F("Start NTP Task now"));
  NTPC.ReadSettings();
  NTPC.begin( &timec );
  NTPC.Sync();

  /* As we are in a sperate thread we can run in an endless loop */
  while( 1==1 ){
    /* This will send the Task to sleep for one second */
    vTaskDelay( 1000 / portTICK_PERIOD_MS );  
    NTPC.Tick();
    NTPC.Task();
  }
}

void MQTT_Task( void* prarm ){
   const size_t capacity = JSON_OBJECT_SIZE(4);
   DynamicJsonDocument root(capacity);
   String JsonString = "";
   uint32_t ulNotificationValue;
   int32_t last_message = millis();
   mqttsettings_t Settings = eepread_mqttsettings();
                         
   Serial.println(F("MQTT Thread Start"));
   WiFiClient espClient;                       // WiFi ESP Client  
   PubSubClient client(espClient);             // MQTT Client 
   client.setCallback(callback);             // define Callback function
   while(1==1){

   /* if settings have changed we need to inform this task that a reload and reconnect is requiered */ 
   ulNotificationValue = ulTaskNotifyTake( pdTRUE, 0 );

   if( (ulNotificationValue&0x01) != 0 ){
      Serial.println(F("Reload MQTT Settings"));
      /* we need to reload the settings and do a reconnect */
      if(true == client.connected() ){
        client.disconnect();
      }
      Settings = eepread_mqttsettings();
   }

   if(!client.connected()) {             
        /* sainity check */
        digitalWrite(27,LOW);
        if( (Settings.mqttserverport!=0) && (Settings.mqttservename[0]!=0) && ( Settings.enable != false ) ){
      
              Serial.print("Connecting to MQTT...");  // connect to MQTT
              client.setServer(Settings.mqttservename, Settings.mqttserverport); // Init MQTT     
              if (client.connect(Settings.mqtthostname, Settings.mqttusername, Settings.mqttpassword)) {
                Serial.println("connected");          // successfull connected  
                client.subscribe(Settings.mqtttopic);             // subscibe MQTT Topic
                digitalWrite(27,HIGH);
              } else {
                Serial.print("failed with state ");   // MQTT not connected       
                digitalWrite(27,LOW);
           
              }
        }
   } else{
        digitalWrite(27,HIGH);
        client.loop();                            // loop on client
   }
   delay(10);
 }
}

/***************************
 * callback - MQTT message
 ***************************/
void callback(char* topic, byte* payload, unsigned int length) {
 uint16_t led_val =0;
 
 if(length < 1024 ){ /* we only process messages up to 1024 elements */
  DynamicJsonDocument doc(1024);
 
  if(0 != deserializeJson(doc, payload,length) ){
    return;
  }
  
 
    JsonObject light = doc["light"];
      
    JsonVariant light_ch0_var = light["ch0"];
    if(false == light_ch0_var.isNull() ){
      int light_ch0 = light_ch0_var.as<int>();
      if ( (light_ch0 >= 0 ) && (light_ch0 <= UINT16_MAX ) ){
           
           LED_SetValue( LED_CH0, light_ch0 );
      } else {
        /* We ignore the value */
      }
    }

  
    JsonVariant light_ch1_var = light["ch1"];
    if(false == light_ch1_var.isNull() ){
      int light_ch1 = light_ch1_var.as<int>();
      if ( (light_ch1 >= 0 ) && (light_ch1 <= UINT16_MAX ) ){
           LED_SetValue( LED_CH1, light_ch1 );
          
      } else {
        /* We ignore the value */
      }
    }
  
    JsonVariant light_ch2_var = light["ch2"];
    if(false == light_ch2_var.isNull() ){
      int light_ch2 = light_ch2_var.as<int>();
      if ( (light_ch2 >= 0 ) && (light_ch2 <= UINT16_MAX ) ){
           LED_SetValue( LED_CH2, light_ch2 );
          
      } else {
        /* We ignore the value */
      }
    }
  
  
    JsonVariant light_ch3_var = light["ch3"];
    if(false == light_ch3_var.isNull() ){
      int light_ch3 = light_ch3_var.as<int>();
      if ( (light_ch3 >= 0 ) && (light_ch3 <= UINT16_MAX ) ){
           LED_SetValue( LED_CH3, light_ch3 );
           Serial.print("Update ch3");
      } else {
        /* We ignore the value */
      }
    }
  
   JsonVariant light_intense_var = light["intense"];
   if(false == light_intense_var.isNull() ){
    int light_intense = light_intense_var.as<int>();
    if ( (light_intense >= 0 ) && (light_intense <= UINT16_MAX ) ){
         LED_SetIntensity( light_intense );
        
    } else {
      /* We ignore the value */
    }
   }
 

   JsonObject timer = doc["timer"];
   lightactivespan_t span = LED_GetLEDActiveSpan();

    JsonVariant timer_enable_var = timer["enable"]; 
    if(false == timer_enable_var.isNull() ){      
        bool timer_enable = timer_enable_var.as<bool>();
        span.ena = timer_enable;
      
   
      JsonObject timer_start = timer["start"];

      JsonVariant timer_start_hour_var = timer_start["hour"]; 
      JsonVariant timer_start_minute_var = timer_start["minute"]; 
      JsonVariant timer_start_second_var = timer_start["second"]; 
      if( (false == timer_start_hour_var.isNull() ) &&
          (false == timer_start_minute_var.isNull() ) &&                                               
          (false == timer_start_second_var.isNull() ) ){
        int timer_start_hour = timer_start_hour_var.as<int>();
        int timer_start_minute = timer_start_minute_var.as<int>();
        int timer_start_second = timer_start_second_var.as<int>();
        /* rangecheck */
        if( ( ( timer_start_hour >=0 ) && ( timer_start_hour <=23 ) )     && 
            ( ( timer_start_minute >=0 ) && ( timer_start_minute <=59 ) ) && 
            ( ( timer_start_second >=0 ) && ( timer_start_second <=59 ) ) ){
                span.start.hour = timer_start_hour;
                span.start.minute = timer_start_minute;
                span.start.second = timer_start_second;
               
            }
                  
        
        }
      }
    
      JsonObject timer_end = timer["end"];
      JsonVariant timer_end_hour_var = timer_end["hour"]; 
      JsonVariant timer_end_minute_var = timer_end["minute"]; 
      JsonVariant timer_end_second_var = timer_end["second"]; 
      if( (false == timer_end_hour_var.isNull() ) &&
          (false == timer_end_minute_var.isNull() ) &&                                               
          (false == timer_end_second_var.isNull() ) ){
       int timer_end_hour = timer_end_hour_var.as<int>();
        int timer_end_minute = timer_end_minute_var.as<int>();
        int timer_end_second = timer_end_second_var.as<int>();
         /* rangecheck */
        if( ( ( timer_end_hour >=0 ) && ( timer_end_hour <=23 ) )     && 
            ( ( timer_end_minute >=0 ) && ( timer_end_minute <=59 ) ) && 
            ( ( timer_end_second >=0 ) && ( timer_end_second <=59 ) ) ){
                span.end.hour = timer_end_hour;
                span.end.minute = timer_end_minute;
                span.end.second = timer_end_second;
               
            }
                  
       }
   
    LED_SetLEDActiveSpan( span );
  }
}




  









 
