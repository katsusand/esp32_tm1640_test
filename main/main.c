#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "esp_check.h"
#include "esp_timer.h"

#define TM1640_CLOCK_PIN 16
#define TM1640_DATA_PIN 17

#define COMMAND_DATA 0x40
#define COMMAND_DISPLAY 0x80
#define COMMAND_ADDRESS 0xc0

#define DATA_ADDRESS_AUTOINCREMENT 0x00
#define DATA_ADDRESS_FIXED 0x04
#define DATA_NORMALMODE 0x00
#define DATA_TESTMODE 0x08
#define DISPLAY_ON 0x08
#define DISPLAY_OFF 0x00


#define DATA_LOW gpio_set_level(TM1640_DATA_PIN, 0)
#define DATA_HIGH gpio_set_level(TM1640_DATA_PIN, 1)
#define SET_DATA_PIN(level) gpio_set_level(TM1640_DATA_PIN, level)

#define CLOCK_LOW gpio_set_level(TM1640_CLOCK_PIN, 0)
#define CLOCK_HIGH gpio_set_level(TM1640_CLOCK_PIN, 1)
#define DELAY(usec) delay_microseconds(usec)

/*
 *     00
 *   5    1
 *   5    1
 *     66
 *   4    2
 *   4    2
 *     33   7
 *
 *		0		1	2		3	4		5	6		7	8		9
 *     0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x67
 */


void delay_microseconds(uint64_t usec) {
	int64_t start = esp_timer_get_time();
	uint32_t counter = 0;
	while(esp_timer_get_time() - start < usec) {++counter;}
}

esp_err_t send_start(void) {
	esp_err_t ret = 0;
	ret = DATA_LOW;
	DELAY(1);
	if(ret) return ret;
	ret = CLOCK_LOW;
	DELAY(1);
	return ret;
}

esp_err_t send_byte(uint8_t send_data) {
	esp_err_t ret = 0;
	for(int i = 0; i < 8; ++i) {
		bool current_bit = ( (send_data & (1 << i) ) >> i);
		ret = SET_DATA_PIN(current_bit);
		DELAY(1);
		if(ret) return ret;

		ret = CLOCK_HIGH;
		DELAY(2);
		if(ret) return ret;

		ret = CLOCK_LOW;
		DELAY(1);
	}
	return ret;
}

esp_err_t send_end(void) {
	esp_err_t ret = 0;
	ret = DATA_LOW;
	DELAY(1);
	if(ret) return ret;

	ret = CLOCK_HIGH;
	DELAY(1);
	if(ret) return ret;

	ret = DATA_HIGH;
	DELAY(2);
	return ret;
}

esp_err_t write_sram(uint8_t* write_buf, size_t buf_size, uint8_t offset) {
	if(offset + (uint8_t)buf_size > 16) return ESP_ERR_INVALID_SIZE;
	esp_err_t ret = 0;

	ret = send_start();
	if(ret) return ret;

	uint8_t command = COMMAND_DATA | DATA_NORMALMODE | DATA_ADDRESS_AUTOINCREMENT; // data command: normal mode, address auto increment
	ret = send_byte(command);
	if(ret) return ret;

	ret = send_end();
	if(ret) return ret;

	ret = send_start();
	if(ret) return ret;

	command = COMMAND_ADDRESS | offset; // address command: display address 0x0
	ret = send_byte(command);
	if(ret) return ret;

	for(int i = 0; i < buf_size; ++i) {
		ret = send_byte(write_buf[i]);
		if(ret) return ret;
	}
	return send_end();
}


esp_err_t display_brightness(uint8_t value) {
	value = value > 8 ? 8 : value;
	esp_err_t ret = 0;

	ret = send_start();
	if(ret) return ret;

	if(value) {
		ret = send_byte(COMMAND_DISPLAY | DISPLAY_ON | (value - 1) );
	}
	else {
		ret = send_byte(COMMAND_DISPLAY | DISPLAY_OFF);
	}
	if(ret) return ret;

	return send_end();
}

esp_err_t tm1640_init(uint8_t clock_pin, uint8_t data_pin) {
	esp_err_t ret = 0;

	ret = gpio_set_direction(clock_pin, GPIO_MODE_OUTPUT);
	if(ret) return ret;

	ret = gpio_pullup_en(clock_pin);
	if(ret) return ret;

	ret = CLOCK_HIGH;
	if(ret) return ret;

	ret = gpio_set_direction(TM1640_DATA_PIN, GPIO_MODE_OUTPUT);
	if(ret) return ret;

	ret = gpio_pullup_en(TM1640_DATA_PIN);
	if(ret) return ret;

	return DATA_HIGH;
}

void app_main(void)
{

	ESP_ERROR_CHECK(tm1640_init(TM1640_CLOCK_PIN, TM1640_DATA_PIN) );
	uint8_t full[] = {
			0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0xff
	};
	uint8_t segments[] = {
			0x6d, 0x66, 0x4f, 0x5b, 0x06, 0x3f, 0xe2
	};
	uint8_t segments2[] = {
			0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x67, 0x1d
	};
	sleep(2);
	ESP_ERROR_CHECK(write_sram(full, sizeof(full) / sizeof(*full), 0) );

	for(int i = 0; i < 9; ++i) {
		delay_microseconds(200 * 1000);
		ESP_ERROR_CHECK(display_brightness((uint8_t)i) );
	}

    while (true) {
    	ESP_ERROR_CHECK(write_sram(segments, sizeof(segments) / sizeof(*segments), 0) );
//        printf("Hello from app_main!\n");
        sleep(1);
        ESP_ERROR_CHECK(write_sram(segments2, sizeof(segments2) / sizeof(*segments), 0) );
        sleep(1);
    }
}
