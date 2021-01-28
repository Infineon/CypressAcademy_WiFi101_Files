/******************************************************************************
* File Name:   http_client.h
*
* Description: This file contains declaration of task related to HTTP client
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

#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

/*******************************************************************************
* Macros
********************************************************************************/
/* Wi-Fi Credentials: Modify WIFI_SSID, WIFI_PASSWORD and WIFI_SECURITY_TYPE
 * to match your Wi-Fi network credentials.
 * Note: Maximum length of the Wi-Fi SSID and password is set to
 * CY_WCM_MAX_SSID_LEN and CY_WCM_MAX_PASSPHRASE_LEN as defined in cy_wcm.h file.
 */

#define WIFI_SSID							"ssid"
#define WIFI_PASSWORD						"pswd"

#define SERVERHOSTNAME						"groker.initialstate.com"
#define SERVERPORT							(443)

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY_TYPE                 CY_WCM_SECURITY_WPA2_AES_PSK

/* Maximum number of connection retries to the Wi-Fi network. */
#define MAX_WIFI_CONN_RETRIES             (10u)

/* Wi-Fi re-connection time interval in milliseconds */
#define WIFI_CONN_RETRY_INTERVAL_MSEC     (1000)

#define MAKE_IPV4_ADDRESS(a, b, c, d)     ((((uint32_t) d) << 24) | \
                                          (((uint32_t) c) << 16) | \
                                          (((uint32_t) b) << 8) |\
                                          ((uint32_t) a))

#define SSL_CLIENTCERT_PEM      \
"-----BEGIN CERTIFICATE-----\n"\
"MIIEHTCCAwWgAwIBAgIUXbJ1O2vQBqwoLQfeDsclV92K3OEwDQYJKoZIhvcNAQEL\n"\
"BQAwgZ0xCzAJBgNVBAYTAklOMRIwEAYDVQQIDAlLYXJuYXRha2ExEjAQBgNVBAcM\n"\
"CUJlbmdhbHVydTEPMA0GA1UECgwGTXlfb3JnMRUwEwYDVQQLDAxNeV91bml0X25h\n"\
"bWUxHTAbBgNVBAMMFG15X3RjcF9zZWN1cmVfY2xpZW50MR8wHQYJKoZIhvcNAQkB\n"\
"FhBteV9lbWFpbF9hZGRyZXNzMB4XDTIwMDQyMjA0MDQzMVoXDTIxMDQyMjA0MDQz\n"\
"MVowgZ0xCzAJBgNVBAYTAklOMRIwEAYDVQQIDAlLYXJuYXRha2ExEjAQBgNVBAcM\n"\
"CUJlbmdhbHVydTEPMA0GA1UECgwGTXlfb3JnMRUwEwYDVQQLDAxNeV91bml0X25h\n"\
"bWUxHTAbBgNVBAMMFG15X3RjcF9zZWN1cmVfY2xpZW50MR8wHQYJKoZIhvcNAQkB\n"\
"FhBteV9lbWFpbF9hZGRyZXNzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"\
"AQEA7IHHX47X3e6k2EQhAVRtoWAu1ZDM3QcoIJgBTogChgyZeBpFfqOPtbprXqta\n"\
"O0WunvKnjxeDYl+o8M2ypeXNTjSdkBskQMQSXhvXwUdo/inERO5uAkZGRRxJdhyf\n"\
"aiHqBTxlDSS1EhbTrEnIhO4nNBLa47mfwy+b3gLgL79QUTQOZghDykOv77J5rSHE\n"\
"TWlGoOoYi7OBHBenft3GjZHNpRj8NxfYb8JhoS95xMvhAdvE43XJGpaI7nbrlxHM\n"\
"ubkzHEbhpm8QPq+Rs9yxSUju0TUq2NGGsYLA8Fe+XlB4s1rD5eazcgIlIm7xR+OK\n"\
"d7kwCOTWItO9tmA0vO+AmAraCwIDAQABo1MwUTAdBgNVHQ4EFgQUSpHfzaRQqDDV\n"\
"ujoKaO6CyIEY9OcwHwYDVR0jBBgwFoAUSpHfzaRQqDDVujoKaO6CyIEY9OcwDwYD\n"\
"VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAwGuni6FtWoUDdCYDUyHc\n"\
"YVIWIZxXt1eHDfaHRngHZ5O3xFUnAh7iiFCieUPr9zZvvdWfbLB/k6YMhmvPviEF\n"\
"ODBnIbOL2wYv5q1a4U0koQAy/7zCsCAgS71klFupdeLiyHf13s8HkmxOPRjHuout\n"\
"DElcCJY0f9su9MtsCRiF87j9xqBi9JThw7lFK6AuTa9sOhB23gHafTgxOQ5SXeRk\n"\
"GULhFsbp9p7IQ7+z+igMFOqUkbWCf914PmqG37mYq/Z039l6rXGcjoV6PH4dySH6\n"\
"CPH38iG5aeYCepi7+Q7JG/tu7DJdXwGtXuWYKto8d2/JqZVMAB80XM1kFXDUn45X\n"\
"Nw==\n"\
"-----END CERTIFICATE-----\n"

