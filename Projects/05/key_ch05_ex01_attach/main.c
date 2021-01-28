#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"
/* Configuration file for Wi-Fi */
#include "wifi_config.h"
/* Middleware libraries */
#include "cy_retarget_io.h"
#include "cy_wcm.h"

void wifi_connect(void *arg)
{
    cy_rslt_t result;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;
    uint32_t retry_count;

    /* Configure the interface as a Wi-Fi STA (i.e. Client). */
    cy_wcm_config_t config = {.interface = CY_WCM_INTERFACE_TYPE_STA};

    /* Initialize the Wi-Fi Connection Manager and return if the operation fails. */
    result = cy_wcm_init(&config);

    printf("\nWi-Fi Connection Manager initialized.\n");

    /* Configure the connection parameters for the Wi-Fi interface. */
    memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    connect_param.ap_credentials.security = WIFI_SECURITY;

    /* Connect to the Wi-Fi AP. */
    for (retry_count = 0; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++)
    {
        printf("Connecting to Wi-Fi AP '%s'\n", connect_param.ap_credentials.SSID);
        result = cy_wcm_connect_ap(&connect_param, &ip_address);

        if (result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                    connect_param.ap_credentials.SSID);
            break;
        }
    }
    
    for(;;)
    {
    	if(result == CY_RSLT_SUCCESS)
    	{
    		/* Turn on LED and exit */
    		cyhal_gpio_write(CYBSP_USER_LED,CYBSP_LED_STATE_ON);
    		vTaskDelete(NULL);
    	}
    	else
    	{
    		cyhal_gpio_toggle(CYBSP_USER_LED);
    		vTaskDelay(100);
    	}
    }
}

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* Initaize LED pin */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    __enable_irq();

    printf("\x1b[2J\x1b[;H\n"); /* ANSI ESC sequence to clear screen. */

    /* Create the MQTT Client task. */
    xTaskCreate(wifi_connect, "wifi_connect_task", 1024, NULL, 5, NULL);

    /* Never Returns */
    vTaskStartScheduler();
}
