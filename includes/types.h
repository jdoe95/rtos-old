/**********************************************************************************
 * KERNEL STRUCTURE DESCRIPTION FILE
 *
 * AUTHOR BUYI YU
 * 	1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * Kernel Structures
 *
 * ABOUT LISTS+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Most kernel objects (various control blocks) are managed by doubly linked lists,
 * (especially those that can be added and removed dynamically), and the items in the
 * list are called list items. The end item and first item of the list are linked
 * together, forming a ring structure, so that no matter how an iterator traverses
 * the list, it will always point to items within the list, making pointer sanity
 * checks unnecessary. List header structures hold information of the list by pointing
 * to the first item of the list. All items can be accessed starting from any item in
 * any direction.
 *
 * $$--listItemCookie:
 *
 * 	The very basic common portion of all doubly-linked list items used in this kernel.
 * 	All pointer to advanced list item types can be converted to pointer to this type,
 * 	as a result, all list insert and remove operations of more advanced list item types
 * 	can collapse back into this type to reduce final code size.
 *
 * $$--prioritizedList & notPrioritizedList:
 *
 *  List headers.
 *
 * $$--notPrioritizedListItem:
 *
 * 	More advanced list item. The items are arranged in the list following no specific
 * 	order. The list pointer allows its list header (thus the whole list) to be accessed
 * 	from this item if this item is in a list, while the container pointer usually
 * 	points to a bigger structure that holds this list item (i.e. the object control
 * 	block this list item is in), allowing the object control block to be accessed.
 *
 * $$--prioritizedListItem:
 *
 *  More advanced list item, but prioritized. The only difference between
 *  notPrioritizedListItem and prioritizedListItem is that the latter contains an
 *  additional value field which allows the list item to be inserted into the list
 *  with the items with the smallest numbers at the begining of the list. This list
 *  is usually used by the scheduler to keep track of priorities or timeout values.
 *
 *  Item remove functions on prioritized and notPrioritized lists are essentially the
 *  same, because the structs are convertable. In some places having this can be
 *  beneficial.
 *
 * ABOUT WAIT STRUCTS++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * In functions calling thread_blockCurrent, after the thread is blocked, some
 * operations need to be completed by other thread/ISRs. The information about the
 * operations is stored in a structure called wait struct. The wait struct will be
 * set upon entering blocking mode, and other threads/ISRs can extract the information
 * and complete the operations so that the thread can be readied again, and the result
 * of these operations will be stored back into the wait struct to be extracted by the
 * original thread.
 *
 * ABOUT THE HEAP++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * $$--INTRODUCTION
 *
 * Heap allows the kernel and application to allocate and free memory dynamically.
 * Freed memory can be allocated to other threads again. This enables highly efficient
 * memory utilization. The memory in the heap are managed by linking the unused memory
 * blocks together forming a doubly linked list, while the used memory are removed from
 * the list and given to the application, and the freed memory returned by the application
 * are inserted back into the heap to be used again.
 *
 * $$--MANAGEMENT OF HEAP BLOCKS
 *
 * Since the application might request a block of memory with a size of any possible
 * values, larger blocks in the heap will be splitted and then removed from the heap if the
 * application only requests only a small amount of memory. As a result, the heap blocks
 * can be of any size. But we can't define so many structs to describe each block of
 * a certain size. Instead, all heap blocks have a common section in the begining bytes,
 * called a block header, in which reside the list item structure and an unsigned number
 * that stores the total size of the memory block (including the header itself). The
 * memory after the block header, is simply left empty. When the block is given to the
 * application, a pointer to the empty portion of the it will be returned (the famous
 * void* return value of malloc), and the applicaiton can make as many modifications
 * on the portion as it wants, so long as it doesn't make the pointer go beyound the
 * heap block or go into the header section, damaging this or other  blocks. From the
 * kernel's point of view, operations on the blocks, simply become
 * operations on the block headers.
 *
 * $$--MERGING ADJACENT BLOCKS
 *
 * When the operating system starts, a chunk of memory will be added to the heap as an
 * initial block. During normal operation, the block will be splitted many times. At a
 * certain time, some portion splitted will be returned, while some are still used by
 * the application. As time goes by, more and more portions get splitted, and more
 * memory blocks will be created. Because the header portion of the blocks cannot be
 * used by the application, as more blocks get created, more block headers get created,
 * the avaliable memory will shrink. In the end, a large amount of small memory fragments
 * will be all over the heap and the system will less likely to be able to accept further
 * allocation requests from the application. However, if we can merge the adjacent blocks,
 * eliminating the unnecessary headers, the fragmentation will be reduced and the system
 * will have very good long term performance. In order to do this, the memory blocks need
 * to be arranged by the value of their pointers, so that when
 * pointer_to_start_of_this_memory_block + size_of_this_memory_block =
 * pointer_to_start_of_its_next_memory_block, or when
 * pointer_to_start_of_the_previous_block + size_of_the_previous_block =
 * pointer_to_start_of_this_memory_block, the blocks can be merged, forming one large
 * block. The merge check is done every time a memory block is returned to the heap.
 *
 * $$--ALIGNMENT
 *
 * Some platforms will require some data types to be aligned to certain address bounderies.
 * For example, on ARM Cortex-M, under some conditions, the address of a uint32_t should
 * be aligned to 4-byte bounderies (its address can only be a multiple of 4). If the
 * application calls osMemoryAllocate to get a block of memory for an uint32_t* array, and
 * the address returned by the system is not a multiple of 4, the processor will not be
 * happy about it. As a result, the system will assume maximum memory alignment
 * requirements. For example, if the processor requires uint8_t to be aligned to a 1-byte
 * boundary (always aligned), and uint32_t to be aligned to a 4-byte boundary, the system
 * will always return an address of a multiple of 4 when calling osMemoryAllocate. In
 * order to do this, some tricks need to be done to the memory blocks. Assuming the processor
 * is 4 byte aligned, if we make the size of the block header a multiple of 4, the pointer
 * returned to the application will always be a mutiple of 4, if the memory block start
 * address is also aligned to a 4 byte boundary. If the initial memory block is aligned,
 * in order for all memory block start addresses to be aligned, the size of any memory
 * blocks will also need to be a multiple of 4. In summary, the following requirements
 * must be met:
 * 		start address of initial memory block 		IS A MULTIPLE OF 4
 * 		size of block header						IS A MULTIPLE OF 4
 * 		size of any whole block						IS A MULTIPLE OF 4
 * *This is just an example. The alignment is configurable in config.h
 *
 * When the application requests a memory size of 45 bytes, instead of returning a memory
 * block of 45 + sizeof(header) bytes, the system will allocate a memory block of
 * ALIGN_4(header_size) + ALIGN_4(45) bytes, which is ALIGN_4(header_size) + 48 bytes.
 * The application can call osMemoryUsableSize to find out how many bytes are actually
 * allocated in order to utilize the extra memory.
 *
 * $$--NEXT FIT ALGORITHM VERSES OTHERS
 *
 * When the application requests a block of memory, the kernel will traverse the heap
 * to find a block that has a size large enough and decides if the block should be
 * splitted. If the kernel starts at the begining of the heap, and uses the first
 * block it finds that fits the application's need, the method is called a FIRST FIT;
 * if the kernel starts at the block after the last block that was splitted, and uses
 * the first block that it found legit, the method is called a NEXT FIT; if the
 * kernel traverses the whole heap and only use the smallest block that is large
 * enough for the application, this is called a BEST FIT.
 *
 * FIRST FIT -- Fast, but causes a large amout of very small pieces of memory to
 * build up at start of the heap. Degrades allocation speed over time because the
 * kernel have to go through a lot of small blocks at the begining to find a legit
 * large one. (TECHNICAL TERM: EXCESSIVE FRAGMENTATION AT START OF HEAP)
 *
 * NEXT FIT -- Fast, causes small pieces of memory blocks to scatter all over the heap;
 * preventing regional fragmentation build-up, thus having good long-term performance.
 * But makes large blocks harder to find.
 *
 * BEST FIT -- Slow, needs to go through the whole heap to make a decision, but can
 * reduce fragmentation build-up. Many large pieces of memory will also be more likely
 * to exist.
 *
 * This kernel uses the NEXT FIT allocation algorithm.
 *
 * ABOUT THE SCHEDULER++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * $$--INTRODUCTION
 *
 * The scheduler is a made-up component consisting of the functions that manipulates
 * the scheduler lists, which is threads_ready, threads_timed, as well as those thread
 * lists in various kernel objects (queues, semaphores, mutexes, etc).
 *
 * $$--WHY LISTS
 *
 * Managing threads in lists has some major advantages. Lists can hold ideally any number
 * of items, the memory of a list does not have to be continuous in space and time like
 * an array does, and any item can be added or removed at any time very quickly. As a result,
 * this kernel supports ideally an unlimited number of threads. The priority is managed
 * by prioritized lists, which puts the item with the smallest numbers at the begining,
 * and items with equal numbers are simply put next to each other. The scheduler will
 * assume the items at the begining of the list to have the highest priority, and thus the
 * smaller the number, the higher the priority.
 *
 * Lists, however, have some disvantages. Besides consuming more memory for each item,
 * lists are undeterministic meaning that the time(machine cycles) it takes for some list
 * operations to complete depend totally on the runtime situation (i.e. number of items in
 * the list), which is usually the case for prioritized insert functions, where the
 * processor needs to go through a loop structure to find where to place the new item,
 * making this unfit for hard real-time situations.
 *
 * $$--SCHEDULING SCHEMES
 *
 * The scheduler is always made sure to run the highest priority threads in the ready list.
 * If a thread with higher priority is readied, the scheduler will immediately send
 * a request wishing to switch to that thread. This is the key feature of a real time
 * operating system. If multiple threads has the highest priority, the scheduler will
 * run them all individually for a period of time, because a reschedule request is generated
 * every time the tick timer times out, until it reaches a list item with a value larger
 * than the highest priority number, in which case it will start at the begining of the list
 * again. In short, this scheduler integrates both preemptive and round-robin scheduling.
 *
 * $$--HOW TIMED BLOCKING IS ACHIEVED
 *
 * If a non-zero(finite) timeout value is specified when calling thread_blockCurrent,
 * the scheduler will calculate the time at which the thread should wake up based on
 * the current system time, and the timer list item will be set and inserted into the
 * system time out list thread_timed. Upon every tick, the scheduler will check if any
 * threads in the timeout list has expired, and if so, the scheduler list item of the
 * thread will be remved from its list, if any, and inserted into the ready list,
 * marking the readiness of the thread. A reschedule request will be generated if the
 * thread has a higher priority than the existing highest priority thread. Additionally,
 * the expired timer list item will also be removed from threads_timed.
 *
 * osThreadDelay exports this feature to the user and can achieve an 'idle' delay
 * -- where the processor can switch to execute something else for a period of time,
 * compared to a 'busy' delay which works by wasting processor cycles. The precision
 * of osThreadDelay is 1 tick period and the error will vary based on the calling
 * thread's priority and how heavily the processor is loaded.
 *
 * Interesting to note that because the expiration time is calculated every time the thread
 * enters timed blocking mode, timing errors might accumulate, and this error will almost
 * always be a positive error (longer blocking time than expected).
 *
 * GLOBAL VARIABLES AND MUTUAL EXCLUSION+++++++++++++++++++++++++++++++++++++++++++
 *
 * $$--DEVASTATING EFFECTS OF LEAVING A GLOBAL RESOURCE HALF-MODIFIED
 *
 * For a data operation to be successful, it first needs to be done right, and secondly,
 * it has to be completed. This would seem obvious but on multi-threaded enviroment,
 * for multi-step operations (operations that take multiple instructions to complete or
 * operations that use 1 single instruction, but the instruction can be interrupted),
 * it's very easy for a resource to be half-modified, because another thread or an ISR
 * can preempt in the middle of an operation, and trys to read the incomplete resource
 * or tries to perform other operations on it! Since global resources can be accessed
 * by all threads, and shared resources can be accessed by multiple designated threads,
 * multi-step operations on these resources have to be protected in a critical section,
 * where only one thread is allowed to access it.
 *
 * $$--KERNEL LEVEL MUTUAL EXCLUSION
 *
 * There are many types of mutual exclusion mechanisms, and the kernel can provide
 * more advanced mutual exclusion mechanisms such as semaphores and mutexes to the
 * user. But in the kernel itself, these software mutual exclusions are not available.
 *
 * The best way to implement mutual exclusion is to assign a lock for each set of
 * resources. All operations on the lock are atomic(undividable), thus cannot be
 * interrupted. But in order to be locked, the lock has to be in an unlocked state,
 * otherwise the lock operation will halt until the lock is unlocked. Before
 * accessing the resource, every thread has to lock the lock to gain access rights
 * to it, thus no more than one thread can modify it at the same time, and when the
 * operation is completed, the lock will be unlocked. This mechanism is provided by
 * many but not all processor's hardware, and usually there is a limited number of
 * locks that can be used. Furthermore, when other threads try to lock a locked lock,
 * the processor will halt, doing nothing until the thread is scheduled out, resulting
 * in low effective CPU utilization. Other similar methods also exist, depending on the
 * architecture.
 *
 * The simpliest and the most widely available way to ensure mutual exclusion is to
 * disable interrupt, thus disabling all ISRs and the scheduler (which works by
 * interrupting the processor), so that all processes and ISRs that MIGHT access this
 * resource cannot execute until this operation is done, no matter if they actually
 * will access this resource or not. This will have higher effective CPU utilization
 * (only on single core processors), but will result in prolonged interrupt service
 * time, thus degraded real-time response.
 *
 * This kernel disables interrupts to ensure mutual exclusion. On global kernel
 * resources, when the thread is performing operations of the following types:
 *
 * 		atomic read-only		-- no interrupt disabling/enabling needed
 * 		atomic modification		-- no interrupt disabling/enabling needed
 * 		non-atomic read-only	-- disable interrupt before write, enable when write done
 * 		non-atomic modification	-- disable interrupt before write, enable when write done
 *
 * If, in the future, this kernel should adapt to mutual exclusion mechanisms other
 * than disabling interrupts, which allows other threads to run concurrently while
 * performing operations on certain resource (i.e. ported to use processor hardware
 * locks), when the thread is performing operations of the following types:
 *
 * 		atomic read-only		-- no locking/unlocking needed
 * 		atomic modification		-- no locking/unlocking needed
 * 		non-atomic read-only	-- acquire lock before read, release lock after read
 * 		non-atomic modification	-- acquire lock before read, release lock after read
 *
 * $$--THE VOLATILE KEYWORD
 *
 * Declaring a field volatile will inhibit any compiler optimizations that assume
 * access by a single thread. For example, when writing to a variable multiple times
 * with the same value, the compiler will sometimes optimize out all unncessary
 * consecutive writes, leaving only 1 write instruction, because it assumes that the
 * variable does not change between writes. When declared volatile, the compiler
 * will generate each and every write instruction.
 *
 * On multi-threaded platforms, it is necessary to ensure modifications to shared
 * resources happen in a timely manner.
 *
 * ABOUT TIMERS+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Timer is a convenient way to run periodic tasks. Instead of defining a function that
 * performs the periodic task and then calls osThreadDelay, the system will provide
 * threads of different priorities that periodically call callback functions.
 * When a timer is started, a thread that has the specified priority will be created.
 * If another timer of a different priority is started, a different thread with that
 * priority will be also created. The created timer threads are managed by the timer
 * thread management list, in which one item indicates a timer thread. The items in
 * the timer thread management list also has a list that has all the timer callback
 * functions of the same priority. These callback functions will all be run by the
 * same thread, whose delay time is set to the smallest period of the timer callback
 * functions in the timer callback function list of the timer thread management list
 * item.
 **********************************************************************************/
