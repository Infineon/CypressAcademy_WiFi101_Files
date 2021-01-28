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

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define GPIO_INTERRUPT_PRIORITY (7u)
#define TOGGLE_THREAD_PRIORITY (5)
#define QUEUE_LENGTH (10)
#define QUEUE_ITEM_SIZE (sizeof(int))


QueueHandle_t myQueue;

void toggleThread(void *arg){
	while(1){
		//Pop the number of the queue and blink that many times
		static int numberBlinks;
		xQueueReceive(myQueue, &numberBlinks, portMAX_DELAY);
		for(int i = 0; i < numberBlinks; i++){
			cyhal_gpio_toggle(CYBSP_USER_LED);
			vTaskDelay(100);
			cyhal_gpio_toggle(CYBSP_USER_LED);
			vTaskDelay(100);
		}
		// Add extra delay between blink sequences to tell them apart
		vTaskDelay(500);
	}
}

void button_isr(void *handler_arg, cyhal_gpio_irq_event_t event){
	//static int that will increment with every press
	static int pushCount = 0;
	pushCount++;
	//push pushCount onto the queue
	BaseType_t xHigherPriorityTaskWoken;
	xQueueSendFromISR(myQueue, &pushCount, &xHigherPriorityTaskWoken);
}


int main(void)
{
    cy_rslt_t result;

    //Create the queue
    myQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    /*BSP init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize the User LED */
	result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
	/* GPIO init failed. Stop program execution */
	if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}

    /* Initialize the user button */
	result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
	/* GPIO init failed. Stop program execution */
	if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}

	/* Configure GPIO interrupt */
	cyhal_gpio_register_callback(CYBSP_USER_BTN, button_isr, NULL);
	cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, GPIO_INTERRUPT_PRIORITY, true);

	// Enable Interrupts
    __enable_irq();

    //Create the toggleThread task
    BaseType_t retval;
    retval = xTaskCreate(toggleThread, "toggleThread", 1024, NULL, TOGGLE_THREAD_PRIORITY, NULL);
    //Start the task scheduler if the toggleThread task was created successfully
    if(pdPASS == retval){
    	vTaskStartScheduler();
    }
}

/* [] END OF FILE */
