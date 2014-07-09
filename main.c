#include "debug.h"
#include "hal.h"
#include "sppb.h"
#include "scanner.h"


int main(void)
{					
	hal_init(getSppbTask(), getScannerTask());
	sppb_init(getHalTask());
	scanner_init(getHalTask(), getSppbTask());
	
	MessageLoop();	
    
    return 0;
}



