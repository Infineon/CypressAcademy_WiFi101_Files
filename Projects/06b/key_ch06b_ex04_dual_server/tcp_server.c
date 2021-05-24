/******************************************************************************
* File Name:   tcp_server.c
*
* Description: This file contains declaration of task and functions related to
* TCP server operation.
*
*******************************************************************************
* (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
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

/* Cypress secure socket header file */
#include "cy_secure_sockets.h"
#include "cy_tls.h"

/* Standard C header file */
#include <stdlib.h>
#include <string.h>

/* TCP server task header file. */
#include "tcp_server.h"

/* Linked list */
#include "linkedList.h"

/* isxdigit() */
#include <ctype.h>

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros for TCP server task. */


/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_tcp_server_socket(bool* security, cy_socket_t* server_handle, cy_socket_sockaddr_t* server_addr, cy_socket_t* client_handle);
cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg);
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg);

/*******************************************************************************
* Global Variables
********************************************************************************/

/* Flags to track the LED state. */
bool led_state = CYBSP_LED_STATE_OFF;

/* Variable to store the TLS identity (certificate and private key).*/
void *tls_identity;

// IP address of the device
extern cy_wcm_ip_address_t ip_address;

// Buffers to print to
char secureBuffer[100];
char nonSecureBuffer[100];
/*******************************************************************************
 * Function Name: tcp_server_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a remote TCP client.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void tcp_server_task(void *arg){

	cy_rslt_t result;

	// var passed in when task is created, 0 for nonsecure, 1 for secure
	bool security = (*(bool*)arg);

	//server handle and address
	static cy_socket_t server_handle;
	cy_socket_t client_handle;
	cy_socket_sockaddr_t server_addr;

	// Populate the ip var with the device ip and correct port
	server_addr.ip_address.ip.v4 = ip_address.ip.v4;
	server_addr.ip_address.version = CY_SOCKET_IP_VER_V4;

	// secure specific setup
	if(security){

		server_addr.port = SECURE_TCP_SERVER_PORT;

		/* TLS credentials of the TCP server. */
		static const char tcp_server_cert[] = keySERVER_CERTIFICATE_PEM;
		static const char server_private_key[] = keySERVER_PRIVATE_KEY_PEM;

		/* Root CA certificate for TCP client identity verification. */
		static const char tcp_client_ca_cert[] = keyCLIENT_ROOTCA_PEM;

		/* Create TCP server identity using the SSL certificate and private key. */
		result = cy_tls_create_identity(tcp_server_cert, strlen(tcp_server_cert), server_private_key, strlen(server_private_key), &tls_identity);
		if(result != CY_RSLT_SUCCESS){
			printf("Failed cy_tls_create_identity! Error code: %d\n", (int)result);
			CY_ASSERT(0);
		}

		/* Initializes the global trusted RootCA certificate. This examples uses a self signed
		 * certificate which implies that the RootCA certificate is same as the TCP client
		 * certificate. */
		result = cy_tls_load_global_root_ca_certificates(tcp_client_ca_cert, strlen(tcp_client_ca_cert));
		if( result != CY_RSLT_SUCCESS){
			printf("cy_tls_load_global_root_ca_certificates failed\n");
			CY_ASSERT(0);
		}
		else{
			printf("Global trusted RootCA certificate loaded\n");
		}

	}
	// non-secure specific setup
	else{
		server_addr.port = TCP_SERVER_PORT;
	}

	/* Create TCP server socket. */
	result = create_tcp_server_socket(&security, &server_handle, &server_addr, &client_handle);
	if (result != CY_RSLT_SUCCESS){
		printf("Failed to create socket!\n");
		CY_ASSERT(0);
	}

	/* Start listening on the TCP server socket. */
	result = cy_socket_listen(server_handle, TCP_SERVER_MAX_PENDING_CONNECTIONS);
	if (result != CY_RSLT_SUCCESS){
		cy_socket_delete(server_handle);
		printf("cy_socket_listen returned error. Error: %d\n", (int)result);
		CY_ASSERT(0);
	}
	else{
		printf("===============================================================\n");
		printf("Listening for incoming TCP client connection on Port: %d\n",
				server_addr.port);
	}

	while(true){
		vTaskDelay(1);
	}

 }

/*******************************************************************************
 * Function Name: create_tcp_server_socket
 *******************************************************************************
 * Summary:
 *  Function to create a socket and set the socket options
 *
 *******************************************************************************/
