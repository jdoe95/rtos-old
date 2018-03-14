
#ifndef H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9
#define H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9

/** *************************************************************************
 * @ingroup os_list
 * @{
 */
struct listItemCookie;
struct prioritizedListItem;
struct notPrioritizedListItem;
struct prioritizedList;
struct notPrioritizedList;
typedef struct listItemCookie 				ListItemCookie_t;			/**< @brief Typedef for @ref listItemCookie */
typedef struct prioritizedListItem 			PrioritizedListItem_t;		/**< @brief Typedef for @ref prioritizedListItem */
typedef struct notPrioritizedListItem 		NotPrioritizedListItem_t;	/**< @brief Typedef for @ref notPrioritizedListItem */
typedef struct prioritizedList 				PrioritizedList_t;			/**< @brief Typedef for @ref prioritizedList */
typedef struct notPrioritizedList 			NotPrioritizedList_t;		/**< @brief Typedef for @ref notPrioritizedList */
/**
 * @}
 */

/** *************************************************************************
 * @ingroup os_heap
 * @{
 */
struct memoryBlock;
struct memoryList;
struct heap;
typedef struct memoryBlock 					MemoryBlock_t;	/**< @brief Typedef for @ref memoryBlock */
typedef struct memoryList 					MemoryList_t;	/**< @brief Typedef for @ref memoryList */
typedef struct heap 						Heap_t;			/**< @brief Typedef for @ref heap */
/** *************************************************************************
 * @}
 */

/* thread control block */
struct thread;
typedef struct thread 						Thread_t;

/* signal related */
struct signal;
struct signalWait;
typedef struct signal 						Signal_t;
typedef struct signalWait 					SignalWait_t;

/* mutex related */
struct mutex;
struct recursiveMutex;
struct mutexWait;
typedef struct mutex 						Mutex_t;
typedef struct recursiveMutex 				RecursiveMutex_t;
typedef struct mutexWait 					MutexWait_t;

/* semaphore related */
struct semaphore;
struct semaphoreWait;
typedef struct semaphore 					Semaphore_t;
typedef struct semaphoreWait 				SemaphoreWait_t;

/* queue related */
struct queue;
struct queueWriteWait;
struct queueReadWait;
typedef struct queue 						Queue_t;
typedef struct queueWriteWait 				QueueWriteWait_t;
typedef struct queueReadWait 				QueueReadWait_t;

/* timer related */
struct timer;
struct timerPriority;
typedef struct timer Timer_t;
typedef struct timerPriorityBlock TimerPriorityBlock_t;

#include "config.h"
#include "types_external.h"

/**
 * @brief Basic list item structure, used for insert and remove operations
 * @details All pointer to advanced list item types can be converted to pointer
 * to this type, as a result, all list insert and remove operations of more
 * advanced list item types can collapse back into this type to reduce code size.
 */
struct listItemCookie
{
	ListItemCookie_t *volatile prev;	/**< @brief points to the previous item */
	ListItemCookie_t *volatile next;	/**< @brief points to the next item */
};

/**
 * @brief List item used in prioritized lists.
 * @details This structure is usually put into another structure to
 * allow the latter to be inserted into a list. Pointer to this
 * type can be converted to @ref ListItemCookie_t so that insert and remove
 * functions on ListItemCookie_t can also be used on it. Similarly,
 * it can also be converted to pointer to @ref NotPrioritizedListItem_t, so
 * that not prioritized list item functions can also be used.
 */
struct prioritizedListItem
{
	PrioritizedListItem_t *volatile prev;	/**< @brief points to the previous item */
	PrioritizedListItem_t *volatile next;	/**< @brief points to the next item */

	/** @brief pointer to the list this item is in, or NULL for none */
	PrioritizedList_t *volatile list;

	/** @brief pointer to the struct that holds this list item. This allows
	 * access to the struct from the list item. */
	void *volatile container;

	/** @brief value of this list item. Small values are at the front of the list */
	volatile osCounter_t value;
};

/**
 * @brief List item used in not prioritized lists.
 * @details This structure is usually put into another structure to
 * allow the latter to be inserted into a list. Pointer to this
 * type can be converted to @ref ListItemCookie_t and insert and remove
 * functions on ListItemCookie_t can also be used on it.
 */
struct notPrioritizedListItem
{
	NotPrioritizedListItem_t *volatile prev;	/**< @brief points to the previous item */
	NotPrioritizedListItem_t *volatile next;	/**< @brief points to the next item */

