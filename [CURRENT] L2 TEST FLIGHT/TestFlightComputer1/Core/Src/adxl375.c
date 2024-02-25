#include <main.h>
#include "adxl375.h"

static uint8_t adxl1_data_buffer[6];
static uint8_t adxl2_data_buffer[6];
static uint8_t adxl3_data_buffer[6];

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi6;

static GPIO_TypeDef *adxl2_gpio_x = GPIOA;
static uint16_t adxl2_cs_pin = 15;
static GPIO_TypeDef *adxl3_gpio_x = GPIOC;
static uint16_t adxl3_cs_pin = 13;

/*
 * Initialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl2_adxl3_init() {
	__HAL_RCC_SPI1_CLK_ENABLE();
}

/*
 * Initialize ADXL1 (SPI 2) clock
 */
void adxl1_init() {
	__HAL_RCC_SPI6_CLK_ENABLE();
}

/*
 * Deinitialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl2_adxl3_deinit() {
	__HAL_RCC_SPI1_CLK_DISABLE();
}

/*
 * Deinitialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl1_deinit() {
	__HAL_RCC_SPI6_CLK_DISABLE();
}

uint8_t _read_register(SPI_HandleTypeDef *hspi, uint8_t addr)
{
    HAL_StatusTypeDef hal_status;
    uint8_t rx_data[2];
    uint8_t tx_data[2];

    tx_data[0] = addr | 0x80;  // read operation
    tx_data[1] = 0;            // dummy byte for response

    hal_status = HAL_SPI_TransmitReceive_DMA(hspi, tx_data, rx_data, 2);

    return rx_data[1];
}

void _adxl_read(SPI_HandleTypeDef *hspi, uint8_t *readings) {
	for (int i = 0; i < 6; i++) {
		readings[i] = _read_register(hspi, 0x32 + i);
	}
}

adxl375_readings _formatted_adxl_readings(uint8_t buffer[6]) {
	return (adxl375_readings) {
		.x = ((uint16_t) buffer[1] << 8) | buffer[0],
		.y = ((uint16_t) buffer[3] << 8) | buffer[2],
		.z = ((uint16_t) buffer[5] << 8) | buffer[4]
	};
}


adxl375_readings adxl1_read() {
	_adxl_read(&hspi6, adxl1_data_buffer);
	return _formatted_adxl_readings(adxl1_data_buffer);
}

adxl375_readings adxl2_read() {
    HAL_GPIO_WritePin(adxl3_gpio_x, adxl3_cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(adxl2_gpio_x, adxl2_cs_pin, GPIO_PIN_RESET);

	_adxl_read(&hspi1, adxl2_data_buffer);
	return _formatted_adxl_readings(adxl2_data_buffer);
}

adxl375_readings adxl3_read() {
    HAL_GPIO_WritePin(adxl2_gpio_x, adxl2_cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(adxl3_gpio_x, adxl3_cs_pin, GPIO_PIN_RESET);

	_adxl_read(&hspi1, adxl3_data_buffer);
	return _formatted_adxl_readings(adxl3_data_buffer);
}
