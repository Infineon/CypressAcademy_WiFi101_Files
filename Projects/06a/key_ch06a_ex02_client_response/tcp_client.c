/******************************************************************************
* File Name:   tcp_client.c
*
* Description: This file contains task and functions related to TCP client
* operation.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2019-2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
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
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/* Header file includes. */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Standard C header file. */
#include <string.h>

/* Cypress secure socket header file. */
#include "cy_secure_sockets.h"

/* Wi-Fi connection manager header files. */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* TCP client task header file. */
#include "tcp_client.h"

#include "cy_lwip.h"

/* Standard C header files */
#include <inttypes.h>

/*******************************************************************************
* Macros
********************************************************************************/
#define MAKE_IP_PARAMETERS(a, b, c, d)           ((((uint32_t) d) << 24) | \
                                                 (((uint32_t) c) << 16) | \
                                                 (((uint32_t) b) << 8) |\
                                                 ((uint32_t) a))

/* Wi-Fi Credentials: Modify WIFI_SSID, WIFI_PASSWORD, and WIFI_SECURITY_TYPE
 * to match your Wi-Fi network credentials.
 * Note: Maximum length of the Wi-Fi SSID and password is set to
 * CY_WCM_MAX_SSID_LEN and CY_WCM_MAX_PASSPHRASE_LEN as defined in cy_wcm.h file.
 */
#define WIFI_SSID                             "SSID"
#define WIFI_PASSWORD                         "PSSWD"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY_TYPE                    CY_WCM_SECURITY_WPA2_AES_PSK
/* Maximum number of connection retries to a Wi-Fi network. */
#define MAX_WIFI_CONN_RETRIES                 (10u)

/* Wi-Fi re-connection time interval in milliseconds */
#define WIFI_CONN_RETRY_INTERVAL_MSEC         (1000u)

/* Maximum number of connection retries to the TCP server. */
#define MAX_TCP_SERVER_CONN_RETRIES               (5u)

/* Length of the TCP data packet. */
#define MAX_TCP_DATA_PACKET_LENGTH                (20u)

/* TCP keep alive related macros. */
#define TCP_KEEP_ALIVE_IDLE_TIME_MS               (10000u)
#define TCP_KEEP_ALIVE_INTERVAL_MS                (1000u)
#define TCP_KEEP_ALIVE_RETRY_COUNT                (2u)

/* Length of the LED ON/OFF command issued from the TCP server. */
#define TCP_LED_CMD_LEN                           (1u)
#define LED_ON_CMD                                '1'
#define LED_OFF_CMD                               '0'
#define ACK_LED_ON                                "LED ON ACK"
#define ACK_LED_OFF                               "LED OFF ACK"
#define MSG_INVALID_CMD                    		"Invalid command"

#define TCP_SERVER_PORT                           (27708u)

/* Interrupt priority of the user button. */
#define USER_BTN_INTR_PRIORITY                    (5)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_tcp_client_socket();
cy_rslt_t tcp_client_recv_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t connect_to_tcp_server(cy_socket_sockaddr_t address);
static cy_rslt_t connect_to_wifi_ap(void);
void isr_button_press( void *callback_arg, cyhal_gpio_event_t event);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* TCP client socket handle */
cy_socket_t client_handle;

/* Binary semaphore handle to keep track of TCP server connection. */
SemaphoreHandle_t connect_to_server;

/* Flags to track the LED state. */
bool led_state = CYBSP_LED_STATE_OFF;

/* TCP server task handle. */
extern TaskHandle_t client_task_handle;

/* var for mac address checksum */
uint16_t mac_checksum;