#ifndef H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9
#define H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9

/* list related */
struct listItemCookie;
struct prioritizedListItem;
struct notPrioritizedListItem;
struct prioritizedList;
struct notPrioritizedList;
typedef struct listItemCookie 				ListItemCookie_t;
typedef struct prioritizedListItem 			PrioritizedListItem_t;
typedef struct notPrioritizedListItem 		NotPrioritizedListItem_t;
typedef struct prioritizedList 				PrioritizedList_t;
typedef struct notPrioritizedList 			NotPrioritizedList_t;

/* mmeory related */
struct memoryBlock;
struct memoryList;
struct heap;
typedef struct memoryBlock 					MemoryBlock_t;
typedef struct memoryList 					MemoryList_t;
typedef struct heap 						Heap_t;

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

/*************************************************************************/
/* basic list item structure, used for insert and remove operations */
struct listItemCookie
{
	ListItemCookie_t *volatile prev;
	ListItemCookie_t *volatile next;
};

/* advanced list item structure, can collapse back to listItemCookie */
struct prioritizedListItem
{
	PrioritizedListItem_t *volatile prev;
	PrioritizedListItem_t *volatile next;

	/* pointer to the list the item is in, or NULL */
	PrioritizedList_t *volatile list;

	/* pointer to the struct that this item is a member of,
	 * used to access control structs from list item */
	void *volatile container;

