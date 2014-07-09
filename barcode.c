#include "barcode.h"

static bool isDisplayableChar(uint8 chr);
static bool isLineFeed(uint8 chr);
static bool isCarriageReturn(uint8 chr);
static bool fill_byte(barcode_t* bcode, uint8 chr);

/** tab is not included **/
static bool isDisplayableChar(uint8 chr) {
	
	if (chr >= ' ' && chr <='~') {
		
		return TRUE;
	}
	
	return FALSE;
}

static bool isLineFeed(uint8 chr) {
	
	return (chr == 0x0A);
}

static bool isCarriageReturn(uint8 chr) {
	
	return (chr == 0x0D);
}

static bool fill_byte(barcode_t* bcode, uint8 chr) {
	
	uint16 len = bcode ->length;
	
	/** if overflow, refuse **/
	if ( len >= MAXIMUM_BARCODE_LENGTH ) {
		
		return FALSE;
	}
	
	/** if first character **/
	if ( len == 0) {
		
		if ( isDisplayableChar(chr) ) {	/** only displayable char is allowed for the first char **/
			
			bcode ->code[0] = chr;
			bcode ->length++;
			
			return TRUE;
		}
		
		return FALSE;
	}
	
	if ( isLineFeed(bcode ->code[len - 1]) ) {	/** if trailing char is LF, then no more char is allowed **/
		
		return FALSE;
	}
	else if ( isCarriageReturn(bcode ->code[len - 1]) ) { /** if trailing char is CR, then only LF is allowed **/
		
		if ( isLineFeed(chr) ) {
			
			bcode ->code[len] = chr;
			bcode ->length++;
			
			return TRUE;
		}
		
		return FALSE;
	}
	else { /** only displayable char or CR is allowed **/
		
		if ( isDisplayableChar(chr) || isCarriageReturn(chr) ) {
			
			bcode ->code[len] = chr;
			bcode ->length++;
			
			return TRUE;
		}
		
		return FALSE;
	}
}

void barcode_clear(barcode_t* bcode) {
	
	if (bcode) {
		
		uint16 i;
		
		bcode ->length = 0;
		
		for (i = 0; i < MAXIMUM_BARCODE_LENGTH; i++) {
			
			bcode ->code[i] = 0;
		}
	}
}

bool barcode_fill_raw_bytes(barcode_t* bcode, const uint8* src, uint16 len) {
	
	uint16 i;
	
	if (len == 0) {
		
		return TRUE;
	}
	
	if (bcode ->length + len > MAXIMUM_BARCODE_LENGTH) {
		
		return FALSE;
	}
	
	for (i = 0; i < len; i++) {
		
		if (FALSE == fill_byte(bcode, src[i])) {
			
			return FALSE;
		}
	}
	
	return TRUE;
}

bool barcode_is_terminated(barcode_t* bcode) {
	
	if (bcode ->length < 3) /** at least 3, one displayable char and CR/LF **/
	{
		return FALSE;
	}
	
	if ( isCarriageReturn(bcode ->code[bcode ->length - 2]) && isLineFeed(bcode ->code[bcode ->length - 1]) ) {
		
		return TRUE;
	}
	
	return FALSE;
}
