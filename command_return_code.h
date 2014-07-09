#ifndef COMMAND_RETURN_CODE_H
#define COMMAND_RETURN_CODE_H

/******************************************
  
  This file defines at command return code
  
  */

typedef enum {
	
	CMD_RET_OK,
	CMD_RET_UNRECOGNIZED,
	CMD_RET_UNSUPPORTED_BAUDRATE,
	CMD_RET_UNSUPPORTED_STOP,
	CMD_RET_UNSUPPORTED_PARITY

} at_command_return_code_t;


#endif /** COMMAND_RETURN_CODE_H **/




