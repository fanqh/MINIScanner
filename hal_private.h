#ifndef HAL_PRIVATE_H
#define HAL_PRIVATE_H

#include <message.h>
#include <battery.h>
#include "spp_dev_b_buttons.h"
#include "app_state.h"


#define PIO_CHARGE_DETECTION (1UL << 10)
#define PIO_LDO_ENABLE (1UL << 8)

/** hal internal message id **/
enum {
	HAL_POWER_BUTTON_HELD_SHORT,
	HAL_POWER_BUTTON_HELD_LONG,
	HAL_ACTIVATING_TIMEOUT,
	HAL_DEACTIVATING_TIMEOUT,
	HAL_ACTIVE_AUTO_SHUTDOWN_TIMEOUT,
    HAL_FUNCTION_BUTTON_DFU_TIMEOUT
};

typedef enum {
	
	/** initialising, this is a stable state, no timed exit **/
	INITIALISING,
	
	/** this is a timed and blocking state to play beep **/
	ACTIVATING,
	
	/** in this state, charging or battery fine, wireless start **/
	ACTIVE,
	
	/** this is a timed session **/
	DEACTIVATING
	
} hal_state_t;

typedef enum {
	
	CHARGING_UNKNOWN,
	CHARGING_NOT_CHARGING,
	CHARGING_CHARGING
	
} charging_t;

typedef struct {

	/** task data **/
	TaskData 			task;	
	
	/** profile task **/
	Task 				profile_task;
	
	/** function task **/
	Task				function_task;

	/** hal state **/
	hal_state_t			state;
	
	/** data storage for pio **/
	PioState 			pio_state;
	
	/** data storage for battery lib **/
	BatteryState 		battery_state;
	
	/** charging state, cached, avoiding async reading when needed **/
	charging_t			charging_state;	
	
	/** battery voltage, in millivolts, or 0xFFFF if unknown **/
	uint32				voltage;
		
	/** the following state activating state locals **/
	bool				beep_finished;
	bool				power_button_released;
	
	/** power button press count **/
	uint16				power_button_press_count;
	
} halTaskData;

#endif /** HAL_PRIVATE_H **/


