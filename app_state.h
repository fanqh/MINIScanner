#ifndef APP_EXT_STATE_H
#define APP_EXT_STATE_H

#include "messagebase.h"


/**************************************
  
  these struct and messages define app layer's external state, i.e.,  from the viewpoint of hal and consider
  the app (such as sppb-serial) as a BLACKBOX controllable object with state indication. Don't confuse them 
  with sppb's internal state. The object can map several internal state into its external state, or just use
  some composite expression to determine it, it's up to the object to choose their choice. 
  
  RESPONSIBILITY:
  
  the upper layer must send a message to hal when it's external state changed, exactly in the way the PIO_RAW
  works, i.e., like an interrupt-driven scheme.
  
  PURPOSE:
  
  the hal layer need to know this state to take action, such as start or not, shutdown immediately
  or wait for a while. this design is better than designing protocols and impose mandatory behavior or 
  responsibilitis to each module for each operation/event, which is not flexible when requirement changed.
  modifying protocol may involve at least two parties.
  
  *************************************/

typedef enum {
	
	APP_EXT_STATE_IDLE,
	APP_EXT_STATE_WORKING,
	APP_EXT_STATE_UNKNOWN		/** this is used for hal layer to represent uninitialised state **/		
	
} app_ext_state_t;

typedef struct {
	
	app_ext_state_t state;
	
} app_ext_state_change_message_t;

enum {

	APP_EXT_STATE_CHANGE_MESSAGE = APP_MESSAGE_BASE
};

#endif /** APP_STATE_H **/


