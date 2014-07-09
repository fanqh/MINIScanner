#include <csrtypes.h>
#include <battery.h>
#include <pio.h>
#include <panic.h>
#include "spp_dev_b_buttons.h"
#include "spp_dev_b_leds.h"
#include "hal.h"
#include "hal_config.h"
#include "hal_private.h"
/*#include "battery_probe.h"*/
#include "errman.h"
#include "debug.h"
#include "indication.h"

#define BEEP_ON_TIME	300
#define BEEP_OFF_TIME	100

#define BEEP_ONCE_DURATION 		(BEEP_ON_TIME + BEEP_OFF_TIME)
#define BEEP_TWICE_DURATION		((BEEP_ON_TIME + BEEP_OFF_TIME) * 2)
#define BEEP_THREE_TIME_DURATION	((BEEP_ON_TIME + BEEP_OFF_TIME) * 3)


/** task **/
halTaskData hal;


/** hal task handler **/
static void hal_handler(Task task, MessageId id, Message message);

static void hal_set_state(hal_state_t state);

/** state event handlers **/
static void hal_initialising_handler(Task task, MessageId id, Message message);
static void hal_activating_handler(Task task, MessageId id, Message message);
static void hal_active_handler(Task task, MessageId id, Message message);
static void hal_deactivating_handler(Task task, MessageId id, Message message);

/** state entry/exit functions **/
static void hal_initialising_state_enter(void);
static void hal_initialising_state_exit(void);

static void hal_activating_state_enter(void);
static void hal_activating_state_exit(void);

static void hal_active_state_enter(void);
static void hal_active_state_exit(void);

static void hal_deactivating_state_enter(void);
static void hal_deactivating_state_exit(void);

/** fully initialized means charging state, voltage, and bluetooth status all initialized **/
/** bool isFullyInitialized(void); **/

/** turn on ldo to power myself **/
static void enableLDO(void);

/** turn off ldo to unpower myself **/
static void disableLDO(void);

static void pio_raw_handler(Message message);
static void battery_message_handler(Message message);

static bool powerAllowedToTurnOn(void);
static bool powerAllowedToContinue(void);

		
/** get hal task **/
Task getHalTask() {
	
	return &hal.task;
}

static void hal_set_state(hal_state_t state) {
	
	switch(hal.state) {
		
		case INITIALISING:
			
			hal_initialising_state_exit();
			break;	
		
		case ACTIVATING:
			
			hal_activating_state_exit();
			break;
		
		case ACTIVE:
			
			hal_active_state_exit();
			break;
		
		case DEACTIVATING:
			
			hal_deactivating_state_exit();
			break;
		
		default:
			break;
	}
	
	hal.state = state;
	
	switch(state) {
		
		case INITIALISING:
		
			hal_initialising_state_enter();
			break;
		
		case ACTIVATING:
			
			hal_activating_state_enter();
			break;
		
		case ACTIVE:
			
			hal_active_state_enter();
			break;
		
		case DEACTIVATING:
			
			hal_deactivating_state_enter();
			break;
		
		default:
			break;
	}
}

/** hal task handler **/
static void hal_handler(Task task, MessageId id, Message message) {

	/** old pattern **/
	switch (hal.state) {
		
		case INITIALISING:
			hal_initialising_handler(task, id, message);
			break;
			
		case ACTIVATING:
			hal_activating_handler(task, id, message);
			break;
			
		case ACTIVE:
			hal_active_handler(task, id, message);
			break;
			
		case DEACTIVATING:
			hal_deactivating_handler(task, id, message);
			break;
	}
}

static void hal_initialising_state_enter(void) {
	
	DEBUG(("hal initialising state enter...\n"));
	
	/** init pio **/
	pioInit(&hal.pio_state, getHalTask());
	
	/** init battery lib, set 0 to read once **/
	/*BatteryInit(&hal.battery_state, getHalTask(), BATTERY_READING_SOURCE, BATTERY_POLLING_PERIOD);*/	
	BatteryInit(&hal.battery_state, getHalTask(), BATTERY_READING_SOURCE, 0);
	
	/** floating, if user release button and power cable unplugged, system stop **/
	disableLDO();	
	
	update_indication();
}

static void hal_initialising_state_exit(void) {

	DEBUG(("hal initialising state exit...\n"));

	/** battery power working **/
	enableLDO();
}

