#ifndef HAL_H
#define HAL_H

#include <message.h>
#include "messagebase.h"

/***********************************************************************
  
  This module encapsulate all hardware related logic, including:
  
  1. button operation
  2. power on / off, no matter it is triggered by user action or by power management policy.
  3. leds play, application has no idea about how to display the information, it just call logical state indication.
  4. charging status
  5. battery reading and power management
  
  **********************************************************************/


/***********************************************************************
  
  HAL send switching on/off message to profile layer, these messages
  represent an external event.
  
  **********************************************************************/

#define K_VoltageInit 0xFFFF

typedef struct {
	
	/** true for forced_restart, false for normal start **/
	bool forced_restart;
	
} hal_message_switching_on_t;

typedef enum {
	
	HAL_MESSAGE_FUNCTION_BUTTON_PRESS = HAL_MESSAGE_BASE,
	HAL_MESSAGE_SWITCHING_ON,
	HAL_MESSAGE_SWITCHING_OFF, /** switching off means it's going to switching off, no matter what apps do. **/
	HAL_MESSAGE_TOP
	
} HAL_MESSAGE_ENUM;

typedef enum {
	
	SHORT_ONCE,
	SHORT_TWICE,
	SHORT_THRICE,
	
	LONG_ONCE,
	LONG_TWICE,
	LONG_THRICE
	
} hal_beep_pattern_t;

/** hal layer init **/
void hal_init(Task profileTask, Task functionTask);

/** return hal task **/
Task getHalTask(void);

#endif /* HAL_H */

