/* Copyright (C) Cambridge Silicon Radio Limited 2005-2009 */
/* Part of BlueLab 4.1.2-Release */


#include <connection.h>
#include <panic.h>
#include <stdio.h>
#include <stream.h>
#include <pio.h>
#include <panic.h>
#include <sink.h>
#include <source.h>
#include <string.h>

#include "debug.h"
#include "sppb.h"
#include "indication.h"
#include "spp_dev_private.h"
#include "spp_dev_auth.h"
#include "spp_dev_b_leds.h"
#include "spp_dev_b_buttons.h"
#include "hal.h"
#include "errman.h"
#include "scanner.h"
#include "barcode.h"

/** task data **/
static sppb_task_t sppb;

/**************************************************************************************************
  
  from spp_dev_init.h & .c

  */
static void sppDevInit(void);

static void sppDevInit()
{
    spp_init_params init;

    init.client_recipe = 0;
    init.size_service_record = 0;
	init.service_record = 0;
	init.no_service_record = 0;
	
    /* Initialise the spp profile lib, stating that this is device B */ 
    SppInitLazy(getSppbTask(), getSppbTask(), &init);
}

#define CLASS_OF_DEVICE		0x1F00


/**************************************************************************************************
  
  sppb
  
  */

/** debug output **/
static void unhandledSppState(sppb_state_t state, MessageId id);


/** state entry / exit and handers **/
static void sppb_handler(Task task, MessageId id, Message message);

/** separate spp layer and cl layer **/
static void cl_handler(Task task, MessageId id, Message message);

/** main state handlers & entry/exits **/
static void sppb_initialising_state_handler(Task task, MessageId id, Message message);
static void sppb_ready_state_handler(Task task, MessageId id, Message message);
static void sppb_scan_state_handler(Task taks, MessageId id, Message message);
static void sppb_connected_state_handler(Task task, MessageId id, Message message);
static void sppb_disconnecting_state_handler(Task task, MessageId id, Message message);

static void sppb_initialising_state_enter(void);
static void sppb_initialising_state_exit(void);
static void sppb_ready_state_enter(void);
static void sppb_ready_state_exit(void);
static void sppb_scan_state_enter(void);		
static void sppb_scan_state_exit(void);
static void sppb_connected_state_enter(void);
static void sppb_connected_state_exit(void);
static void sppb_disconnecting_state_enter(void);
static void sppb_disconnecting_state_exit(void);


Task getSppbTask(void)
{
    return &sppb.task;
}

static void unhandledSppState(sppb_state_t state, MessageId id)
{
    DEBUG(("SPP current state %d message id 0x%x\n", state, id));   
}


static void setSppState(const sppb_state_t state)
{
    DEBUG(("SPP State - C=%d N=%d\n",sppb.state, state));
    sppb.state = state;
}

/**************************************************************************************************
  
  initialising state 
  
  */
static void sppb_initialising_state_enter(void) {
	
	DEBUG(("spp initialising state enter...\n"));

	update_indication();
	
	/** init connection library **/
	ConnectionInit(getSppbTask());
}

static void sppb_initialising_state_exit(void) {
	
	DEBUG(("spp initialising state exit...\n"));
	/** nothing to do **/
}

static void sppb_initialising_state_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case SPP_INIT_CFM:

			DEBUG(("spp initialising state, SPP_INIT_CFM message arrived...\n"));
			
            /* Check for spp_init_success. What do we do if it failed? */
            if (((SPP_INIT_CFM_T *) message)->status == spp_init_success)
            {
				/** switch to ready state, unconnectable, undiscoverable **/
				sppb.spp_initialised = TRUE;
				
				sppb_initialising_state_exit();
				sppb.state = SPPB_READY;
				sppb_ready_state_enter();
            }
			else {
				
				/** don't panic **/
			}
            break;
			
		default:
			unhandledSppState(sppb.state, id);
			break;
	}
}

/**************************************************************************************************
  
  ready state
  
  revert state map design, in fact the idle/working external state is the superstate of pairable, connecting, connected and disconnecting.
  
  TODO: if switched back from pairable/connecting state (when powering off), 
	there may be some connect_ind/cfm message in queue, check it and reject them politely
  
  */
