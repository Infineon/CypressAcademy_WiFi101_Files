/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the Empty PSoC6 Application
*              for ModusToolbox.
*
* Related Document: See README.md
*
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

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy_json_parser.h"
#include "stdlib.h" //atof

float temperatureValue;
char  temperatureString[10];

/* This function is called during the parsing of the JSON text.  It is called when a
   complete item is parsed. */
cy_rslt_t jsonCallback(cy_JSON_object_t *obj_p, void *arg)
{
    /* This conditional ensures that the path is state: reported: temperature and the value is a number */
    if( (obj_p->parent_object != NULL) &&
        (obj_p->parent_object->parent_object != NULL) &&
        (strncmp(obj_p->parent_object->parent_object->object_string, "state", strlen("state")) == 0) &&
        (strncmp(obj_p->parent_object->object_string, "reported", strlen("reported")) == 0) &&
        (strncmp(obj_p->object_string, "temperature", strlen("temperature")) == 0) &&
        (obj_p->value_type == JSON_NUMBER_TYPE)
      )
    {
        snprintf(temperatureString, obj_p->value_length+1, "%s", obj_p->value);
        temperatureValue = atof(temperatureString);
    }
    return CY_RSLT_SUCCESS;
}


int main(void){
    cy_rslt_t result;
    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port. */
	cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

	/* Enable interrupts */
    __enable_irq();

    // Hard Coded json string
    const char *jsonString = "{\"state\" : {\"reported\" : {\"temperature\":25.4} } }";

    //Register Callback
    cy_JSON_parser_register_callback(jsonCallback, NULL);
    //Parse and print
    cy_JSON_parser(jsonString, strlen(jsonString));
    printf("Temperature: %.1f\n", temperatureValue);


    for(;;){
    }
}

/* [] END OF FILE */