/*******************************************************************************
 * Function Name: tcp_client_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a remote TCP server and
 *  control the LED state (ON/OFF) based on the command received from TCP server.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused).
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void tcp_client_task(void *arg)
{
    cy_rslt_t result;

    /* Variable to receive LED ON/OFF command from the user button ISR. */
	uint32_t led_state_cmd = LED_OFF_CMD;

	/* Variable to hold number of bytes sent*/
	uint32_t bytes_sent;

	/* Initialize the user button (CYBSP_USER_BTN) and register interrupt on falling edge. */
	cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
	cyhal_gpio_register_callback(CYBSP_USER_BTN, isr_button_press, NULL);
	cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, USER_BTN_INTR_PRIORITY, true);

	/* Connect to Wi-Fi AP */
	result = connect_to_wifi_ap();
	if(result!= CY_RSLT_SUCCESS )
	{
		printf("\n Failed to connect to Wi-Fi AP! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
		CY_ASSERT(0);
	}

    /* Create a binary semaphore to keep track of TCP server connection. */
    connect_to_server = xSemaphoreCreateBinary();

    /* Give the semaphore so as to connect to TCP server.  */
    xSemaphoreGive(connect_to_server);

    /* Initialize secure socket library. */
    result = cy_socket_init();

    if (result != CY_RSLT_SUCCESS)
    {
        printf("Secure Socket initialization failed!\n");
        CY_ASSERT(0);
    }
    printf("Secure Socket initialized\n");

    /* IP address and TCP port number of the TCP server to which the TCP client
	 * connects to.
	 */
    cy_socket_sockaddr_t tcp_server_address;
	result = cy_socket_gethostbyname("awep.local", CY_SOCKET_IP_VER_V4, &tcp_server_address.ip_address);
	if(result == CY_RSLT_MODULE_SECURE_SOCKETS_HOST_NOT_FOUND){
		printf("Server not found!\n");
		CY_ASSERT(0);
	}
	tcp_server_address.port = TCP_SERVER_PORT;
	printf("AWEP server found! Press the user button to send a message!\n");

    for(;;)
    {
    	/* Wait till user button is pressed to send LED ON/OFF command to TCP server. */
		xTaskNotifyWait(0, 0, &led_state_cmd, portMAX_DELAY);

        /* Wait till semaphore is acquired so as to connect to a TCP server. */
        xSemaphoreTake(connect_to_server, portMAX_DELAY);

        /* Connect to the TCP server. If the connection fails, retry
         * to connect to the server for MAX_TCP_SERVER_CONN_RETRIES times.
         */
        printf("Connecting to TCP Server (IP Address: %s, Port: %d)\n\n",
                      ip4addr_ntoa((const ip4_addr_t *)&tcp_server_address.ip_address.ip.v4),
                      TCP_SERVER_PORT);
        result = connect_to_tcp_server(tcp_server_address);
        if(result != CY_RSLT_SUCCESS)
        {
            printf("Failed to connect to TCP server.\n");
            CY_ASSERT(0);
        }

        //message buffer
		char message[MAX_TCP_DATA_PACKET_LENGTH];

		//construct message
		if(led_state_cmd == LED_ON_CMD){
			snprintf(message, sizeof(message), "W%04x050001", mac_checksum);

		}
		else{
			snprintf(message, sizeof(message), "W%04x050000", mac_checksum);
		}

		/* Send the command to TCP server. */
		result = cy_socket_send(client_handle, message, MAX_TCP_DATA_PACKET_LENGTH,
					   CY_SOCKET_FLAGS_NONE, &bytes_sent);
		if(result == CY_RSLT_SUCCESS )
		{
			if(led_state_cmd == LED_ON_CMD)
			{
				printf("LED ON command sent to TCP server\n");
			}
			else
			{
				printf("LED OFF command sent to TCP server\n");
			}
		}
		else
		{
			printf("Failed to send command to server. Error: %d\n", (int)result);
			if(result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
			{
				/* Disconnect the socket. */
				cy_socket_disconnect(client_handle, 0);
			}
		}
    }
 }

/*******************************************************************************
 * Function Name: connect_to_wifi_ap()
 *******************************************************************************
 * Summary:
 *  Connects to Wi-Fi AP using the user-configured credentials, retries up to a
 *  configured number of times until the connection succeeds.
 *
 *******************************************************************************/
cy_rslt_t connect_to_wifi_ap(void)
{
    cy_rslt_t result;

    /* Variables used by Wi-Fi connection manager.*/
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_config_t wifi_config = { .interface = CY_WCM_INTERFACE_TYPE_STA };

    cy_wcm_ip_address_t ip_address;

    /* Initialize Wi-Fi connection manager. */
	result = cy_wcm_init(&wifi_config);

	if (result != CY_RSLT_SUCCESS)
	{
		printf("Wi-Fi Connection Manager initialization failed! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
		CY_ASSERT(0);
	}
	printf("Wi-Fi Connection Manager initialized.\r\n");

     /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY_TYPE;

    printf("Connecting to Wi-Fi Network: %s\n", WIFI_SSID);

    /* Join the Wi-Fi AP. */
    for(uint32_t conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ )
    {
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                                wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %s\n",
                    ip4addr_ntoa((const ip4_addr_t *)&ip_address.ip.v4));

            cy_wcm_mac_t MAC_addr;
			cy_wcm_get_mac_addr(CY_WCM_INTERFACE_TYPE_STA, &MAC_addr, 1);
			mac_checksum = MAC_addr[0] +  MAC_addr[1] +  MAC_addr[2] +  MAC_addr[3] +  MAC_addr[4] +  MAC_addr[5];
            return result;
        }

        printf("Connection to Wi-Fi network failed with error code %d."
               "Retrying in %d ms...\n", (int)result, WIFI_CONN_RETRY_INTERVAL_MSEC);

        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MSEC));
    }

    /* Stop retrying after maximum retry attempts. */
    printf("Exceeded maximum Wi-Fi connection attempts\n");

    return result;
}

