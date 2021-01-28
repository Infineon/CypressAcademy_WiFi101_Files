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
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Cypress secure socket header file. */
#include "cy_secure_sockets.h"
#include "cy_tls.h"

/* Wi-Fi connection manager header files. */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* TCP client task header file. */
#include "tcp_client.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros. */
#define TCP_CLIENT_TASK_STACK_SIZE        (5 * 1024)
#define CONNECT_WIFI_TASK_STACK_SIZE      (1024)
#define TCP_CLIENT_TASK_PRIORITY          (1)
#define CONNECT_WIFI_TASK_PRIORITY        (1)

/* Interrupt priority of the user button. */
#define USER_BTN_INTR_PRIORITY                    (5)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* TCP Client task handle. */
TaskHandle_t client_task_handle, secure_client_task_handle, connect_wifi_task_handle;

/* Binary semaphore handle to keep track of TCP server connection. */
SemaphoreHandle_t connect_to_wifi_sem;

/* var for mac address checksum */
uint16_t mac_checksum;

bool nonsecurity = false;
bool security = true;

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
int main(){

    cy_rslt_t result;

    /* Create a binary semaphore to keep track of TCP server connection. */
	connect_to_wifi_sem = xSemaphoreCreateBinary();

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package. */
    result = cybsp_init() ;
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    /* Initialize the User LED. */
	cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
						CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

	/* Initialize the user button (CYBSP_USER_BTN2) and register interrupt on falling edge. */
	cyhal_gpio_init(CYBSP_USER_BTN2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
	cyhal_gpio_register_callback(CYBSP_USER_BTN2, isr_button_press2, NULL);
	cyhal_gpio_enable_event(CYBSP_USER_BTN2, CYHAL_GPIO_IRQ_FALL, USER_BTN_INTR_PRIORITY, true);

	/* Initialize the user button (CYBSP_USER_BTN1) and register interrupt on falling edge. */
	cyhal_gpio_init(CYBSP_USER_BTN1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
	cyhal_gpio_register_callback(CYBSP_USER_BTN1, isr_button_press1, NULL);
	cyhal_gpio_enable_event(CYBSP_USER_BTN1, CYHAL_GPIO_IRQ_FALL, USER_BTN_INTR_PRIORITY, true);

	/* Initialize secure socket library. */
	result = cy_socket_init();
	if (result != CY_RSLT_SUCCESS){
		printf("Secure Socket initialization failed!\n");
		CY_ASSERT(0);
	}
	printf("Secure Socket initialized\n");

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("============================================================\n");
    printf("WiFi101 - 6B: Dual Client\n");
    printf("============================================================\n\n");

    /* Create the connect to wifi task. */
    xTaskCreate(connect_to_wifi_ap, "Connect to Wifi Task", CONNECT_WIFI_TASK_STACK_SIZE, NULL, CONNECT_WIFI_TASK_PRIORITY, &connect_wifi_task_handle);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/*******************************************************************************
 * Function Name: connect_to_wifi_ap()
 *******************************************************************************
 * Summary:
 *  Connects to Wi-Fi AP using the user-configured credentials, retries up to a
 *  configured number of times until the connection succeeds.
 *
 *******************************************************************************/
void connect_to_wifi_ap(void *arg){

    cy_rslt_t result;

    /* Variables used by Wi-Fi connection manager.*/
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_config_t wifi_config = { .interface = CY_WCM_INTERFACE_TYPE_STA };

    cy_wcm_ip_address_t ip_address;

     /* Initialize Wi-Fi connection manager. */
    result = cy_wcm_init(&wifi_config);

    if (result != CY_RSLT_SUCCESS){
        printf("Wi-Fi Connection Manager initialization failed!\n");
        CY_ASSERT(0);
    }
    printf("Wi-Fi Connection Manager initialized.\r\n");

     /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY_TYPE;

    /* Join the Wi-Fi AP. */
    for(uint32_t conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ ){
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                                wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %d.%d.%d.%d\n", (uint8)ip_address.ip.v4,
                    (uint8)(ip_address.ip.v4 >> 8), (uint8)(ip_address.ip.v4 >> 16),
                    (uint8)(ip_address.ip.v4 >> 24));

            cy_wcm_mac_t MAC_addr;
			cy_wcm_get_mac_addr(CY_WCM_INTERFACE_TYPE_STA, &MAC_addr, 1);
			mac_checksum = MAC_addr[0] +  MAC_addr[1] +  MAC_addr[2] +  MAC_addr[3] +  MAC_addr[4] +  MAC_addr[5];

			// Connected
			/* Create the network tasks once the wifi is connected. */
			xTaskCreate(tcp_client_task, "Network task", TCP_CLIENT_TASK_STACK_SIZE, &nonsecurity, TCP_CLIENT_TASK_PRIORITY, &client_task_handle);
			xTaskCreate(tcp_client_task, "Secure Network task", TCP_CLIENT_TASK_STACK_SIZE, &security, TCP_CLIENT_TASK_PRIORITY, &secure_client_task_handle);

			while(true){
				vTaskDelay(1);
			}
        }

        printf("Connection to Wi-Fi network failed with error code %d."
               "Retrying in %d ms...\n", (int)result, WIFI_CONN_RETRY_INTERVAL_MSEC);

        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MSEC));
    }

    /* Stop retrying after maximum retry attempts. */
    printf("Exceeded maximum Wi-Fi connection attempts\n");
    CY_ASSERT(0);
}

 /* [] END OF FILE */
