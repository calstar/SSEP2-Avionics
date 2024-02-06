# SSEP2-Avionics


CORDIC for atan2 function for quaternion euler conversion on bmi270
RNG + SHA256 encryption for secure wireless communication to ground station
USB high speed data
550MHz max clock speed
will handle all data aquisition and data processing
sends data to ESP32 via uart for wireless communication



Needs USB DFU option
Should have 6 pin plug of nails debug programmer
Should support JTAG / SWD
Preferrably SWIM (single wire interface)
Final pins: USB support via switch on 1 usb, swd, and uart (via test pads and ESP

Seems like Boot0 high allows for programming via DFU
and boot0 low runs the program


SPI1 = ADXL375_2, ADXL375_3
SPI2 = BMI270_1
SPI3 = BMI270_2
SPI4(fast) = BMI270_3
SPI6(slow) = ADXL375_1
I2C1 = MS5611_1 (fast data rate)
I2C2 = MS5611_2 (high accuracy)
I2C3 = MS5607_3 (telemega compatible)
I2C4 = SAM-M10Q

ADC(IN4) => BAT
ADC(IN10) => PYROCURRENT