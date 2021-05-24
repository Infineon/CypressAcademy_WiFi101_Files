/******************************************************************************
* File Name:   tcp_client.c
*
* Description: This file contains task and functions related to TCP client
* operation.
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

/* Standard C header file. */
#include <string.h>

/* Cypress secure socket header file. */
#include "cy_secure_sockets.h"
#include "cy_tls.h"

/* TCP client task header file. */
#include "tcp_client.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Maximum number of connection retries to the TCP server. */
#define MAX_TCP_SERVER_CONN_RETRIES        (5u)

/* Length of the TCP data packet. */
#define MAX_TCP_DATA_PACKET_LENGTH         (20)

/* Length of the LED ON/OFF command issued from the TCP server. */
#define TCP_LED_CMD_LEN                    (1)
#define LED_ON_CMD                         '1'
#define LED_OFF_CMD                        '0'
#define ACK_LED_ON                         "LED ON ACK"
#define ACK_LED_OFF                        "LED OFF ACK"
#define MSG_INVALID_CMD                    "Invalid command"

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_tcp_client_socket(bool *security);
cy_rslt_t tcp_client_recv_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t connect_to_tcp_server(cy_socket_sockaddr_t address, bool *security);

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
extern TaskHandle_t client_task_handle, secure_client_task_handle;

/* var for mac address checksum */
extern uint16_t mac_checksum;

/* TLS credentials of the TCP client. */
static const char tcp_client_cert[] = keyCLIENT_CERTIFICATE_PEM;
static const char client_private_key[] = keyCLIENT_PRIVATE_KEY_PEM;

/* Root CA certificate for TCP server identity verification. */
static const char tcp_server_ca_cert[] = keySERVER_ROOTCA_PEM;

