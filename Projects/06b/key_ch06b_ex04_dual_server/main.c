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

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* TCP server task header file. */
#include "tcp_server.h"

/* Cypress secure socket header file */
#include "cy_secure_sockets.h"
#include "cy_tls.h"

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* mDNS */
#include "mdns.h"
#include "cy_lwip.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros for TCP server task. */
#define TCP_SERVER_TASK_STACK_SIZE                (1024 * 5)
#define CONNECT_TO_WIFI_TASK_STACK_SIZE           (1024)
#define TCP_SERVER_TASK_PRIORITY                  (1)
#define CONNECT_TO_WIFI_TASK_PRIORITY			  (2)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* TCP server task handle. */
TaskHandle_t server_task_handle, secure_server_task_handle, connect_to_wifi_task_handle;

// args to pass to the two tasks. One task for secure, one for non-secure
bool security = true;
bool noSecurity = false;

// IP address of the device
cy_wcm_ip_address_t ip_address;

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

    cy_rslt_t result ;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1 ;

    /* Initialize the board support package. */
    result = cybsp_init() ;
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("Wifi101 - 6B: Dual Server\n");
    printf("===============================================================\n\n");

	/* Initialize secure socket library. */
	result = cy_socket_init();
	if (result != CY_RSLT_SUCCESS){
		printf("Secure Socket initialization failed!\n");
		CY_ASSERT(0);
	}
	printf("Secure Socket initialized\n");

    /* Create connect to wifi task. */
    xTaskCreate(connect_to_wifi_ap_task, "connect to WiFi task", CONNECT_TO_WIFI_TASK_STACK_SIZE, NULL, CONNECT_TO_WIFI_TASK_PRIORITY, &connect_to_wifi_task_handle);

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
void connect_to_wifi_ap_task(void *arg){

    cy_rslt_t result;

    /* Variables used by Wi-Fi connection manager.*/
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_config_t wifi_config = {
            .interface = CY_WCM_INTERFACE_TYPE_STA
    };

    /* Variable to track the number of connection retries to the Wi-Fi AP specified by WIFI_SSID macro. */
    int conn_retries = 0;

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
    for(conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ ){
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS){
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                                wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %d.%d.%d.%d\n", (uint8)ip_address.ip.v4,
                    (uint8)(ip_address.ip.v4 >> 8), (uint8)(ip_address.ip.v4 >> 16),
                    (uint8)(ip_address.ip.v4 >> 24));

            // Connected

            /* Start mDNS responder */
			err_t error;
			mdns_resp_init();
			/* IP of my device */
			struct netif *myNetif;
			myNetif = cy_lwip_get_interface(CY_LWIP_STA_NW_INTERFACE);
			error = mdns_resp_add_netif(myNetif, "awep", 100);
			if(error == ERR_OK){
				printf("mDNS responder initialized successfully.\n");
			}

            /* Create the network tasks. */
            xTaskCreate(tcp_server_task, "non-secure network task", TCP_SERVER_TASK_STACK_SIZE, &noSecurity, TCP_SERVER_TASK_PRIORITY, &server_task_handle);
        	xTaskCreate(tcp_server_task, "secure Network task", TCP_SERVER_TASK_STACK_SIZE, &security, TCP_SERVER_TASK_PRIORITY, &secure_server_task_handle);

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
