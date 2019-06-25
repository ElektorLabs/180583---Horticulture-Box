#include "timecore.h"
#include "datastore.h"
#include "led.h"

// setting PWM properties
// We use 5kHz for the PWM
#define PWM_FREQ ( 244 )
// We use 16 Bit resulution 
#define PWM_RES ( 8 )


typedef struct {
  LEDCH_t ledcch;
  uint8_t physicalpin;
} LED_MAP_t;

/* This is for the cannelmapping , use IOxx for the PIN No.*/
const LED_MAP_t LED_MAPPING[LED_CNT] = {
 {.ledcch=LED_CH0,.physicalpin=02},
 {.ledcch=LED_CH1,.physicalpin=15},
 {.ledcch=LED_CH2,.physicalpin=12},
 {.ledcch=LED_CH3,.physicalpin=13}
};

volatile ledsettings_t settings;

volatile int32_t delayedsave_timer;
volatile lightactivespan_t ActiveTimespan;

volatile uint16_t soft_intensity=0;

extern Timecore timec;

/**************************************************************************************************
 *    Function      : LED_WriteToChannel ( prototype )
 *    Description   : Converts the Input to the used Bitwidth for the PWM
 *    Input         : uint16_t led_value, uint8_t channel 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_WriteToChannel(uint16_t led_value, uint8_t channel);

/**************************************************************************************************
 *    Function      : LED_ConvertToMappingValue ( prototype ) 
 *    Description   : Converts the Input to the used Bitwidth for the PWM
 *    Input         : uint16_t led_value
 *    Output        : uint16_t led_value
 *    Remarks       : Mapps this to the supported range
 **************************************************************************************************/
uint16_t LED_ConvertToMappingValue(uint16_t led_value);

/**************************************************************************************************
 *    Function      : LED_Setup
 *    Description   : Setup for the LED PWM
 *    Input         : none 
 *    Output        : none
 *    Remarks       : Loads the last stored values 
 **************************************************************************************************/
void LED_Setup( void ){
  for(uint32_t i=0; i < ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) ) ; i++ ){ 
     ledcSetup(LED_MAPPING[i].ledcch, PWM_FREQ ,PWM_RES );
     ledcAttachPin(LED_MAPPING[i].physicalpin, LED_MAPPING[i].ledcch);
     ledcWrite(LED_MAPPING[i].ledcch, 0); 
  }
  LED_LoadSettings();
  delayedsave_timer=-1;
}

/**************************************************************************************************
 *    Function      : LED_SetValue
 *    Description   : Sets the value for a given LED 
 *    Input         : uint8_t , uint16_t  
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetValue( LEDCH_t channel, uint16_t value ){
  if( channel < ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) )  ){
    settings.led_value[channel]=value;
  }
  delayedsave_timer=5;
}

/**************************************************************************************************
 *    Function      : LED_GetValue
 *    Description   : Gets the value for a given LED 
 *    Input         : uint8_t 
 *    Output        : uint16_t  
 *    Remarks       : none
 **************************************************************************************************/
uint16_t LED_GetValue( LEDCH_t channel){
  uint16_t Result=0;
  if( channel <  ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) )  ){
      Result = settings.led_value[channel];
  } 
  
  return Result;
}

/**************************************************************************************************
 *    Function      : LED_GetHwValue
 *    Description   : Gets the value for a given LED 
 *    Input         : uint8_t 
 *    Output        : uint16_t  
 *    Remarks       : none
 **************************************************************************************************/
uint16_t LED_GetHwValue( LEDCH_t channel){
  uint16_t Result=0;
  if( channel <  ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) )  ){
    Result = ledcRead(LED_MAPPING[channel].ledcch); 
  } 
  
  return Result;
}

/**************************************************************************************************
 *    Function      : LED_SetIntensity
 *    Description   : Sets the intensity for all channels
 *    Input         : uint16_t  
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetIntensity( uint16_t value ){
  Serial.printf("Write intensity %i",value);
  settings.intensity = value;
  delayedsave_timer=5;
}

/**************************************************************************************************
 *    Function      : LED_GetIntensity
 *    Description   : Gets the Intensity for the LEDs
 *    Input         : none 
 *    Output        : uint16_t  
 *    Remarks       : none
 **************************************************************************************************/
