/*
    This file was autogenerated by buttonparse
*/

#include "spp_dev_b_buttons.h"

#include <pio.h>
#include <app/message/system_message.h>
#include <panic.h>
#include <ps.h>
#include <stdlib.h>

enum {internal_pio_timer_message = 0, double_pio_press_timer};

static InternalState pio_encode(uint16 pressed)
{
	switch(pressed)
	{
		case (1UL<<4) : return sPOWER_BUTTON;
		case (1UL<<5) : return sSCAN_BUTTON;
		case (1UL<<4)|(1UL<<5) : return sPOWER_BUTTON_SCAN_BUTTON;
		default : return Unknown;
	}
}

typedef struct
{
	unsigned release:1;
	unsigned double_tap:1;
	unsigned timeout:15;
	MessageId id;
} EnterMessage;

/* Messages sent on state entry */
static const EnterMessage enter_messages_sPOWER_BUTTON[] = { { 0, 0, 0, POWER_BUTTON_PRESS },
												{ 1, 0, 0, POWER_BUTTON_RELEASE } };

static const EnterMessage enter_messages_sSCAN_BUTTON[] = { { 0, 0, 0, FUNCTION_BUTTON_PRESS } };

static const struct
{
	uint16 count; const EnterMessage *send;
} enter_messages[] = {
	{ 2, enter_messages_sPOWER_BUTTON },
	{ 1, enter_messages_sSCAN_BUTTON },
	{ 0, 0 }
};

static void send_pio_enter_messages(PioState *pioState, InternalState state)
{
	uint16 init_count = enter_messages[state].count;
	uint16 count = init_count;
	const EnterMessage *p = enter_messages[state].send;
	while(count--)
	{
		if (p->double_tap)
		{
			if (pioState->pio_states.double_press == state)
			{
				if (MessageCancelAll(&pioState->task, double_pio_press_timer))
				{
					pioState->pio_states.store_held = Unknown;
					MessageSend(pioState->client, p->id, 0);
				}
				else
					MessageSendLater(&pioState->task, double_pio_press_timer, 0, p->timeout);
			}
			else
			{
				pioState->pio_states.double_press = state;
				(void) MessageCancelAll(&pioState->task, double_pio_press_timer);
				MessageSendLater(&pioState->task, double_pio_press_timer, 0, p->timeout);
			}
		}
		else
		if (!p->release)
			MessageSend(pioState->client, p->id, 0);
		else
		{
			pioState->pio_states.store_held = state;
			pioState->pio_states.store_count = init_count - (count + 1);
		}
		p++;
	}
}

/* Timed messages to be sent to the client */
typedef struct
{
	unsigned repeat:1;
	unsigned msec:15; /* Limit to 32767ms. Sounds reasonable. */
	unsigned msecRepeat:15;
	unsigned release:1;
	MessageId id;
} TimedMessage;

static const TimedMessage timed_messages_sPOWER_BUTTON_SCAN_BUTTON[] =
{
	{ 0, 10000, 0, 0, FUNCTION_BUTTON_DFU }
};

static const struct
{
	uint16 count;
	const TimedMessage *send;
} timed_messages[] =
{
	{ 0, 0 },
	{ 0, 0 },
	{ 1, timed_messages_sPOWER_BUTTON_SCAN_BUTTON }
};

static void send_pio_timed_message(PioState *pioState, const TimedMessage *p, int hold_repeat)
{
	const TimedMessage **m = (const TimedMessage **) PanicNull(malloc(sizeof(const TimedMessage *)));
	*m = p;
	if (hold_repeat)
		MessageSendLater(&pioState->task, internal_pio_timer_message, m, p->msecRepeat);
	else
		MessageSendLater(&pioState->task, internal_pio_timer_message, m, p->msec);
}

static void send_pio_timed_messages(PioState *pioState, InternalState state)
{
	uint16 count = timed_messages[state].count;
	const TimedMessage *p = timed_messages[state].send;
	while(count--)
		send_pio_timed_message(pioState, p++, 0);
}

