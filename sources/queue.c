/** **************************************************************
 * @file
 * @brief Queue implementation
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of the queue.
 ****************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/**
 * @brief Writes data to the queue
 * @param queue pointer to the queue where data is to be written
 * @param data pointer to the data to be written
 * @param size size of the data in bytes
 */
void
queue_write( Queue_t* queue, const void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	for( ; counter < size; counter++ )
	{
		queue->memory[queue->write] = ( (const osByte_t*) data )[counter];

		if( queue->write < queue->size - 1 )
			queue->write++;
		else
			queue->write = 0;
	}
}

/**
 * @brief Reads data from the queue
 * @param queue pointer to the queue where data is to be read
 * @param data pointer to the buffer data to be put in
 * @param size size of the data in bytes
 */
void
queue_read( Queue_t* queue, void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	for( ; counter < size; counter++ )
	{
		( (osByte_t*) data )[counter] = queue->memory[queue->read];

		if( queue->read < queue->size - 1 )
			queue->read++;
		else
			queue->read = 0;
	}
}

/**
 * @brief Returns the amount of data in the queue, in bytes
 * @param queue pointer to the queue
 * @return the amount of data in the queue, in bytes
 */
osCounter_t
queue_getUsedSize( Queue_t* queue )
{
	if( queue->write >= queue->read )
		return queue->write - queue->read;
	else
		/* (queue->size - 1) - ( queue->read - queue->write  - 1) */
		return queue->size - queue->read + queue->write;
}

/**
 * @brief Returns the amount of free space in the queue, in bytes
 * @param queue pointer to the queue
 * @return the amount of free space in the queue, in bytes
 */
osCounter_t
queue_getFreeSize( Queue_t* queue )
{
	if( queue->read > queue->write )
		return queue->read - queue->write - 1;
	else
		/* (queue->size - 1) - ( queue->write - queue->read ) */
		return queue->size - 1 - queue->write + queue->read;
}

/**
 * @brief Perform read and write operations until there are no
 * more operations to be performed.
 * @param p pointer to the queue who has pending read and write
 * operations
 * @details When called on a queue who has threads blocked for
 * reading or writing data, the function will try to resolve the
 * requests of pending threads by writing to and reading from
 * the queue. The function will return when there are no more
 * operations that can be performed on the queue.
 */
void
queue_solveEquation( Queue_t* p )
{
	osBool_t canRead = true, canWrite = true;

	Thread_t* thread;
	QueueReadWait_t* readWait;
	QueueWriteWait_t* writeWait;

	while( canRead || canWrite )
	{
		if( canWrite )
		{
			if( p->writingThreads.first != NULL )
			{
				thread = (Thread_t*)( p->writingThreads.first->container );
				writeWait = (QueueWriteWait_t*) thread->wait;

				if( writeWait->size <= queue_getFreeSize(p) )
				{
					queue_write( p, writeWait->data, writeWait->size );
					writeWait->result = true;
					thread_makeReady(thread);
					canRead = true;
				}
				else
					canWrite = false;
			}
			else
				canWrite = false;
		}

		if( canRead )
		{
			if( p->readingThreads.first != NULL )
			{
				thread = (Thread_t*)( p->readingThreads.first->container );
				readWait = (QueueReadWait_t*) thread->wait;

				if( readWait->size <= queue_getUsedSize(p) )
				{
					queue_read( p, readWait->data, readWait->size );
					readWait->result = true;
					thread_makeReady(thread);
					canWrite = true;
				}
				else
					canRead = false;
			}
			else
				canRead = false;
		}

	} // while( canRead || canWrite );

	if( threads_ready.first->value < currentThread->priority )
	{
		thread_setNew();
		port_yield();
	}
}

/**
 * @brief Creates a queue
 * @param size minimum number of bytes allocated for the queue's memory
 * @return handle to the queue, if the queue is created successfully;
 * 0, if the creation failed.
 * @details The actual memory allocated to the queue might be larger than
 * requested due to alignment requirements in the heap.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 *
 */
osHandle_t
osQueueCreate( osCounter_t size )
{
	Queue_t* queue;
	osByte_t* memory;

	osThreadEnterCritical();
	queue = memory_allocateFromHeap( sizeof(Queue_t), &kernelMemoryList );
	osThreadExitCritical();

	if( queue == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	OS_ASSERT( size >= 1 );

	osThreadEnterCritical();
	/* allocate size + 1 for the memory of the circular buffer */
	memory = memory_allocateFromHeap( size + 1, &kernelMemoryList );
	osThreadExitCritical();

	if( memory == NULL )
	{
		osThreadEnterCritical();
		memory_returnToHeap( queue, & kernelMemoryList );
		osThreadExitCritical();

		OS_ASSERT(0);
		return 0;
	}

	queue->memory = memory;
	queue->read = 0;
	queue->write = 0;
	queue->size = osMemoryUsableSize(memory);

	prioritizedList_init( &queue->readingThreads );
	prioritizedList_init( &queue->writingThreads );

	return (osHandle_t) queue;
}

/**
 * @brief Deletes a queue
 * @param h handle to the queue to be deleted
 * @details This function deletes a queue and releases the resources occupied
 * by the queue buffer itself and the control structures. The handle should
 * not be used again after calling this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osQueueDelete( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		thread_makeAllReady( &queue->readingThreads );
		thread_makeAllReady( &queue->writingThreads );

		memory_returnToHeap( queue->memory, & kernelMemoryList );
		memory_returnToHeap( queue, & kernelMemoryList );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Resets the queue to the initial state
 * @param h handle to the queue to be reset
 * @details This function resets the queue to the initial state right
 * after it was created. This can be used to clear the content in the
 * queue.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osQueueReset( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		queue->read = 0;
		queue->write = 0;
		queue_solveEquation(queue);
	}
	osThreadExitCritical();
}

/**
 * @brief Gets actual size allocated to the queue buffer
 * @param h handle to the queue
 * @return size of the queue buffer in bytes.
 * @details This function returns the size of the queue buffer. The
 * size might be larger than the requested size when the queue
 * was first created.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osQueueGetSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t ret;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* 1 byte in the circular buffer must always be left empty */
		ret = queue->size - 1;
	}
	osThreadExitCritical();

	return ret;
}