/*******************************************************************************
 * Function Name: create_tcp_client_socket
 *******************************************************************************
 * Summary:
 *  Function to create a socket and set the socket options
 *  to set call back function for handling incoming messages, call back
 *  function to handle disconnection.
 *
 *******************************************************************************/
cy_rslt_t create_tcp_client_socket()
{
    cy_rslt_t result;

    /* TCP keep alive parameters. */
    int keep_alive = 1;
    uint32_t keep_alive_interval = TCP_KEEP_ALIVE_INTERVAL_MS;
    uint32_t keep_alive_count    = TCP_KEEP_ALIVE_RETRY_COUNT;
    uint32_t keep_alive_idle_time = TCP_KEEP_ALIVE_IDLE_TIME_MS;

    /* Variables used to set socket options. */
    cy_socket_opt_callback_t tcp_recv_option;
    cy_socket_opt_callback_t tcp_disconnect_option;

    /* Create a new secure TCP socket. */
    result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                              CY_SOCKET_IPPROTO_TCP, &client_handle);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("Failed to create socket!\n");
        return result;
    }

    /* Register the callback function to handle messages received from TCP server. */
    tcp_recv_option.callback = tcp_client_recv_handler;
    tcp_recv_option.arg = NULL;
    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_RECEIVE_CALLBACK,
                                  &tcp_recv_option, sizeof(cy_socket_opt_callback_t));
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_RECEIVE_CALLBACK failed\n");
        return result;
    }

    /* Register the callback function to handle disconnection. */
    tcp_disconnect_option.callback = tcp_disconnection_handler;
    tcp_disconnect_option.arg = NULL;

    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_DISCONNECT_CALLBACK,
                                  &tcp_disconnect_option, sizeof(cy_socket_opt_callback_t));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_DISCONNECT_CALLBACK failed\n");
    }


    /* Set the TCP keep alive interval. */
    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                  CY_SOCKET_SO_TCP_KEEPALIVE_INTERVAL,
                                  &keep_alive_interval, sizeof(keep_alive_interval));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_INTERVAL failed\n");
        return result;
    }

    /* Set the retry count for TCP keep alive packet. */
    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                  CY_SOCKET_SO_TCP_KEEPALIVE_COUNT,
                                  &keep_alive_count, sizeof(keep_alive_count));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_COUNT failed\n");
        return result;
    }

    /* Set the network idle time before sending the TCP keep alive packet. */
    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TCP,
                                  CY_SOCKET_SO_TCP_KEEPALIVE_IDLE_TIME,
                                  &keep_alive_idle_time, sizeof(keep_alive_idle_time));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_IDLE_TIME failed\n");
        return result;
    }

    /* Enable TCP keep alive. */
    result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET,
                                      CY_SOCKET_SO_TCP_KEEPALIVE_ENABLE,
                                          &keep_alive, sizeof(keep_alive));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_TCP_KEEPALIVE_ENABLE failed\n");
        return result;
    }

    return result;
}