static void pioChanged(Task task, PioState *pioState, uint16 pio_bits)
{
	InternalState next;
	uint16 raw_bits = 0;
	uint16 mState = pio_bits;
	bool pio_changed = TRUE;

	mState &= ~ (1UL<<10);
	raw_bits = (pio_bits & (1UL<<10));
	if (raw_bits != pioState->pio_states.pio_raw_bits)
	{
		PIO_RAW_T *message = malloc(sizeof(PIO_RAW_T));
		message->pio = raw_bits;
		MessageSend(pioState->client, PIO_RAW, message);
		pioState->pio_states.pio_raw_bits = raw_bits;
	}

	next = pio_encode(mState);

	if (mState == pioState->pio_states.store_bits)
		pio_changed = FALSE;

	if (pio_changed)
	{
		if ((pioState->pio_states.store_held != Unknown) && (next != pioState->pio_states.store_held))
		{
			uint16 changed = mState ^ pioState->pio_states.store_bits;
			uint16 released = changed & pioState->pio_states.store_bits;
			if (released == pioState->pio_states.store_bits)
			{
				const EnterMessage *p = enter_messages[pioState->pio_states.store_held].send;
				MessageSend(pioState->client, p[pioState->pio_states.store_count].id, 0);
			}
			pioState->pio_states.store_held = Unknown;
		}

		if (pioState->pio_states.timed_id)
		{
			MessageSend(pioState->client, pioState->pio_states.timed_id, 0);
			pioState->pio_states.timed_id = 0;
		}

		(void) MessageCancelAll(task, internal_pio_timer_message);
		if(next != Unknown)
		{
			send_pio_enter_messages(pioState, next);
			send_pio_timed_messages(pioState, next);
		}
		pioState->pio_states.store_bits = mState;
	}
}

static void pioHandler(Task task, MessageId id, Message data)
{
	PioState *pioState = (PioState *) task;
	switch(id)
	{
	case internal_pio_timer_message:
	{
		const TimedMessage **m = (const TimedMessage **) data;
		const TimedMessage *p = *m;
		uint16 temp_timed_id = pioState->pio_states.timed_id;

		if (p->release)
			pioState->pio_states.timed_id = p->id;
		else
		{
			MessageSend(pioState->client, p->id, 0);
			pioState->pio_states.timed_id = 0;
		}
		pioState->pio_states.store_held = Unknown;
		if(p->repeat)
		{
			if(p->msecRepeat)
			{
				pioState->pio_states.timed_id = temp_timed_id;
				send_pio_timed_message(pioState, p, 1);
			}
			else
			{
				send_pio_timed_message(pioState, p, 0);
			}
		}
	}
	break;
	case MESSAGE_PIO_CHANGED:
	{
		const MessagePioChanged *m = (const MessagePioChanged *)data;
		pioChanged(task, pioState, m->state^pioState->pio_states.pskey_wakeup);
	}
	break;
	case double_pio_press_timer:
	default:
		break;
	}
}

void pioInit(PioState *pioState, Task client)
{
	MessagePioChanged *m = malloc(sizeof(MessagePioChanged));
	uint16 pio_get = PioGet();
	uint16 pskey_wakeup = 0xFFFF;

	pioState->task.handler = pioHandler;
	pioState->client       = client;

	PsFullRetrieve(PSKEY_PIO_WAKEUP_STATE, &pskey_wakeup, sizeof(pskey_wakeup));

	pskey_wakeup = ~pskey_wakeup;

	pioState->pio_states.store_held = Unknown;
	pioState->pio_states.double_press = Unknown;
	pioState->pio_states.store_count = 0;
	pioState->pio_states.store_bits = 0;
	pioState->pio_states.timed_id = 0;
	pioState->pio_states.pskey_wakeup = pskey_wakeup;
	pioState->pio_states.pio_raw_bits = ~(pio_get^pskey_wakeup) & ((1UL<<10));

	(void) MessagePioTask(&pioState->task);
	PioDebounce((1UL<<4)|(1UL<<5)|(1UL<<10), 2, 20);

	m->state = pio_get & ((1UL<<4)|(1UL<<5)|(1UL<<10));
	m->time  = 0;
	MessageSend(&pioState->task, MESSAGE_PIO_CHANGED, m);
}
