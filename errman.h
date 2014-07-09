#ifndef ERRMAN_H
#define ERRMAN_H

#include <csrtypes.h>

#define BUZZ_MASK (1UL << 11)
#define K_BeepCounter_DriveHigh 5000
#define K_BeepCounter_DriveLow	1000
#define K_BeepTimes 			5

void busy_beep(uint16 times);
void busy_beep_unit(void);

void DoErrorCheck( bool flag );
void initialisationFinished(void);

void raise_exception(uint16 m, uint16 n);

#endif /** ERRMAN_H **/