	/** @brief pointer to the list this item is in, or NULL for none */
	NotPrioritizedList_t *volatile list;

	/** @brief pointer to the struct that holds this list item. This allows
	 * access to the struct from the list item. */
	void *volatile container;
};

/** @brief The prioritized list header */
struct prioritizedList
{
	PrioritizedListItem_t *volatile first;	/**< @brief points to the first item in the list */
};

/** @brief The not prioritized list header */
struct notPrioritizedList
{
	NotPrioritizedListItem_t *volatile first;	/**< @brief points to the first item in the list */
};

/**
 * @brief The memory block header
 * @details This structure is used in the heap and memory lists to manage
 * used or free memory. The pointer to this type can be casted to @ref
 * ListItemCookie_t so that insert and remove functions used on ListItemCookie_t
 * can also be used on it.
 */
struct memoryBlock
{
	MemoryBlock_t *volatile prev;	/**< @brief points to the previous memory block */
	MemoryBlock_t *volatile next;	/**< @brief points to the next memory block */

	volatile osCounter_t size;	/**< @brief size of this memory block */
};

/**
 * @brief The memory list
 * @details This structure is used to provide a way to manage the blocks removed from
 * the heap. Those memory blocks are usually in use.
 */
struct memoryList
{
	MemoryBlock_t *volatile first;	/**< @brief points to the first memory block */
};

/**
 * @brief The heap
 * @details The heap manages memory by forming memory blocks using the free memory
 * and linking them together. Adjacent memory blocks are merged.
 */
struct heap
{
	MemoryBlock_t *volatile first;		/**< @brief points to the first memory block */

	/** @brief points to the current memory block, used by next fit algorithm */
	MemoryBlock_t *volatile current;
};

/**
 * @brief The thread control block
 */
struct thread
{
	/**
	 * @brief the process stack pointer
	 * @details Only used when the thread is not in the running state.
	 */
	osByte_t *volatile PSP;

	/**
	 * @brief The scheduler list item
	 * @details Together with priority, forms a prioritized list item.
	 */
	NotPrioritizedListItem_t schedulerListItem;
	volatile osCounter_t priority;	/**< @brief the priority of the thread */

	/**
	 * @brief the timer list item
	 * @details This item will be inserted into the system timing list @ref threads_timed
	 * when the thread is blocked and has a finite timeout.
	 */
	PrioritizedListItem_t timerListItem;

	/**
	 * @brief pointer to the stack memory
	 * @details This item is used to release the stack memory when the thread finishes
	 * execution.
	 */
	osByte_t *volatile stackMemory;

	/**
	 * @brief Local memory list
	 * @details This list is used to free all the user-allocated heap memory when
	 * the thread finishes execution.
	 */
	MemoryList_t localMemory;

	/**
	 * @brief docking position for the wait struct
	 * @details Before the thread blocks, a wait struct (defined on the thread's stack)
	 * can be docked here to store information related to the blocked operation.
	 */
	void *volatile wait;

	/**
	 * @brief The thread state
	 * @details This item is used to allow the state of the thread to be returned quickly
	 * when the user calls @ref osThreadGetState.
	 */
	volatile osThreadState_t state;
};

/**
 * @brief The signal control block
 */
struct signal
{
	/**
	 * @brief List of threads blocked for the signal
	 * @details The signal value each thread is waiting for is docked on the
	 * thread control block.
	 */
	PrioritizedList_t threadsOnSignal;
};

/**
 * @brief the docking struct for signal
 */
struct signalWait
{
	/**
	 * @brief the signal value the thread is waiting for
	 * @details set before entering block state
	 */
	osSignalValue_t signalValue;

	/**
	 * @brief buffer for the information the waiting thread wish
	 * to receive once a signal value is received.
	 * @details set before entering block state
	 */
	void* info;

	/**
	 * @brief the wait result
	 * @details set to false by the thread entering docking state
	 */
	volatile osBool_t result;
};

/**
 * @brief the mutex control block
 */
struct mutex
{
	/**
	 * @brief list of threads blocked for the mutex
	 */
	PrioritizedList_t threads;

	/**
	 * @brief the state of the mutex
	 * @details set to true if the mutex is locked
	 */
	osBool_t volatile locked;
};

/**
 * @brief the recursive mutex control block
 */