static void hal_initialising_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case PIO_RAW:
			{
				DEBUG(("hal initialising state, PIO_RAW message arrived...\n"));
			
				/** update charging state, and no check, even battery low we have nothing to do **/
				pio_raw_handler(message);
				update_indication();
			}
			break;			
		
		case BATTERY_READING_MESSAGE:
			{
				DEBUG(("hal warming-up state, BATTERY_READING_MESSAGE message arrived...\n"));
				/** update battery reading and no check, even battery low we have nothing to do **/
				if (hal.voltage == K_VoltageInit) {

					BatteryInit(&hal.battery_state, getHalTask(), BATTERY_READING_SOURCE, BATTERY_POLLING_PERIOD);
				}
				battery_message_handler(message);
				update_indication();
			}
			break;		
			
		case HAL_POWER_BUTTON_HELD_SHORT:
			{
				DEBUG(("hal initialising state, POWER_BUTTON_HELD_SHORT message arrived...\n"));
			
				/** let error mananger check initialization result, and determing the control flow **/
				initialisationFinished();
				hal_set_state(ACTIVATING);
			}
			break;
			
		case POWER_BUTTON_PRESS:
			
			DEBUG(("hal initialising state, POWER_BUTTON_PRESS message arrived...\n"));

			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT, 0, 2000);
			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_LONG, 0, 8000);
			break;

			
		case POWER_BUTTON_RELEASE:
			
			DEBUG(("hal initialising state, POWER_BUTTON_RELEASE message arrived...\n"));
			
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT);
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_LONG);
			break;
			
		default:
			break;
	}
}

static void hal_activating_state_enter(void) {
	
	DEBUG(("hal activating state enter...\n"));
	update_indication();	

	hal.beep_finished = FALSE;
	hal.power_button_released = FALSE;
	/**
	ledsPlay(ALL_LEDS_OFF);
	**/
	
	ledsPlay(BEEP_TWICE);
    ledsPlay(MOTOR_TWICE);
	
	MessageSendLater(getHalTask(), HAL_ACTIVATING_TIMEOUT, 0, BEEP_TWICE_DURATION + 100);
}

static void hal_activating_state_exit(void) {
	
	DEBUG(("hal activating state exit...\n"));
	
	if (hal.beep_finished == FALSE) {
		
		MessageCancelAll(getHalTask(), HAL_ACTIVATING_TIMEOUT);
	}	
}