	/* value of this item. Items with lower values are placed at
	 * the head of the list */
	volatile osCounter_t value;
};

/* advanced list item structure, can collapse back to listItemCookie */
struct notPrioritizedListItem
{
	NotPrioritizedListItem_t *volatile prev;
	NotPrioritizedListItem_t *volatile next;

	/* pointer to the list the item is in, or NULL */
	NotPrioritizedList_t *volatile list;

	/* pointer to the struct that this item is a member of,
	 * used to access control structs from list item */
	void *volatile container;
};

/* items are ordered in this list, with smallest values at the begining */
struct prioritizedList
{
	PrioritizedListItem_t *volatile first;
};

/* items are following no specific order */
struct notPrioritizedList
{
	NotPrioritizedListItem_t *volatile first;
};
/*************************************************************************/
/* memory block header, can collapse back to listItemCookie */
struct memoryBlock
{
	MemoryBlock_t *volatile prev;
	MemoryBlock_t *volatile next;

	/* size of the memory block */
	volatile osCounter_t size;

	/* The block is of various size, many bytes in the block might follow. */
};

/* holds blocks removed from the heap. Blocks placed in this struct are in use */
struct memoryList
{
	MemoryBlock_t *volatile first;
};

/* The heap. Manages all free memory blocks.
 * 'current' pointer is used by the next fit algorithm */
