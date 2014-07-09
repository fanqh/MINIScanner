#include <stdlib.h>
#include <string.h>
#include <pio.h>
#include <stream.h>
#include <source.h>
#include "debug.h"
#include "hal.h"
#include "spp_dev_b_buttons.h"
#include "indication.h"
#include "scanner.h"

#define SCANNER_POWER_MASK			(1 << 9)
#define SCANNER_TRIGGER_MASK		(1 << 3)

#define SCANNER_TRIGGER_TIMEOUT		(250)

#define SCANNER_WARMUP_WAIT_TIMEOUT				(500)
#define SCANNER_SCANNING_TIMEOUT				(4000)

/** pulling down for only 20ms is a WRONG logic, should keep pulling down until read finishe **/
/** #define SCANNER_SCANNING_PULLDOWN_TIMEOUT		(20) **/

/** internal message **/
enum {
	
	SCANNER_WARMUP_WAIT_TIMEOUT_IND,
	SCANNER_TRIGGER_TIMEOUT_IND,			/** obsolete **/
	SCANNER_SCANNING_TIMEOUT_IND,
	SCANNER_SCANNING_PULLDOWN_TIMEOUT_IND
};

/** task data **/
static scanner_task_t scanner;

/** state handlers, entries, and exits **/
static void scanner_off_state_enter(void);
static void scanner_off_state_exit(void);
static void scanner_off_state_handler(Task task, MessageId id, Message message);

static void scanner_on_state_enter(void);
static void scanner_on_state_exit(void);
static void scanner_on_state_handler(Task task, MessageId id, Message message);

static void scanner_warmup_state_enter(void);
static void scanner_warmup_state_exit(void);
static void scanner_warmup_state_handler(Task task, MessageId id, Message message);

static void scanner_ready_state_enter(void);
static void scanner_ready_state_exit(void);
static void scanner_ready_state_handler(Task task, MessageId id, Message message);

static void scanner_scanning_state_enter(void);
static void scanner_scanning_state_exit(void);
static void scanner_scanning_state_handler(Task task, MessageId id, Message message);

static void trigger_drive_low(void);
static void trigger_pull_up(void);


static void trigger_drive_low(void) {
	
	/** set output **/
	PioSetDir(SCANNER_TRIGGER_MASK, SCANNER_TRIGGER_MASK);
	
	/** drive low **/
	PioSet(SCANNER_TRIGGER_MASK, 0);
}

static void trigger_pull_up(void) {
	
	/** set as input **/
	PioSetDir(SCANNER_TRIGGER_MASK, 0);
	
	/** weak pull-up according to pio.h document **/
	PioSet(SCANNER_TRIGGER_MASK, SCANNER_TRIGGER_MASK);	
}

#if 0
static void enable_scanner(void) {
	
	/** drive high **/
	PioSetDir(SCANNER_POWER_MASK, SCANNER_POWER_MASK);
	PioSet(SCANNER_POWER_MASK, SCANNER_POWER_MASK);
}

static void disable_scanner(void) {
	
	/** drive low **/
	PioSetDir(SCANNER_POWER_MASK, SCANNER_POWER_MASK);
	PioSet(SCANNER_POWER_MASK, 0);
}
#endif

static void scanner_off_state_enter(void) {
	
	DEBUG(( "scanner, off state enter... \n" ));
	update_indication();
#if 0	
	disable_scanner();
#endif
	trigger_pull_up();
	
	if (StreamUartSource()) {
		
		StreamUartConfigure(VM_UART_RATE_9K6, VM_UART_STOP_ONE, VM_UART_PARITY_NONE);
		
		MessageSinkTask(StreamUartSink(), &scanner.task);
		
		scanner.uart_initialized = TRUE;
	}
}

static void scanner_off_state_exit(void) {
	
	DEBUG(( "scanner, off state exit...\n" ));
}

static void scanner_off_state_handler(Task task, MessageId id, Message message) {
	
	switch(id) {
		
		case HAL_MESSAGE_SWITCHING_ON:
		
			DEBUG(( "scanner, scanner_off_state_handler, HAL_MESSAGE_SWITCHING_ON arrived... \n" ));
		
			scanner_off_state_exit();
			scanner.state = SCANNER_ON;
			scanner_on_state_enter();
		
		break;
	}
}

static void scanner_on_state_enter(void) {
	
	DEBUG(( "scanner, on state enter... \n" ));
	
	trigger_pull_up();
#if 0
	enable_scanner();
#endif
	
	scanner.on_state = SCANNER_ON_WARMUP;
	scanner_warmup_state_enter();
}