static void sppb_ready_state_enter(void) {
	
	DEBUG(("spp ready state enter...\n"));	

	update_indication();	
}

static void sppb_ready_state_exit(void) {
	
	DEBUG(("spp ready state exit...\n"));
	/** nothing to do **/
}

static void sppb_ready_state_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case HAL_MESSAGE_SWITCHING_ON:
			{
				DEBUG(("spp ready state, HAL_MESSAGE_SWITCHING_ON message arrived...\n"));
			
				sppb.start_as_pairable = ((hal_message_switching_on_t*)message) ->forced_restart;
				
				sppb_ready_state_exit();
				setSppState(SPPB_SCAN);
				sppb_scan_state_enter();
			}
			break;
			
		default:
			unhandledSppState(sppb.state, id);
			break;			
	}
}

/**************************************************************************************************
  
  scan super-state
  
  */

static void sppb_scan_state_enter() {

	DEBUG(("spp scan state enter...\n"));
	
	/** the code is from original bluelab sample in spp_dev_b spp_dev_inquire() **/
	/* don't know if some initialisation could be done multiple times */
    /* Turn off security */
    ConnectionSmRegisterIncomingService(0x0000, 	
										0x0001, 
										0x0000);
    /* Write class of device */
    ConnectionWriteClassOfDevice(CLASS_OF_DEVICE);
    /* Start Inquiry mode */
    /** setSppState(SPPB_PAIRABLE); **/
    /* Set devB device to inquiry scan mode, waiting for discovery */
    ConnectionWriteInquiryscanActivity(0x400, 0x200);
    ConnectionSmSetSdpSecurityIn(TRUE);
	
	sppb.connecting = FALSE;
	
	if (sppb.start_as_pairable == TRUE) {
		
		/** one-time parameter **/
		sppb.start_as_pairable = FALSE;
		
		/** Make this device discoverable (inquiry scan), and connectable (page scan) **/
    	ConnectionWriteScanEnable(hci_scan_enable_inq_and_page);	
		
		sppb.pairable = TRUE;
		
		MessageSendLater(getSppbTask(), SPPB_PAIRABLE_TIMEOUT_IND, 0, SPPB_PAIRABLE_DURATION);

	}
	else {
		
		/** Make this device connectable (page scan) only **/
		ConnectionWriteScanEnable(hci_scan_enable_page);
		
		sppb.pairable = FALSE;
	}
	
	update_indication();
}

static void sppb_scan_state_exit() {

	DEBUG(("spp scan state exit...\n"));

	/* turn off scan **/
	ConnectionWriteScanEnable(hci_scan_enable_off);
	
	if (sppb.pairable == TRUE) {
		
		MessageCancelAll(getSppbTask(), SPPB_PAIRABLE_TIMEOUT_IND);	
	}
}

