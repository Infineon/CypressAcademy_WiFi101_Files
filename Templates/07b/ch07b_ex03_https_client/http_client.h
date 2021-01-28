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

#define SERVERHOSTNAME						"mysecurehttpserver.local"
#define SERVERPORT							(50007)

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

/* SSL client private key. */
#define SSL_CLIENTKEY_PEM          \
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

#define SSL_ROOTCA_PEM            \
"-----BEGIN CERTIFICATE-----\n"\
"MIID5DCCAsygAwIBAgIUMtOU0QhhuaJaCSavNGLH2c7Kf5YwDQYJKoZIhvcNAQEL\n"\
"BQAwezELMAkGA1UEBhMCSU4xEjAQBgNVBAgMCUthcm5hdGFrYTESMBAGA1UEBwwJ\n"\
"QmVuZ2FsdXJ1MQswCQYDVQQKDAJDWTEUMBIGA1UECwwLRW5naW5lZXJpbmcxITAf\n"\
"BgNVBAMMGG15c2VjdXJlaHR0cHNlcnZlci5sb2NhbDAeFw0yMDA2MjgwOTE1MzRa\n"\
"Fw0zMDA2MjYwOTE1MzRaMHsxCzAJBgNVBAYTAklOMRIwEAYDVQQIDAlLYXJuYXRh\n"\
"a2ExEjAQBgNVBAcMCUJlbmdhbHVydTELMAkGA1UECgwCQ1kxFDASBgNVBAsMC0Vu\n"\
"Z2luZWVyaW5nMSEwHwYDVQQDDBhteXNlY3VyZWh0dHBzZXJ2ZXIubG9jYWwwggEi\n"\
"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDB2yIgT/9TAY1kpwC+NmNjJ0Pj\n"\
"bnTyrig34jiWX2sW/Qn9plyprmrhqoX8v/CD5zatpbd+fj8quVsfjHueP/BIgMZj\n"\
"s2Rt2Q7cTzMzgfhfY5n/Tg4B8L/cH5tQYwS1/SwouCpp1tioJLuvvVj2XBC76XUd\n"\
"ybyqQteIloALA5HFfjChrh1BaDM3bKIqhqW15lfRmDa50QAvq2+Tr6/zv0I3MP6x\n"\
"y2kaNRwebMSQipuOrFboEMc7fYl7V5rA6Rz3Ph3m3zXfFN+s7CWa9rkdsbfpXIfF\n"\
"lSoHVi/f/brlX7xNpDx8PYz1sX4nMHecniMEfBCoAWh19ypXBvF1YOsAGGtxAgMB\n"\
"AAGjYDBeMB8GA1UdIwQYMBaAFM6gm80hMJNIu/e4szg9d9YDnHivMAkGA1UdEwQC\n"\
"MAAwCwYDVR0PBAQDAgTwMCMGA1UdEQQcMBqCGG15c2VjdXJlaHR0cHNlcnZlci5s\n"\
"b2NhbDANBgkqhkiG9w0BAQsFAAOCAQEAM7u6dA/3Y9tHkgYY6f04VOBjIgUXRcto\n"\
"pWLh0MiQG41m+c9F0AJxaljY1BW6W21pQEIB2hV3I6/p70x2VC8cPks7RbE/8kjX\n"\
"k9P1ZgKvgaHpA1opjKDfpEs9E0Q57ycWNofFlkhytl02nyR30Uma65FXcdISSmcz\n"\
"SH5eGZSwZIF6yKpJomfTX711UeGMu8uF3EwsTappzyOCtgdBy5emEgC22Zq5Moeq\n"\
"zlDP/zz/xLtjpOHKMI0wERLQQ08GVtYIjCkA5u4FaF9fabFVEAJxDaKpIqxrBu3N\n"\
"PtB9swI5SFuVLX4g2tgfM5+QIv8aYV4QddHiOT19+AW56bY1C0ZzFA==\n"\
"-----END CERTIFICATE-----"

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void http_client_task(void *arg);

#endif /* HTTP_CLIENT_H_ */
