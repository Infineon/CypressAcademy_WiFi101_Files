/*
 * main.c
 *
 *  Created on: May 14, 2020
 *	   Author: yfs
 *
 * Create tasks and start the OS
 */
 
#include "main.h"

volatile int uxTopUsedPriority;								// OpenOCD debug

/*
 * main()
 *
 * Initialize MCU peripherals (including printf)
 * Create capsense and weather tasks
 * Start the OS
 */
int main( void )
{
	static TaskHandle_t weather_thread;						// Use static for persistent storage
	
	/* This enables RTOS aware debugging in Eclipse with OpenOCD */
	uxTopUsedPriority = configMAX_PRIORITIES - 1;

	/* Initialize the device and board peripherals */
	cybsp_init();
	cy_retarget_io_init( CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE );

	printf( "\x1b[2J\x1b[;H" );								// ANSI ESC sequence for clear screen */
	printf( "Creating weatherstack application threads\n" );
	
	/*
	Create application tasks
	*/
	xTaskCreate(
		weather_task,
		"Weather",
		WEATHER_THREAD_STACK_SIZE,
		NULL,												// No parameter passed to weather_task()
		WEATHER_THREAD_PRIORITY,
		&weather_thread	);									// Returns the thread ID

    xTaskCreate(
		capsense_task,
		"CapSense",
		CAPSENSE_THREAD_STACK_SIZE,
		&weather_thread,									// Weather thread ID passed to capsense_task()
		CAPSENSE_THREAD_PRIORITY,
		NULL );												// No need to keep this thread ID

	/*
	Start the FreeRTOS scheduler
	*/
	vTaskStartScheduler();
}