static void sppb_scan_state_handler(Task taks, MessageId id, Message message) {
	
	switch(id) {
		
		case SPP_CONNECT_IND:
		
			DEBUG(("spp scan state, SPP_CONNECT_IND message arrived... \n"));
			
			/** only accept connection in disconnected sub-state **/
			if (sppb.connecting == FALSE) {
				
				sppDevAuthoriseConnectInd(&sppb,(SPP_CONNECT_IND_T*)message);
				sppb.connecting = TRUE;
				update_indication();
			}
			else {
				
				/** should we respond with rejection ??? **/
			}
			
			break;
			
		case SPP_CONNECT_CFM:
			
			DEBUG(("spp scan state, SPP_CONNECT_CFM message arrived... \n"));
			
			if (sppb.connecting == FALSE) {	/** disconnected state **/

				SPP_CONNECT_CFM_T *cfm = (SPP_CONNECT_CFM_T *) message;
				if (cfm->status == rfcomm_connect_success)
                {
                    /* Device has been reset to pairable mode. Disconnect from current device, this is code from official sppb example */
                    SppDisconnect(cfm->spp);
                }					
			}
			else {	/** connecting state **/
				
				SPP_CONNECT_CFM_T *cfm = (SPP_CONNECT_CFM_T *) message;
				if (cfm ->status == rfcomm_connect_success) {
												
                	sppb.spp = cfm->spp;
					sppb.spp_sink = cfm ->sink;
					
					sppb_scan_state_exit();
                	setSppState(SPPB_CONNECTED);
					sppb_connected_state_enter();
				}
				else {
					
					sppb.connecting = FALSE;
					update_indication();
				}	
			}
			
			break;
			
		case SPPB_PAIRABLE_TIMEOUT_IND:
			
			DEBUG(("spp scan state, SPPB_PAIRABLE_TIMEOUT_IND message arrived...\n"));
			
			/** turn off inquiry scan **/
			ConnectionWriteScanEnable(hci_scan_enable_page);
			sppb.pairable = FALSE;
			
			break;
			
		case HAL_MESSAGE_SWITCHING_OFF:
			
			DEBUG(("spp scan state, HAL_MESSAGE_SWITCHING_OFF message arrived...\n"));
				
			sppb_scan_state_exit();
			sppb.state = SPPB_READY;
			sppb_ready_state_enter();
			
			break;			
	}
}


/**************************************************************************************************
  
  connected state
  
  */
static void sppb_connected_state_enter() {

	Sink sink;
	Source source;
	
	DEBUG(("spp connected state enter...\n"));
	
	update_indication();
	
	sink = sppb.spp_sink;
	source = StreamSourceFromSink(sink);
	
	StreamConnectDispose(source);
}


static void sppb_connected_state_exit() {

	DEBUG(("spp connected state exit...\n"));

	sppb.spp_sink = 0;	
	/** dont clear sppb.spp, the next state need it, it covers both connected state AND disconnecting state **/
}

static void sppb_connected_state_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case SPP_DISCONNECT_IND:	/** passively disconnected, switching to scan **/

			DEBUG(("spp connected state, SPP_DISCONNECT_IND message arrived...\n"));
			
			sppb_connected_state_exit();
			setSppState(SPPB_SCAN);
			sppb_scan_state_enter();
			break;
			
		case HAL_MESSAGE_SWITCHING_OFF:
			
			DEBUG(("spp connected state, HAL_MESSAGE_SWITCHING_OFF message arrived...\n"));
			
			sppb_connected_state_exit();
			setSppState(SPPB_DISCONNECTING);
			sppb_disconnecting_state_enter();
			
			break;
			
		case SCANNER_RESULT_MESSAGE:
			{
				barcode_t* barcode = (barcode_t*)message;
				if (barcode && SinkIsValid(sppb.spp_sink) && SinkSlack(sppb.spp_sink) >= barcode ->length) {
					
					uint16 offset;
					uint8* dst;
					Sink sink = sppb.spp_sink;
					
					offset = SinkClaim(sink, barcode ->length);
					dst = SinkMap(sink);
					memcpy(dst + offset, barcode ->code, barcode ->length);
					SinkFlush(sink, barcode ->length);
					
				}
			}
			break;

		default:
			
			unhandledSppState(sppb.state, id);
			break;
	}					 
}

static void sppb_disconnecting_state_enter() {
	
	DEBUG(("spp disconnecting state enter...\n"));
	
	update_indication();
	/** check reason and output debug **/
	SppDisconnect(sppb.spp);
}

static void sppb_disconnecting_state_exit() {
	
	DEBUG(("spp disconnecting state exit...\n"));
	/** nothing to do **/
}

static void sppb_disconnecting_state_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case SPP_DISCONNECT_IND:
		
			DEBUG(("spp disconnecting state, SPP_DISCONNECT_IND message arrived...\n"));
			sppb_disconnecting_state_exit();
			setSppState(SPPB_READY);
			sppb_ready_state_enter();
			
			break;
		
		default:
			unhandledSppState(sppb.state, id);
			break;		
	}
}

