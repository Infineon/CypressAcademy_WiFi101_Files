/*
 * weather.c
 *
 * Created on: May 14, 2020
 *		Author: yfs
 *
 * Implements the weather thread
 */
 
#include "main.h"

/*
Defines for the weather thread
*/
#define JSON_START											('{')
#define BUFFERSIZE											(2048)
#define CURRENTRESOURCE										"/current"
#define SERVERHOSTNAME										"api.weatherstack.com"
#define SENDRECEIVETIMEOUT									(5000)


static void wifi_init( void );
static void display_update( const char* http_data, int num_bytes );
void disconnect_callback(void *arg);

// Global var to represent the connection to the HTTP server
bool connected;

/* wifi_init()
 *
 * Turn on the Wi-Fi and connect to the access point
 */
void wifi_init( void )
{
	cy_rslt_t result;

	/* Variables used by Wi-Fi connection manager */
	cy_wcm_config_t wifi_config;
	cy_wcm_connect_params_t wifi_conn_param;
	cy_wcm_ip_address_t ip_address;

	/*
	Initialize the Wi-Fi connection manager (WCM)
	*/
	wifi_config.interface = CY_WCM_INTERFACE_TYPE_STA;
	cy_wcm_init( &wifi_config );

	/*
	Connect to the Access Point (AP)
		Note: AP_NAME and SSID are #define strings in private_data.h
		e.g. #define AP_NAME "MY_SSID"
	*/
	 memset( &wifi_conn_param, 0, sizeof( cy_wcm_connect_params_t ) );
	 memcpy( wifi_conn_param.ap_credentials.SSID, AP_NAME, strlen( AP_NAME ) );
	 memcpy( wifi_conn_param.ap_credentials.password, PASSWORD, strlen( PASSWORD ) );
	 wifi_conn_param.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;

	printf( "Connecting to access point\n" );
	for( ; ; )
	{
		result = cy_wcm_connect_ap( &wifi_conn_param, &ip_address );
		if( CY_RSLT_SUCCESS == result )
			{ break; }
		printf( "Retrying...\n" );
	}
	
	PRINT_IPV4_ADDRESS( "Local IP", ip_address.ip.v4 );
}

/* weather_task()
 *
 * Turn on Wi-Fi
 * Connect to access point
 * Send HTTP requet to weatherstack.com
 * Receive and process returned JSON
 */  
void weather_task( void* arg )
{
	cy_rslt_t result;
	
	char* city;

	wifi_init();
	cy_http_client_init();

	// HTTP Client Handle
	cy_http_client_t clientHandle;

	// Set up server info
	cy_awsport_server_info_t serverInfo;
	(void) memset(&serverInfo, 0, sizeof(serverInfo));
	serverInfo.host_name = SERVERHOSTNAME;
	serverInfo.port = HTTP_PORT_NUMBER;

	// Disconnection Handler
	cy_http_disconnect_callback_t disconnectCallback = (void*)disconnect_callback;

	// Create HTTP Client
	result = cy_http_client_create(NULL, &serverInfo, disconnectCallback, NULL, &clientHandle);
	if(result != CY_RSLT_SUCCESS){
		printf("Failed to create HTTP Client!\n");
		CY_ASSERT(0);
	}

/*
	Task loop waits for a city, sends the request, receives the response and displays the data
	*/
	for( ; ; )
	{
		/*
		Sleep until user requests data from the server by sending a pointer to the city name
		*/
		printf( "Press CapSense button to send/receive\n" );
		city = (char*) ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		/* Create the HTTP message */
		// Set up request
		uint8_t buffer[BUFFERSIZE];
		cy_http_client_request_header_t request;
		request.buffer = buffer;
		request.buffer_len = BUFFERSIZE;
		request.method = CY_HTTP_CLIENT_METHOD_GET;
		request.range_start = -1;
		request.range_end = -1;
		char buff[300];
		sprintf(buff, "%s?access_key=%s&query=%s", CURRENTRESOURCE, ACCESS_KEY, city);
		request.resource_path = buff;

		// Create Headers
		uint32_t num_header = 1;
		cy_http_client_header_t header[num_header];
		header[0].field = "Host";
		header[0].field_len = strlen("Host");
		header[0].value = SERVERHOSTNAME;
		header[0].value_len = strlen(SERVERHOSTNAME);
		// Write Header
		result = cy_http_client_write_header(clientHandle, &request, header, num_header);
		if(result != CY_RSLT_SUCCESS){
			printf("HTTP Client Header Write Failed!\n");
			CY_ASSERT(0);
		}

		// Variable to store server's response
		cy_http_client_response_t response;
		
		// Send the HTTP message
		if(connected){
			result = cy_http_client_send(clientHandle, &request, NULL, 0, &response);
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
				printf("Connected to HTTP Server Successfully\n");
				connected = true;
			}
			// Send get request to /html resource
			result = cy_http_client_send(clientHandle, &request, NULL, 0, &response);
			if(result != CY_RSLT_SUCCESS){
				printf("HTTP Client Send Failed!\n");
				CY_ASSERT(0);
			}
		}

		display_update((char*)response.body, response.body_len);
	}
}

