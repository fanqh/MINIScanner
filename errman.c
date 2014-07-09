#include <pio.h>

#include "hal.h"
#include "hal_private.h"
#include "sppb.h"
#include <panic.h>
#include "spp_dev_b_leds.h"

#include "errman.h"


void DoErrorCheck( bool flag )
{
#if 0 /*test branch*/
	if( flag )
#else
	if( TRUE != flag ) 
#endif
	{
		/* beep 5 times here before panic */
		busy_beep( K_BeepTimes );
		
		Panic();
	}
}

void initialisationFinished(void) 
{
	halTaskData* hal_task;
	sppb_task_t* spp_Task;
	
	hal_task = (halTaskData*)getHalTask();

	if( hal_task )
	{
		DoErrorCheck( hal_task->charging_state!= CHARGING_UNKNOWN );
		DoErrorCheck( hal_task->voltage != K_VoltageInit );
	}

	
	spp_Task = (sppb_task_t*)getSppbTask();

	if( spp_Task )
	{
		DoErrorCheck( spp_Task->cl_initialised );
		DoErrorCheck( spp_Task->spp_initialised );
	}
	
}


void busy_beep_unit(void) {
	
	uint16 i;
	
	for (i = 0; i < K_BeepCounter_DriveHigh; i++) {
		
		/** drive pio high **/
		PioSetDir(BUZZ_MASK, BUZZ_MASK);
		PioSet(BUZZ_MASK, BUZZ_MASK);
	}
	
	for (i = 0; i < K_BeepCounter_DriveLow; i++) {
		
		/** set pio as input **/
		PioSetDir(BUZZ_MASK, 0);	
	}
}

void busy_beep(uint16 times) {
	
	uint16 i;
	for (i = 0; i < times; i++) {
		
		busy_beep_unit();
	}
	
	/** interval **/
	for (i = 0; i < 5000; i++) {
		
		PioSetDir(BUZZ_MASK, 0);
	}
}

void raise_exception(uint16 m, uint16 n) {
	
	busy_beep(m);
	busy_beep(n);
	
	Panic();
}
