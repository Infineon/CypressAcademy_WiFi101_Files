/*
 * main.h
 *
 *  Created on: May 14, 2020
 *      Author: yfs
 */

#ifndef MAIN_H_
#define MAIN_H_

/* PSoC MCU headers */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cycfg.h"
#include "cycfg_capsense.h"

/* Middleware headers */
#include "FreeRTOS.h"
#include "task.h"
#include "cJSON.h"
#ifdef TFT_SUPPORTED
#include "GUI.h"
#include "cy8ckit_028_tft.h"
#endif

/* Wi-Fi headers */
#include "cy_lwip.h"
#include "cy_secure_sockets.h"
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* Standard C headers */
#include <string.h>
#include <stdio.h>

/* HTTP Client */
#include "cy_http_client_api.h"

/* Private data for my router and weatherstack account */
#include "private_data.h"
/*
	#define AP_NAME		"My-AP-Name"
	#define PASSWORD	"My-AP-Password"
	#define ACCESS_KEY	"My-Weatherstack-API-Key"
*/

/*
CapSense thread defines and declaration
*/
void capsense_task( void* arg );
#define CAPSENSE_THREAD_PRIORITY		(5)
#define CAPSENSE_THREAD_STACK_SIZE		(1024)
#define CAPSENSE_INTERRUPT_PRIORITY		(7u)

/*
Weather thread defines
*/
void weather_task( void* arg );
#define WEATHER_THREAD_PRIORITY			(5)
#define WEATHER_THREAD_STACK_SIZE		(1024)
#define HTTP_PORT_NUMBER				(80)

/*
TFT screen position defines
*/
#ifdef TFT_SUPPORTED
#define TFT_LEFT_ALIGNED				(0)
#define TFT_ROW_CITY					(0)
#define TFT_ROW_WEATHER					(40)
#define TFT_ROW_WIND					(80)
#define TFT_ROW_RAIN					(120)
#define TFT_ROW_TEMP					(160)
#endif


/* Handy macro function for printing IP addresses */
#define PRINT_IPV4_ADDRESS(msg,addr)	printf( "%s\t: %d.%d.%d.%d\n", msg,	\
											(int)( addr >> 24 ) & 0xFF,		\
											(int)( addr >> 16 ) & 0xFF,		\
											(int)( addr >>  8 ) & 0xFF,		\
											(int)( addr >>  0 ) & 0xFF )

#endif /* MAIN_H_ */
