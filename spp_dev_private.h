/****************************************************************************
Copyright (C) Cambridge Silicon Radio Limited 2004-2009
Part of BlueLab 4.1.2-Release

FILE NAME
    spp_dev_private.h
    
DESCRIPTION
    
*/

#ifndef _SPP_DEV_PRIVATE_H_
#define _SPP_DEV_PRIVATE_H_

#include <spp.h>
#include <message.h>
#include <app/message/system_message.h>
#include "sppb.h"

#define FRAME_SIZE		0

#define SPP_MSG_BASE    (0x0)

#define SPP_DEV_INQUIRE_TIMEOUT_DURATION 	50000
#define SPP_DEV_CONNECTED_TIMEOUT_DURATION	50000

enum {
    SPPB_PAIRABLE_TIMEOUT_IND = SPP_MSG_BASE,
	SPPB_ECHO_COMMAND_TIMEOUT,					/** for echo command timeout **/
	SPPB_ECHO_TIMEOUT_IND,						/** for echo time-out **/
	SPPB_ECHO_SINK_READY,						/** designed for blocked sending to sink, not used now **/
	SPPB_PIPE_UART_SINK_READY,					/** uart sink is ready to send, schedule this message when SPP_MESSAGE_MORE_DATA **/
	SPPB_PIPE_SPP_SINK_READY,					/** spp sink is ready to send, schedule this message when MESSAGE_MORE_DATA **/
	SPPB_PIPE_COUNT_DOWN						/** instead of message time-out, we use count down scheme to do timeout **/
};

typedef struct {
	
	uint16 size;
	uint8 buf[1];
	
} buffered_bytes_t;

typedef struct {
	
	uint8 output[32];
			
} sppb_echo_back_message_t;


#endif /* _SPP_DEV_PRIVATE_H_ */

