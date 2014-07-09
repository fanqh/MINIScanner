#include <csrtypes.h>
#include <stdlib.h>
#include <message.h>
#include "battery_probe.h"
#include "battery_probe_private.h"


/** internal message id **/
enum {
	
	BATTERY_PROBE_TICK
			
};

/** task data **/
battery_probe_task_t battery_probe;


/**************************************************************************
  
  function prototypes
  
  */

/** task **/
static Task getBatteryProbeTask(void);

/** battrey probe task handler **/
static void battery_probe_handler(Task task, MessageId id, Message message);

/** adc result handler **/
static void adc_result_handler(vm_adc_source_type source, uint8 reading);

/** tick handler **/
static void tick_handler(void);


/**************************************************************************
  
  function definitions
  
  */


static Task getBatteryProbeTask(void) {
	
	return &battery_probe.task;
}

static void battery_probe_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case MESSAGE_ADC_RESULT:
			{
				MessageAdcResult* mar = (MessageAdcResult*)message;
				if (mar) {
					adc_result_handler(mar ->adc_source, mar ->reading);
				}
			}	
			break;
		
		case BATTERY_PROBE_TICK:
		
			tick_handler();
			break;
			
		default:
			
			
			break;
	}
}

static void adc_result_handler(vm_adc_source_type source, uint8 reading) {
	
	bool vref_first_update = FALSE;
	
	if (source == VM_ADC_SRC_VREF) {
		
		if (battery_probe.vref_reading == 255) {
			
			vref_first_update = TRUE;
		}
		
		battery_probe.vref_reading = reading;
	}
	else if (source == battery_probe.source) {
		
		battery_probe.source_reading = reading;
	}
	else {
		
		/** should not happen, but if it happens, we should not panic **/
		
		return;
	}
	
	/*******************
	  
	  condition 1: both reading must be valid
	  contition 2: either this is a source update or it is the vref update for the first time, that is, only first time vref update, we throw the message
	  
	  ******************/
	if ((battery_probe.source_reading != 255) && (battery_probe.vref_reading != 255) &&
		((source == battery_probe.source) || (source == VM_ADC_SRC_VREF && vref_first_update == TRUE))) {
		
		/** to avoid integer overflow **/
		unsigned long src, vref;
		uint32* mV;
		
		src = battery_probe.source_reading;
		vref = battery_probe.vref_reading;
		
		if (vref == 0) { /** to avoid division overflow **/
			return;
		}
		
		mV = (uint32*)malloc(sizeof(uint32));
		*mV = 1250UL * src / vref;
		
		MessageSend(battery_probe.client, BATTERY_PROBING_MESSAGE, mV);
	}
}

static void tick_handler(void) {
	
	(void)AdcRequest(&battery_probe.task, battery_probe.source);
	(void)AdcRequest(&battery_probe.task, VM_ADC_SRC_VREF);
	
	MessageSendLater(getBatteryProbeTask(), BATTERY_PROBE_TICK, 0, battery_probe.tick_interval);
}

/** start the probe **/
void battery_probe_start(Task task, vm_adc_source_type source, uint16 tick_interval) {
	
	battery_probe.task.handler = battery_probe_handler;
	battery_probe.client = task;
	battery_probe.source = source;
	battery_probe.tick_interval = tick_interval;
	battery_probe.vref_reading = 255;
	battery_probe.source_reading = 255;
	
	tick_handler();
}

/** stop the probe **/
void battery_probe_stop(void) {
	
	/** cancell all MESSAGE_ADC_RESULT and BATTERY_PROBE_TICK message **/
	MessageFlushTask(getBatteryProbeTask());
	
	/** cancell all BATTERY_PROBE_READING message sent to client **/
	MessageCancelAll(battery_probe.client, BATTERY_PROBING_MESSAGE);
}