/*******************************************************************************
 * Function Name: connect_to_tcp_server
 *******************************************************************************
 * Summary:
 *  Function to connect to TCP server.
 *
 * Parameters:
 *  cy_socket_sockaddr_t address: Address of TCP server socket
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t connect_to_tcp_server(cy_socket_sockaddr_t address)
{
    cy_rslt_t result = CY_RSLT_MODULE_SECURE_SOCKETS_TIMEOUT;
    cy_rslt_t conn_result;

    for(uint32_t conn_retries = 0; conn_retries < MAX_TCP_SERVER_CONN_RETRIES; conn_retries++)
    {
        /* Create a TCP socket */
        conn_result = create_tcp_client_socket();
        
        if(conn_result != CY_RSLT_SUCCESS)
        {
            printf("Socket creation failed!\n");
            CY_ASSERT(0);
        }

        conn_result = cy_socket_connect(client_handle, &address, sizeof(cy_socket_sockaddr_t));
        
        if (conn_result == CY_RSLT_SUCCESS)
        {
            printf("============================================================\n");
            printf("Connected to TCP server\n");

            return conn_result;
        }

        printf("Could not connect to TCP server. Error code: 0x%08"PRIx32"\n", (uint32_t)result);
        printf("Trying to reconnect to TCP server... Please check if the server is listening\n");

        /* The resources allocated during the socket creation (cy_socket_create)
         * should be deleted.
         */
        cy_socket_delete(client_handle);
    }

     /* Stop retrying after maximum retry attempts. */
     printf("Exceeded maximum connection attempts to the TCP server\n");

     return result;
}

/*******************************************************************************
 * Function Name: tcp_client_recv_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming TCP server messages.
 *
 * Parameters:
 *  cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_client_recv_handler(cy_socket_t socket_handle, void *arg)
{
	/* Variable to store number of bytes received. */
	    uint32_t bytes_received = 0;

	    char message_buffer[MAX_TCP_DATA_PACKET_LENGTH];
	    cy_rslt_t result ;

	    result = cy_socket_recv(socket_handle, message_buffer, MAX_TCP_DATA_PACKET_LENGTH,
	                            CY_SOCKET_FLAGS_NONE, &bytes_received);
	    printf("message received: %s\n",message_buffer);

	    if(message_buffer[0] == 'A')
	    {
	        printf("Write Accepted\n");
	        if(message_buffer[10] == '1') /* LED state in response message is ON */
	        {
	        	/* LED ON */
	            cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
	        }
	        else
	        {
	        	/* LED OFF */
	            cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
	        }
	    }
	    else if(message_buffer[0] == 'X')
	    {
	        printf("Write Rejected\n");
	    }
	    else{
	        printf("Invalid command\n");
	    }

	    /* Disconnect the socket once the response message has been received. */
		cy_socket_disconnect(client_handle, 0);
		printf("Disconnecting from TCP Server.\n");
		/* Free the resources allocated to the socket. */
		cy_socket_delete(client_handle);
		/* Give the semaphore so as to connect to TCP server.  */
		xSemaphoreGive(connect_to_server);

	    return result;
}

/*******************************************************************************
 * Function Name: tcp_disconnection_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle TCP socket disconnection event.
 *
 * Parameters:
 *  cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg)
{
    cy_rslt_t result;

    /* Disconnect the TCP client. */
    result = cy_socket_disconnect(socket_handle, 0);

    /* Free the resources allocated to the socket. */
    cy_socket_delete(socket_handle);

    printf("Disconnected from the TCP server! \n");

    /* Give the semaphore so as to connect to TCP server. */
    xSemaphoreGive(connect_to_server);

    return result;
}

/*******************************************************************************
 * Function Name: isr_button_press
 *******************************************************************************
 *
 * Summary:
 *  GPIO interrupt service routine. This function detects button presses and
 *  sets the command to be sent to TCP client.
 *
 * Parameters:
 *  void *callback_arg : pointer to the variable passed to the ISR
 *  cyhal_gpio_event_t event : GPIO event type
 *
 * Return:
 *  None
 *
 *******************************************************************************/
void isr_button_press( void *callback_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Variable to hold the LED ON/OFF command to be sent to the TCP client. */
    uint32_t led_state_cmd = LED_OFF_CMD;

    /* Set the command to be sent to TCP client. */
    if(led_state == CYBSP_LED_STATE_ON)
    {
        led_state_cmd = LED_OFF_CMD;
        led_state = CYBSP_LED_STATE_OFF;
        cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
    }
    else
    {
        led_state_cmd = LED_ON_CMD;
        led_state = CYBSP_LED_STATE_ON;
        cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
    }

    /* Set the flag to send command to TCP client. */
    xTaskNotifyFromISR(client_task_handle, led_state_cmd,
                      eSetValueWithoutOverwrite, &xHigherPriorityTaskWoken);

    /* Force a context switch if xHigherPriorityTaskWoken is now set to pdTRUE. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* [] END OF FILE */
