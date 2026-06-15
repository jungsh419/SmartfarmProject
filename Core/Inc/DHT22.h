/*
 * DHT22.h
 *
 *  Created on: Jun 1, 2026
 *      Author: wjdtj
 */

#ifndef INC_DHT22_H_
#define INC_DHT22_H_

#include "main.h"

#define DHT22_OK        0
#define DHT22_ERROR     1

typedef struct
{
    float temperature;
    float humidity;
} DHT22_Data_t;

void DHT22_Init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint8_t DHT22_ReadData(DHT22_Data_t *data);

#endif /* INC_DHT22_H_ */