static void scanner_on_state_exit(void) {
	
	switch(scanner.on_state) {
		
		case SCANNER_ON_WARMUP:
		
			scanner_warmup_state_exit();
			break;
			
		case SCANNER_ON_READY:
			
			scanner_ready_state_exit();
			break;
			
		case SCANNER_ON_SCANNING:
			scanner_scanning_state_exit();
			break;
			
		case SCANNER_ON_INVALID:
			break;
	}
	
	scanner.on_state = SCANNER_ON_INVALID;
	
	trigger_pull_up();
#if 0
	disable_scanner();
#endif
}

static void scanner_on_state_handler(Task task, MessageId id, Message message) {
	
	if (id == HAL_MESSAGE_SWITCHING_OFF) {
		
		scanner_on_state_exit();
		scanner.state = SCANNER_OFF;
		scanner_off_state_enter();
		
		return;
	}
	
	switch (scanner.on_state) {
		
		case SCANNER_ON_WARMUP:
		
			scanner_warmup_state_handler(task, id, message);
			
		break;
		
		case SCANNER_ON_READY:
		
			scanner_ready_state_handler(task, id, message);
		
		break;
		
		case SCANNER_ON_SCANNING:
		
			scanner_scanning_state_handler(task, id, message);
		
		break;
		
		case SCANNER_ON_INVALID:
		
			DEBUG(( "scanner, scanner_on_state_handler, scanner.on_state is SCANNER_ON_INVALID, ERROR !!! \n" ));
			
			break;
	}
}

static void scanner_warmup_state_enter(void) {
	
	DEBUG(( "scanner, warmup state enter... \n" ));
	
	update_indication();
	
	/** start clock **/
	MessageSendLater(&scanner.task, SCANNER_WARMUP_WAIT_TIMEOUT_IND, 0, SCANNER_WARMUP_WAIT_TIMEOUT);
}

static void scanner_warmup_state_exit(void) {
	
	DEBUG(( "scanner, warmup state exit... \n" ));
	
	MessageCancelAll(&scanner.task, SCANNER_WARMUP_WAIT_TIMEOUT_IND);
}

static void scanner_warmup_state_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case SCANNER_WARMUP_WAIT_TIMEOUT_IND:
		
			DEBUG(( "scanner, scanner_warmup_state_handler, SCANNER_WARMUP_WAIT_TIMEOUT_IND arrived... \n" ));
			
			scanner_warmup_state_exit();
			scanner.on_state = SCANNER_ON_READY;
			scanner_ready_state_enter();
			break;
			
		case HAL_MESSAGE_FUNCTION_BUTTON_PRESS:
			
			DEBUG(( "scanner, scanner_warmup_state_handler, FUNCTION_BUTTON_0 arrived... ignored... \n" ));
			break;
			
		default:
			break;
	}
}

static void scanner_ready_state_enter(void) {
	
	DEBUG(( "scanner, ready state enter... \n" ));
	
	update_indication();
}

static void scanner_ready_state_exit(void) {
	
	DEBUG(( "scanner, ready state exit... \n" ));
}

static void scanner_ready_state_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case HAL_MESSAGE_FUNCTION_BUTTON_PRESS:
		
			DEBUG(( "scanner, scanner_ready_state_handler, FUNCTION_BUTTON_0 arrived... \n" ));
		
			scanner_ready_state_exit();
			scanner.on_state = SCANNER_ON_SCANNING;
			scanner_scanning_state_enter();
			break;
	}
}

static void scanner_scanning_state_enter(void) {
	
	Source source;
	
	DEBUG(( "scanner, scanning state enter... \n" ));
	
	update_indication();
	
	/** start scanning timeout timer **/
	MessageSendLater(&scanner.task, SCANNER_SCANNING_TIMEOUT_IND, 0, SCANNER_SCANNING_TIMEOUT);
	
	/** pull-down and start timeout timer **/
	trigger_drive_low();
	
	/** MessageSendLater(&scanner.task, SCANNER_SCANNING_PULLDOWN_TIMEOUT_IND, 0, SCANNER_SCANNING_PULLDOWN_TIMEOUT); **/
	
	/** clean-up uart source **/
	source = StreamUartSource();
	if (source && SourceSize(source) > 0) {
		
		SourceDrop(source, SourceSize(source));
	}
	
	/** clear barcode **/
	barcode_clear(&scanner.barcode);
}

