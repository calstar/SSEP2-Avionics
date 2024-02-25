
#ifndef ADXL375_H
#define ADXL375_H

typedef struct {
	int16_t	x;
	int16_t	y;
	int16_t	z;
} adxl375_readings;

/*
 * Initialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl2_adxl3_init();
/*
 * Initialize ADXL1 (SPI 6) clock
 */
void adxl1_init();

/*
 * Deinitialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl2_adxl3_deinit();

/*
 * Deinitialize ADXL2 and ADXL3 (SPI 1) clock
 */
void adxl1_deinit();

/*
 * Read data from ADXLs
 */
adxl375_readings adxl1_read();

adxl375_readings adxl2_read();

adxl375_readings adxl3_read();


#endif