cy_rslt_t create_tcp_server_socket(bool* security, cy_socket_t* server_handle, cy_socket_sockaddr_t* server_addr, cy_socket_t* client_handle){

    cy_rslt_t result;

    /* TCP socket receive timeout period. */
    uint32_t tcp_recv_timeout = TCP_SERVER_RECV_TIMEOUT_MS;

    /* Variables used to set socket options. */
    cy_socket_opt_callback_t tcp_receive_option;
    cy_socket_opt_callback_t tcp_connection_option;
    cy_socket_opt_callback_t tcp_disconnection_option;

    /* TLS authentication mode.*/
    cy_socket_tls_auth_mode_t tls_auth_mode = CY_SOCKET_TLS_VERIFY_REQUIRED;

    //secure specific setup
    if(*security){

    	/* Create a Secure TCP socket. */
		result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM, CY_SOCKET_IPPROTO_TLS, server_handle);
		if(result != CY_RSLT_SUCCESS){
			printf("Failed to create socket! Error code: %d\n", (int)result);
			return result;
		}
		printf("Created secure socket\n");

		/* Set the TCP socket to use the TLS identity. */
		result = cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_TLS, CY_SOCKET_SO_TLS_IDENTITY, tls_identity, sizeof(tls_identity));
		if(result != CY_RSLT_SUCCESS){
			printf("Failed cy_socket_setsockopt! Error code: %d\n", (int)result);
			return result;
		}

		/* Set the TLS authentication mode. */
		cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_TLS, CY_SOCKET_SO_TLS_AUTH_MODE, &tls_auth_mode, sizeof(cy_socket_tls_auth_mode_t));
    }

    //nonsecure specific setup
    else{

    	/* Create a non-secure TCP socket. */
		result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM, CY_SOCKET_IPPROTO_TCP, server_handle);
		if(result != CY_RSLT_SUCCESS)
		{
			printf("Failed to create socket! Error code: %d\n", (int)result);
			return result;
		}
		printf("Created non-secure socket\n");

    }

	/* Set the TCP socket receive timeout period. */
	result = cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_RCVTIMEO, &tcp_recv_timeout, sizeof(tcp_recv_timeout));
	if(result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_RCVTIMEO failed\n");
		return result;
	}

	/* Register the callback function to handle connection request from a TCP client. */
	tcp_connection_option.callback = tcp_connection_handler;
	tcp_connection_option.arg = client_handle;
	result = cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK, &tcp_connection_option, sizeof(cy_socket_opt_callback_t));
	if(result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK failed\n");
		return result;
	}

	/* Register the callback function to handle messages received from a TCP client. */
	tcp_receive_option.callback = tcp_receive_msg_handler;
	tcp_receive_option.arg = security;
	result = cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_RECEIVE_CALLBACK, &tcp_receive_option, sizeof(cy_socket_opt_callback_t));
	if(result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_RECEIVE_CALLBACK failed\n");
		return result;
	}

	/* Register the callback function to handle disconnection. */
	tcp_disconnection_option.callback = tcp_disconnection_handler;
	tcp_disconnection_option.arg = NULL;
	result = cy_socket_setsockopt(*server_handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_DISCONNECT_CALLBACK, &tcp_disconnection_option, sizeof(cy_socket_opt_callback_t));
	if(result != CY_RSLT_SUCCESS){
		printf("Set socket option: CY_SOCKET_SO_DISCONNECT_CALLBACK failed\n");
		return result;
	}

	/* Bind the TCP socket created to Server IP address and to TCP port. */
	result = cy_socket_bind(*server_handle, server_addr, sizeof(*server_addr));
	if(result != CY_RSLT_SUCCESS){
		printf("Failed to bind to socket! Error code: %d\n", (int)result);
	}


    return result;
}

 /*******************************************************************************
 * Function Name: tcp_connection_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming TCP client connection.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP server socket
 *  void *args : Unused
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg){

    cy_rslt_t result;

    //pointer to client handle passed through arg
    cy_socket_t* client_handle = arg;

    // var to store the address of the connecting client
    cy_socket_sockaddr_t peer_addr;

    /* Size of the peer socket address. */
    uint32_t peer_addr_len;

    /* Accept new incoming connection from a TCP client.*/
    result = cy_socket_accept(socket_handle, &peer_addr, &peer_addr_len, client_handle);

    // Vars to determine if the connection was secure or non-secure
	cy_socket_tls_auth_mode_t tls_auth_mode;
	uint32_t length = sizeof(cy_socket_tls_auth_mode_t);
	cy_socket_getsockopt(socket_handle, CY_SOCKET_SOL_TLS, CY_SOCKET_SO_TLS_AUTH_MODE, &tls_auth_mode, &length);

    if(result == CY_RSLT_SUCCESS){
		// Print Connection Info to the appropriate buffer
		if(tls_auth_mode == CY_SOCKET_TLS_VERIFY_REQUIRED){
			sprintf(secureBuffer,"Connection from IP: %d.%d.%d.%d\tConnection: Secure\t",(uint8)peer_addr.ip_address.ip.v4,
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 8),
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 16),
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 24));
		}
		else{
			sprintf(nonSecureBuffer,"Connection from IP: %d.%d.%d.%d\tConnection: Non-Secure\t",(uint8)peer_addr.ip_address.ip.v4,
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 8),
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 16),
																						 (uint8)(peer_addr.ip_address.ip.v4 >> 24));
		}
    }
    else{
        printf("Failed to accept incoming client connection. Error: %d\n", (int)result);
    }

    return result;
}
/*******************************************************************************
* Function Name: sendAck
*******************************************************************************
* Summary:
* Function to send acknowledgement to tcp client.
*
* Parameters:
* char *message: message to send
* cy_socket_t socket_handle: Connection handle for the TCP client socket
*
* Return:
*  void
*
*******************************************************************************/
void sendAck(char *message, cy_socket_t socket_handle, bool security){

	cy_rslt_t result;
	uint32_t bytes_sent;
	// Buffer for creating the connection information prints
	char writeBuffer[30];

	/* Send the command to TCP server. */
	result = cy_socket_send(socket_handle, message, MAX_TCP_DATA_PACKET_LENGTH, CY_SOCKET_FLAGS_NONE, &bytes_sent);
	if(result == CY_RSLT_SUCCESS ){
		if(security){
			sprintf(writeBuffer,"Response: %s\n", message);
			strcat(secureBuffer, writeBuffer);

		}
		else{
			sprintf(writeBuffer,"Response: %s\n", message);
			strcat(nonSecureBuffer, writeBuffer);
		}
	}
	else{
		printf("Failed to send ack to client. Error: %d\n", (int)result);
	}


	// Disconnect once the ack has been sent
	result = cy_socket_disconnect(socket_handle, 0);
	if(result != CY_RSLT_SUCCESS){
		printf("Disconnect Failed!\n");
		CY_ASSERT(0);
	}
	/* Delete the client socket. */
	result = cy_socket_delete(socket_handle);
	if(result != CY_RSLT_SUCCESS){
		printf("Socket Delete Failed!\n");
		CY_ASSERT(0);
	}

	// Print the connection information
	if(security){
		printf("%s", secureBuffer);
	}
	else{
		printf("%s", nonSecureBuffer);
	}
}


 /*******************************************************************************
 * Function Name: tcp_receive_msg_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming TCP client messages.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *arg : Bool representing whether the received message came from the secure or non secure socket
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg){

	// Security var passed in
	bool security = (*(bool*)arg);

    cy_rslt_t result;

    // buffer to store the message that is being recieved
    char message_buffer[MAX_TCP_RECV_BUFFER_SIZE];

    // Buffer for creating the connection information prints
    char writeBuffer[30];

    // linked list to store connection information in
    static dbEntry_t head = {
    	.next = NULL,
    	.deviceId = 0,
    	.regId = 0
    };

    //new database entry to add onto the list
    dbEntry_t receive;

    // var to store commandID - W/R
    char commandId;

    /* Variable to store number of bytes received from TCP client. */
    uint32_t bytes_received = 0;

    // Receive
    result = cy_socket_recv(socket_handle, message_buffer, MAX_TCP_RECV_BUFFER_SIZE,
                            CY_SOCKET_FLAGS_NONE, &bytes_received);

    // buffer to store message to send
    char returnMessage[MAX_TCP_RECV_BUFFER_SIZE];

    //create char* containing message so strlen() can be used to find length
    char *messageString = &message_buffer[0];

    if(result == CY_RSLT_SUCCESS)
    {

        // to many characters, reject
        if(strlen(messageString) > 12){
			snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE , "X illegal length");
			if(security){
				sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
				strcat(secureBuffer, writeBuffer);
			}
			else{
				sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
				strcat(nonSecureBuffer, writeBuffer);
			}
			sendAck(returnMessage, socket_handle, security);
			return result;
		}

        // Check that it is the correct length and has a legal command
        if(!((strlen(messageString) == 7 && message_buffer[0] == 'R') || (strlen(messageString) == 11 && message_buffer[0] == 'W'))){
        	snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE, "X illegal command");
        	if(security){
				sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
				strcat(secureBuffer, writeBuffer);
			}
			else{
				sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
				strcat(nonSecureBuffer, writeBuffer);
			}
        	sendAck(returnMessage, socket_handle, security);
        	return result;
        }

        // All of the bytes must be a ASCII hex digit from 1->end of string
        for(int i = 1; i < strlen(messageString); i++){
        	if(!isxdigit((int)message_buffer[i])){
        		snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE, "X illegal character");
        		if(security){
					sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
					strcat(secureBuffer, writeBuffer);
				}
				else{
					sprintf(writeBuffer,"Message: Length: %d\t", strlen(messageString));
					strcat(nonSecureBuffer, writeBuffer);
				}
        		sendAck(returnMessage, socket_handle, security);
        		return result;
        	}
        }

        // Write command
        if(message_buffer[0] == 'W'){
        	//parse the string
        	sscanf((const char*)message_buffer,"%c%4x%2x%4x", (char *)&commandId, (int*)&receive.deviceId, (int*)&receive.regId, (int*)&receive.value);
        	receive.next = NULL;

        	//See if the device is already in the database, or if there's room to add it
        	if((dbFind(&head, &receive) != NULL) || (dbGetCount(&head) <= dbGetMax())){
        		sprintf(returnMessage,"A%04X%02X%04X",(unsigned int)receive.deviceId,(unsigned int)receive.regId,(unsigned int)receive.value);
        		dbEntry_t *newDB = malloc(sizeof(dbEntry_t)); // make a new entry to put in the database
        		memcpy(newDB,&receive,sizeof(dbEntry_t)); // copy the received data into the new entry
        		dbSetValue(&head, newDB); // save it.
        		if(security){
					sprintf(writeBuffer,"Message: %s\t", messageString);
					strcat(secureBuffer, writeBuffer);
				}
				else{
					sprintf(writeBuffer,"Message: %s\t", messageString);
					strcat(nonSecureBuffer, writeBuffer);
				}
        		sendAck(returnMessage, socket_handle, security);
        		return result;
        	}
        	else{
        		sprintf(returnMessage,"X Database Full %d",(int)dbGetCount(&head));
        		sendAck(returnMessage, socket_handle, security);
        		return result;
        	}
        }
        // read
        if(message_buffer[0] == 'R'){
        	// Parse the string
			sscanf((const char *)message_buffer,"%c%4x%2x",(char *)&commandId,( int *)&receive.deviceId,( int *)&receive.regId);
			dbEntry_t *foundValue = dbFind(&head, &receive); // look through the database to find a previous write of the deviceId/regId
			if(foundValue){
				sprintf(returnMessage,"A%04X%02X%04X",(unsigned int)foundValue->deviceId,(unsigned int)foundValue->regId,(unsigned int)foundValue->value);
				if(security){
					sprintf(writeBuffer,"Message: %s\t", messageString);
					strcat(secureBuffer, writeBuffer);
				}
				else{
					sprintf(writeBuffer,"Message: %s\t", messageString);
					strcat(nonSecureBuffer, writeBuffer);
				}
				sendAck(returnMessage, socket_handle, security);
				return result;
			}
			else{
				sprintf(returnMessage,"X Not Found");
				sendAck(returnMessage, socket_handle, security);
				return result;
			}
		}

    }
    // cy_socket_recv did not return CY_RSLT_SUCCESS
    else
    {
        printf("Failed to receive message from the TCP client. Error: %d\n",
              (int)result);
        if(result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
        {
            /* Disconnect the socket. */
			result = cy_socket_disconnect(socket_handle, 0);

			/* Delete the client socket. */
			result = cy_socket_delete(socket_handle);
        }

    }

    return result;
}

 /*******************************************************************************
 * Function Name: tcp_disconnection_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle TCP client disconnection event.
 *
 * Parameters:
 * cy_socket_t socket_handle: Connection handle for the TCP client socket
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg){

    cy_rslt_t result;

    /* Disconnect the socket. */
	result = cy_socket_disconnect(socket_handle, 0);
	if(result != CY_RSLT_SUCCESS){
		printf("Disconnect Failed!\n");
		CY_ASSERT(0);
	}
	/* Delete the client socket. */
	result = cy_socket_delete(socket_handle);
	if(result != CY_RSLT_SUCCESS){
		printf("Socket Delete Failed!\n");
		CY_ASSERT(0);
	}

    return result;
}

/* [] END OF FILE */

