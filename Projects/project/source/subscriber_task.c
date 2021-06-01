/******************************************************************************
* File Name:   subscriber_task.c
*
* Description: This file contains the task that initializes the user LED GPIO,
*              subscribes to the topic 'MQTT_TOPIC', and actuates the user LED
*              based on the notifications received from the MQTT subscriber
*              callback.
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
#include "string.h"

/* Middleware libraries */
#include "FreeRTOS.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "cy_mqtt_api.h"
#include "cJSON.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/******************************************************************************
* Macros
******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT                      (1)

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_topic(void);

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

// Defined in main.c
extern int setTemp;
extern SemaphoreHandle_t setTempSemaphore;

/* Configure the subscription information structure. */
cy_mqtt_subscribe_info_t subscribe_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = UPDATE_DOCUMENTS_TOPIC,
    .topic_len = (sizeof(UPDATE_DOCUMENTS_TOPIC) - 1)
};

/******************************************************************************
 * Function Name: subscriber_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user LED GPIO, subscribes to topic - 'MQTT_TOPIC',
 *  and controls the user LED based on the received task notification.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void subscriber_task(void *pvParameters){

    /* Variable to recieve setTemp value in. */
    uint32_t received_setTemp_Value;

    /* To avoid compiler warnings */
    (void)pvParameters;

    /* Subscribe to the specified MQTT topic. */
	subscribe_to_topic();

    while (true){
        /* Block until a notification is received from the subscriber callback. */
        xTaskNotifyWait(0, 0, &received_setTemp_Value, portMAX_DELAY);
        /* Update the current device setTemp Value with the one just recieved. */
        xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
        setTemp = received_setTemp_Value;
        xSemaphoreGive(setTempSemaphore);
        // Notify the display thread that setTemp has changed
        xTaskNotifyGive(display_task_handle);
        // Notify the publisher task to overwrite the desired shadow member
		xTaskNotify(publisher_task_handle, DESIRED, eSetValueWithoutOverwrite);
    }
}

/******************************************************************************
 * Function Name: subscribe_to_topic
 ******************************************************************************
 * Summary:
 *  Function that subscribes to the MQTT topic specified by the macro
 *  'MQTT_SUB_TOPIC'. This operation is retried a maximum of
 *  'MAX_SUBSCRIBE_RETRIES' times with interval of
 *  'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void subscribe_to_topic(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++){
        result = cy_mqtt_subscribe(mqtt_connection, &subscribe_info, SUBSCRIPTION_COUNT);
        if (result == CY_RSLT_SUCCESS){
            printf("MQTT client subscribed to the topic '%.*s' successfully.\n\n",
                    subscribe_info.topic_len, subscribe_info.topic);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }

    if (result != CY_RSLT_SUCCESS){
        printf("MQTT Subscribe failed with error 0x%0X after %d retries...\n\n",
               (int)result, MAX_SUBSCRIBE_RETRIES);

        /* Notify the MQTT client task about the subscription failure */
        mqtt_task_cmd = HANDLE_MQTT_SUBSCRIBE_FAILURE;
        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
    }
}

/******************************************************************************
 * Function Name: mqtt_subscription_callback
 ******************************************************************************
 * Summary:
 *  Callback to handle incoming MQTT messages. This callback prints the 
 *  contents of an incoming message and notifies the subscriber task with the  
 *  LED state as per the received message.
 *
 * Parameters:
 *  void *pCallbackContext : Parameter defined during MQTT Subscribe operation
 *                           using the IotMqttCallbackInfo_t.pCallbackContext
 *                           member (unused)
 *  IotMqttCallbackParam_t * pPublishInfo : Information about the incoming 
 *                                          MQTT PUBLISH message passed by
 *                                          the MQTT library.
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info){

    /* Received MQTT message */
	const char *received_msg = received_msg_info->payload;

    /* Print information about the incoming PUBLISH message. */
    printf("  Subscriber: Incoming MQTT message received:\n"
		   "    Publish topic name: %.*s\n"
		   "    Publish QoS: %d\n"
		   "    Publish payload: %.*s\n\n",
		   received_msg_info->topic_len, received_msg_info->topic,
		   (int) received_msg_info->qos,
		   (int) received_msg_info->payload_len, (const char *)received_msg_info->payload);

    cJSON *root = cJSON_Parse(received_msg); // Read the JSON
    cJSON *current = cJSON_GetObjectItem(root, "current"); // Search for the key "current"
    cJSON *state = cJSON_GetObjectItem(current, "state"); // Search for the key "state"
    cJSON *reported = cJSON_GetObjectItem(state, "desired");// Search for the key "desired"
    cJSON *setTempRecieved = cJSON_GetObjectItem(reported, "setTemp");// Search for the key "setTemp"

    // If the read value does not equal the current global setTemp value, and is within range notify the subscriber task
    xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
    if(setTempRecieved != NULL  && setTemp != setTempRecieved->valueint && setTempRecieved->valueint >= ACTUALTEMPMIN && setTempRecieved->valueint <= ACTUALTEMPMAX){
    	uint32_t setTempValue = setTempRecieved->valueint;
		xTaskNotify(subscriber_task_handle, setTempValue, eSetValueWithoutOverwrite);
    }
    xSemaphoreGive(setTempSemaphore);
}
/* [] END OF FILE */
