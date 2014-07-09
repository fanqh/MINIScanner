#ifndef SPPB_H
#define SPPB_H

#include <message.h>
#include <spp.h>

#include "messagebase.h"


/** **/
#define SPPB_PAIRABLE_DURATION 		(90000)

/** sppb state **/
typedef enum
{
    SPPB_INITIALISING,
    SPPB_READY,				/** this is the initialized and stable state **/
    SPPB_SCAN,
    SPPB_CONNECTED,
	SPPB_DISCONNECTING
	
} sppb_state_t;


/** sppb task data, noting that many of them are state- or substate-specific, do create/destroy in entry/exit funcs **/
typedef struct 
{
	/** task **/
    TaskData            task;
	
	/** hal task **/
	Task				hal_task;
	
	/** initialisation result **/	
	bool				cl_initialised;		
	bool				spp_initialised;
	
	/** scanning state locals **/
    bdaddr              bd_addr;
	bool				start_as_pairable;
	bool				pairable;
	bool				connecting;
	
	/** connected state-specific parameters 						**/
    SPP*                spp;					/* connected state 	**/  /** for connected state parameters, don't clean up when transition between sub-state, 	**/
	Sink				spp_sink;				/* connected state 	**/  /** init and clean in connected enter/exit 											**/

	/** main state & sub state */
    sppb_state_t        state;
	
} sppb_task_t;

void sppb_init(Task hal_task);

Task getSppbTask(void);


#endif /** SPPB_H **/


