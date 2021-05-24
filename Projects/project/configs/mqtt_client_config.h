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

#include "cy_mqtt_api.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* MQTT Broker/Server address and port used for the MQTT connection. */
#define MQTT_BROKER_ADDRESS               "amk6m51qrxr2u-ats.iot.us-east-1.amazonaws.com"
#define MQTT_PORT                         8883

/* Set this macro to 1 if a secure (TLS) connection to the MQTT Broker is
 * required to be established, else 0.
 */
#define MQTT_SECURE_CONNECTION            ( 1 )

/* Configure the user credentials to be sent as part of MQTT CONNECT packet */
#define MQTT_USERNAME                     "User"
#define MQTT_PASSWORD                     ""

/* The MQTT topic on which the LED control messages will be published and 
 * subscribed.
 */
#define UPDATE_TOPIC                        "$aws/things/KEY_Thermostat/shadow/update"
#define UPDATE_DOCUMENTS_TOPIC				"$aws/things/KEY_Thermostat/shadow/update/documents"

/* Configuration for the 'Last Will and Testament (LWT)'. It is an MQTT message
 * that will be published by the MQTT broker if the MQTT connection is
 * unexpectedly closed. This configuration is sent to the MQTT broker during
 * MQTT connect operation and the MQTT broker will publish the Will message on
 * the Will topic when it recognizes an unexpected disconnection from the client.
 *
 * If you want to use the last will message, set this macro to 1 and configure
 * the topic and will message, else 0.
 */
#define MQTT_WILL_TOPIC_NAME               "$aws/things/KEY_Thermostat/will"
#define MQTT_WILL_MESSAGE                 ("MQTT client unexpectedly disconnected!")

/* Set the QoS that is associated with the MQTT publish, and subscribe messages.
 * Valid choices are 0, and 1. The MQTT library currently does not support 
 * QoS 2, and hence should not be used in this macro.
 */
#define MQTT_MESSAGES_QOS                 ( 1 )

/* The timeout in milliseconds for MQTT operations in this example. */
#define MQTT_TIMEOUT_MS                   ( 5000 )

/* The keep-alive interval in seconds used for MQTT ping request. */
#define MQTT_KEEP_ALIVE_SECONDS           ( 60 )

/* A unique client identifier to be used for every MQTT connection. */
#define MQTT_CLIENT_IDENTIFIER            "KEY_Thermostat"

/* The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. However some MQTT brokers support
 * longer client IDs. Configure this macro as per the MQTT broker specification.
 */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 23 )

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

/* A Network buffer is allocated for sending and receiving MQTT packets over
 * the network. Specify the size of this buffer using this macro.
 *
 * Note: The minimum buffer size is defined by 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'
 * macro in the MQTT library. Please ensure this macro value is larger than
 * 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'.
 */
#define MQTT_NETWORK_BUFFER_SIZE          ( 10 * CY_MQTT_MIN_NETWORK_BUFFER_SIZE )

/* Maximum MQTT connection re-connection limit. */
#define MAX_MQTT_CONN_RETRIES            (150u)

/* MQTT re-connection time interval in milliseconds. */
#define MQTT_CONN_RETRY_INTERVAL_MS      (2000)

/* Configure the below credentials in case of a secure MQTT connection. */
/* PEM-encoded client certificate */
#define CLIENT_CERTIFICATE     \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIURtrVRcdcuy8sf64FqL0R3sQBUwIwDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIwMDgwNzE4NTAz\n"\
"MloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALvwBDPHxIS+ssLO5uCv\n"\
"XsGO1vKIoJQyjoVditRy2K5m8Y+MkA8bbi78VBqwsXg1/g6vilh2mfU0ZZFNcPtt\n"\
"kVlCJP4iKesTckBZtgJD7KWxDm5grtEr8J/XlOKX2N016YALyYfu/BwGyddHknwm\n"\
"JavSFaqB2p4/F5YTawHJ+Rn9aPXHrJLDftCviWCcja/6F+4vHxNFwhyGs+ndZrh8\n"\
"Arh5E1eYAKWAe3DS4YMTssrHQbKPFhQLzsXCUjXnFxClij6kmheSKGXkKxPmOpec\n"\
"Kkz3Mi2dkLVgCmT2W2ePsVPYzoXhvK3+pvvNlGwdWbd4pMBjzKQxP0Vr3IlhlCON\n"\
"5u8CAwEAAaNgMF4wHwYDVR0jBBgwFoAU6eZ32EtaXBSzqhS5PF49omfN0h0wHQYD\n"\
"VR0OBBYEFEc7TeJNia/nrHGXlYeR8iADj3N/MAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCCUscML88+tLkbYG/OZ5fDSqd9\n"\
"ufaOCBlTGpTguFF12LmyMU+uiUNr/kmdB5WBN/nHJFNtCgj+mrPNZo5Wud3XuKVI\n"\
"GVBjcT1yEiN3TXhLJZK7dvFBkZYU4PVRaiBOZppFsLwPH8dM7eJyI+dKUC8RMdm2\n"\
"xrjeSiIU6ZptZ+5sJ/lgTDAbOe/6dpkC4uQLzqwUNbKPU3Gcm6V16bkVGFe4r1dW\n"\
"lidLovMxrwKy8dMpD5EBJLt/1Flc36p9kK7bMP0Fft54cOBVU6qiYeSsJMIpElCX\n"\
"KDUMuzdxLCCSpqytZ96mH5BaE1vCP6uDdzfL8w+Y/h8PTZPVZQVt7L7TwDnX\n"\
"-----END CERTIFICATE-----"

