/*
 * can_data.h
 *
 *  Created on: May 30, 2026
 *      Author: wjdtj
 */

#ifndef CAN_DATA_H
#define CAN_DATA_H

#include <stdint.h>

/* CAN ID */
#define CAN_ID_ENV      0x101
#define CAN_ID_EVENT    0x102

/* env */
typedef struct
{
    uint8_t  temp;      // DHT22 temp
    uint8_t  humi;      // DHT22 humi
    uint16_t co2;       // MH-Z19 CO2(ppm)
    uint16_t soil;      // soil
} ENV_DATA_t;

/*event*/
typedef struct
{
    uint8_t pir;        // pir
    uint8_t flame;      // flmae
} EVENT_DATA_t;

#endif
