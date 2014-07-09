#ifndef SCANNER_H
#define SCANNER_H

#include <message.h>
#include "barcode.h"

enum {
	SCANNER_RESULT_MESSAGE = 0x3800
};

typedef struct {
	
	uint16 length;
	uint8 barcode[32];
	
} scanner_result_t;

typedef enum {
	
	SCANNER_OFF,
	SCANNER_ON
	
} scanner_state_t;

typedef enum {
	
	SCANNER_ON_WARMUP,
	SCANNER_ON_READY,
	SCANNER_ON_SCANNING,
	SCANNER_ON_INVALID
	
} scanner_on_state_t;

typedef struct {
	
	TaskData task;
	Task hal;
	Task client;
	
	bool uart_initialized;
	
	scanner_state_t				state;
	scanner_on_state_t			on_state;
	
	barcode_t barcode;
	
} scanner_task_t;


Task getScannerTask(void);

/** this function will enable scanner automatically **/
void scanner_init(Task hal, Task client); 

#endif /* SCANNER_H */