struct heap
{
	MemoryBlock_t *volatile first;
	MemoryBlock_t *volatile current;
};
/*************************************************************************/
struct thread
{
	/* the process stack pointer, only used when the thread is not executing, when
	 * it will be set to the last processor stack pointer register value.
	 * When the thread is executing, the processor stack pointer register will
	 * point to the exact stack top location instead.
	 * The PSP should be easily accessible to the context switcher which is
	 * writtern in assembly, so it is placed at the begining here.*/
	osByte_t *volatile PSP;

	/* the scheduler list item, as name indicates, will be put into scheduler's lists.
	 * In order to be converted to a single prioritized list item,
	 * the following two members have to stay together */
	NotPrioritizedListItem_t schedulerListItem;
	volatile osCounter_t priority;

	/* only used when the thread is in blocking mode with a non-zero timeout.
	 * This list item with its value set to a future system time will be inserted
	 * into the system timeout list upon blocking, checked regularly by the tick
	 * timer, and the whole thread will be put into ready state when the
	 * timeout have passed. */
	PrioritizedListItem_t timerListItem;

	/* pointer to the stack memory. the PSP should be pointing into the stack memory.
	 * Otherwise the stack would overflow, damaging important data, and the kernel would
	 * go mayhem. This usually occures when using stack-consuming functions like
	 * printf, also don't define a very large array in functions. Currently the kernel
	 * doesn't check stack pointers, even if it does, it can't make sure overflows don't
	 * happen. The best way is to protect the processor memory map with a memory
	 * protection unit, but this is only available on certain architectures.
	 * Work is still to be done on memory protection. For now, try assigning a 1-2 KiB
	 * stack size when using printf. */
	osByte_t *volatile stackMemory;

