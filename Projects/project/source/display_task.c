/******************************************************************************
* File Name: display_task.c
*
* Description: This task manages the display
*
******************************************************************************/
// PSoC MCU Headers
#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"
#include "cycfg.h"
#include "cy_retarget_io.h"

// Task Headers
#include "display_task.h"
#include "mqtt_task.h"

// Middleware Headers
#include "semphr.h"
#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h"

// Bitmaps
#include <fire80.h>
#include <snowflake80.h>
#include <black80.h>
#include <wifi80.h>
#include <wifiDisconnect80.h>

/*******************************************************************************
* Macros
********************************************************************************/
// Macros for GUI
#define TFT_LEFT_ALIGNED				(0)
#define TFT_ROW_ONE						(0)
#define TFT_ROW_TWO						(40)
#define TFT_ROW_THREE					(80)
#define TFT_ROW_FOUR					(120)
#define TFT_ROW_FIVE					(160)
#define X_BMP_POS						(90)
#define X_WIFI_POS						(240)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void displayUpdate();

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Task handle for this task. */
TaskHandle_t display_task_handle;

// Defined in main.c
extern int actualTemp, setTemp;
extern char * mode;
extern bool isConnected;
extern SemaphoreHandle_t actualTempSemaphore;
extern SemaphoreHandle_t setTempSemaphore;
extern SemaphoreHandle_t modeSemaphore;
extern SemaphoreHandle_t isConnectedSemaphore;

/*******************************************************************************
* Function Name: display_task
********************************************************************************
* Summary:
*  Initializes the display, then waits for notifications to update the display
*
* Return:
*  void
*
*******************************************************************************/
void display_task(void *pvParameters){
	/* To avoid compiler warnings */
	(void)pvParameters;

	/* The pins above are defined by the CY8CKIT-028-TFT library. If the display is being used on different hardware the mappings will be different. */
	const mtb_st7789v_pins_t tft_pins =
	{
		.db08 = CY8CKIT_028_TFT_PIN_DISPLAY_DB8,
		.db09 = CY8CKIT_028_TFT_PIN_DISPLAY_DB9,
		.db10 = CY8CKIT_028_TFT_PIN_DISPLAY_DB10,
		.db11 = CY8CKIT_028_TFT_PIN_DISPLAY_DB11,
		.db12 = CY8CKIT_028_TFT_PIN_DISPLAY_DB12,
		.db13 = CY8CKIT_028_TFT_PIN_DISPLAY_DB13,
		.db14 = CY8CKIT_028_TFT_PIN_DISPLAY_DB14,
		.db15 = CY8CKIT_028_TFT_PIN_DISPLAY_DB15,
		.nrd  = CY8CKIT_028_TFT_PIN_DISPLAY_NRD,
		.nwr  = CY8CKIT_028_TFT_PIN_DISPLAY_NWR,
		.dc   = CY8CKIT_028_TFT_PIN_DISPLAY_DC,
		.rst  = CY8CKIT_028_TFT_PIN_DISPLAY_RST
	};

	/* Initialize the display */
	mtb_st7789v_init8(&tft_pins);
	GUI_Init();
	GUI_SetBkColor(GUI_BLACK); // Background Color
	GUI_SetColor(GUI_WHITE); // Text Color
	GUI_SetFont(&GUI_Font32B_ASCII); // Font Size

	printf("Display initialized\n");

    for (;;){
    	// Update the display whenever notified
    	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    	displayUpdate();
    }
}

/*******************************************************************************
* Function Name: displayUpdate
********************************************************************************
* Summary:
*  Updates the display with the current values
*
* Return:
*  void
*
*******************************************************************************/
void displayUpdate(){
	printf("Updating display\n");

	// Actual Temp
	GUI_DispStringAt("Actual Temperature: ", TFT_LEFT_ALIGNED, TFT_ROW_ONE);

	xSemaphoreTake(actualTempSemaphore, portMAX_DELAY);
	GUI_DispDecMin(actualTemp);
	xSemaphoreGive(actualTempSemaphore);

	// Set Temp
	GUI_DispStringAt("Set Temperature: ", TFT_LEFT_ALIGNED, TFT_ROW_TWO);

	xSemaphoreTake(setTempSemaphore, portMAX_DELAY);
	GUI_DispDecMin(setTemp);
	xSemaphoreGive(setTempSemaphore);

	// Mode
	GUI_DispStringAt("Mode: ", TFT_LEFT_ALIGNED, TFT_ROW_THREE);

	xSemaphoreTake(modeSemaphore, portMAX_DELAY);

	if(strcmp(mode, MODE_HEAT) == 0){
		GUI_DrawBitmap(&bmfire80, X_BMP_POS, TFT_ROW_THREE);
	}
	else if(strcmp(mode, MODE_COOL) == 0){
		GUI_DrawBitmap(&bmsnowflake80, X_BMP_POS, TFT_ROW_THREE);
	}
	else{
		GUI_DrawBitmap(&bmblack80, X_BMP_POS, TFT_ROW_THREE);
		GUI_DispString(mode);
	}

	xSemaphoreGive(modeSemaphore);

	// WiFi symbol
	xSemaphoreTake(isConnectedSemaphore, portMAX_DELAY);
	if(isConnected){
		GUI_DrawBitmap(&bmwifi80, X_WIFI_POS, TFT_ROW_FIVE);
	}
	else{
		GUI_DrawBitmap(&bmwifiDisconnect80, X_WIFI_POS, TFT_ROW_FIVE);
	}
	xSemaphoreGive(isConnectedSemaphore);
}

/* [] END OF FILE */
