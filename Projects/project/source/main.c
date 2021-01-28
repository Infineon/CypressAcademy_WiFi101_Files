/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for MQTT Client Example in ModusToolbox.
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
#include "cy_retarget_io.h"
// MiddleWare Headers
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
// Task Headers
#include "mqtt_task.h"

/******************************************************************************
* Global Variables
******************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

// Vars to store the state of the thermostat
int setTemp = 70;
int actualTemp = 0;
char *mode = MODE_IDLE;
bool isConnected = false;
// Semaphores for thermostat state vars
SemaphoreHandle_t actualTempSemaphore;
SemaphoreHandle_t setTempSemaphore;
SemaphoreHandle_t modeSemaphore;
SemaphoreHandle_t isConnectedSemaphore;

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, sets up 
 *  the MQTT client task, and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main(){
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    // Initialize Semaphores
    actualTempSemaphore = xSemaphoreCreateMutex();
    setTempSemaphore = xSemaphoreCreateMutex();
    modeSemaphore = xSemaphoreCreateMutex();
    isConnectedSemaphore = xSemaphoreCreateMutex();

    // Initialize GPIOs for red and blue LEDs
	cyhal_gpio_init(CYBSP_LED_RGB_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
	cyhal_gpio_init(CYBSP_LED_RGB_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("WiFi101 - Final Project: IoT Thermostat\n");
    printf("===============================================================\n\n");

    /* Create the MQTT Client task. */
    xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE, NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    printf("Starting Task Scheduler\n");
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