	/* when calling osMemoryAllocate, all the memory blocks taken from the heap will
	 * be put here. This list is cleared when the thread terminates, recycling all
	 * un-freed memory automatically. */
	MemoryList_t localMemory;

	/* pointer to the wait struct. Before the current thread blocks, a wait struct
	 * will be allocated on the current thread's stack with important values set and
	 * is then docked here.
	 * The wait structs are usually very small, allocating it in the heap would be
	 * too expensive and will cause more fragmentation. So the wait struct is on the
	 * thread's stack. Simply define it in the function body and let the compiler
	 * take care of the allocation and deallocation. */
	void *volatile wait;

	/* the thread's state. the kernel ignores this because it can determine the
	 * state of the thread based on the scheduling lists, before it gained access
	 * to this control block. This is for the user to quickly get the state without
	 * needing to search for this thread in those scheduling lists. */
	volatile osThreadState_t state;
};
/*************************************************************************/
struct signal
{
	/* threads blocked for signal */
	PrioritizedList_t threadsOnSignal;

	/* size of the signal info block, set on creation */
	osCounter_t infoSize;
};

/* wait struct for threads intended to wait for a particular signal */
struct signalWait
{
	/* Signal value to wait for, set by the waiting thread */
	osSignalValue_t signalValue;

	/* Additional signal information block buffer, set by the waiting thread */
	void* info;

