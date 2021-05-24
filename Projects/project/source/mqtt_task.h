/******************************************************************************
* File Name:   mqtt_task.h
*
* Description: This file is the public interface of mqtt_task.c
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
#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

// Middleware headers
#include "FreeRTOS.h"
#include "queue.h"
#include "cy_mqtt_api.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for MQTT Client Task. */
#define MQTT_CLIENT_TASK_PRIORITY       (2)
#define MQTT_CLIENT_TASK_STACK_SIZE     (1024 * 2)

// Modes that the thermostat can be in
#define MODE_HEAT                   "Heating"
#define MODE_COOL                  	"Cooling"
#define MODE_IDLE					"Idle"

// Identifiers for notifying the publisher thread of what value to publish
#define ACTUALTEMP 								1
#define SETTEMP									2
#define DESIRED									3
#define MODE									4

// Thermostat temp range
#define ACTUALTEMPMAX							90
#define ACTUALTEMPMIN							50

// Default current/desired value
#define SETTEMPDEFAULT 							-100

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the MQTT Client Task. */
typedef enum
{
    HANDLE_MQTT_SUBSCRIBE_FAILURE,
    HANDLE_MQTT_PUBLISH_FAILURE,
    HANDLE_DISCONNECTION
} mqtt_task_cmd_t;

/*******************************************************************************
 * Extern variables
 ******************************************************************************/
// Defined in mqtt_task.c
extern cy_mqtt_t mqtt_connection;
extern QueueHandle_t mqtt_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void mqtt_client_task(void *pvParameters);

#endif /* MQTT_TASK_H_ */

/* [] END OF FILE */
