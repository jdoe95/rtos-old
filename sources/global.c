/***********************************************************************
 * GLOBAL VARIABLES
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 ************************************************************************/
#include "../includes/config.h"
#include "../includes/global.h"
#include "../includes/functions.h"

Heap_t heap;
MemoryList_t kernelMemoryList;

PrioritizedList_t threads_timed;
PrioritizedList_t threads_ready;

Thread_t *volatile currentThread;
Thread_t *volatile nextThread;

volatile osCounter_t systemTime;
volatile osCounter_t criticalNesting;

Thread_t idleThread;
NotPrioritizedList_t timerPriorityList;
