/******************************************************************************
* File Name:   http_client.c
*
* Description: This file contains task and functions related to HTTP client
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

/* Wi-Fi connection manager header files. */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* TCP client task header file. */
#include "http_client.h"

/* HTTP Client Library*/
#include "cy_http_client_api.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define BUFFERSIZE							(2048 * 2)
#define SENDRECEIVETIMEOUT					(5000)
#define ANYTHINGRESOURCE					"/anything"
#define REQUEST_BODY						"{\"AnyCloud\":\"yes\"}"
#define REQUEST_BODY_LENGTH					( sizeof( REQUEST_BODY ) - 1 )

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t connect_to_wifi_ap(void);
void disconnect_callback(void *arg);

/*******************************************************************************
* Global Variables
********************************************************************************/
bool connected;

/*******************************************************************************
 * Function Name: http_client_task
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
void http_client_task(void *arg){
    cy_rslt_t result;

	result = connect_to_wifi_ap();
	CY_ASSERT(result == CY_RSLT_SUCCESS);

    result = cy_http_client_init();
    if(result != CY_RSLT_SUCCESS){
    	printf("HTTP Client Library Initialization Failed!\n");
    	CY_ASSERT(0);
    }

    // Initialize serverInfo and credentials structs
	cy_awsport_server_info_t serverInfo;
	cy_awsport_ssl_credentials_t credentials;
	(void) memset(&credentials, 0, sizeof(credentials));
	(void) memset(&serverInfo, 0, sizeof(serverInfo));

    // Server Info
    serverInfo.host_name = SERVERHOSTNAME;
    serverInfo.port = SERVERPORT;

    // Set the credentials information
    // Since the server (website) does not validate the client, the certificate and private key of the client are not required
	credentials.root_ca = SSL_ROOTCA_PEM;
	credentials.root_ca_size = sizeof( SSL_ROOTCA_PEM );

    // Disconnection Callback
    cy_http_disconnect_callback_t disconnectCallback = (void*)disconnect_callback;

    // Client Handle
    cy_http_client_t clientHandle;

    // Create the HTTP Client
    result = cy_http_client_create(&credentials, &serverInfo, disconnectCallback, NULL, &clientHandle);
    if(result != CY_RSLT_SUCCESS){
		printf("HTTP Client Creation Failed!\n");
		CY_ASSERT(0);
	}

    // Connect to the HTTP Server
    result = cy_http_client_connect(clientHandle, SENDRECEIVETIMEOUT, SENDRECEIVETIMEOUT);
    if(result != CY_RSLT_SUCCESS){
		printf("HTTP Client Connection Failed!\n");
		CY_ASSERT(0);
	}
    else{
    	printf("\nConnected to HTTP Server Successfully\n\n");
    	connected = true;
    }

    // Create Request
    uint8_t buffer[BUFFERSIZE];
    cy_http_client_request_header_t request;
    request.buffer = buffer;
    request.buffer_len = BUFFERSIZE;
    request.method = CY_HTTP_CLIENT_METHOD_POST;
    request.range_start = -1;
    request.range_end = -1;
    request.resource_path = ANYTHINGRESOURCE;

    // Create Headers
    uint32_t num_header = 2;
    cy_http_client_header_t header[num_header];
    header[0].field = "Host";
    header[0].field_len = strlen("Host");
	header[0].value = SERVERHOSTNAME;
	header[0].value_len = strlen(SERVERHOSTNAME);
	header[1].field = "Content-Type";
	header[1].field_len = strlen("Content-Type");
	header[1].value = "application/json";
	header[1].value_len = strlen("application/json");
	// Content length header is automatically added!

	result = cy_http_client_write_header(clientHandle, &request, header, num_header);
	if(result != CY_RSLT_SUCCESS){
		printf("HTTP Client Header Write Failed!\n");
		CY_ASSERT(0);
	}

	// Var to hold the servers responses
	cy_http_client_response_t response;

	// Send post request to /html resource
	if(connected){
		result = cy_http_client_send(clientHandle, &request, (uint8_t *)REQUEST_BODY, REQUEST_BODY_LENGTH, &response);
		if(result != CY_RSLT_SUCCESS){
			printf("HTTP Client Send Failed!\n");
			CY_ASSERT(0);
		}
	}
	else{
		// Connect to the HTTP Server
		result = cy_http_client_connect(clientHandle, SENDRECEIVETIMEOUT, SENDRECEIVETIMEOUT);
		if(result != CY_RSLT_SUCCESS){
			printf("HTTP Client Connection Failed!\n");
			CY_ASSERT(0);
		}
		else{
			printf("\nConnected to HTTP Server Successfully\n\n");
			connected = true;
		}
		// Send post request to /html resource
		result = cy_http_client_send(clientHandle, &request, (uint8_t *)REQUEST_BODY, REQUEST_BODY_LENGTH, &response);
		if(result != CY_RSLT_SUCCESS){
			printf("HTTP Client Send Failed!\n");
			CY_ASSERT(0);
		}
	}

	// Print response message
	printf("Response received from httpbin.org/anything:\n");
	for(int i = 0; i < response.body_len; i++){
		printf("%c", response.body[i]);
	}
	printf("\n");

	while(1){
		vTaskDelay(1);
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
        printf("Wi-Fi Connection Manager initialization failed!\n");
        return result;
    }
    printf("Wi-Fi Connection Manager initialized.\r\n");

     /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY_TYPE;

    /* Join the Wi-Fi AP. */
    for(uint32_t conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ )
    {
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                                wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %d.%d.%d.%d\n", (uint8)ip_address.ip.v4,
                    (uint8)(ip_address.ip.v4 >> 8), (uint8)(ip_address.ip.v4 >> 16),
                    (uint8)(ip_address.ip.v4 >> 24));
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
 * Function Name: disconnect_callback
 *******************************************************************************
 * Summary:
 *  Invoked when the server disconnects
 *
 * Parameters:
 *  void *arg : unused
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void disconnect_callback(void *arg){
    printf("Disconnected from HTTP Server\n");
    connected = false;
}

