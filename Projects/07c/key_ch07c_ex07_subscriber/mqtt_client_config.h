/******************************************************************************
* File Name: mqtt_client_config.h
*
* Description: This file contains all the configuration macros used by the 
*              MQTT client in this example.
*
* Related Document: See README.md
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

#ifndef MQTT_CLIENT_CONFIG_H_
#define MQTT_CLIENT_CONFIG_H_

#include "iot_mqtt.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* MQTT Broker/Server address and port used for the MQTT connection. */
#define MQTT_BROKER_ADDRESS               "amk6m51qrxr2u-ats.iot.us-east-1.amazonaws.com"
#define MQTT_PORT                         8883

/* Set this macro to 'true' if the MQTT Broker being used is hosted by AWS IoT 
 * Core service, else 'false'.
 */
#define AWS_IOT_MQTT_MODE                 ( true )

/* Set this macro to 'true' if a secure connection to the MQTT Broker is  
 * required to be established, else 'false'.
 */
#define MQTT_SECURE_CONNECTION            ( true )

/* The MQTT topic on which the LED control messages will be published and 
 * subscribed.
 */
#define MQTT_TOPIC                        "KEY_TestTopic"

/* Configuration for the 'Will message' that will be published by the MQTT 
 * broker if the MQTT connection is unexpectedly closed. This configuration is 
 * sent to the MQTT broker during MQTT connect operation and the MQTT broker
 * will publish the Will message on the Will topic when it recognizes an 
 * unexpected disconnection from the client.
 */
#define MQTT_WILL_TOPIC_NAME              MQTT_TOPIC "/will"
#define MQTT_WILL_MESSAGE                 ("MQTT client unexpectedly disconnected!")

/* Set the QoS that is associated with the MQTT publish, and subscribe messages.
 * Valid choices are 0, and 1. The MQTT library currently does not support 
 * QoS 2, and hence should not be used in this macro.
 */
#define MQTT_MESSAGES_QOS                 ( 1 )

/* Configure the MQTT username and password that can be used for AWS IoT  
 * Enhanced Custom Authentication.
 */
#define MQTT_USERNAME                     "User"
#define MQTT_PASSWORD                     ""

/* The timeout in milliseconds for MQTT operations in this example. */
#define MQTT_TIMEOUT_MS                   ( 5000 )

/* The keep-alive interval in seconds used for MQTT ping request. */
#define MQTT_KEEP_ALIVE_SECONDS           ( 60 )

/* MQTT client identifier prefix. */
#define MQTT_CLIENT_IDENTIFIER_PREFIX     "KEY_TestThing"

/* The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. Add 1 to include the length of the
 * NULL terminator.
 */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 24 )

/* MQTT messages which are published and subscribed on the MQTT_TOPIC that
 * controls the device (user LED in this example) state.
 */
#define MQTT_DEVICE_ON_MESSAGE            "TURN ON"
#define MQTT_DEVICE_OFF_MESSAGE           "TURN OFF"

/* As per Internet Assigned Numbers Authority (IANA) the port numbers assigned 
 * for MQTT protocol are 1883 for non-secure connections and 8883 for secure
 * connections. In some cases there is a need to use other ports for MQTT like
 * port 443 (which is reserved for HTTPS). Application Layer Protocol 
 * Negotiation (ALPN) is an extension to TLS that allows many protocols to be 
 * used over a secure connection. The ALPN ProtocolNameList specifies the 
 * protocols that the client would like to use to communicate over TLS.
 * 
 * This macro specifies the ALPN Protocol Name to be used that is supported
 * by the MQTT broker in use.
 * Note: For AWS IoT, currently "x-amzn-mqtt-ca" is the only supported ALPN 
 *       ProtocolName and it is only supported on port 443.
 */
#define MQTT_ALPN_PROTOCOL_NAME           "x-amzn-mqtt-ca"

/* Configure the below credentials in case of a secure MQTT connection. */
/* PEM-encoded client certificate */
#define CLIENT_CERTIFICATE      \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIUITA8HBoCDrCv2IndSiGkWyOsGQswDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMDEwNDE2MTEw\n"\
"NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALfGbgdtkOzE48H8/1Gk\n"\
"T78y5XSDoFQ3q7q2QO+RlVrpHG+EriYICnjVFYCbww/vzElWhRLABLfgJQbL30AG\n"\
"zRAjnqmuhLRMSa0iUMl3V3zjVwqQX+JT/S1osjbn2cp9YQsV+GFLXq0VJkB/jpl1\n"\
"xB7fzTG4YSscbUvFvONrf0Ay3Zm+eE948lYFkowp/0w5Ho13yuy4CYwb5eG8pdCp\n"\
"b9t+Bs2BNzjelBRuwzm2DgsKk8gkGwWSWFaT+NAxGCmgwYwqCKGLs/8yChiJbGgE\n"\
"VIlihBDpqPd2nzHMgpehSOCkwt4hzi8Oan4J88oHSW3/GAG3VTrVft2UGn6pvOPZ\n"\
"rZECAwEAAaNgMF4wHwYDVR0jBBgwFoAUnltXlpC2ttrbz7oxAjNPzfoC3vgwHQYD\n"\
"VR0OBBYEFPQQ6poRs+dBm23DMh7zoYOaVvOQMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCSkxjBjN+MVlfAm+ReQtC6i3ob\n"\
"Ed6IZRGX782+5pxmy8o9m94Uqb+EiqGdcT1XAUrtkKjsnxb2NzZMEwypUj9wZ/NI\n"\
"IFinU1qhSp9Of3C9BThh+Kwrj8Ca/fc9wWLMjczSPoOL8CFYdnOC8sjhIeUr60Rk\n"\
"pinDE05I3ob3UnBs8v+uKfLPHyxOuToJdrTGRi9N84j3x/iUNywJcQGIjXwV0PZT\n"\
"QsvdQoByqlMs/7MsG3BZQtqvIQEfXTgHuDysZoMDY2qGIyNlcIQzRh5R1drNs2fW\n"\
"o8dWcSVsYuYhleLFc+4Sl8iaFXOzso0bZtWbXzyueUrEMC3t3TS72VqC3hmN\n"\
"-----END CERTIFICATE-----"

