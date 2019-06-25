#ifndef _LED_H_
#define _LED_H_

#include <Arduino.h>
#include "timecore.h"
/**************************************************************************************************
 *    typdef      : enum
 *    Remarks     : Used for the channel addressing
 **************************************************************************************************/
typedef enum {
  LED_CH0=0,
  LED_CH1,
  LED_CH2,
  LED_CH3,
  LED_CNT
} LEDCH_t;

typedef struct{
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} timestamp_t;

typedef struct{
  timestamp_t start;
  timestamp_t end;
  bool ena;
} lightactivespan_t;

typedef struct{
  uint16_t led_value[LED_CNT];
  uint16_t intensity;
} ledsettings_t;


/**************************************************************************************************
 *    Function      : LED_Setup
 *    Description   : Setup for the LED PWM
 *    Input         : none 
 *    Output        : none
 *    Remarks       : Loads the last stored values 
 **************************************************************************************************/
void LED_Setup( void );

/**************************************************************************************************
 *    Function      : LED_SetValue
 *    Description   : Sets the value for a given LED 
 *    Input         : LEDCH_t , uint16_t  
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetValue( LEDCH_t channel, uint16_t value );

/**************************************************************************************************
 *    Function      : LED_GetValue
 *    Description   : Gets the value for a given LED 
 *    Input         : LEDCH_t 
 *    Output        : uint16_t  
 *    Remarks       : none
 **************************************************************************************************/
uint16_t LED_GetValue( LEDCH_t channel);

/**************************************************************************************************
 *    Function      : LED_SetValue
 *    Description   : Sets the value for a given LED 
 *    Input         : uint16_t  
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetIntensity( uint16_t value );

/**************************************************************************************************
 *    Function      : LED_GetValue
 *    Description   : Gets the value for a given LED 
 *    Input         : void 
 *    Output        : uint16_t  
 *    Remarks       : none
 **************************************************************************************************/
uint16_t LED_GetIntensity( void);


/**************************************************************************************************
 *    Function      : LED_SaveSettings
 *    Description   : Saves the current settigs to EEPROM / Flash
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_SaveSettings( void );


/**************************************************************************************************
 *    Function      : LED_LoadSettings
 *    Description   : Loads the current settigs to EEPROM / Flash
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_LoadSettings( void );

/**************************************************************************************************
 *    Function      : LED_GetDefaultSettings
 *    Description   : Returns default settings
 *    Input         : none 
 *    Output        : ledsettings_t  
 *    Remarks       : none
 **************************************************************************************************/
ledsettings_t LED_GetDefaultSettings( void );


/**************************************************************************************************
 *    Function      : LED_Task
 *    Description   : Housekeeping for the leds
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_Task( void );

/**************************************************************************************************
 *    Function      : LED_Tick
 *    Description   : Used for internal timing 
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : To be called periodically ( 1s )
 **************************************************************************************************/
void LED_Tick( void );

/**************************************************************************************************
 *    Function      : LED_GetActiveTimespan
 *    Description   : Used to retrive the timespan where the leds should be on 
 *    Input         : none 
 *    Output        : lightoffspan_t  
 *    Remarks       : none
 **************************************************************************************************/
lightactivespan_t LED_GetLEDActiveSpan( void );

/**************************************************************************************************
 *    Function      : LED_SetActiveTimespan
 *    Description   : Used to set the timespan where the leds should be on 
 *    Input         : none 
 *    Output        : none  
 *    Remarks       : none
 **************************************************************************************************/
void LED_SetLEDActiveSpan( lightactivespan_t span );

/**************************************************************************************************
 *    Function      : GetSpanActive
 *    Description   : Determins if we are within the span
 *    Input         : datum_t d
 *    Output        : bool
 *    Remarks       : none
 *************************************************************************************************
 */
bool GetSpanActive( datum_t d );

#endif