uint16_t LED_GetIntensity( void){
  Serial.printf("Read intensity %i",settings.intensity);
  return settings.intensity;
}

/**************************************************************************************************
 *    Function      : LED_SaveSettings
 *    Description   : Saves the current settigs to EEPROM / Flash
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_SaveSettings( void ){
  ledsettings_t save_settings;
  memcpy((void*)(&save_settings), (const void*)(&settings), sizeof(ledsettings_t));
  eepwrite_ledsettings(save_settings);

}


/**************************************************************************************************
 *    Function      : LED_LoadSettings
 *    Description   : Loads the current settigs to EEPROM / Flash
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_LoadSettings( void ){
   ledsettings_t load_settings;
  /* Load settings from EEPROM */
  load_settings = eepread_ledsettings();  
  memcpy((void*)(&settings), (const void*)(&load_settings), sizeof(ledsettings_t));
  /*                          */
  for(uint32_t i=0;i <  ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) ); i++  ){
    LED_WriteToChannel(settings.led_value[i] ,LED_MAPPING[i].ledcch );
    
  } 
}

/**************************************************************************************************
 *    Function      : LED_WriteToChannel
 *    Description   : Converts the Input to the used Bitwidth for the PWM
 *    Input         : uint16_t led_value, uint8_t channel 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_WriteToChannel(uint16_t led_value, uint8_t channel){
    /* we modify the value with the intensity given for all channels */
   
    uint16_t scaled_val = LED_ConvertToMappingValue( led_value );
    Serial.printf("Write %i to Channel %i\n\r", scaled_val , channel ); 
    ledcWrite(channel,  scaled_val );  
}


/**************************************************************************************************
 *    Function      : LED_ConvertToMappingValue ( prototype ) 
 *    Description   : Converts the Input to the used Bitwidth for the PWM
 *    Input         : uint16_t led_value
 *    Output        : uint16_t led_value
 *    Remarks       : Mapps this to the supported range
 **************************************************************************************************/
uint16_t LED_ConvertToMappingValue(uint16_t led_value){
  uint16_t mapped_value = 0;
  uint32_t scaled_val = led_value*soft_intensity;
  scaled_val = scaled_val >> 16;
  led_value = (uint16_t)(scaled_val); 
  if(PWM_RES != 16 ){
      /* Resulution > 16 Bit */
      if(PWM_RES >= 16){
        mapped_value = ( led_value << (PWM_RES - 16) ); 
      } else {
        mapped_value = ( led_value >> ( 16 - PWM_RES ) );   
      }
  } else {
    mapped_value = led_value;
  }
  /* Last step is to calculate the value modified with the intensity */
  
  return mapped_value;
  
}

/**************************************************************************************************
 *    Function      : LED_GetDefaultSettings
 *    Description   : Returns default settings
 *    Input         : none 
 *    Output        : ledsettings_t  
 *    Remarks       : none
 **************************************************************************************************/
ledsettings_t LED_GetDefaultSettings( void ){
  ledsettings_t retval;
  for(uint32_t i=0;i<LED_CNT;i++){
    retval.led_value[i]=0;  
  }
  retval.intensity = UINT16_MAX;
  return retval;
}

