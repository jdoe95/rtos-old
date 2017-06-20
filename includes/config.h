/*************************************************************************
 * IMPORT CONFIG FROM THE PORTABLE FOLDER
 *
 * AUTHOR BUYI YU
 * 	1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	Import config from the portable folder and also perform sanity checks.
 * 	This file will be included in every source file in the RTOS.
 *************************************************************************/
#ifndef H35FB3D3C_A33A_41DD_982A_5A216B9FCD28
#define H35FB3D3C_A33A_41DD_982A_5A216B9FCD28

#include "../portable/config.h"

#ifndef INTERRUPT
	#define INTERRUPT
#endif

#ifndef INLINE
	#error define INLINE in portable/config.h
#endif

#ifndef NORETURN
	#error define NORETURN in portable/config.h
#endif

#ifndef MEMORY_ALIGNMENT
	#error define MEMORY_ALIGNMENT in portable/config.h
#endif

#ifndef IDLE_THREAD_STACK_SIZE
	#error define IDLE_THREAD_STACK_SIZE in portable/config.h
#endif

#ifndef TIMER_THREAD_STACK_SIZE
	#error define TIMER_THREAD_STACK_SIZE in portable/config.h
#endif

#ifndef OS_TICK_HANDLER_NAME
	#error define OS_TICK_HANDLER_NAME with your heart tick interrupt handler name in portable/config.h
#endif

#ifndef OS_ASSERT
	#define OS_ASSERT( cond )
#endif

#endif /* H35FB3D3C_A33A_41DD_982A_5A216B9FCD28 */
