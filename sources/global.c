/** ***********************************************************************
 * @file
 * @brief Global Variables Definition
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the definitions of global variables.
 *************************************************************************/
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