/* Variable to store the TLS identity (certificate and private key).*/
void *tls_identity;

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
void tcp_client_task(void *arg){

	bool security = (*(bool*)arg);

    cy_rslt_t result;

    /* Variable to hold number of bytes sent*/
	uint32_t bytes_sent;

	/* Create a binary semaphore to keep track of TCP server connection. */
	connect_to_server = xSemaphoreCreateBinary();

    /* Give the semaphore so as to connect to TCP server.  */
    xSemaphoreGive(connect_to_server);

    // Server Address
    cy_socket_sockaddr_t tcp_server_address;
    result = cy_socket_gethostbyname("awep.local", CY_SOCKET_IP_VER_V4, &tcp_server_address.ip_address);
    if(result == CY_RSLT_MODULE_SECURE_SOCKETS_HOST_NOT_FOUND && !security){
    	printf("Server not found!\n");
    	CY_ASSERT(0);
    }
    if(!security){
    	printf("AWEP server found!\n");
    }

    /* Variable to receive LED ON/OFF command from the user button ISR. */
	uint32_t led_state_cmd = LED_OFF_CMD;


    // Secure specific setup
    if(security){

		// Secure Port
		tcp_server_address.port = SECURE_TCP_SERVER_PORT;

		/* Initializes the global trusted RootCA certificate. This examples uses a self signed
		 * certificate which implies that the RootCA certificate is same as the certificate of
		 * TCP secure server to which client is connecting to.
		 */
		result = cy_tls_load_global_root_ca_certificates(tcp_server_ca_cert, strlen(tcp_server_ca_cert));
		if( result != CY_RSLT_SUCCESS){
			printf("cy_tls_load_global_root_ca_certificates failed\n");
		}
		else{
			printf("Global trusted RootCA certificate loaded\n");
		}

		/* Create TCP client identity using the SSL certificate and private key. */
		result = cy_tls_create_identity(tcp_client_cert, strlen(tcp_client_cert), client_private_key, strlen(client_private_key), &tls_identity);
		if(result != CY_RSLT_SUCCESS){
			printf("Failed cy_tls_create_identity! Error code: %d\n", (int)result);
			CY_ASSERT(0);
		}
		printf("Press user button 1 to send a secure message!\n");
    }

    // Non-Secure specific setup
	else{

		//Non Secure Port
		tcp_server_address.port = TCP_SERVER_PORT;
		vTaskDelay(20);
		printf("Press user button 2 to send a non-secure message!\n");
	}

	while(1){

		/* Wait till user button is pressed to send LED ON/OFF command to TCP server. */
		xTaskNotifyWait(0, 0, &led_state_cmd, portMAX_DELAY);

		// Semaphore to make sure both threads don't connect at the same time
		xSemaphoreTake(connect_to_server, portMAX_DELAY);

		/* Connect to the TCP server. If the connection fails, retry
		 * to connect to the server for MAX_TCP_SERVER_CONN_RETRIES times.
		 */
		if(security){
			printf("Connecting to secure TCP server...\n");
		}
		else{
			printf("Connecting to non-secure TCP server...\n");
		}
		result = connect_to_tcp_server(tcp_server_address, &security);

		if(result != CY_RSLT_SUCCESS){
			printf("Failed to connect to TCP server.\n");
			CY_ASSERT(0);
		}

		if(security){
			printf("Connected to secure port.\n");
		}
		else{
			printf("Connected to non-secure port.\n");
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
		if(result == CY_RSLT_SUCCESS ){
			if(led_state_cmd == LED_ON_CMD){
				printf("LED ON command sent to TCP server\n");
			}
			else{
				printf("LED OFF command sent to TCP server\n");
			}
		}
		else{
			printf("Failed to send command to server. Error: %d\n", (int)result);
			if(result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED){
				/* Disconnect the socket. */
				cy_socket_disconnect(client_handle, 0);
			}
		}

	}
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
cy_rslt_t create_tcp_client_socket(bool* security){

    cy_rslt_t result;

    /* Variables used to set socket options. */
    cy_socket_opt_callback_t tcp_recv_option;
    cy_socket_opt_callback_t tcp_disconnect_option;
	
	/* TLS authentication mode.*/
    cy_socket_tls_auth_mode_t tls_auth_mode = CY_SOCKET_TLS_VERIFY_REQUIRED;

    // Non-Secure specific setup
    if(*security == false){

    	printf("Creating non-secure socket\n");

    	/* Create a new secure TCP socket. */
		result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM, CY_SOCKET_IPPROTO_TCP, &client_handle);
		if (result != CY_RSLT_SUCCESS){
			printf("Failed to create socket!\n");
			return result;
		}
    }

    //Secure Specific setup
    else{

    	printf("Creating secure socket\n");

    	/* Create a new secure TCP socket. */
		result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM, CY_SOCKET_IPPROTO_TLS, &client_handle);
		if (result != CY_RSLT_SUCCESS)
		{
			printf("Failed to create socket!\n");
			return result;
		}

		/* Set the TCP socket to use the TLS identity. */
		result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TLS, CY_SOCKET_SO_TLS_IDENTITY, tls_identity, sizeof(tls_identity));
		if(result != CY_RSLT_SUCCESS){
			printf("Failed cy_socket_setsockopt! Error code: %d\n", (int)result);
		}
		
		/* Set the TLS authentication mode. */
		result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_TLS, CY_SOCKET_SO_TLS_AUTH_MODE,
							&tls_auth_mode, sizeof(cy_socket_tls_auth_mode_t));
		if(result != CY_RSLT_SUCCESS)
		{
			printf("Set socket option: CY_SOCKET_SO_TLS_AUTH_MODE failed! "
				   "Error Code: %lu\n", result);
		}
    }

    /* Register the callback function to handle messages received from TCP server. */
	tcp_recv_option.callback = tcp_client_recv_handler;
	tcp_recv_option.arg = security;
	result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_RECEIVE_CALLBACK, &tcp_recv_option, sizeof(cy_socket_opt_callback_t));
	if (result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_RECEIVE_CALLBACK failed\n");
		return result;
	}

	/* Register the callback function to handle disconnection. */
	tcp_disconnect_option.callback = tcp_disconnection_handler;
	tcp_disconnect_option.arg = NULL;
	result = cy_socket_setsockopt(client_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_DISCONNECT_CALLBACK, &tcp_disconnect_option, sizeof(cy_socket_opt_callback_t));
	if(result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_DISCONNECT_CALLBACK failed\n");
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
cy_rslt_t connect_to_tcp_server(cy_socket_sockaddr_t address, bool *security){

    cy_rslt_t result = CY_RSLT_MODULE_SECURE_SOCKETS_TIMEOUT;
    cy_rslt_t conn_result;

    for(uint32_t conn_retries = 0; conn_retries < MAX_TCP_SERVER_CONN_RETRIES; conn_retries++){
        /* Create a TCP socket */
        conn_result = create_tcp_client_socket(security);
        if(conn_result != CY_RSLT_SUCCESS){
            printf("Socket creation failed!\n");
            CY_ASSERT(0);
        }

        conn_result = cy_socket_connect(client_handle, &address, sizeof(cy_socket_sockaddr_t));

        if (conn_result == CY_RSLT_SUCCESS){
            printf("============================================================\n");

            return conn_result;
        }

        printf("Could not connect to TCP server.\n");
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
 *  void *args : bool representing whether the message was received by the secure or nonsecure socket
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_client_recv_handler(cy_socket_t socket_handle, void *arg)
{
	// Secure/Unsecure var
	bool security = (*(bool*)arg);

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
    else if(message_buffer[0] == 'X'){
        printf("Write Rejected\n");
    }
    else{
        printf("Invalid command\n");
    }

    //disconnect
	cy_socket_disconnect(client_handle, 0);
	/* Free the resources allocated to the socket. */
	cy_socket_delete(client_handle);
	xSemaphoreGive(connect_to_server);
	if(security){
		printf("Disconnected from Secure Server\n");
	}
	else{
		printf("Disconnected from Non-Secure Server\n");
	}

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
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg){

    cy_rslt_t result;

    /* Disconnect the TCP client. */
    result = cy_socket_disconnect(socket_handle, 0);

    /* Free the resources allocated to the socket. */
    cy_socket_delete(socket_handle);

    printf("Disconnected from the TCP server! \n");

    /* Give the semaphore so as to connect to TCP server.  */
    xSemaphoreGive(connect_to_server);

    return result;
}

/*******************************************************************************
 * Function Name: isr_button_press2
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
void isr_button_press2( void *callback_arg, cyhal_gpio_event_t event){

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Variable to hold the LED ON/OFF command to be sent to the TCP client. */
    uint32_t led_state_cmd = LED_OFF_CMD;

    /* Set the command to be sent to TCP client. */
    if(led_state == CYBSP_LED_STATE_ON){
        led_state_cmd = LED_OFF_CMD;
        led_state = CYBSP_LED_STATE_OFF;
    }
    else{
        led_state_cmd = LED_ON_CMD;
        led_state = CYBSP_LED_STATE_ON;
    }

    /* Set the flag to send command to TCP client. */
    xTaskNotifyFromISR(client_task_handle, led_state_cmd, eSetValueWithoutOverwrite, &xHigherPriorityTaskWoken);

    /* Force a context switch if xHigherPriorityTaskWoken is now set to pdTRUE. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
 * Function Name: isr_button_press1
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
void isr_button_press1( void *callback_arg, cyhal_gpio_event_t event){

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Variable to hold the LED ON/OFF command to be sent to the TCP client. */
	uint32_t led_state_cmd = LED_OFF_CMD;

    /* Set the command to be sent to TCP client. */
    if(led_state == CYBSP_LED_STATE_ON){
        led_state_cmd = LED_OFF_CMD;
        led_state = CYBSP_LED_STATE_OFF;
    }
    else{
        led_state_cmd = LED_ON_CMD;
        led_state = CYBSP_LED_STATE_ON;
    }

    /* Set the flag to send command to TCP client. */
    xTaskNotifyFromISR(secure_client_task_handle, led_state_cmd, eSetValueWithoutOverwrite, &xHigherPriorityTaskWoken);

    /* Force a context switch if xHigherPriorityTaskWoken is now set to pdTRUE. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* [] END OF FILE */