/**************************************************************************************************
 *    Function      : LED_Task
 *    Description   : Housekeeping for the leds
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_Task( void ){
  datum_t d = timec.GetLocalTimeDate(); 
  if(delayedsave_timer==0){
    Serial.println("Save led settings");
    LED_SaveSettings();
    delayedsave_timer--;
  }

  if ( ( ( true == ActiveTimespan.ena ) && ( true == GetSpanActive( d )  ) ) 
       || ( false == ActiveTimespan.ena ) ) {
      /* We are within the span and it is active */
      for(uint32_t i=0;i <  ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) ); i++  ){
        uint16_t hw_val = LED_GetHwValue(LED_MAPPING[i].ledcch);
        uint16_t cf_val = LED_ConvertToMappingValue(settings.led_value[i]);
          if( hw_val != cf_val  ){ 
            Serial.printf(" HW: %i , CONF: %i , CHANNEL %i\n\r",hw_val,cf_val,i);
                LED_WriteToChannel(settings.led_value[i],LED_MAPPING[i].ledcch);
          }
          
      } 
       
    } else {
      /* turn the led off */ 
      for(uint32_t i=0;i <  ( sizeof( LED_MAPPING ) / sizeof(LED_MAP_t) ); i++  ){ 
          if( 0 != LED_GetValue(LED_MAPPING[i].ledcch) ){ 
            LED_WriteToChannel( 0,LED_MAPPING[i].ledcch);
          }
      }  
    }
  
}

/**************************************************************************************************
 *    Function      : LED_Tick
 *    Description   : Used for internal timing 
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : To be called periodically ( 0.1s )
 **************************************************************************************************/
#define FADESTEPVALUE ( 2000 )
void LED_Tick ( void ){
  static uint8_t divider =0;
  
  if(divider >9){
    if(delayedsave_timer>=0){
      delayedsave_timer--;
    }
    divider = 0;  
  } else {
    divider++;
  }
  
  if ( soft_intensity != settings.intensity ){
    if(soft_intensity < settings.intensity ){
      /* We need to add a bit :-) */
      uint16_t delta = settings.intensity - soft_intensity;
      if(delta > FADESTEPVALUE ){
        soft_intensity +=FADESTEPVALUE; 
      } else {
        soft_intensity+=delta;
      }
         
    } else {
      uint16_t delta = soft_intensity - settings.intensity;
      if(delta > FADESTEPVALUE ){
        soft_intensity -=FADESTEPVALUE; 
      } else {
        soft_intensity-=delta;
      }
    }

    
  }
  
}

/**************************************************************************************************
 *    Function      : LED_GetActiveTimespan
 *    Description   : Used to retrive the timespan where the leds should be on 
 *    Input         : none 
 *    Output        : lightoffspan_t  
 *    Remarks       : none
 **************************************************************************************************/
lightactivespan_t LED_GetLEDActiveSpan( void ){
  lightactivespan_t RetVal;
  memcpy((void*)(&RetVal), (const void*)(&ActiveTimespan), sizeof(lightactivespan_t));
  return RetVal;
}

/**************************************************************************************************
 *    Function      : LED_SetActiveTimespan
 *    Description   : Used to set the timespan where the leds should be on 
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetLEDActiveSpan( lightactivespan_t Span ){
  memcpy((void*)(&ActiveTimespan), (const void*)(&Span), sizeof(lightactivespan_t));
  /* We need to save it also to the eeprom */
  
}


/**************************************************************************************************
 *    Function      : GetSpanActive
 *    Description   : Determins if we are within the span
 *    Input         : datum_t d
 *    Output        : bool
 *    Remarks       : none
 *************************************************************************************************
 */
bool GetSpanActive( datum_t d ){
 bool result = false;
  /* Calculate what to do */
  uint32_t startseconds = 0;
  uint32_t endseconds = 0;
  uint32_t currentseconds = 0;
 
 if (ActiveTimespan.ena==true){

  startseconds = (ActiveTimespan.start.hour*3600)+(ActiveTimespan.start.minute*60)+ActiveTimespan.start.second;
  
  endseconds = (ActiveTimespan.end.hour*3600)+(ActiveTimespan.end.minute*60)+ActiveTimespan.end.second;
  
  currentseconds = (d.hour*3600)+(d.minute*60)+d.second;
 
  if(startseconds>endseconds){

    /* the end is in the noon and stop is in the morning */
    if((currentseconds>startseconds)||(currentseconds<endseconds)){
 
      result = true;
    } 
  } else {
 

    /* the end is in the morning and stop is in the noon */ 
    if((currentseconds>startseconds) && ( currentseconds<endseconds)){
 
      result = true;
    } 
  }
 }
 
 return result;
}