static void hal_activating_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case PIO_RAW:
		
			DEBUG(("hal activating state, PIO_RAW message arrived...\n"));
			
			pio_raw_handler(message);
			if (!powerAllowedToTurnOn) {
				
				/***** old comment
				  
				  Theoretically we should do something here because user may unplug the charging cable
				  right after entering this state and the battery is low. But it seems to not
				  triggering severe problem, we can let this issue to be solved in next state, 
				  when next battery update message comes, the program will do the checking.
				  
				  Other possible solutions: 
				  1. in initialising state, device is allowed to be powered on when battery is not too low,
				  even if it is in charging. This is common design in mobile phone.
				  2. checking it when entering the next state.
				  
				  In fact, we should go back to initialising state, but up to now, nothing to do. :)
				  
				  ****/
				
				/**** new comment, UGLEE 20110710
				  
				  if beep is not finished, we do nothing. beep_finished event should process batt-low condition ****/
				
				if (hal.beep_finished) {
					
					hal_set_state(DEACTIVATING);
				}
			}
			
			break;
			
		case BATTERY_READING_MESSAGE:
			
			DEBUG(("hal activating state, BATTERY_READING_MESSAGE message arrived...\n"));
			
			battery_message_handler(message);
			
			/** see above comment on PIO_RAW case **/
			if (!powerAllowedToTurnOn() && hal.beep_finished) {
				
				hal_set_state(DEACTIVATING);
			}
			
			break;
			
		case POWER_BUTTON_PRESS:
			
			DEBUG(("hal initialising state, POWER_BUTTON_PRESS message arrived...\n"));

			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT, 0, 2000);
			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_LONG, 0, 8000);
			break;
			
		case POWER_BUTTON_RELEASE:
			
			DEBUG(("hal initialising state, POWER_BUTTON_RELEASE message arrived...\n"));
			
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT);
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_LONG);
			
			hal.power_button_released = TRUE;
							
			if (hal.beep_finished == TRUE) {
				
				hal_message_switching_on_t* msg1 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
				hal_message_switching_on_t* msg2 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
				msg1 -> forced_restart = FALSE;
				msg2 -> forced_restart = FALSE;
				
				MessageSend(hal.profile_task, HAL_MESSAGE_SWITCHING_ON, msg1);
				MessageSend(hal.function_task, HAL_MESSAGE_SWITCHING_ON, msg2);
				
				hal_set_state(ACTIVE);
			}
			
			break;			
						
		case HAL_POWER_BUTTON_HELD_LONG:
			
			DEBUG(("hal activating state, HAL_POWER_BUTTON_HELD_LONG message arrived...\n"));
			
			/** this must happen AFTER beep finished and BEFORE POWER_BUTTON_RELEASE, otherwise, correct the time constant settings **/
			if (hal.beep_finished == FALSE || hal.power_button_released == TRUE) {
				
				Panic();
			}
			else {
				
				hal_message_switching_on_t* msg1 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
				hal_message_switching_on_t* msg2 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
				msg1 -> forced_restart = TRUE;
				msg2 -> forced_restart = TRUE;
				
				MessageSend(hal.profile_task, HAL_MESSAGE_SWITCHING_ON, msg1);
				MessageSend(hal.function_task, HAL_MESSAGE_SWITCHING_ON, msg2);
				
				hal_set_state(ACTIVE);
			}
			break;
	
		case HAL_ACTIVATING_TIMEOUT:
			
			DEBUG(("hal activating state, HAL_ACTIVATING_TIMEOUT message arrived...\n"));
			
			hal.beep_finished = TRUE;
			
			if (!powerAllowedToTurnOn()) {
				
				hal_set_state(DEACTIVATING);
			}
			else {
				
				if (hal.power_button_released == TRUE) {
					
					hal_message_switching_on_t* msg1 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
					hal_message_switching_on_t* msg2 = (hal_message_switching_on_t*) malloc(sizeof(hal_message_switching_on_t));
					msg1 -> forced_restart = FALSE;
					msg2 -> forced_restart = FALSE;
				
					MessageSend(hal.profile_task, HAL_MESSAGE_SWITCHING_ON, msg1);
					MessageSend(hal.function_task, HAL_MESSAGE_SWITCHING_ON, msg2);
					
					hal_set_state(ACTIVE);
				}
				else {
					
					update_indication();
				}
			}
			break;
	}
}

static void hal_active_state_enter(void) {
	
	DEBUG(("hal active state enter...\n"));
	update_indication();
	
	MessageSendLater(getHalTask(), HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT, 0, HAL_ACTIVE_AUTO_SHUTDOWN_DURATION);
}

static void hal_active_state_exit(void) {
	
	DEBUG(("hal active state exit...\n"));
	
	MessageCancelAll(getHalTask(), HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT);
}

static void hal_active_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case PIO_RAW:
		
			DEBUG(("hal active state, PIO_RAW message arrived...\n"));
			
			pio_raw_handler(message);
			
			if (!powerAllowedToContinue()) {
				
				hal_set_state(DEACTIVATING);
			}
			else {
				
				update_indication();
			}
			
			break;
			
		case POWER_BUTTON_PRESS:
			
			DEBUG(("hal active state, POWER_BUTTON_PRESS message arrived...\n"));

			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT, 0, 2000);
			MessageSendLater(getHalTask(), HAL_POWER_BUTTON_HELD_LONG, 0, 8000);
			break;

			
		case POWER_BUTTON_RELEASE:
			
			DEBUG(("hal active state, POWER_BUTTON_RELEASE message arrived...\n"));
			
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_SHORT);
			MessageCancelAll(getHalTask(), HAL_POWER_BUTTON_HELD_LONG);
			break;

		case HAL_POWER_BUTTON_HELD_SHORT:
			
			DEBUG(("hal active state, HAL_POWER_BUTTON_HELD_SHORT message arrived...\n"));
		
			/** turning off **/
			hal_set_state(DEACTIVATING);
			break;
		
			
		case BATTERY_READING_MESSAGE:
			
			DEBUG(("hal active state, BATTERY_READING_MESSAGE message arrived...\n"));
			
			battery_message_handler(message);
			if (!powerAllowedToContinue()) {
				
				DEBUG(("hal active state, powerAllowedToContinue failed...\n"));
				hal_set_state(DEACTIVATING);
			}
			else {
				update_indication();
			}
			break;
			
		case FUNCTION_BUTTON_PRESS:
			
			DEBUG(("hal active state, FUNCTION_BUTTON_PRESS message arrived...\n"));
			
			/** restart timer **/
			MessageCancelAll(getHalTask(), HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT);
			MessageSendLater(getHalTask(), HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT, 0, HAL_ACTIVE_AUTO_SHUTDOWN_DURATION);
			
			/** pass event **/
			MessageSend(hal.function_task, HAL_MESSAGE_FUNCTION_BUTTON_PRESS, 0);
			
			break;
			
		case HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT:
			
			DEBUG(("hal active state, HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT message arrived ... \n"));
			
			hal_set_state(DEACTIVATING);
			break;
	}
}

