/***************************************************************************
 * RTOS EXPORTED TYPES DEFINITIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	Types visible to the user should be defined here. Types invisible to
 * 	the user, but visible to all kernel components should be defined
 * 	in types.h. Types visible to only 1 kernel component should be
 * 	defined in the source file of the corresponding component.
 *
 **************************************************************************/
#ifndef H16488323_48F4_461D_8B3F_D30921D74E5A
#define H16488323_48F4_461D_8B3F_D30921D74E5A

#include "config.h"

typedef enum {OSTHREAD_READY = 0, OSTHREAD_BLOCKED, OSTHREAD_SUSPENDED}
osThreadState_t;

typedef enum {OSTIMERMODE_ONESHOT = 0, OSTIMERMODE_PERIODIC}
osTimerMode_t;

#endif /* H16488323_48F4_461D_8B3F_D30921D74E5A */
