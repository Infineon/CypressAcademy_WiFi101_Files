/******************************************************************************
* File Name: pot_task.c
*
* Description: This example measures an external voltage using CSDADC and
*              displays the voltage along with the equivalent digital count
*              via the UART terminal.
*
******************************************************************************/
//PSoC MCU Headers
#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"
#include "cycfg.h"
#include "cy_retarget_io.h"

// Task Headers
#include "pot_task.h"
#include "mqtt_task.h"

// Middleware Headers
#include "semphr.h"

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Task handle for this task. */
TaskHandle_t pot_task_handle;

// Defined in main.c
extern int actualTemp, setTemp;
extern char * mode;
extern bool isConnected;
extern SemaphoreHandle_t actualTempSemaphore;
extern SemaphoreHandle_t setTempSemaphore;
extern SemaphoreHandle_t modeSemaphore;
extern SemaphoreHandle_t isConnectedSemaphore;

/*******************************************************************************
* Function Name: pot_task
********************************************************************************
* Summary:
*  This function performs initial setup of device,
*  configures CSD ADC block, converts the input voltage in equivalent digital
*  value, converts to a temperature, and sends that value to the publish task.
*
* Return:
*  void
*
*******************************************************************************/
void pot_task(void *pvParameters){
	/* To avoid compiler warnings */
	(void)pvParameters;
	// status variable
	cy_rslt_t rslt;

	// Delay
	const TickType_t sampleDelay = pdMS_TO_TICKS(100);

    // ADC object
    cyhal_adc_t adc_obj;
    // ADC channel object
    cyhal_adc_channel_t adc_chan_0_obj;
    /* Variable used for storing conversion result */
	uint32_t adc_out;

	/* Initialize ADC. The ADC block which can connect to pin 10[6] is selected */
	rslt = cyhal_adc_init(&adc_obj, P10_6, NULL);

	// ADC configuration structure
	const cyhal_adc_config_t ADCconfig ={
		.continuous_scanning = false,
		.resolution = 12,
		.average_count = 1,
		.average_mode_flags = 0,
		.ext_vref_mv = 0,
		.vneg = CYHAL_ADC_VNEG_VREF,
		.vref = CYHAL_ADC_REF_VDDA,
		.ext_vref = NC,
		.is_bypassed = false,
		.bypass_pin = NC
	};

	// Configure to use VDD as Vref
	rslt = cyhal_adc_configure(&adc_obj, &ADCconfig);

	/* Initialize ADC channel, allocate channel number 0 to pin 10[6] as this is the first channel initialized */
	// pin 10_6 is connected to the potentiometer
	const cyhal_adc_channel_config_t channel_config =
	{
		.enable_averaging = false,
		.min_acquisition_ns = 220,
		.enabled = true
	};
	rslt = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adc_obj, P10_6, CYHAL_ADC_VNEG, &channel_config);
	if(rslt != CY_RSLT_SUCCESS){
		cyhal_adc_channel_free(&adc_chan_0_obj);
		cyhal_adc_free(&adc_obj);
		CY_ASSERT(0);
	}

	// Var to update temp
    int newActualTemp;

    for (;;){
    	/* Read the ADC conversion result for corresponding ADC channel in millivolts. */
		adc_out = cyhal_adc_read_uv(&adc_chan_0_obj) / 1000;

		// adc_out values range from 0-3303 - actualTemp values range from 50-90
		newActualTemp = (adc_out * (ACTUALTEMPMAX - ACTUALTEMPMIN) / (3303 - 0) + ACTUALTEMPMIN);

		// If the temp changed, record it, then notify the publisher task so it can publish the new value
		xSemaphoreTake(actualTempSemaphore, portMAX_DELAY);
		if(newActualTemp != actualTemp){
			actualTemp = newActualTemp;
			// Notify the display task and publisher task that the actualTemp has changed
			xTaskNotifyGive(display_task_handle);
			xSemaphoreTake(isConnectedSemaphore, portMAX_DELAY);
			if(isConnected){
				xTaskNotify(publisher_task_handle, ACTUALTEMP, eSetValueWithoutOverwrite);
			}
			xSemaphoreGive(isConnectedSemaphore);
		}

		// Determine what mode the thermostat should be in
		xSemaphoreTake(modeSemaphore, portMAX_DELAY);
		xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
		// Var to store the previous mode
		char *modePrev = mode;

		// Cooling
		if(actualTemp > setTemp){
			mode = MODE_COOL;
			cyhal_gpio_write(CYBSP_LED_RGB_RED, CYBSP_LED_STATE_OFF);
			cyhal_gpio_write(CYBSP_LED_RGB_BLUE, CYBSP_LED_STATE_ON);
		}
		// Heating
		else if(actualTemp < setTemp){
			mode = MODE_HEAT;
			cyhal_gpio_write(CYBSP_LED_RGB_RED, CYBSP_LED_STATE_ON);
			cyhal_gpio_write(CYBSP_LED_RGB_BLUE, CYBSP_LED_STATE_OFF);
		}
		// Idle
		else{
			mode = MODE_IDLE;
			cyhal_gpio_write(CYBSP_LED_RGB_RED, CYBSP_LED_STATE_OFF);
			cyhal_gpio_write(CYBSP_LED_RGB_BLUE, CYBSP_LED_STATE_OFF);
		}
		xSemaphoreGive(actualTempSemaphore);
		xSemaphoreGive(setTempSemaphore);
		// If the mode changed notify the display task to update the display and notify the publisher task to update the cloud
		if(modePrev != mode){
			xTaskNotifyGive(display_task_handle);
			xSemaphoreTake(isConnectedSemaphore, portMAX_DELAY);
			if(isConnected){
				xTaskNotify(publisher_task_handle, MODE, eSetValueWithoutOverwrite);
			}
			xSemaphoreGive(isConnectedSemaphore);
		}
		xSemaphoreGive(modeSemaphore);


		/* Give delay between samples */
		vTaskDelay(sampleDelay);
    }
}

/* [] END OF FILE */
