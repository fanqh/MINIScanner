#ifndef INDICATION_H
#define INDICATION_H

#include "spp_dev_b_leds.h"

/***************************************************
 
	indication definition 

Indication Logic

First, check hal state.

1.Hal init
1.charging? Display batt level
2.not charging? Display batt level
2.Activating, no leds, just beep
3.active
1.bt ???¡ì?C pairable, connecting, Fast blue flash, no batt or charging indication
2.connected - echo, Fast white flash, no batt or charging indication
3.connected ???¡ì?C pipe, Slow blue flash, with or without batt, depends on charging
4.disconnecting, ready, connection off, so no blue; if charging, persistent batt, otherwise, off.
4.Deactivating, no leds, just beep.


IND_NONE
IND_UNKNOWN
IND_BATT_LOW
IND_BATT_MODEST
IND_BATT_FINE
IND_BLUE_FAST_FLASH
IND_WHITE_FAST_FLASH
IND_SLOW_BLUE_FLASH
IND_BATT_LOW_BLUE_BREATH
IND_BATT_MODEST_BLUE_BREATH
IND_BATT_FINE_BLUE_BREATH

****************************************************/

#define IND_NONE 						ALL_LEDS_OFF		/** ready & disconnecting **/
#define IND_UNKNOWN						WHITE_ON
#define IND_BATT_LOW					RED_ON
#define IND_BATT_MODEST					RED_GREEN_ON
#define IND_BATT_FINE					GREEN_ON
#define IND_INQUIRY_PAGE_ON				BLUE_FAST_FLASH		/** pairable & connecting **/

#define IND_ACTIVATING_WAITING			BLUE_ON
#define IND_SCANNING					RAINBOW

#define IND_CONNECTED_BATT_FINE			BLUE_SLOW_BURST
#define IND_CONNECTED_BATT_LOW			RED_SLOW_BURST
#define IND_DISCONNECTED_BATT_FINE		BLUE_SLOW_FLASH
#define IND_DISCONNECTED_BATT_LOW		RED_SLOW_FLASH

/***************************************************
  
  This function update leds, call this function when:
  
  1. PIO change
  2. Battery Change
  3. state change
  
****************************************************/

void update_indication(void);




#endif /* INDICATION_H */



