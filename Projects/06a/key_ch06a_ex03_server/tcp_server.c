/******************************************************************************
* File Name:   tcp_server.c
*
* Description: This file contains declaration of task and functions related to
* TCP server operation.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2021, Cypress Semiconductor Corporation (an Infineon company) or
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

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file */
#include <FreeRTOS.h>
#include <task.h>

/* Cypress secure socket header file */
#include "cy_secure_sockets.h"

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* Standard C header file */
#include <string.h>

/* TCP server task header file. */
#include "tcp_server.h"

/* lwIP related header files. */
#include "cy_lwip.h"

/* Standard C header files */
#include <inttypes.h>

/* Linked list */
#include "linkedList.h"

/* mDNS */
#include "mdns.h"
#include "cy_lwip.h"

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
#define WIFI_SSID                             "ssid"
#define WIFI_PASSWORD                         "psswd"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY_TYPE                    CY_WCM_SECURITY_WPA2_AES_PSK
/* Maximum number of connection retries to a Wi-Fi network. */
#define MAX_WIFI_CONN_RETRIES                 (10u)

/* Wi-Fi re-connection time interval in milliseconds */
#define WIFI_CONN_RETRY_INTERVAL_MSEC         (1000u)

/* TCP server related macros. */
#define TCP_SERVER_PORT                           (27708)
#define TCP_SERVER_MAX_PENDING_CONNECTIONS        (5u)
#define TCP_SERVER_RECV_TIMEOUT_MS                (2000u)
#define MAX_TCP_RECV_BUFFER_SIZE                  (20u)
#define MAX_TCP_DATA_PACKET_LENGTH				  (20u)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
static cy_rslt_t create_tcp_server_socket(void);
static cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg);
static cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg);
static cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg);
static cy_rslt_t connect_to_wifi_ap(void);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Secure socket variables. */
cy_socket_sockaddr_t tcp_server_addr, peer_addr;
cy_socket_t server_handle, client_handle;

/* Size of the peer socket address. */
uint32_t peer_addr_len;

/*******************************************************************************
 * Function Name: tcp_server_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a TCP client.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void tcp_server_task(void *arg)
{
    cy_rslt_t result;

	/* Connect to Wi-Fi AP */
	result = connect_to_wifi_ap();
	if(result != CY_RSLT_SUCCESS )
	{
		printf("\n Failed to connect to Wi-Fi AP! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
		CY_ASSERT(0);
	}

    /* Initialize secure socket library. */
    result = cy_socket_init();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Secure Socket initialization failed! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
        CY_ASSERT(0);
    }
    printf("Secure Socket initialized\n");

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

    /* Create TCP server socket. */
    result = create_tcp_server_socket();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Failed to create socket! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
        CY_ASSERT(0);
    }

    /* Start listening on the TCP server socket. */
    result = cy_socket_listen(server_handle, TCP_SERVER_MAX_PENDING_CONNECTIONS);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_socket_delete(server_handle);
        printf("cy_socket_listen returned error. Error code: 0x%08"PRIx32"\n", (uint32_t)result);
        CY_ASSERT(0);
    }
    else
    {
        printf("===============================================================\n");
        printf("Listening for incoming TCP client connection on Port: %d\n\n",
                tcp_server_addr.port);
    }

    while(true)
    {
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
static cy_rslt_t connect_to_wifi_ap(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Variables used by Wi-Fi connection manager.*/
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_config_t wifi_config = {
			.interface = CY_WCM_INTERFACE_TYPE_STA
	};

	/* Initialize Wi-Fi connection manager. */
	result = cy_wcm_init(&wifi_config);

	if (result != CY_RSLT_SUCCESS)
	{
		printf("Wi-Fi Connection Manager initialization failed!\n");
		return result;
	}
	printf("Wi-Fi Connection Manager initialized.\n");

    cy_wcm_ip_address_t ip_address;

    /* Variable to track the number of connection retries to the Wi-Fi AP specified
     * by WIFI_SSID macro.
     */
     int conn_retries = 0;

     /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY_TYPE;

    printf("Connecting to Wi-Fi Network: %s\n", WIFI_SSID);

    /* Join the Wi-Fi AP. */
    for(conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ )
    {
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                                wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %s\n", ip4addr_ntoa((const ip4_addr_t *)&ip_address.ip.v4));

            /* IP address and TCP port number of the TCP server */
            tcp_server_addr.ip_address.ip.v4 = ip_address.ip.v4;
            tcp_server_addr.ip_address.version = CY_SOCKET_IP_VER_V4;
            tcp_server_addr.port = TCP_SERVER_PORT;
            return result;
        }

        printf("Connection to Wi-Fi network failed with error code 0x%08"PRIx32"\n."
               "Retrying in %d ms...\n", (uint32_t)result, WIFI_CONN_RETRY_INTERVAL_MSEC);
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MSEC));
    }

    /* Stop retrying after maximum retry attempts. */
    printf("Exceeded maximum Wi-Fi connection attempts\n");

    return result;
}