static void scanner_scanning_state_exit(void) {
	
	Source source;
	
	DEBUG(( "scanner, scanning state exit... \n" ));
	
	MessageCancelAll(&scanner.task, SCANNER_SCANNING_TIMEOUT_IND);
	
	trigger_pull_up();	/** don't forget this, we may exit when pulling-down triggering line **/
	/** MessageCancelAll(&scanner.task, SCANNER_SCANNING_PULLDOWN_TIMEOUT_IND); **/
	
	source = StreamUartSource();
	if (source && SourceSize(source) > 0) {
		
		SourceDrop(source, SourceSize(source));
	}
	
	MessageCancelAll(&scanner.task, MESSAGE_MORE_DATA);
	MessageCancelAll(&scanner.task, MESSAGE_MORE_SPACE);
	
	/** clear barcode **/
	barcode_clear(&scanner.barcode);
}

static void scanner_scanning_state_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case HAL_MESSAGE_FUNCTION_BUTTON_PRESS:
		
			DEBUG(( "scanner, scanner_scanning_state_handler, FUNCTION_BUTTON_0 arrived... ignored... \n" ));
			
			break;
		
		/** this is something wrong 	
		case SCANNER_SCANNING_PULLDOWN_TIMEOUT_IND:
			
			DEBUG(( "scanner, scanner_scanning_state_handler, SCANNER_SCANNING_PULLDOWN_TIMEOUT_IND arrived... \n" ));
			
			trigger_pull_up();
			break;
			
		**/
			
		case SCANNER_SCANNING_TIMEOUT_IND:
			
			DEBUG(( "scanner, scanner_scanning_state_handler, SCANNER_SCANNING_TIMEOUT_IND arrived... \n" ));
			
			scanner_scanning_state_exit();
			scanner.on_state = SCANNER_ON_READY;
			scanner_ready_state_enter();
			break;
			
		case MESSAGE_MORE_DATA:
			{
				
				Source source;
				uint16 size;
				const uint8* ptr;
				bool fill_success;
				
				DEBUG(( "scanner, scanner_scanning_state_handler, MESSAGE_MORE_DATA arrived... \n" ));	
						
				source = StreamUartSource();
				size = SourceSize(source);
				
				DEBUG(( "    source has %d bytes of data... \n", size ));
				
				ptr = SourceMap(source);
				
				fill_success = barcode_fill_raw_bytes(&scanner.barcode, ptr, size);
				
				SourceDrop(source, size);
				
				if ( TRUE == fill_success ) {
					
					DEBUG(( "    barcode_fill_raw_bytes succeed... \n" ));
					
					if ( barcode_is_terminated(&scanner.barcode) ) {
						
						barcode_t* result;
						
						DEBUG(( "    barcode is terminated... \n" ));
						DEBUG(( "    %s", scanner.barcode.code ));
					
						result = (barcode_t*)malloc(sizeof(barcode_t));
						if (0 == result) { /** mem fail, then neglect the result **/
							
							DEBUG(( "    mem error, no result sent... \n" ));
							
							scanner_scanning_state_exit();
							scanner.on_state = SCANNER_ON_READY;
							scanner_ready_state_enter();
						}
						else {	/** throw out message to client **/
							
							DEBUG(( "    result sent to client in SCANNER_RESULT_MESSAGE... \n" ));
							
							memcpy(result, &scanner.barcode, sizeof(barcode_t));
							
							MessageSend(scanner.client, SCANNER_RESULT_MESSAGE, result);
							scanner_scanning_state_exit();
							scanner.on_state = SCANNER_ON_READY;
							scanner_ready_state_enter();
						}
					}
					else {
						
						DEBUG(( "    barcode is not terminated... \n" ));
					}
				}
				else {
					
					DEBUG(( "    barcode_fill_raw_bytes failed... \n" ));
					
					scanner_scanning_state_exit();
					scanner.on_state = SCANNER_ON_READY;
					scanner_ready_state_enter();
				}
			}
			break;
	}
}

static void scanner_handler(Task task, MessageId id, Message message) {
	
	scanner_state_t state = scanner.state;
	
	switch(state) {

		case SCANNER_OFF:
		
			scanner_off_state_handler(task, id, message);
			break;
			
		case SCANNER_ON:
			
			scanner_on_state_handler(task, id, message);
			break;
	}
}

void scanner_init(Task hal, Task client) {

	trigger_pull_up();
#if 0
	enable_scanner();	
#endif	
	scanner.task.handler = scanner_handler;
	scanner.hal = hal;
	scanner.client = client;
	
	scanner.state = SCANNER_OFF;
	scanner.on_state = SCANNER_ON_INVALID;
	
	barcode_clear(&scanner.barcode);
	
	scanner_off_state_enter();
}

Task getScannerTask(void) {
	
	return &scanner.task;
}