static void hal_deactivating_state_enter(void) {
	
	DEBUG(("hal deactivating state enter...\n"));
	
	update_indication();

	ledsPlay(BEEP_TWICE);
    ledsPlay(MOTOR_TWICE);
	MessageSendLater(getHalTask(), HAL_DEACTIVATING_TIMEOUT, 0, BEEP_TWICE_DURATION + 100);
	
	/** send message to profile **/
	MessageSend(hal.profile_task, HAL_MESSAGE_SWITCHING_OFF, 0);
	MessageSend(hal.function_task, HAL_MESSAGE_SWITCHING_OFF, 0);
}

static void hal_deactivating_state_exit(void) {
	
	DEBUG(("hal deactivating state exit...\n"));
}

static void hal_deactivating_handler(Task task, MessageId id, Message message) {
	
	/** no need to react to any message except timeout, after all, we are going to panic **/
	
	switch(id) {
			
		case HAL_DEACTIVATING_TIMEOUT:
			
			DEBUG(("hal deactivating state, HAL_DEACTIVATING_TIMEOUT message arrived...\n"));
			
			/** brute way !!! */
			Panic();
			break;
	}
}


void hal_init(Task profileTask, Task functionTask) {
	
	/** set task hander **/
	hal.task.handler = hal_handler;
	
	/** set profile task **/
	hal.profile_task = profileTask;
	
	/** set function task **/
	hal.function_task = functionTask;
	
	/** init charging state **/
	hal.charging_state = CHARGING_UNKNOWN;
	
	/** set voltage to invalid value **/
	hal.voltage = 0xFFFF;
	
	/** no need to init state locals here **/
	
	/** power button press count **/
	hal.power_button_press_count = 0;
	
	/** set init state **/
	hal.state = INITIALISING;
	hal_initialising_state_enter();
}

/************
  
  common functions 
  
  ************/

static void enableLDO(void) {
	
	/** set output and drive high **/
	PioSetDir(PIO_LDO_ENABLE, PIO_LDO_ENABLE);
	PioSet(PIO_LDO_ENABLE, PIO_LDO_ENABLE);
}

static void disableLDO(void) {
	
	/** set input, don't know pull-up detail, need to clarify **/
	PioSetDir(PIO_LDO_ENABLE, 0);
	
	/** maybe this could change, according the pio.h document, all pios has weak pull-up / pull-down, according the bit value in corresponding output register **/
	PioSet(PIO_LDO_ENABLE, 0); 
}

/** this function may need further refine **/
static void pio_raw_handler(Message message) {

	PIO_RAW_T* pio_raw = (PIO_RAW_T*)message;
	
	hal.charging_state = (pio_raw ->pio & PIO_CHARGE_DETECTION) ? CHARGING_CHARGING : CHARGING_NOT_CHARGING;
}

/** see $bluelab$\src\lib\battery\battery.c, sendReading function for message type **/
static void battery_message_handler(Message message) {
	
	uint32* mV = (uint32*)message;
	
	/** should we need unsigned long ??? **/
	hal.voltage = (*mV) * (22 + 15) / 15;
}

static bool powerAllowedToTurnOn(void) {

	if (hal.charging_state == CHARGING_NOT_CHARGING && hal.voltage < BATTERY_LOW_HYSTERESIS_HIGH_BOUND) {
		
		return FALSE;
	}

	return TRUE;
}

static bool powerAllowedToContinue(void) {
	
	if (hal.charging_state == CHARGING_NOT_CHARGING && hal.voltage < BATTERY_LOW_HYSTERESIS_LOW_BOUND) {
		
		return FALSE;
	}
	
	return TRUE;
}