/* SSL client private key. */
#define SSL_CLIENTKEY_PEM          \
"-----BEGIN PRIVATE KEY-----\n"\
"MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQDsgcdfjtfd7qTY\n"\
"RCEBVG2hYC7VkMzdByggmAFOiAKGDJl4GkV+o4+1umteq1o7Ra6e8qePF4NiX6jw\n"\
"zbKl5c1ONJ2QGyRAxBJeG9fBR2j+KcRE7m4CRkZFHEl2HJ9qIeoFPGUNJLUSFtOs\n"\
"SciE7ic0EtrjuZ/DL5veAuAvv1BRNA5mCEPKQ6/vsnmtIcRNaUag6hiLs4EcF6d+\n"\
"3caNkc2lGPw3F9hvwmGhL3nEy+EB28TjdckalojuduuXEcy5uTMcRuGmbxA+r5Gz\n"\
"3LFJSO7RNSrY0YaxgsDwV75eUHizWsPl5rNyAiUibvFH44p3uTAI5NYi0722YDS8\n"\
"74CYCtoLAgMBAAECggEBAOAXw+J+RXnK0jT8sM1CwzHiId5H+mT/j/z6KlrPspxz\n"\
"OcM8GVb6AYQuo+eqsq9wwhlnUG7b16iRfVDVho18pcCRSC4wGXSok8LJ3PANCqWw\n"\
"y4CI7oGHNrPrLowUaNQ2WDgn1pPSkSMXr+8SZkXWJh1INFEMJB3ccXK2BNgZDTAy\n"\
"MZdT9nmmEHdsEPrCL3JGQmeCYs8Cbqq9mXNHczqp2B01bf0IucwtAX23v5kswOr7\n"\
"VN65tEIsfdIjZpeu/cFvaxLRBSHt/udDVrPhxxXVaM8b+Jo1QM37kdZGsrWjpX4J\n"\
"8n3IK4om/Oh+ZT9uknyHZyP1cfSOPJUvkXFjCU59ThECgYEA99O/rCGX3ajjtgs2\n"\
"z2BXrFm6KQxdpQArTS8V6uzweJUVQforqpKdkZujyusutqYzdUzJk5jPIL16PssM\n"\
"dLIDVCXzAA/1QzJhHSSmX9F+dNdgJ0LPyS/H1VQmMwtzuMXC4rakagQuVlqHREKe\n"\
"7O64oIrT/WwfjQVi+FG8C8EruTMCgYEA9E517vKqSY2itFnODn0AbvRub8tu4p0h\n"\
"03VWfBxivXI1RqfEyu42TD6UoLGEKFGF68GBAz2lYzEzUihStedX4/UzqdacOrY5\n"\
"17CsVvPbpgPrw3uyc1lJPJx76Vu/xpIk88Qlu4UotTxBYEZ5xKZZ73K8Co9Q4Gop\n"\
"oSd8UOm5y8kCgYEA48cYNRIPLejLmgcGkmWfT0aM5qt56myUX/V19J1fi8SibCiO\n"\
"N9FXx3vAeIHeRnvNcfNoliLtxM7B67LeOOdgTdgvVxBTwTYtsgZ8cDbxPlyyAIPU\n"\
"zLoAtl8IWQQekWYacBukU9iR6kZ8as28mlHzGYNfwl29RfT8ePVOm5MpJ6cCgYEA\n"\
"iX6u+PAxznucGoyAX78SQ1/l151Sps9wfiYHqon/OEfSnBVHuD4g6QA/fcDqdBaN\n"\
"nIWnKj/DmPPTwmVXR3EQEgeqL+fMvW5ZV37jP3y3HdQPGFHBuRGDGk2nn0o3wHX3\n"\
"vgmigssKghWLZbfESFynORAFrJvEFMkq7LvgR1mDa6ECgYBBZqlzMQabQ523XIBY\n"\
"drf5rm0E3WX2m1lS+ha/MqNOfIuykfxa9Q8PsQiyJdfjX9+6ffrE/1AKRrLCSxF3\n"\
"/041afDur+5JKt5ibpOVmCar2igqluLOmsysVtMF5wLhEoSp/U6ep9iGLNTyZ78D\n"\
"l7JLwL2fT19Z91NIRJpckqs+KQ==\n"\
"-----END PRIVATE KEY-----\n"

#define SSL_ROOTCA_PEM            \
"-----BEGIN CERTIFICATE-----\n"\
"MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n"\
"MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n"\
"U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n"\
"NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n"\
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n"\
"ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n"\
"DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n"\
"8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n"\
"+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n"\
"X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n"\
"K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n"\
"1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n"\
"A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n"\
"zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n"\
"YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n"\
"bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n"\
"DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n"\
"L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n"\
"eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n"\
"xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n"\
"VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n"\
"WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n"\
"-----END CERTIFICATE-----\n"

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void http_client_task(void *arg);

#endif /* HTTP_CLIENT_H_ */
