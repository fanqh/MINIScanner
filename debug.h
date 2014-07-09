#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG_ENABLED
#define DEBUG(x) {printf x;}
#else
#define DEBUG(x)
#endif

#endif /** DEBUG_H */

