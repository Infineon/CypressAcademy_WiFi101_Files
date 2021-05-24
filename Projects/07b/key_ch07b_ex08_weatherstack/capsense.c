/*
 * capsense.c
 *
 * Created on: May 14, 2020
 *		Author: yfs
 *
 * Implements the capsense thread
 */
 
#include "main.h"

static char* cities[] =										// Arbitrary ist of cities
{
	"Lexington",
	"London",
	"Manilla",
	"Kathmandu",
	"Nuuk"
};

static TaskHandle_t capsense_thread;						// Allow the ISR to notify the thread

/* SysPm callback params */
cy_stc_syspm_callback_params_t callback_params =
{
    .base       = CYBSP_CSD_HW,
    .context    = &cy_capsense_context
};
// Capsense deep sleep callback
cy_stc_syspm_callback_t capsense_deep_sleep_cb =
{
    Cy_CapSense_DeepSleepCallback,
    CY_SYSPM_DEEPSLEEP,
    (CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_BEFORE_TRANSITION | CY_SYSPM_SKIP_AFTER_TRANSITION),
    &callback_params,
    NULL,
    NULL
};

/*
 * capsense_isr()
 *
 * Call the library interrupt handler (required)
 * If the scan is complete, notify the capsense thread
 */
static void capsense_isr( void )
{
	Cy_CapSense_InterruptHandler( CYBSP_CSD_HW, &cy_capsense_context );
	
	if( CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy( &cy_capsense_context ) )
	{
		BaseType_t xHigherPriorityTaskWoken;
		vTaskNotifyGiveFromISR( capsense_thread, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

/*
 * capsense_task()
 *
 * Call the library interrupt handler (required)
 * If the scan is complete, notify the capsense thread
 */
void capsense_task( void* arg )
{
	uint32_t button0_status = 0;							// Save the button state to detect change				

	int selected = 0;										// Variables to control the city array
	int num_cities = sizeof( cities ) / sizeof( char* );
	
	/* Get the task handles to enable notifications */
	TaskHandle_t weather_thread  = *(TaskHandle_t*)arg;		// Notify the weather thread to get info
	capsense_thread = xTaskGetCurrentTaskHandle();			// Notify the capsense thread that scanning is complete

	/*
	Initialize CapSense
	*/
	const cy_stc_sysint_t CapSense_interrupt_config =
	{
		.intrSrc = CYBSP_CSD_IRQ,
		.intrPriority = CAPSENSE_INTERRUPT_PRIORITY,
	};
	
	Cy_CapSense_Init( &cy_capsense_context );

	Cy_SysInt_Init( &CapSense_interrupt_config, capsense_isr );
	NVIC_ClearPendingIRQ( CapSense_interrupt_config.intrSrc );
	NVIC_EnableIRQ( CapSense_interrupt_config.intrSrc );
	Cy_SysPm_RegisterCallback(&capsense_deep_sleep_cb);

	Cy_CapSense_Enable( &cy_capsense_context );
	
	/*
	Initialize the TFT display
	*/
	#ifdef TFT_SUPPORTED
	cy8ckit_028_tft_init(NULL, NULL, NULL, NULL);
	GUI_Init();
	GUI_SetBkColor( GUI_BLACK );
	GUI_SetColor( GUI_GRAY );								// Low contrast for laptop camera
	GUI_SetFont( &GUI_Font32B_ASCII );						// Big, clear font
	GUI_Clear();
	#endif
	
	/*
	Task loop scans for buttons and notifies the weather task with a new city
	*/
	for(;;)
	{
		Cy_CapSense_ScanAllWidgets( &cy_capsense_context );

		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );			// Wait for interrupt to signal end of scan

		Cy_CapSense_ProcessAllWidgets( &cy_capsense_context );

		if( Cy_CapSense_IsWidgetActive( CY_CAPSENSE_BUTTON0_WDGT_ID, &cy_capsense_context) )
		{
			/* Button 0 is pressed but only send notification once per press */
			if( !button0_status )
			{
				button0_status = 1;							// This prevents multiple notifications
				
				printf( "Selecting city %s\n", cities[selected] );
				#ifdef TFT_SUPPORTED
				GUI_SetColor( GUI_GRAY );
				GUI_Clear();
				GUI_DispStringAt( cities[selected], TFT_LEFT_ALIGNED, TFT_ROW_CITY );
				#endif

				/* Send the city name to the weather thread */
				xTaskNotify( weather_thread, (uint32_t)cities[selected], eSetValueWithOverwrite );
				
				/* Advance to the next city */
				selected++;
				if( num_cities <= selected )
					{ selected = 0; }
			}
		}
		else
		{ button0_status = 0; }
	}
}
