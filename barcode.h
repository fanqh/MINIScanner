#ifndef BARCODE_H
#define BARCODE_H

#include <csrtypes.h>

#define MAXIMUM_BARCODE_LENGTH 				(48)	/** CR/LF trailing included **/

typedef struct {
	
	uint16 length;
	uint16 cursor;	/** this is used in hid only **/
	uint8 code[MAXIMUM_BARCODE_LENGTH];
		
} barcode_t;

void barcode_clear(barcode_t* bcode);

bool barcode_fill_raw_bytes(barcode_t* bcode, const uint8* src, uint16 length);

bool barcode_is_terminated(barcode_t* bcode);

#endif /* BARCODE_H */



