#include <pio.h>
#include <message.h>
#include "buzzer.h"

#define BUZZ_MASK (1UL << 11)

void buzzer_on(void) {
	
	/** set output and drive high **/
	PioSetDir(BUZZ_MASK, BUZZ_MASK);
	PioSet(BUZZ_MASK, BUZZ_MASK);
}

void buzzer_off(void) {
	
	/** set input and pull down **/
	PioSetDir(BUZZ_MASK, 0);
	PioSet(BUZZ_MASK, 0);
}

enum {
	
	BEEP_MESSAGE_TIMEOUT = 0
};

typedef struct {
	
	TaskData task;
	
} beep_task_t;

static beep_task_t beeper;

static void beep_handler(Task task, MessageId id, Message message);

static void beep_handler(Task task, MessageId id, Message message) {
	
	switch (id) {
		
		case BEEP_MESSAGE_TIMEOUT:
			
			buzzer_off();
			break;
			
		default:
			
			break;
	}
}

void beep(uint16 t) {
	
	beeper.task.handler = beep_handler;
	
	buzzer_on();
	MessageSendLater(&beeper.task, BEEP_MESSAGE_TIMEOUT, 0, t);
}
