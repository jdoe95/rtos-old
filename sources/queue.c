/****************************************************************
 * QUEUE FUNCITONS FILE
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 ****************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/* write data to queue */
void
queue_write( Queue_t* queue, const void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	/* this function has to be called in a critical section because it accesses shared
	 * queue resources. */
	OS_ASSERT( criticalNesting );

	for( ; counter < size; counter++ )
	{
		queue->memory[queue->write] = ( (const osByte_t*) data )[counter];

		if( queue->write < queue->size )
			queue->write++;
		else
			queue->write = 0;
	}
}

/* write data to queue ahead( so it can be read first) */
void
queue_writeAhead( Queue_t* queue, const void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	/* this function has to be called in a critical section because it accesses shared
	 * queue resources. */
	OS_ASSERT( criticalNesting );

	for( ; counter < size; counter++ )
	{
		if( queue->read > 0 )
			queue->read--;
		else
			queue->read = queue->size - 1;

		queue->memory[queue->read] = ( (const osByte_t*) data )[counter];
	}
}

/* read data from queue */
void
queue_read( Queue_t* queue, void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	/* this function has to be called in a critical section because it accesses shared
	 * queue resources. */
	OS_ASSERT( criticalNesting );

	for( ; counter < size; counter++ )
	{
		( (osByte_t*) data )[counter] = queue->memory[queue->read];

		if( queue->read < queue->size )
			queue->read++;
		else
			queue->read = 0;
	}
}

/* read data from queue behind */
void
queue_readBehind( Queue_t* queue, void* data, osCounter_t size )
{
	osCounter_t counter = 0;

	/* this function has to be called in a critical section because it accesses shared
	 * queue resources. */
	OS_ASSERT( criticalNesting );

	for( ; counter < size; counter++ )
	{
		if( queue->write > 0 )
			queue->write--;
		else
			queue->write = queue->size - 1;

		( (osByte_t*) data )[counter] = queue->memory[queue->write];
	}
}

/* a pretty complicated algorithm
 *
 * For regular and ahead lists of READ or WRITE operations, when:
 * 1) regular=EMPTY, ahead=EMPTY -> can=false
 * 2) regular=EMPTY, ahead=NEMPTY-> ahead
 * 3) regular=NEMPTY, ahead=EMPTY-> regular
 * 4) regular=NEMPTY, ahead=NEMPTY
 * 		if ahead->prio lower than regular->prio, ->regular
 * 		else -> ahead
 * */
void
queue_solveEquation( osHandle_t queue )
{
	Queue_t* p = (Queue_t*) queue;
	osBool_t canRead = true, canWrite = true;

	Thread_t* thread;
	QueueReadWait_t* readWait;
	QueueWriteWait_t* writeWait;

	enum { REGULAR = 0, SPECIAL } mode;

	/* this function has to be called in a critical section because it accesses shared
	 * queue resources. */
	OS_ASSERT( criticalNesting );

	while( canRead || canWrite )
	{
		/* process writing threads */
		if( p->writingThreads.first != NULL )
		{
			if( p->writingAheadThreads.first == NULL )
				mode = REGULAR;
			else if( p->writingThreads.first->value < p->writingAheadThreads.first->value )
				mode = REGULAR;
			else
				mode = SPECIAL;
		}
		else
		{
			if( p->writingAheadThreads.first != NULL )
				mode = SPECIAL;
			else
				canWrite = false;
		}

		/* perform write */
		if( canWrite )
		{
			if( mode == SPECIAL )
			{
				thread = (Thread_t*)( p->writingAheadThreads.first->container );
				writeWait = (QueueWriteWait_t*) thread->wait;

				if( writeWait->size <= osQueueGetFreeSize(queue) )
				{
					queue_writeAhead( p, writeWait->data, writeWait->size );
					writeWait->result = true;
					thread_makeReady( thread );

					canRead = true;
				}
				else
					canWrite = false;
			}
			else
			{
				thread = (Thread_t*)( p->writingThreads.first->container );
				writeWait = (QueueWriteWait_t*) thread->wait;

				if( writeWait->size <= osQueueGetFreeSize(queue) )
				{
					queue_write( p, writeWait->data, writeWait->size );
					writeWait->result = true;
					thread_makeReady(thread);

					canRead = true;
				}
				else
					canWrite = false;
			}
		}

		/* process reading threads */
		if( p->readingThreads.first != NULL )
		{
			if( p->readingBehindThreads.first == NULL )
				mode = REGULAR;
			else if( p->readingThreads.first->value < p->readingBehindThreads.first->value )
				mode = REGULAR;
			else
				mode = SPECIAL;
		}
		else
		{
			if( p->readingBehindThreads.first != NULL )
				mode = SPECIAL;
			else
				canRead = false;
		}

		/* perform read */
		if( canRead )
		{
			if( mode == SPECIAL )
			{
				thread = (Thread_t*)( p->readingBehindThreads.first->container );
				readWait = (QueueReadWait_t*) thread->wait;

				if( readWait->size <= osQueueGetUsedSize(queue) )
				{
					queue_readBehind( p, readWait->data, readWait->size );
					readWait->result = true;
					thread_makeReady(thread);

					canWrite = true;
				}
				else
					canRead = false;

			}
			else
			{
				thread = (Thread_t*)( p->readingThreads.first->container );
				readWait = (QueueReadWait_t*) thread->wait;

				if( readWait->size <= osQueueGetUsedSize(queue) )
				{
					queue_read( p, readWait->data, readWait->size );
					readWait->result = true;
					thread_makeReady( thread );

					canWrite = true;
				}
				else
					canRead = false;

			}
		}

	} // while( canRead || canWrite );

	if( threads_ready.first->value < currentThread->priority )
	{
		thread_setNew();
		port_yield();
	}
}