/* display_update()
 *
 * Extract information from JSON and display on UART and TFT (if supported)
 */
static void display_update( const char* http_data, int num_bytes )
{
	/*
	Extract the current conditions from the JSON returned from weatherstack. Data format:

	weather_data ->	request
					location
					current ->	weather_descriptions[]
								wind_dir
								precip
	*/
	CY_ASSERT( http_data );

	cJSON* weather_data;									// Root of the JSON data */
	cJSON* current;											// Pointer to the current weather */
	cJSON* item;											// Pointer to the actual data */

	/* New font style and color in this thread */
	#ifdef TFT_SUPPORTED
	GUI_SetColor( GUI_DARKGREEN );							// Low contrast for laptop camera
	GUI_SetFont( &GUI_Font32_ASCII );						// Big, clear font
	#endif
	
	/*
	Scan the HTTP message for JSON data
	*/
	while( JSON_START != *http_data )
	{
		CY_ASSERT( num_bytes > 0 );
		num_bytes--;
		http_data++;
	}

	/* Parse the JSON so we know it is good */
	weather_data = cJSON_Parse( http_data );
	CY_ASSERT( NULL != weather_data );

	/* Extract pointer to the current weather */
	current  = cJSON_GetObjectItemCaseSensitive( weather_data, "current" );

	// If current is null then the http message must not have contained weather info
	if(current != NULL){
		/*
		Extract the current weather from array of conditions: "weather_descriptions" : [ "cloudy" ]
		*/
		item = cJSON_GetObjectItemCaseSensitive( current, "weather_descriptions" );
		item = cJSON_GetArrayItem( item, 0 );					// Use the zeroth element of the array
		CY_ASSERT( cJSON_IsString( item ) );

		printf( "Conditions: %s\n", item->valuestring );
		#ifdef TFT_SUPPORTED
		GUI_DispStringAt( item->valuestring, TFT_LEFT_ALIGNED, TFT_ROW_WEATHER );
		#endif

		/*
		Extract the wind direction (wind_dir)
		*/
		item = cJSON_GetObjectItemCaseSensitive( current, "wind_dir" );
		CY_ASSERT( cJSON_IsString( item ) );

		printf( "Wind:\t\t%s\n", item->valuestring );
		#ifdef TFT_SUPPORTED
		GUI_DispStringAt( "Wind:  ", TFT_LEFT_ALIGNED, TFT_ROW_WIND );
		GUI_DispString( item->valuestring );
		#endif

		/*
		Extract the rain (precip)
		*/
		item = cJSON_GetObjectItemCaseSensitive( current, "precip" );
		CY_ASSERT( cJSON_IsNumber( item ) );

		printf( "Rain:\t\t%d %%\n", item->valueint );
		#ifdef TFT_SUPPORTED
		GUI_DispStringAt( "Rain:   ", TFT_LEFT_ALIGNED, TFT_ROW_RAIN );
		GUI_DispDecMin( item->valueint );
		GUI_DispString( " %" );
		#endif

		/*
		Extract the temperature
		*/
		item = cJSON_GetObjectItemCaseSensitive( current, "temperature" );
		CY_ASSERT( cJSON_IsNumber( item ) );
	
		printf( "Temp:\t\t%d C\n", item->valueint );
		#ifdef TFT_SUPPORTED
		GUI_DispStringAt( "Temp: ", TFT_LEFT_ALIGNED, TFT_ROW_TEMP );
		GUI_DispDecMin( item->valueint );
		GUI_DispString( " C" );
		#endif
	}
	else{
		printf("No weather data received in response from server!\n");
	}

	cJSON_Delete( weather_data );
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
