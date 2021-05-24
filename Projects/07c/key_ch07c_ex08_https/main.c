/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the Empty PSoC6 Application
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* (c) 2019-2020, Cypress Semiconductor Corporation. All rights reserved.
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

/* Header file includes. */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file. */
#include <FreeRTOS.h>
#include <task.h>

/* TCP client task header file. */
#include "http_client.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros. */
#define HTTP_CLIENT_TASK_STACK_SIZE        (5 * 1024)
#define HTTP_CLIENT_TASK_PRIORITY          (1)
#define GPIO_INTERRUPT_PRIORITY (7u)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* HTTP Client task handle. */
TaskHandle_t client_task_handle;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

// Button Interrupt handler
static void button_isr(void *handler_arg, cyhal_gpio_irq_event_t event)
{
	vTaskNotifyGiveFromISR(client_task_handle, &xHigherPriorityTaskWoken);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  System entrance point. This function sets up user tasks and then starts
*  the RTOS scheduler.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD. */
	uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warnings. */
	(void) result;

	/* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
	cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

	/* Initialize the user button */
	result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
	/* GPIO init failed. Stop program execution */
	if (result != CY_RSLT_SUCCESS){
		CY_ASSERT(0);
	}

	/* Configure GPIO interrupt */
	cyhal_gpio_register_callback(CYBSP_USER_BTN, button_isr, NULL);
	cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, GPIO_INTERRUPT_PRIORITY, true);

	/* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
	printf("\x1b[2J\x1b[;H");
	printf("============================================================\n");
	printf("WiFi 101 - 7C: HTTPS GET from AWS\n");
	printf("============================================================\n\n");

	/* Create the client task. */
	xTaskCreate(http_client_task, "Network task", HTTP_CLIENT_TASK_STACK_SIZE, NULL, HTTP_CLIENT_TASK_PRIORITY, &client_task_handle);

	/* Start the FreeRTOS scheduler. */
	vTaskStartScheduler();

	/* Should never get here. */
	CY_ASSERT(0);
}

/* [] END OF FILE */
