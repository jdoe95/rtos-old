RTOS
====

A light weight embedded realtime operating system for microcontrollers.

****

General Description
----

### Why Realtime Operating System?

Most of the time it can be a good idea to run small pieces of naked code directly on a microcontroller, but as the project gets bigger, problems are often going out of hand... There are so many ISRs and task functions, and some of them have a significance over the others and should be completed as quickly as possible even when another function is already in progress. Sometimes you find out that the program doesn't behave like intended; you go back to fix it and the source code is already chaotic...

If this is the case, then you should consider using a [realtime operating system][]. It allows functions to execute concurrently, called threads, and each thread is assigned a priority based on their significance. The operating system will automatically schedule the most important thread to execute, even when the core is already busy with another thread, so that the higher priority thread can be completed more quickly, and as soon as it does, the old thread can resume. The RTOS also provides [inter-process communication mechanisms][] to ensure synchronization between threads. Everything is handled by the RTOS and you can finally focus on the application. 

Below is a blinky program example using RTOS. Compared to naked code using interrupts, it is significantly simplier and easily extendable. The RTOS uses a single interrupt to periodically check the delay timeout of the blocked thread. Furthermore, after calling `osThreadDelay()`, the current thread blocks, but another ready thread can be scheduled.  The CPU cycles are not wasted during the delay. Or if no other threads are ready, the RTOS will execute the idle thread, which can put the CPU in a low power sleep state until any interrupt take place.


```c
#include <rtos.h>

#define BLINK_DELAY ( (osCounter_t) 100 )

void taskBlinkLed( void )
{
	initLed();
	
	for( ; ; )
	{
		ledOn();
		
		/* another thread can be scheduled during delay */
		
		/* prototype
		void osThreadDelay( osCounter_t timeout ); */
		
		osThreadDelay( BLINK_DELAY );
		
		ledOff();
		
		osThreadDelay( BLINK_DELAY );
	}
}

int main(int argc, char **argv)
{
	/* prototype
	 osHandle_t osThreadCreate( 
	 	osCounter_t priority, osCode_t code, osCounter_t stackSize, const void* argument ); */

	osThreadCreate( 100, (osCode_t)taskBlinkLed, 256, NULL );
	
	return 0;
}
```

### Intention Of This Repository

For now the source code has been thoroughly reviewed but is not thoroughly tested. This RTOS is still in the early stages of development. It is intended for learning purposes, writtern for people who would like to look at the code to see how it works. There are also many [commercial and non-commercial RTOSes][]. Most of these RTOSes are very robust products that can adapt to multiple platforms, as a result, a single function in the source code can be encapsulated in multiple layers, making it difficult to read. 

Maybe after extensive testing and debugging, this repository can be used in products. If this is what you want, it's totally free and [you are welcome to do so][]. 

The following kind of people might be interested:
- Students wishing to get a hands-on experience with the internals of RTOSes
- Electronics enthusiasts working on demostration projects


Features
----

For a complete API list, see [includes/functions_external.h][].

### Kernel
- Fully preemptive kernel
- Supports multiple same-priority threads
- Round-Robin scheduling on same-priority threads
- Maximum number of threads limited only by target platform

### Dynamic Memory Management
- Dynamic memory allocation using [next fit algorithm][], resistant to long-term fragmentation build-up
- Configurable heap alignment

### Inter Process Communication
- Queue
- Semaphore
- Mutex
- Recursive mutex
- Signal

### Others
- Handy software timer support





Getting Started
----

TODO


Documentation
----
TODO


[realtime operating system]: 
	https://en.wikipedia.org/wiki/Real-time_operating_system
	"Realtime Operating System on Wikipedia"
	
[inter-process communication mechanisms]: 
	https://en.wikipedia.org/wiki/Inter-process_communication
	"Inter Process Communication on Wikipedia"
	
[you are welcome to do so]:
	https://github.com/jdoe95/rtos/blob/master/LICENSE
	"Go to LICENSE"
	
[includes/functions_external.h]:
	https://github.com/jdoe95/rtos/blob/master/includes/functions_external.h
	"View Source Code"

[commercial and non-commercial RTOSes]:
	https://en.wikipedia.org/wiki/Comparison_of_real-time_operating_systems
	"Comparison of RTOSes on Wikipedia"
	
[next fit algorithm]:
	https://www.quora.com/What-are-the-first-fit-next-fit-and-best-fit-algorithms-for-memory-management
	"Go to First Fit, Next Fit and Best Fit Algorithm Expalanation on Quora"
