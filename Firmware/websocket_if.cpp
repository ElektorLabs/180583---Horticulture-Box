#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "led.h"
#include "websocket_if.h"



WebSocketsServer webSocket = WebSocketsServer(8080);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void ws_service_begin(){
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
}

void ws_task( void ){
  webSocket.loop();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    DynamicJsonDocument root(1024);
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
              ws_sendledvalues(); 
            }
            break;
        case WStype_TEXT:
                printf("[%u] get Text: %s\n", num, payload);
                if(length<1024){
                  Serial.println("Parse json");
                  DeserializationError error = deserializeJson(root, payload);
                  if(error){
                    return;
                  }
                  if(true == root.containsKey("ch0") ){
                     uint16_t ch0  = root["ch0"]; 
                     /* Update led settings */
                     LED_SetValue(LED_CH0,ch0);
                  }

                  if(true == root.containsKey("ch1") ){
                     uint16_t ch1  = root["ch1"]; 
                     /* Update led settings */ 
                     LED_SetValue(LED_CH1,ch1);                   
                  }

                  if(true == root.containsKey("ch2") ){
                     uint16_t ch2  = root["ch2"];
                     /* Update led settings */ 
                     LED_SetValue(LED_CH2,ch2);
                    
                  }

                  if(true == root.containsKey("ch3") ){
                     uint16_t ch3  = root["ch3"];
                     /* Update led settings */ 
                     LED_SetValue(LED_CH3,ch3);
                    
                  }

                  if(true == root.containsKey("intensity") ){
                     uint16_t intensity  = root["intensity"];
                     /* Update led settings */ 
                      LED_SetIntensity( intensity );
                    
                  }
                  ws_sendledvalues();
                }
              
            break;
        case WStype_BIN:
                printf("[%u] get binary length: %u\n", num, length);
            break;

       default:{
        
       }break;
    }

}


void ws_sendledvalues( ){

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
  webSocket.broadcastTXT(response);
}
