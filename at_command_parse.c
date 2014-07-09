#include "at_command.h"
#include "sppb.h"
#include "spp_dev_private.h"
#include <stream.h>
#include "command_return_code.h"

/***
The UART baud rate. 
Enumeration values: 

VM_UART_RATE_SAME 
The same. 
VM_UART_RATE_9K6 
9600 baud. 
VM_UART_RATE_19K2 
19200 baud. 
VM_UART_RATE_38K4 
38400 baud. 
VM_UART_RATE_57K6 
57600 baud. 
VM_UART_RATE_115K2 
115200 baud. 
VM_UART_RATE_230K4 
230400 baud. 
VM_UART_RATE_460K8 
460800 baud. 
VM_UART_RATE_921K6 
921600 baud. 
VM_UART_RATE_1382K4 
1382400 baud. 

enum vm_uart_stop   
The number of stop bits. 
Enumeration values: 

VM_UART_STOP_ONE 
One. 
VM_UART_STOP_TWO 
Two. 
VM_UART_STOP_SAME 
The same. 

enum vm_uart_parity 

The parity to use. 
Enumeration values: 

VM_UART_PARITY_NONE 
None. 
VM_UART_PARITY_ODD 
Odd. 
VM_UART_PARITY_EVEN 
Even. 
VM_UART_PARITY_SAME 
The same. 






***/





void connect(Task task, const struct connect * config) {
	
	bool success = TRUE;
	uint16 baudrate, stop, parity;
	sppb_task_t* task_data;
	
	task_data = (sppb_task_t*)task;
	
	/** step 1: check baudrate **/
	switch(config -> baudrate) {
		
		case 48:
			baudrate = 19;	/** this is a guessed value, test it **/
			break;
			
		case 96:
			baudrate = VM_UART_RATE_9K6;
			break;
			
		case 192:
			baudrate = VM_UART_RATE_19K2;
			break;
			
		case 384:
			baudrate = VM_UART_RATE_38K4;
			break;
			
		case 576:
			baudrate = VM_UART_RATE_57K6;
			break;
			
		case 1152:
			baudrate = VM_UART_RATE_115K2;
			break;
			
		default:
			
			baudrate = 0;
			success = FALSE;
			break;
	}
	
	if (!success) {
		
		task_data ->command_result = CMD_RET_UNSUPPORTED_BAUDRATE;
		return;
	}
	
	/** step 2: check stop bit **/
	switch(config ->stop) {
		
		case 1:
			stop = VM_UART_STOP_ONE;
			break;
			
		case 2:
			stop = VM_UART_STOP_TWO;
			break;
		
		default:
			stop = 0;
			success = FALSE;
			break;
	}
	
	if(!success) {

		task_data ->command_result = CMD_RET_UNSUPPORTED_STOP;
		return;
	}
	
	/** step 3: check parity **/
	switch(config ->parity) {
		
		case 0:
			parity = VM_UART_PARITY_NONE;
			break;
			
		case 1:
			parity = VM_UART_PARITY_ODD;
			break;
			
		case 2:
			parity = VM_UART_PARITY_EVEN;
			break;
			
		default:
			parity = 0;
			success = FALSE;
			break;
	}
	
	if (!success) {

		task_data ->command_result = CMD_RET_UNSUPPORTED_PARITY;
		return;
	}
	
	task_data ->command_result = CMD_RET_OK;
	
	StreamUartConfigure(baudrate, stop, parity);
}