osHandle_t
osQueueCreate( osCounter_t size )
{
	Queue_t* queue = memory_allocateFromHeap( sizeof(Queue_t), &kernelMemoryList );
	if( queue == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	osByte_t* memory = memory_allocateFromHeap( size, &kernelMemoryList );
	if( memory == NULL )
	{
		memory_returnToHeap( queue, & kernelMemoryList );
		OS_ASSERT(0);
		return 0;
	}

	queue->memory = memory;
	queue->read = 0;
	queue->write = 0;
	queue->size = osMemoryUsableSize(memory);

	prioritizedList_init( &queue->readingBehindThreads );
	prioritizedList_init( &queue->readingThreads );
	prioritizedList_init( &queue->writingThreads );
	prioritizedList_init( &queue->writingAheadThreads );

	return (osHandle_t) queue;
}

void
osQueueDelete( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osThreadEnterCritical();
	{
		thread_makeAllReady( &queue->readingBehindThreads );
		thread_makeAllReady( &queue->readingThreads );
		thread_makeAllReady( &queue->writingAheadThreads );
		thread_makeAllReady( &queue->writingThreads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();

	memory_returnToHeap( queue->memory, & kernelMemoryList );
	memory_returnToHeap( queue, & kernelMemoryList );
}

void
osQueueReset( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;

	osThreadEnterCritical();
	{
		queue->read = 0;
		queue->write = 0;
		queue_solveEquation( h );
	}
	osThreadExitCritical();
}

osCounter_t
osQueueGetSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t ret;

	osThreadEnterCritical();
	{
		ret = queue->size;
	}
	osThreadExitCritical();

	return ret;
}

osCounter_t
osQueueGetUsedSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t result;

	osThreadEnterCritical();
	{
		if( queue->write >= queue->read )
			result = queue->write - queue->read;
		else
			result = queue->size - ( queue->read - queue->write );
	}
	osThreadExitCritical();

	return result;
}

osCounter_t
osQueueGetFreeSize( osHandle_t h )
{
	Queue_t* queue = (Queue_t*) h;
	osCounter_t result;

	osThreadEnterCritical();
	{
		if( queue->write >= queue->read )
			result = queue->size - ( queue->write - queue->read );
		else
			result = queue->read - queue->write;
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueIsFull( osHandle_t h )
{
	return osQueueGetFreeSize( h ) == 0;
}

osBool_t
osQueueIsEmpty( osHandle_t h )
{
	return osQueueGetUsedSize( h ) == 0;
}

osBool_t
osQueuePeekSend( osHandle_t h, osCounter_t size )
{
	return size <= osQueueGetFreeSize( h );
}

osBool_t
osQueuePeekReceive( osHandle_t h, osCounter_t size )
{
	return size <= osQueueGetUsedSize( h );
}

osBool_t
osQueueSendNonBlock( osHandle_t h, const void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( osQueuePeekSend( h, size ) )
		{
			result = true;
			queue_write( queue, data, size );
			queue_solveEquation( h );
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueSendAheadNonBlock( osHandle_t h, const void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( osQueuePeekSend( h, size ) )
		{
			result = true;
			queue_writeAhead( queue, data, size );
			queue_solveEquation( h );
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueReceiveNonBlock( osHandle_t h, void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( osQueuePeekReceive( h, size ) )
		{
			result = true;
			queue_read( queue, data, size );
			queue_solveEquation( h );
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueReceiveBehindNonBlock( osHandle_t h, void* data, osCounter_t size )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( osQueuePeekReceive( h, size ) )
		{
			result = true;
			queue_readBehind( queue, data, size );
			queue_solveEquation( h );
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueSend( osHandle_t h, const void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueWriteWait_t writeWait;

	osThreadEnterCritical();
	{
		if( osQueuePeekSend( h, size ) )
		{
			result = true;
			queue_write( queue, data, size );
			queue_solveEquation( h );
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

osBool_t
osQueueSendAhead( osHandle_t h, const void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueWriteWait_t writeWait;

	osThreadEnterCritical();
	{
		if( osQueuePeekSend( h, size ) )
		{
			result = true;
			queue_writeAhead( queue, data, size );
			queue_solveEquation( h );
		}
		else
		{
			writeWait.data = data;
			writeWait.result = false;
			writeWait.size = size;
			thread_blockCurrent( &queue->writingAheadThreads, timeout, &writeWait );
			result = writeWait.result;
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osQueueReceive( osHandle_t h, void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueReadWait_t readWait;

	osThreadEnterCritical();
	{
		if( osQueuePeekReceive( h, size ) )
		{
			result = true;
			queue_read( queue, data, size );
			queue_solveEquation( h );
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

osBool_t
osQueueReceiveBehind( osHandle_t h, void* data, osCounter_t size, osCounter_t timeout )
{
	Queue_t* queue = (Queue_t*) h;
	osBool_t result = false;
	QueueReadWait_t readWait;

	osThreadEnterCritical();
	{
		if( osQueuePeekReceive( h, size ) )
		{
			result = true;
			queue_readBehind( queue, data, size );
			queue_solveEquation( h );
		}
		else
		{
			readWait.data = data;
			readWait.result = false;
			readWait.size = size;
			thread_blockCurrent( &queue->readingBehindThreads, timeout, &readWait );
			result = readWait.result;
		}
	}
	osThreadExitCritical();

	return result;
}
