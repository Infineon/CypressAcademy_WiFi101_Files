/******************************************************************************
* File Name:   publisher_task.c
*
* Description: This file contains the task that initializes the user button
*              GPIO, configures the ISR, and publishes MQTT messages on the 
*              topic 'MQTT_TOPIC' to control a device that is actuated by the
*              subscriber task. The file also contains the ISR that notifies
*              the publisher task about the new device state to be published.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/
// PSoC MCU Headers
#include "cyhal.h"
#include "cybsp.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"
#include "display_task.h"

// Middleware Headers
#include "semphr.h"
#include "cy_retarget_io.h"
#include "cy_mqtt_api.h"
#include "semphr.h"
#include "cy_wcm.h"
#include "cy_lwip.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

// Standard C header
#include <stdio.h>

/******************************************************************************
* Macros
******************************************************************************/
/* The maximum number of times each PUBLISH in this example will be retried. */
#define PUBLISH_RETRY_LIMIT             (10)

/* A PUBLISH message is retried if no response is received within this 
 * time (in milliseconds).
 */
#define PUBLISH_RETRY_MS                (1000)
#define MAX_MQTT_CHARS					(100)

/******************************************************************************
* Function Prototypes
*******************************************************************************/
void publish(char valueToPublish[]);

/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Structure to store publish message information. */
cy_mqtt_publish_info_t publish_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = UPDATE_TOPIC,
    .topic_len = (sizeof(UPDATE_TOPIC) - 1),
    .retain = false,
    .dup = false
};

// Defined in main.c
extern int actualTemp, setTemp;
extern char *mode;
extern SemaphoreHandle_t actualTempSemaphore;
extern SemaphoreHandle_t setTempSemaphore;
extern SemaphoreHandle_t modeSemaphore;

/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 *  Task that handles initialization of the user button GPIO, configuration of
 *  ISR, and publishing of MQTT messages to control the device that is actuated
 *  by the subscriber task.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters){

    /* Var to store strings to publish in. */
	char payloadString[MAX_MQTT_CHARS];

    /* Variable to receive new device state from capsense thread, pot thread, or display thread */
    uint32_t valueToUpdate; // either ACTUALTEMP, SETTEMP, or MODE

    /* To avoid compiler warnings */
    (void)pvParameters;

    // Send my IP address to the cloud
    cy_wcm_ip_address_t myIP = {0};
    cy_wcm_get_ip_addr(CY_WCM_INTERFACE_TYPE_STA, &myIP, 0);
    sprintf(payloadString, "{\"state\":{\"reported\":{\"IP Address\":\"%s\"}}}", ip4addr_ntoa((const ip4_addr_t *) &myIP.ip.v4));
    publish(payloadString);

    while(true){
        /* Wait for notification from capsense, pot, or subscriber tasks. */
        xTaskNotifyWait(0, 0, &valueToUpdate, portMAX_DELAY);
        
       if(valueToUpdate == ACTUALTEMP){
    	   xSemaphoreTake(actualTempSemaphore, portMAX_DELAY);
    	   sprintf(payloadString, "{\"state\":{\"reported\":{\"actualTemp\":%d}}}", actualTemp);
    	   xSemaphoreGive(actualTempSemaphore);
       }
       else if(valueToUpdate == SETTEMP){
    	   xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
    	   sprintf(payloadString, "{\"state\":{\"reported\":{\"setTemp\":%d}}}", setTemp);
    	   xSemaphoreGive(setTempSemaphore);
       }
       else if(valueToUpdate == MODE){
    	   xSemaphoreTake(modeSemaphore, portMAX_DELAY);
    	   sprintf(payloadString, "{\"state\":{\"reported\":{\"mode\":\"%s\"}}}", mode);
    	   xSemaphoreGive(modeSemaphore);
       }
       else if(valueToUpdate == DESIRED){
    	   // Overwrite desired shadow member then update current reported setTemp
		   sprintf(payloadString, "{\"state\":{\"desired\":{\"setTemp\":%d}}}", SETTEMPDEFAULT);
		   publish(payloadString);
		   xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
		   sprintf(payloadString, "{\"state\":{\"reported\":{\"setTemp\":%d}}}", setTemp);
		   xSemaphoreGive(setTempSemaphore);
		  }
       publish(payloadString);
    }
}

/******************************************************************************
 * Function Name: publish
 ******************************************************************************
 * Summary:
 *  Publishes the char[] passed in
 *
 * Parameters:
 *  char valueToPublish[] - the value that will be published
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publish(char valueToPublish[]){

	/* Status variable */
	cy_rslt_t result;

	/* Command to the MQTT client task */
	mqtt_task_cmd_t mqtt_task_cmd;

	publish_info.payload = &valueToPublish[0];
	publish_info.payload_len = strlen(publish_info.payload);

	printf("Publishing '%s' of length '%d' on the topic '%s'\n\n", (char *)publish_info.payload, publish_info.payload_len, publish_info.topic);

	/* Publish the MQTT message with the configured settings. */
	result = cy_mqtt_publish(mqtt_connection, &publish_info);
	if (result != CY_RSLT_SUCCESS){
		printf("  Publisher: MQTT Publish failed with error 0x%0X.\n\n", (int)result);

		/* Communicate the publish failure with the the MQTT
		 * client task.
		 */
		mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
		xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
		vTaskSuspend( NULL );
	}
}

/* [] END OF FILE */