static void sppb_handler(Task task, MessageId id, Message message) {
	
	sppb_state_t state = sppb.state;
	
	if ((id & 0xFF00) == CL_MESSAGE_BASE) {
		
		cl_handler(task, id, message);
		return;
	}
	
	switch (state) {
		
		case SPPB_INITIALISING:
			sppb_initialising_state_handler(task, id, message);
			break;
			
		case SPPB_READY:
			sppb_ready_state_handler(task, id, message);
			break;
			
		case SPPB_SCAN:
			sppb_scan_state_handler(task, id, message);
			break;
						
		case SPPB_CONNECTED:
			sppb_connected_state_handler(task, id, message);
			break;
			
		case SPPB_DISCONNECTING:
			sppb_disconnecting_state_handler(task, id, message);
			break;
			
		default:
			break;
	}
}


/** connection library layer **/
static void cl_handler(Task task, MessageId id, Message message) {
	
	switch (id) {	
	case CL_INIT_CFM:
        DEBUG(("CL_INIT_CFM\n"));
        if(((CL_INIT_CFM_T*)message)->status == success) {
			
			/** change sub-state **/
			sppb.cl_initialised = TRUE;
            sppDevInit();   
		}
        else {
			
            /** don't panic **/
		}
        break;
    case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        DEBUG(("CL_DM_LINK_SUPERVISION_TIMEOUT_IND\n"));
        break;
    case CL_DM_SNIFF_SUB_RATING_IND:
        DEBUG(("CL_DM_SNIFF_SUB_RATING_IND\n"));
        break;
	
    case CL_DM_ACL_OPENED_IND:
        DEBUG(("CL_DM_ACL_OPENED_IND\n"));
        break;
    case CL_DM_ACL_CLOSED_IND:
        DEBUG(("CL_DM_ACL_CLOSED_IND\n"));
        break;
    case CL_SM_PIN_CODE_IND:
        DEBUG(("CL_SM_PIN_CODE_IND\n"));
        sppDevHandlePinCodeRequest((CL_SM_PIN_CODE_IND_T *) message);
        break;
    case CL_SM_AUTHORISE_IND:  
        DEBUG(("CL_SM_PIN_CODE_IND\n"));
        sppDevAuthoriseResponse((CL_SM_AUTHORISE_IND_T*) message);
        break;
    case CL_SM_AUTHENTICATE_CFM:
        DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
        sppDevSetTrustLevel((CL_SM_AUTHENTICATE_CFM_T*)message);    
        break;
    case CL_SM_ENCRYPTION_KEY_REFRESH_IND:
        DEBUG(("CL_SM_ENCRYPTION_KEY_REFRESH_IND\n"));
        break;
    case CL_DM_LINK_POLICY_IND:
        DEBUG(("CL_DM_LINK_POLICY_IND\n"));
        break;
    case CL_SM_IO_CAPABILITY_REQ_IND:
        DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
        ConnectionSmIoCapabilityResponse( &sppb.bd_addr, 
                                          cl_sm_io_cap_no_input_no_output,
                                          FALSE,
                                          TRUE,
                                          FALSE,
                                          0,
                                          0 );
        break;
 
    case CL_SM_REMOTE_IO_CAPABILITY_IND:
        {
            CL_SM_REMOTE_IO_CAPABILITY_IND_T *csricit = 
                    ( CL_SM_REMOTE_IO_CAPABILITY_IND_T *) message;

            DEBUG(("CL_SM_REMOTE_IO_CAPABILITY_REQ_IND\n"));
            DEBUG(("\t Remote Addr: nap %04x uap %02x lap %08lx\n",
                    csricit->bd_addr.nap,
                    csricit->bd_addr.uap,
                    csricit->bd_addr.lap ));
            sppb.bd_addr = csricit->bd_addr;
        }
        break;
		
	default:
		break;
	}
}

void sppb_init(Task hal_task) {
	
	
	sppb.task.handler = sppb_handler;
	sppb.hal_task = hal_task;
	
	sppb.spp = 0;
	sppb.spp_sink = 0;
	sppb.cl_initialised = FALSE;
	sppb.spp_initialised = FALSE;
	
	sppb.state = SPPB_INITIALISING;
	sppb_initialising_state_enter();
}