/* PEM-encoded client private key */
#define CLIENT_PRIVATE_KEY         \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEpAIBAAKCAQEAu/AEM8fEhL6yws7m4K9ewY7W8oiglDKOhV2K1HLYrmbxj4yQ\n"\
"DxtuLvxUGrCxeDX+Dq+KWHaZ9TRlkU1w+22RWUIk/iIp6xNyQFm2AkPspbEObmCu\n"\
"0Svwn9eU4pfY3TXpgAvJh+78HAbJ10eSfCYlq9IVqoHanj8XlhNrAcn5Gf1o9ces\n"\
"ksN+0K+JYJyNr/oX7i8fE0XCHIaz6d1muHwCuHkTV5gApYB7cNLhgxOyysdBso8W\n"\
"FAvOxcJSNecXEKWKPqSaF5IoZeQrE+Y6l5wqTPcyLZ2QtWAKZPZbZ4+xU9jOheG8\n"\
"rf6m+82UbB1Zt3ikwGPMpDE/RWvciWGUI43m7wIDAQABAoIBAAmhLLXygHFEzCgL\n"\
"Le1JTxOf3AelVIUDaiYPOvPTxqJ5B9uRGjiL8UNbDnwnnZBJQ/FAsVgDC7RysNmX\n"\
"HDjlzlLCft6+pWM0JeCCOD85d8Ctp82kCpERLU0jYK+TlDsRbhtvfy4F4skU1/a9\n"\
"5hWSZs+8/fs0mB0PoofACvWWgbsBGG+4W/stMZMrdL0fD5EIAm/E4AUghhoEE4HL\n"\
"mHBSSDrCgKGy4kFGEQAC++FRWgNbpfOhPj9BoYqwUmrk9tAB0xpiGcVf75CC295f\n"\
"cLhPH3NTri8FJbKPjD1fdLMs+YN55h39MoJVwbq7DzDHo+ORkQsFUN+75G0kxqKL\n"\
"Sw3qn/ECgYEA8eezQ41qsEgYrOgTyHjgnNLGGveO5ARqVuwZo10b5zykJixErIrV\n"\
"HOJd2ye1oZyksGMoOglNjXEglNOxwSv8asZNz+TI+0TlOF3NAuPqcUkxGCJ46BPY\n"\
"k0wPHNfT1GnYI45bLopklNVJKdj9nd+haX9PMx5eQXO/Mc+Muk0qI+MCgYEAxuNT\n"\
"tXHRM78/gHyDiK1MJF4Xw8PZYo0LBJFF9MOccbQJumF9agxQBJebTbDPZ5PcBp2u\n"\
"YM6GsdSQ4jBlsP9La7CpavGM4Nvk2LJ96wcbtXk+SWiozAbT8etWZAV3gMlrH8TO\n"\
"BsekIQ420HAlBO/5CJa+APiTtBiid9opMsnpVoUCgYEA28XqiqPvWuqVaHmMh6Sy\n"\
"RCgGDbf93NDaH9Bu1omf8SyZ4Yh0d+HGDnDmXlYsYQCkaxAbGucayzcNJ8lQhDTH\n"\
"Jea2CPtNUWNdcSFfQZH5jCS38rpW6FoneGMwk77N1/y1flA2LQlUqhdAWkGm4Qis\n"\
"Z+Fe/g1ICuTyvnlW3GeAsosCgYBS3FL7vTeTf5JNTZLfVpPqcukjvmzLCkgU1xF2\n"\
"MCj1ljSXWhaOICMWRrOFYqyGUqvQddE4s9KaNB1InzE+1J3vd+Kdk5VF+VcWkQa0\n"\
"7Hk22hSBMQFiP1X97DZJ5Z0sIr5mte/AE1zmCK39mTtTG0d9tI/kh6vB1KGbnWCc\n"\
"sFJuoQKBgQCQol0MfAhTpFp1/4gO5y7ekga6fRpNe9SeIhA457U2BFaDM1CWAcWQ\n"\
"BOsMd2cPBYdV08vx33c5wJQv8T9vGfxris1yBr9V7+UbOVT3/yh9GDrKbLH4pTgt\n"\
"fiI7JKf9OQZ3hkjSzxt7sAULCTtaT1sN35OjVX1wNHAV8yN38Q/ldg==\n"\
"-----END RSA PRIVATE KEY-----"

/* PEM-encoded Root CA certificate */
#define ROOT_CA_CERTIFICATE    \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"\
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"\
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"\
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"\
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"\
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"\
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"\
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"\
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"\
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"\
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"\
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"\
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"\
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"\
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"\
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"\
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"\
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"\
"-----END CERTIFICATE-----"

/******************************************************************************
* Global Variables
*******************************************************************************/
extern cy_mqtt_broker_info_t broker_info;
extern cy_awsport_ssl_credentials_t  *security_info;
extern cy_mqtt_connect_info_t connection_info;

#endif /* MQTT_CLIENT_CONFIG_H_ */
