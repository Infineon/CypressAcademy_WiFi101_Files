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
#include "cy_pdl.h"
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
#define SENDRECEIVETIMEOUT					(10000)
#define RESOURCE							"/api/events"
#define ACCESS_KEY							"ACCESSKEY"
#define BUCKET_KEY							"BUCKETKEY"
#define EVENT_KEY							"Virtual LED"
/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t connect_to_wifi_ap(void);
void disconnect_callback(void *arg);

/*******************************************************************************
* Global Variables
********************************************************************************/
bool connected;
uint32_t potPosition = 0;
uint32_t newPotPosition;

/*******************************************************************************
 * Function Name: http_client_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a secure connection to a remote TCP server and
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
	credentials.client_cert = (const char *) &SSL_CLIENTCERT_PEM;
	credentials.client_cert_size = sizeof( SSL_CLIENTCERT_PEM );
	credentials.private_key = (const char *) &SSL_CLIENTKEY_PEM;
	credentials.private_key_size = sizeof( SSL_CLIENTKEY_PEM );
	credentials.root_ca = (const char *) &SSL_ROOTCA_PEM;
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
    	printf("Connected to HTTP Server Successfully\n");
    	connected = true;
    }

	// Var to hold the servers responses
	cy_http_client_response_t response;

	char eventValue[50];
	uint32_t bodyLength;

	// ADC Object
	cyhal_adc_t adc_obj;
	// ADC channel object
	cyhal_adc_channel_t adc_chan_0_obj;
	// Var used for storing conversion result
	uint32_t adc_out;

	// Initialize ADC. The ADC block which can connect to pin 10[6] is selected
	result = cyhal_adc_init(&adc_obj, P10_6, NULL);
	// Initialize ADC channel, allocate channel number 0 to pin 10[6] as this is the first channel initialized
	// pin 10_6 is connected to the potentiometer
	const cyhal_adc_channel_config_t channel_config =
	{
		.enable_averaging = false,
		.min_acquisition_ns = 220,
		.enabled = true
	};
	result = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adc_obj, P10_6, CYHAL_ADC_VNEG, &channel_config);
	if(result != CY_RSLT_SUCCESS){
		cyhal_adc_channel_free(&adc_chan_0_obj);
		cyhal_adc_free(&adc_obj);
		CY_ASSERT(0);
	}

	while(1){
		// Poll the potentiometer once a second
		vTaskDelay(pdMS_TO_TICKS(1000));

		/* Read the ADC conversion result for corresponding ADC channel in millivolts. */
		adc_out = cyhal_adc_read_uv(&adc_chan_0_obj) / 1000;
		// adc_out values range from 0-2399 - pot postition values range from 0 - 360
		newPotPosition = (adc_out * (360 - 0) / (2399 - 0) + 0);

		// If the pot position changed send the new value
		if(newPotPosition != potPosition){
			potPosition = newPotPosition;
			// Create the json representing the position of the potentiometer
			sprintf(eventValue, "{\"key\":\"PotPosition\",\"value\":\"%ld\"}", potPosition);
			printf("Sending potentiometer position: %ld.\n", potPosition);
			bodyLength = strlen(eventValue);

			// Create Request
			uint8_t buffer[BUFFERSIZE];
			cy_http_client_request_header_t request;
			request.buffer = buffer;
			request.buffer_len = BUFFERSIZE;
			request.method = CY_HTTP_CLIENT_METHOD_POST;
			request.range_start = -1;
			request.range_end = -1;
			request.resource_path = RESOURCE;

			// Create Header
			uint32_t num_header = 3;
			cy_http_client_header_t header[num_header];
			header[0].field = "X-IS-AccessKey";
			header[0].field_len = strlen("X-IS-AccessKey");
			header[0].value = ACCESS_KEY;
			header[0].value_len = strlen(ACCESS_KEY);
			header[1].field = "X-IS-BucketKey";
			header[1].field_len = strlen("X-IS-BucketKey");
			header[1].value = BUCKET_KEY;
			header[1].value_len = strlen(BUCKET_KEY);
			header[2].field = "Content-Type";
			header[2].field_len = strlen("Content-Type");
			header[2].value = "application/json";
			header[2].value_len = strlen("application/json");
			// Write Headers
			result = cy_http_client_write_header(clientHandle, &request, header, num_header);
			if(result != CY_RSLT_SUCCESS){
				printf("HTTP Client Header Write Failed!\n");
				CY_ASSERT(0);
			}

			// Send HTTP request
			if(connected){
				result = cy_http_client_send(clientHandle, &request, (uint8_t*)eventValue, bodyLength, &response);
				if(result != CY_RSLT_SUCCESS){
					printf("HTTP Client Send Failed!\n");
					CY_ASSERT(0);
				}
				else{
					// Request Sent
					printf("Potentiometer position sent!\n");
				}
			}
			// Not Connected, Reconnect first
			else{
				// Connect to the HTTP Server
				result = cy_http_client_connect(clientHandle, SENDRECEIVETIMEOUT, SENDRECEIVETIMEOUT);
				if(result != CY_RSLT_SUCCESS){
					printf("HTTP Client Connection Failed!\n");
					CY_ASSERT(0);
				}
				else{
					printf("Connected to HTTP Server Successfully\n");
					connected = true;
				}
				// Send get request to /html resource
				result = cy_http_client_send(clientHandle, &request, (uint8_t*)eventValue, bodyLength, &response);
				if(result != CY_RSLT_SUCCESS){
					printf("HTTP Client Send Failed!\n");
					CY_ASSERT(0);
				}
				else{
					// Request Sent
					printf("Potentiometer position sent!\n");
				}
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

