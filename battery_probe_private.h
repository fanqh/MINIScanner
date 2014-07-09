#ifndef BATTERY_PROBE_PRIVATE_H
#define BATTERY_PROBE_PRIVATE_H

/** battery probe task data struct **/
typedef struct {
	
	TaskData task;
	Task client;
	vm_adc_source_type source;
	uint16 tick_interval;
	uint8 vref_reading;
	uint8 source_reading;
	
} battery_probe_task_t;

#endif /** BATTERY_PROBE_PRIVATE_H **/