/*******************************************************************************
 * Function Name: create_tcp_server_socket
 *******************************************************************************
 * Summary:
 *  Function to create a socket and set the socket options
 *
 *******************************************************************************/
static cy_rslt_t create_tcp_server_socket(void)
{
    cy_rslt_t result;
    /* TCP socket receive timeout period. */
    uint32_t tcp_recv_timeout = TCP_SERVER_RECV_TIMEOUT_MS;

    /* Variables used to set socket options. */
    cy_socket_opt_callback_t tcp_receive_option;
    cy_socket_opt_callback_t tcp_connection_option;
    cy_socket_opt_callback_t tcp_disconnection_option;

    /* Create a TCP socket */
    result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                              CY_SOCKET_IPPROTO_TCP, &server_handle);
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Failed to create socket! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
        return result;
    }

    /* Set the TCP socket receive timeout period. */
    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                 CY_SOCKET_SO_RCVTIMEO, &tcp_recv_timeout,
                                 sizeof(tcp_recv_timeout));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_RCVTIMEO failed\n");
        return result;
    }

    /* Register the callback function to handle connection request from a TCP client. */
    tcp_connection_option.callback = tcp_connection_handler;
    tcp_connection_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK,
                                  &tcp_connection_option, sizeof(cy_socket_opt_callback_t));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_CONNECT_REQUEST_CALLBACK failed\n");
        return result;
    }

    /* Register the callback function to handle messages received from a TCP client. */
    tcp_receive_option.callback = tcp_receive_msg_handler;
    tcp_receive_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_RECEIVE_CALLBACK,
                                  &tcp_receive_option, sizeof(cy_socket_opt_callback_t));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_RECEIVE_CALLBACK failed\n");
        return result;
    }

    /* Register the callback function to handle disconnection. */
    tcp_disconnection_option.callback = tcp_disconnection_handler;
    tcp_disconnection_option.arg = NULL;

    result = cy_socket_setsockopt(server_handle, CY_SOCKET_SOL_SOCKET,
                                  CY_SOCKET_SO_DISCONNECT_CALLBACK,
                                  &tcp_disconnection_option, sizeof(cy_socket_opt_callback_t));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Set socket option: CY_SOCKET_SO_DISCONNECT_CALLBACK failed\n");
        return result;
    }

    /* Bind the TCP socket created to Server IP address and to TCP port. */
    result = cy_socket_bind(server_handle, &tcp_server_addr, sizeof(tcp_server_addr));
    if(result != CY_RSLT_SUCCESS)
    {
        printf("Failed to bind to socket! Error code: 0x%08"PRIx32"\n", (uint32_t)result);
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
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
static cy_rslt_t tcp_connection_handler(cy_socket_t socket_handle, void *arg)
{
	cy_rslt_t result;

	/* Accept new incoming connection from a TCP client.*/
	result = cy_socket_accept(socket_handle, &peer_addr, &peer_addr_len,
							  &client_handle);
	if(result == CY_RSLT_SUCCESS)
	{
		printf("Incoming TCP connection accepted\n");
	}
	else
	{
		printf("Failed to accept incoming client connection. Error: %d\n", (int)result);
		printf("===============================================================\n");
		printf("Listening for incoming TCP client connection on Port: %d\n\n",
				tcp_server_addr.port);
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
void sendAck(char *message, cy_socket_t socket_handle){

	cy_rslt_t result;
	uint32_t bytes_sent;

	/* Send the command to TCP server. */
	result = cy_socket_send(socket_handle, message, MAX_TCP_DATA_PACKET_LENGTH, CY_SOCKET_FLAGS_NONE, &bytes_sent);
	if(result == CY_RSLT_SUCCESS ){
		printf("ack sent: %s\n", message);
	}
	else{
		printf("Failed to send ack to client. Error: %d\n", (int)result);
		if(result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED){
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
		}
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
 *  void *args : Parameter passed on to the function (unused)
 *
 * Return:
 *  cy_result result: Result of the operation
 *
 *******************************************************************************/
static cy_rslt_t tcp_receive_msg_handler(cy_socket_t socket_handle, void *arg)
{
	char message_buffer[MAX_TCP_RECV_BUFFER_SIZE];
	    cy_rslt_t result;

	    // linked list to store connection information in
		static dbEntry_t head = {
			.next = NULL,
			.deviceId = 0,
			.regId = 0
		};
	    dbEntry_t receive;
	    char commandId;

	    /* Variable to store number of bytes received from TCP client. */
	    uint32_t bytes_received = 0;
	    result = cy_socket_recv(socket_handle, message_buffer, MAX_TCP_RECV_BUFFER_SIZE,
	                            CY_SOCKET_FLAGS_NONE, &bytes_received);
	    char returnMessage[MAX_TCP_RECV_BUFFER_SIZE];

	    //create char* containing message so strlen() can be used to find length
	    char *messageString = &message_buffer[0];

	    if(result == CY_RSLT_SUCCESS){
	        printf("Message from TCP Client: %s\n", messageString);

	        // to many characters, reject
	        if(strlen(messageString) > 12){
	        	printf("illegal length\n");
				snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE , "X illegal length");
				sendAck(returnMessage, socket_handle);
				return result;
			}

	        // Check that it is the correct length and has a legal command
	        if(!((strlen(messageString) == 7 && message_buffer[0] == 'R') || (strlen(messageString) == 11 && message_buffer[0] == 'W'))){
	        	printf("illegal command\n");
	        	snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE, "X illegal command");
	        	sendAck(returnMessage, socket_handle);
	        	return result;
	        }

	        // All of the bytes must be a ASCII hex digit from 1->end of string
	        for(int i = 1; i < strlen(messageString); i++){
	        	if(!isxdigit((int)message_buffer[i])){
	        		printf("illegal character\n");
	        		snprintf(returnMessage, MAX_TCP_RECV_BUFFER_SIZE, "X illegal character");
	        		sendAck(returnMessage, socket_handle);
	        		return result;
	        	}
	        }

	        // Write command
	        if(message_buffer[0] == 'W'){
	        	printf("write");
	        	//parse the string
	        	sscanf((const char*)message_buffer,"%c%4x%2x%4x", (char *)&commandId, (int*)&receive.deviceId, (int*)&receive.regId, (int*)&receive.value);
				receive.next = NULL;
	        	//See if the device is already in the database, or if theres room to add it
	        	if((dbFind(&head, &receive) != NULL) || (dbGetCount(&head) <= dbGetMax())){
	        		sprintf(returnMessage,"A%04X%02X%04X",(unsigned int)receive.deviceId,(unsigned int)receive.regId,(unsigned int)receive.value);
	        		dbEntry_t *newDB = malloc(sizeof(dbEntry_t)); // make a new entry to put in the database
	        		memcpy(newDB,&receive,sizeof(dbEntry_t)); // copy the received data into the new entry
	        		dbSetValue(&head, newDB); // save it.
	        		sendAck(returnMessage, socket_handle);
	        		return result;
	        	}
	        	else{
	        		sprintf(returnMessage,"X Database Full %d",(int)dbGetCount(&head));
	        		sendAck(returnMessage, socket_handle);
	        		return result;
	        	}
	        }
	        if(message_buffer[0] == 'R'){
	        	//read
				sscanf((const char *)message_buffer,"%c%4x%2x",(char *)&commandId,( int *)&receive.deviceId,( int *)&receive.regId);
				dbEntry_t *foundValue = dbFind(&head, &receive); // look through the database to find a previous write of the deviceId/regId
				if(foundValue){
					sprintf(returnMessage,"A%04X%02X%04X",(unsigned int)foundValue->deviceId,(unsigned int)foundValue->regId,(unsigned int)foundValue->value);
					sendAck(returnMessage, socket_handle);
					return result;
				}
				else{
					sprintf(returnMessage,"X Not Found");
					sendAck(returnMessage, socket_handle);
					return result;
				}
			}
	    }
	    else{
	        printf("Failed to receive message from the TCP client. Error: %d\n",
	              (int)result);
	        if(result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED){
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
static cy_rslt_t tcp_disconnection_handler(cy_socket_t socket_handle, void *arg)
{
    cy_rslt_t result;

    /* Disconnect the TCP client. */
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

    printf("TCP Client disconnected! Please reconnect the TCP Client\n");
    printf("===============================================================\n");
    printf("Listening for incoming TCP client connection on Port: %d\n\n",
            tcp_server_addr.port);

    return result;
}

/* [] END OF FILE */