struct recursiveMutex
{
	/**
	 * @brief list of all threads blocked for the recursive mutex
	 */
	PrioritizedList_t threads;

	/**
	 * @brief owner of the mutex, NULL if none
	 */
	Thread_t* volatile owner;

	/**
	 * @brief recursive counter for locking and unlocking
	 */
	volatile osCounter_t counter;
};

/**
 * @brief the docking struct for mutex and recursive mutex
 */
struct mutexWait
{
	/**
	 * @brief the wait result
	 * @details set to false by the thread entering docking state
	 * set to true before readying the thread.
	 */
	volatile osBool_t result;
};

/**
 * @brief the semaphore control block
 */
struct semaphore
{
	/**
	 * @brief list of all threads blocked for the semaphore
	 */
	PrioritizedList_t threads;

	/**
	 * @brief the semaphore counter
	 */
	volatile osCounter_t counter;
};

/**
 * @brief the docking struct for semaphore
 */
struct semaphoreWait
{
	/**
	 * @brief the wait result
	 * @details set to false by the thread entering docking state
	 */
	volatile osBool_t result;
};

/**
 * @brief the queue control block
 */
struct queue
{
	/**
	 * @brief list of all threads waiting to read from to the queue
	 */
	PrioritizedList_t readingThreads;

	/**
	 * @brief list of all threads waiting to write to the queue
	 */
	PrioritizedList_t writingThreads;

	/**
	 * @brief the queue internal memory
	 */
	osByte_t *memory;


	/**
	 * @brief size of the internal memory
	 */
	volatile osCounter_t size;

	/**
	 * @brief the counter marking the current read position
	 */
	volatile osCounter_t read;

	/**
	 * @brief the counter marking the current write position
	 */
	volatile osCounter_t write;
};

/**
 * @brief the read docking struct for queue
 */
struct queueReadWait
{
	/**
	 * @brief the wait result
	 * @details set to false before entering blocking state,
	 * set to true before readying the thread.
	 */
	volatile osBool_t result;

	/**
	 * @brief size of the data to read
	 * @details set by the thread entering docking state
	 */
	osCounter_t size;

	/**
	 * @brief pointer to the buffer for holding the data
	 * @details set by the thread entering docking state
	 */
	void *data;
};

/**
 * @brief the write docking struct for queue
 */
struct queueWriteWait
{
	/**
	 * @brief the wait result
	 * @details set to false before entering blocking state,
	 * set to true before readying the thread.
	 */
	volatile osBool_t result;

	/**
	 * @brief size of the data to write
	 * @details set by the thread entering docking state
	 */
	osCounter_t size;

	/**
	 * @brief pointer to the data to be written
	 * @details set by the thread entering docking state
	 */
	const void *data;
};

/**
 * @brief the timer callback function type
 */
typedef void (*TimerCallback_t)( void* argument );

/**
 * @brief the timer control block
 */
struct timer
{
	/**
	 * @brief timer list item, to be put into the timer
	 * list of the corresponding timer priority control block
	 * @details used in conjunction with future time to form
	 * a single prioritized list item
	 */
	NotPrioritizedListItem_t timerListItem;
	osCounter_t volatile futureTime;

	/**
	 * @brief timer mode
	 */
	osTimerMode_t volatile mode;

	/**
	 * @brief period of delay
	 */
	osCounter_t volatile period;

	/**
	 * @brief call back function, called upon timeout
	 */
	TimerCallback_t volatile callback;

	/**
	 * @brief argument to the callback function
	 */
	void * volatile argument;

	/**
	 * @brief allows access to the mother timer priority control
	 * block
	 */
	TimerPriorityBlock_t* volatile timerPriorityBlock;
};

/**
 * @brief the timer priority control block
 */
struct timerPriorityBlock
{
	/**
	 * @brief timer priority list item
	 * @details to be inserted into the system timer priority
	 * list
	 */
	NotPrioritizedListItem_t timerPriorityListItem;

	/**
	 * @brief the pointer to the thread control block of
	 * the thread executing on the timer priority.
	 */
	Thread_t* volatile daemon;

	/**
	 * @brief the list for all active timers of the priority
	 */
	PrioritizedList_t timerActiveList;

	/**
	 * @brief the list for all inactive timers of the priority
	 */
	NotPrioritizedList_t timerInactiveList;
};

#endif /* H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9 */