	/* wait result, set to false by the blocking thread
	 *
	 * If the blocking thread timed out, this value will remain false.
	 *
	 * Set by the thread sending the signal to true, indicating the blocked
	 * thread's wish is fulfilled -- the interested signal is sent.
	 */
	volatile osBool_t result;
};

/*************************************************************************/
struct mutex
{
	/* thread list holding all threads waiting to lock this mutex */
	PrioritizedList_t threads;

	/* indicates whether the mutex is free to lock */
	osBool_t volatile locked;
};

struct recursiveMutex
{
	/* thread list holding all threads waiting to lock this mutex */
	PrioritizedList_t threads;

	/* owner of the mutex */
	Thread_t* volatile owner;

	/* nesting coutner */
	volatile osCounter_t counter;
};

/* wait struct for threads intended to lock the mutex, but the mutex is
 * currently unavailable. */
struct mutexWait
{
	/* wait result, set to false by the thread intended to lock the mutex, but
	 * the mutex is currently not available.
	 *
	 * If the thread timed out, this value will remain false.
	 *
	 * Set by the thread releasing the lock to true, indicating the blocked
	 * thread's wish is fulfilled -- the mutex handed over to the latter.
	 */
	volatile osBool_t result;
};
/*************************************************************************/
struct semaphore
{
	/* thread list holding all threads waiting for the semaphore if the semaphore
	 * counter became 0 */
	PrioritizedList_t threads;

	/* the semaphore counter. counts up on posts; counts down on waits until 0 */
	volatile osCounter_t counter;
};

/* wait struct for threads intended to wait down 1 on the semaphore (decrease the
 * semaphore counter), but the semaphore counter is currently zero */
