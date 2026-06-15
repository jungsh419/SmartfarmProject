/*
 * DHT22.c
 *
 *  Created on: Jun 1, 2026
 *      Author: wjdtj
 */

#include "DHT22.h"

/* DHT22 핀 저장용 */
static GPIO_TypeDef *DHT22_PORT;
static uint16_t DHT22_PIN;

/* 내부 함수 */
static void DHT22_TIM3_Init(void);
static void delay_us(uint16_t us);

static void DHT22_SetPinOutput(void);
static void DHT22_SetPinInput(void);

static void DHT22_Start(void);
static uint8_t DHT22_CheckResponse(void);
static uint8_t DHT22_ReadByte(void);

void DHT22_Init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    DHT22_PORT = GPIOx;
    DHT22_PIN = GPIO_Pin;

    DHT22_TIM3_Init();

    DHT22_SetPinOutput();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
}

/*
 * TIM3를 1MHz로 설정
 * TIM3->CNT 1 증가 = 1us
 */
static void DHT22_TIM3_Init(void)
{
    RCC_ClkInitTypeDef clk_config;
    uint32_t flash_latency;
    uint32_t pclk1;
    uint32_t tim_clk;

    __HAL_RCC_TIM3_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clk_config, &flash_latency);

    pclk1 = HAL_RCC_GetPCLK1Freq();

    if (clk_config.APB1CLKDivider == RCC_HCLK_DIV1)
    {
        tim_clk = pclk1;
    }
    else
    {
        tim_clk = pclk1 * 2;
    }

    TIM3->CR1 = 0;
    TIM3->CNT = 0;
    TIM3->PSC = (tim_clk / 1000000) - 1;
    TIM3->ARR = 0xFFFF;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void delay_us(uint16_t us)
{
    TIM3->CNT = 0;
    while (TIM3->CNT < us);
}

static void DHT22_SetPinOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void DHT22_SetPinInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void DHT22_Start(void)
{
    DHT22_SetPinOutput();

    /*
     * DHT22 시작 신호
     * MCU가 DATA 라인을 LOW로 1ms 이상 유지
     */
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    HAL_Delay(2);

    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30);

    DHT22_SetPinInput();
}

static uint8_t DHT22_CheckResponse(void)
{
    uint16_t timeout = 0;

    /*
     * 응답 대기
     * DHT22 응답 순서:
     * LOW 약 80us -> HIGH 약 80us
     */

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET)
    {
        delay_us(1);
        timeout++;

        if (timeout > 100)
        {
            return 0;
        }
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET)
    {
        delay_us(1);
        timeout++;

        if (timeout > 100)
        {
            return 0;
        }
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET)
    {
        delay_us(1);
        timeout++;

        if (timeout > 100)
        {
            return 0;
        }
    }

    return 1;
}

static uint8_t DHT22_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0;
    uint16_t timeout;

    for (i = 0; i < 8; i++)
    {
        /*
         * 각 비트 시작 LOW 약 50us 대기
         */
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET)
        {
            delay_us(1);
            timeout++;

            if (timeout > 100)
            {
                return 0;
            }
        }

        /*
         * HIGH 길이로 0/1 판단
         * 0: 약 26~28us HIGH
         * 1: 약 70us HIGH
         *
         * 40us 뒤에도 HIGH면 1
         */
        delay_us(40);

        if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET)
        {
            data |= (1 << (7 - i));
        }

        /*
         * HIGH가 끝날 때까지 대기
         */
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET)
        {
            delay_us(1);
            timeout++;

            if (timeout > 100)
            {
                break;
            }
        }
    }

    return data;
}

uint8_t DHT22_ReadData(DHT22_Data_t *data)
{
    uint8_t hum_high;
    uint8_t hum_low;
    uint8_t temp_high;
    uint8_t temp_low;
    uint8_t checksum;

    uint16_t raw_humidity;
    uint16_t raw_temperature;

    DHT22_Start();

    if (!DHT22_CheckResponse())
    {
        return DHT22_ERROR;
    }

    hum_high  = DHT22_ReadByte();
    hum_low   = DHT22_ReadByte();
    temp_high = DHT22_ReadByte();
    temp_low  = DHT22_ReadByte();
    checksum  = DHT22_ReadByte();

    /*
     * 체크섬 확인
     */
    if (((hum_high + hum_low + temp_high + temp_low) & 0xFF) != checksum)
    {
        return DHT22_ERROR;
    }

    raw_humidity = ((uint16_t)hum_high << 8) | hum_low;
    raw_temperature = (((uint16_t)(temp_high & 0x7F)) << 8) | temp_low;

    data->humidity = raw_humidity / 10.0f;
    data->temperature = raw_temperature / 10.0f;

    /*
     * 온도 음수 처리
     */
    if (temp_high & 0x80)
    {
        data->temperature *= -1.0f;
    }

    return DHT22_OK;
}
