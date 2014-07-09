#ifndef BATTERY_PROBE_H
#define BATTERY_PROBE_H

#include <adc.h>
#include "messagebase.h"

/**************************************
  
  this is an asynchronous function, designed to be used in initialization stage. It should function 
  much faster than the battery lib provided by bluelab, but may be not as accurate.
  
  recommendation is using this function for the first time battery voltage check, then start polling from
  battery lib.
  
  **************************************/

enum {
	
	BATTERY_PROBING_MESSAGE = BATTERY_PROBE_MESSAGE_BASE
};

/** start the probe **/
void battery_probe_start(Task task, vm_adc_source_type source, uint16 tick_interval);

/** stop the probe **/
void battery_probe_stop(void);

#endif /** BATTERY_PROBE_H **/