/* PEM-encoded client private key */
#define CLIENT_PRIVATE_KEY          \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEpQIBAAKCAQEAt8ZuB22Q7MTjwfz/UaRPvzLldIOgVDerurZA75GVWukcb4Su\n"\
"JggKeNUVgJvDD+/MSVaFEsAEt+AlBsvfQAbNECOeqa6EtExJrSJQyXdXfONXCpBf\n"\
"4lP9LWiyNufZyn1hCxX4YUterRUmQH+OmXXEHt/NMbhhKxxtS8W842t/QDLdmb54\n"\
"T3jyVgWSjCn/TDkejXfK7LgJjBvl4byl0Klv234GzYE3ON6UFG7DObYOCwqTyCQb\n"\
"BZJYVpP40DEYKaDBjCoIoYuz/zIKGIlsaARUiWKEEOmo93afMcyCl6FI4KTC3iHO\n"\
"Lw5qfgnzygdJbf8YAbdVOtV+3ZQafqm849mtkQIDAQABAoIBAQCcqLfE9z6yx4iM\n"\
"JpqTwykECWQWtdon0KlA6fpi/sy+CbfmzMkTvIkIxkSlNKLM5BNWdT3NZQjgol+N\n"\
"8p8mWmXWmza22QFhHSM1RuptQ6G94aKia76BFi3d1sENEwnSlpzyMVFhxzk2/K92\n"\
"DgbGhOChnRCzkMSsVnhJPpAtocaBNyloF+XqmQGbVqiE3UVSUU5ssC6iB9j7kqVO\n"\
"mdfYcHWhVnNJBkgpM8SSEYqSoBpwuVedDHo/AJShDqn7lrYNq/IDolgOUv7D4Hsv\n"\
"awGswtgNVB/zELLjhzCLTbBpOZwu9qWVqWgIrsLKESJrSyqeYh2pP5ydSstakiSC\n"\
"C5dItVONAoGBANokPJsFXcnECwRLj7WEjFUyE9tfFZ8U3dW80xWojG4taYLTKa77\n"\
"oKiw9gdp+Njo8ts9njCGb5I1Ghfa64yW1ECFRsjMU9FrQmfST6Pe9T3CBMRW9Cw8\n"\
"qx5LgYbNh5guwPrACRSUkqVdCcCoDH0oULaR4PqT9OXz7Hdq3207CvjHAoGBANer\n"\
"VZS2afkqJZyaDOVCe4vUj+8C9lrHUxnbiP0lSzWuVhzxRC9QCx3O+N2qMpxT3w3w\n"\
"mlQveR/takRBcLP7q2z9icNHK8YLXkmJzzGasaIzC44DFJsdKuqD2G+tL5UnDkCt\n"\
"QKuggpmgDzkQmv72uqpj4qmu9WqX0SqIvoFwtT7nAoGAcAgQQCVfZ5IRfmE4Kycp\n"\
"009mt1F8DzluPKk/V246wRL9jzzXuZBtkHVhxaj9LplksIhSx+UjQ9mVzw6Vzpxf\n"\
"4csQ8WtO2MzXS+7NDAIjvVX5kMR/noCB+YsZuJuxucy5O46Im1M6hUblDVbBIz4j\n"\
"RcBh5Q87xSWjGil/GeAq4F0CgYEAo5NtDqUoTXZ9HoDtJPm+0PjAzoyVo5ao3eTX\n"\
"ywD9QVLgN4V6rNPuCobP1Phg+JbYiC8kLpPvL5sXzXtA6xIIHFtUoMsbVVOMCQTA\n"\
"JKRq9FX4/pYxi9Ccm1Cy2Ukm3N7JfZ6qfUH5rGFtCmAg3x7tx4yI/yAXPei2Gaer\n"\
"rmt43KMCgYEAuLb9H7+HaYIbYR/t/6mUf15dI3GGCGLpJxh5PuBLGI9ZQMRw/OLO\n"\
"H8Qi2mgXiibFAFi4Zb164z9TmCLrjisa7cK/u41bIYG++wkEtEKgXQ1YHQKklASe\n"\
"5/Rs4l6eLu3U/0SwcyidkRgx6CEDWJ6yKLT6khoD+HEaaw6LRVelpOY=\n"\
"-----END RSA PRIVATE KEY-----"

/* PEM-encoded Root CA certificate */
#define ROOT_CA_CERTIFICATE     \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----"

/******************************************************************************
* Global Variables
*******************************************************************************/
extern IotMqttNetworkInfo_t networkInfo;
extern IotMqttConnectInfo_t connectionInfo;

#endif /* MQTT_CLIENT_CONFIG_H_ */