/**
 * @brief Gets the occupied number of bytes in the queue buffer
 * @param h handle to the queue
 * @return the occupied number of bytes in the queue buffer
 * @details This function returns the number of bytes with valid
 * data in the queue buffer.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osQueueGetUsedSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t result;

	OS_ASSERT(h);

	osThreadEnterCritical();
	result = queue_getUsedSize(queue);
	osThreadExitCritical();

	return result;
}

/**
 * @brief Gets the free space in the queue buffer
 * @param h handle to the queue
 * @return the free space in the queue buffer, in bytes
 * @details This function returns the number of bytes of free space in
 * the queue buffer.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osQueueGetFreeSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t result;

	OS_ASSERT(h);

	osThreadEnterCritical();
	result = queue_getFreeSize(queue);
	osThreadExitCritical();

	return result;
}

/**
 * @brief Tests to see if data can be sent to the queue
 * @param h handle to the queue the data to be sent to
 * @param size size of the data to be tested
 * @retval true if the data can be sent to the queue
 * @retval false, if the data cannot be sent to the queue (because it is full)
 * @details This function tests if the data of a certain size can
 * be sent to the queue. Beware that another thread can modify the queue
 * as soon as this function returns. In order to prevent this, a critical
 * section should be used if some actions are to be performed based
 * on the result of this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueuePeekSend( osHandle_t h, osCounter_t size )
{
	OS_ASSERT(h);

	return size <= osQueueGetFreeSize( h );
}

/**
 * @brief Tests to see if data can be received from the queue
 * @param h handle to the queue the data to be received from
 * @param size size of the data to be tested
 * @retval true if the data can be received from the queue
 * @retval false if the data cannot be received from the queue
 * @details This function tests if the data of a certain size can be
 * received from the queue. Beware that another thread can modify the
 * queue as soon as this function returns. In order to prevent this, a
 * critical section should be used if some actions are to be performed based
 * on the result of this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueuePeekReceive( osHandle_t h, osCounter_t size )
{
	OS_ASSERT(h);

	return size <= osQueueGetUsedSize(h);
}

/**
 * @brief Sends data onto the queue without blocking
 * @param h handle to the queue the data to be sent to
 * @param data pointer to the data to be sent onto the queue
 * @param size size in bytes of the data to be sent onto the queue
 * @retval true if the data was sent onto the queue successfully
 * @retval false if the data was not sent onto the queue
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueueSendNonBlock( osHandle_t h, const void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( size <= queue_getFreeSize(queue) )
		{
			result = true;
			queue_write( queue, data, size );
			queue_solveEquation(queue);
		}
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Receives data from the queue without blocking
 * @param h handle to the queue the data to be received from
 * @param data pointer to the buffer to hold the received data
 * @param size size in bytes of the data to be received from the queue
 * @retval true if the data was received from the queue successfully
 * @retval false if the data was not received
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueueReceiveNonBlock( osHandle_t h, void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( size <= queue_getUsedSize(queue) )
		{
			result = true;
			queue_read( queue, data, size );
			queue_solveEquation(queue);
		}
	}
	osThreadExitCritical();

	return result;
}


/**
 * @brief Sends data to the queue
 * @param h handle to the queue the data to be sent to
 * @param data pointer to the data to be sent onto the queue
 * @param size size of the data to be sent onto the queue
 * @param timeout the maximum time in ticks to wait, 0 for indefinite
 * @retval true if the data was sent onto the queue successfully
 * @retval false if the data was not sent onto the queue
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueueSend( osHandle_t h, const void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueWriteWait_t writeWait;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( size <= queue_getFreeSize(queue) )
		{
			result = true;
			queue_write( queue, data, size );
			queue_solveEquation(queue);
		}
		else
		{
			writeWait.data = data;
			writeWait.result = false;
			writeWait.size = size;
			thread_blockCurrent( &queue->writingThreads, timeout, &writeWait );
			result = writeWait.result;
		}
	}
	osThreadExitCritical();

	return result;
}


/**
 * @brief Receives data form the queue
 * @param h handle to the queue the data to be received from
 * @param data pointer to the data to be received from the queue
 * @param size size of the data to be received from the queue
 * @param timeout the maximum time in ticks to wait, 0 for indefinite
 * @retval true if the data was sent onto the queue successfully
 * @retval false if the data was not sent onto the queue
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osQueueReceive( osHandle_t h, void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueReadWait_t readWait;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( queue_getFreeSize(queue) )
		{
			result = true;
			queue_read( queue, data, size );
			queue_solveEquation(queue);
		}
		else
		{
			readWait.data = data;
			readWait.result = false;
			readWait.size = size;
			thread_blockCurrent( &queue->readingThreads, timeout, &readWait );
			result = readWait.result;
		}
	}
	osThreadExitCritical();

	return result;
}