struct semaphoreWait
{
	/* wait result, set to false by the thread intended to wait a value down the
	 * semaphore, but the semaphore counter is zero.
	 *
	 * If the thread timed out, this value will remain false.
	 *
	 * Set by the thread posting to the semaphore to true, indicating the blocked
	 * thread's wish is fulfilled, -- the new semaphore is given to the latter.
	 */
	volatile osBool_t result;
};
/*************************************************************************/
struct queue
{
	/* thread waiting to read from the queue, but there's currently not enough data */
	PrioritizedList_t readingThreads;

	/* thread waiting to read from the queue from behind(read the newest data first),
	 * but there's currently not enough data. */
	PrioritizedList_t readingBehindThreads;

	/* thread wating to write to the queue, but there's currently not enough space */
	PrioritizedList_t writingThreads;

	/* thread waiting to write to the queue ahead (so that it can be read first), but
	 * there's currently not enough space. */
	PrioritizedList_t writingAheadThreads;

	/* temporary storage for write before a read is called, holds all the data written
	 * to the queue */
	osByte_t *memory;

	/* size of queue memory, specified by user when calling create */
	volatile osCounter_t size;

	/* offset of the next data to be read, should revert back to beginning of queue
	 * memory */
	volatile osCounter_t read;

	/* offset of the next free byte to write, should revert back to begining of
	 * queue memory */
	volatile osCounter_t write;
};

/* wait struct for threads intended to read data, but there's currently
 * not enough data in the queue to read from */
struct queueReadWait
{
	/* wait result, set to false by the thread intended to read data from the queue,
	 * but there's not enough data in the queue to read from.
	 *
	 * If the thread timed out, this value will remain false.
	 *
	 * Set by the thread writing to the queue, indicating the blocked thread's
	 * wish is fulfilled -- all the data are read.
	 */
	volatile osBool_t result;

	/* size of requested data, set by the blocking thread */
	osCounter_t size;

	/* pointer to data buffers, set by the blocking thread, write only */
	void *data;
};

/* wait struct for threads intended to write data, but there's currently
 * nore enough space in the queue to write to */
struct queueWriteWait
{
	/* wait result, set to false by the thread intended to write data to the queue,
	 * but there's not enough space in the queue to write to.
	 *
	 * If the thread timed out, this value will remain false.
	 *
	 * Set by the thread reading from the queue, indicating the blocked thread's
	 * wish is fulfilled -- all the data are written.
	 */
	volatile osBool_t result;

	/* size of requested data, set by the blocking thread */
	osCounter_t size;

	/* pointer to the data buffer, set by the blocking thread, read only */
	const void *data;
};
/*************************************************************************/
typedef void (*TimerCallback_t)( void* argument );

/* the timer control block, represents one timer */
struct timer
{
	/* the following two items stay together in order to be converted to a
	 * single PrioritizedListItem_t*
	 */
	NotPrioritizedListItem_t timerListItem;
	osCounter_t volatile futureTime;

	/* timer mode, periodic or one shot */
	osTimerMode_t volatile mode;

	/* period of delay */
	osCounter_t volatile period;

	/* the callback function and argument of the timer */
	TimerCallback_t volatile callback;
	void * volatile argument;

	/* allow access to the timerPriorityBlock control structure */
	TimerPriorityBlock_t* volatile timerPriorityBlock;
};

/* the timer priority block, represents a series of timer of the same priority */
struct timerPriorityBlock
{
	/* to be inserted into the system timer priority list */
	NotPrioritizedListItem_t timerPriorityListItem;

	/* The daemon thread of the timers, is of a specific priority */
	Thread_t* volatile daemon;

	/* timers in this list are counting */
	PrioritizedList_t timerActiveList;

	/* timers in this list are not counting */
	NotPrioritizedList_t timerInactiveList;
};

/*************************************************************************/

#endif /* H69A8BA22_43BC_4DD0_A0DE_0D110859E2C9 */
