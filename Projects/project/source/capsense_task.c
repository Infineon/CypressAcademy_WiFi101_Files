/******************************************************************************
* File Name: capsense_task.c
*
* Description: This task manages reading from capsense
*
******************************************************************************/
// PSoC MCU Headers
#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"
#include "cycfg.h"
#include "cy_retarget_io.h"

// Task Headers
#include "capsense_task.h"
#include "mqtt_task.h"

// Middleware Headers
#include "semphr.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "queue.h"
#include "timers.h"

/******************************************************************************
* Global variables
******************************************************************************/
TaskHandle_t capsense_task_handle;

// Defined in main.c
extern int setTemp;
extern bool isConnected;
extern SemaphoreHandle_t setTempSemaphore;
extern SemaphoreHandle_t isConnectedSemaphore;

// Capsense deep sleep callback objects
/* SysPm callback params */
cy_stc_syspm_callback_params_t callback_params =
{
    .base       = CYBSP_CSD_HW,
    .context    = &cy_capsense_context
};
cy_stc_syspm_callback_t capsense_deep_sleep_cb =
{
    Cy_CapSense_DeepSleepCallback,
    CY_SYSPM_DEEPSLEEP,
    (CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_BEFORE_TRANSITION | CY_SYSPM_SKIP_AFTER_TRANSITION),
    &callback_params,
    NULL,
    NULL
};

/*******************************************************************************
* Function Name: capsense_isr
********************************************************************************
* Summary:
*  Call the library interrupt handler
*  If the scan is complete notify the capsense thread
*
* Parameters:
*  void
*
*******************************************************************************/
static void capsense_isr( void ){
	Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);

	if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context)){
		BaseType_t xHigherPriorityTaskWoken;
		vTaskNotifyGiveFromISR(capsense_task_handle, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/*******************************************************************************
* Function Name: task_capsense
********************************************************************************
* Summary:
*  Task that initializes the CapSense block and processes the touch input.
*
* Parameters:
*  void *param : Task parameter defined during task creation (unused)
*
*******************************************************************************/
void capsense_task(void* pvParameters){
	/* To avoid compiler warnings */
	(void)pvParameters;

	// Save the button states to detect changes
	uint32_t button0_status = 0;
	uint32_t button1_status = 0;

	// Initialize Capsense
	const cy_stc_sysint_t CapSense_interrupt_config =
	{
		.intrSrc = CYBSP_CSD_IRQ,
		.intrPriority = CAPSENSE_INTERRUPT_PRIORITY,
	};
	Cy_CapSense_Init(&cy_capsense_context);
	// Init Capsense interrupt
	Cy_SysInt_Init(&CapSense_interrupt_config, capsense_isr);
	NVIC_ClearPendingIRQ(CapSense_interrupt_config.intrSrc);
	NVIC_EnableIRQ(CapSense_interrupt_config.intrSrc);
	// Deep sleep callback
	Cy_SysPm_RegisterCallback(&capsense_deep_sleep_cb);
	// Enable Capsense
	Cy_CapSense_Enable(&cy_capsense_context);

	for(;;){
		Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

		// Wait for interrupt to signal end of scan
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

		xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
		int setTempPrevious = setTemp; // var to store previous settemp
		xSemaphoreGive(setTempSemaphore);

		// Button 0 Pressed
		if(Cy_CapSense_IsWidgetActive(CY_CAPSENSE_BUTTON0_WDGT_ID, &cy_capsense_context)){
			if(!button0_status){
				button0_status = 1;// This prevents multiple notifications
				printf("Button 0 Pressed!\n");

				// Decrement setTemp
				xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
				if(setTemp <= 50){
					setTemp = 50;
				}
				else{
					setTemp--;
				}
				xSemaphoreGive(setTempSemaphore);
			}
		}
		// Button 1 Pressed
		else if(Cy_CapSense_IsWidgetActive(CY_CAPSENSE_BUTTON1_WDGT_ID, &cy_capsense_context)){
			if(!button1_status){
				button1_status = 1;// This prevents multiple notifications
				printf("Button 1 Pressed!\n");

				// Increment setTemp
				xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
				if(setTemp >= 90){
					setTemp = 90;
				}
				else{
					setTemp++;
				}
				xSemaphoreGive(setTempSemaphore);
			}
		}
		else{
			button0_status = 0;
			button1_status = 0;
		}
		// If setTemp changed notify display and publisher threads
		xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
		if(setTempPrevious != setTemp){
			xTaskNotifyGive(display_task_handle);
			xSemaphoreTake(isConnectedSemaphore, portMAX_DELAY);
			if(isConnected){
				xTaskNotify(publisher_task_handle, SETTEMP, eSetValueWithoutOverwrite);
			}
			xSemaphoreGive(isConnectedSemaphore);
		}
		xSemaphoreGive(setTempSemaphore);
	}
}

/* END OF FILE [] */
