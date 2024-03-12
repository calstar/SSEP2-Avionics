/*
 * GNSS.c
 *
 *  Created on: 03.10.2020
 *      Author: SimpleMethod
 *
 *Copyright 2020 SimpleMethod
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy of
 *this software and associated documentation files (the "Software"), to deal in
 *the Software without restriction, including without limitation the rights to
 *use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *of the Software, and to permit persons to whom the Software is furnished to do
 *so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *THE SOFTWARE.
 ******************************************************************************
 */
#include "main.h"
#include "GNSS.h"

union u_Short uShort;
union i_Short iShort;
union u_Long uLong;
union i_Long iLong;

/*!
 * Structure initialization.
 * @param GNSS Pointer to main GNSS structure.
 * @param hi2c4 Pointer to i2c handle.
 */
void GNSS_Init(GNSS_StateHandle *GNSS, I2C_HandleTypeDef *hi2c4) {
	GNSS->hi2c4 = hi2c4;
	GNSS->year = 0;
	GNSS->month = 0;
	GNSS->day = 0;
	GNSS->hour = 0;
	GNSS->min = 0;
	GNSS->sec = 0;
	GNSS->fixType = 0;
	GNSS->lon = 0;
	GNSS->lat = 0;
	GNSS->height = 0;
	GNSS->hMSL = 0;
	GNSS->hAcc = 0;
	GNSS->vAcc = 0;
	GNSS->gSpeed = 0;
	GNSS->headMot = 0; // microseconds??
}

/*!
 * Searching for a header in data buffer and matching class and message ID to buffer data.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseBuffer(GNSS_StateHandle *GNSS) {

	for (int var = 0; var <= 100; ++var) {
		if (GNSS->i2cWorkingBuffer[var] == 0xB5
				&& GNSS->i2cWorkingBuffer[var + 1] == 0x62) {
			if (GNSS->i2cWorkingBuffer[var + 2] == 0x27
					&& GNSS->i2cWorkingBuffer[var + 3] == 0x03) { //Look at: 32.19.1.1 u-blox 8 Receiver description
				GNSS_ParseUniqID(GNSS);
			} else if (GNSS->i2cWorkingBuffer[var + 2] == 0x01
					&& GNSS->i2cWorkingBuffer[var + 3] == 0x21) { //Look at: 32.17.14.1 u-blox 8 Receiver description
				GNSS_ParseNavigatorData(GNSS);
			} else if (GNSS->i2cWorkingBuffer[var + 2] == 0x01
					&& GNSS->i2cWorkingBuffer[var + 3] == 0x07) { //ook at: 32.17.30.1 u-blox 8 Receiver description
				GNSS_ParsePVTData(GNSS);
			} else if (GNSS->i2cWorkingBuffer[var + 2] == 0x01
					&& GNSS->i2cWorkingBuffer[var + 3] == 0x02) { // Look at: 32.17.15.1 u-blox 8 Receiver description
				GNSS_ParsePOSLLHData(GNSS);
			}
		}
	}
}

/*!
 * Make request for unique chip ID data.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_GetUniqID(GNSS_StateHandle *GNSS) {
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4, 0x42, getDeviceID,
			sizeof(getDeviceID) / sizeof(uint8_t));
	HAL_I2C_Master_Receive_IT(GNSS->hi2c4,0x42, GNSS_Handle.i2cWorkingBuffer, 17);
}

/*!
 * Make request for UTC time solution data.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_GetNavigatorData(GNSS_StateHandle *GNSS) {
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4, 0x42, getNavigatorData,
			sizeof(getNavigatorData) / sizeof(uint8_t));
	HAL_I2C_Master_Receive_IT(GNSS->hi2c4, 0x42, GNSS_Handle.i2cWorkingBuffer, 28);
}

/*!
 * Make request for geodetic position solution data.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_GetPOSLLHData(GNSS_StateHandle *GNSS) {
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, getPOSLLHData,
			sizeof(getPOSLLHData) / sizeof(uint8_t));
	HAL_I2C_Master_Receive_IT(GNSS->hi2c4,0x42, GNSS_Handle.i2cWorkingBuffer, 36);
}

/*!
 * Make request for navigation position velocity time solution data.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_GetPVTData(GNSS_StateHandle *GNSS) {
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, getPVTData,
			sizeof(getPVTData) / sizeof(uint8_t));
	HAL_I2C_Master_Receive_IT(GNSS->hi2c4,0x42, GNSS_Handle.i2cWorkingBuffer, 100);
}

/*!
 * Parse data to unique chip ID standard.
 * Look at: 32.19.1.1 u-blox 8 Receiver description
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseUniqID(GNSS_StateHandle *GNSS) {
	for (int var = 0; var < 5; ++var) {
		GNSS->uniqueID[var] = GNSS_Handle.i2cWorkingBuffer[10 + var];
	}
}

/*!
 * Changing the GNSS mode.
 * Look at: 32.10.19 u-blox 8 Receiver description
 */
void GNSS_SetMode(GNSS_StateHandle *GNSS, short gnssMode) {
	if (gnssMode == 0) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setPortableMode,sizeof(setPortableMode) / sizeof(uint8_t));
	} else if (gnssMode == 1) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setStationaryMode,sizeof(setStationaryMode) / sizeof(uint8_t));
	} else if (gnssMode == 2) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setPedestrianMode,sizeof(setPedestrianMode) / sizeof(uint8_t));
	} else if (gnssMode == 3) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setAutomotiveMode,sizeof(setAutomotiveMode) / sizeof(uint8_t));
	} else if (gnssMode == 4) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setAutomotiveMode,sizeof(setAutomotiveMode) / sizeof(uint8_t));
	} else if (gnssMode == 5) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setAirbone1GMode,sizeof(setAirbone1GMode) / sizeof(uint8_t));
	} else if (gnssMode == 6) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setAirbone2GMode,sizeof(setAirbone2GMode) / sizeof(uint8_t));
	} else if (gnssMode == 7) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setAirbone4GMode,sizeof(setAirbone4GMode) / sizeof(uint8_t));
	} else if (gnssMode == 8) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setWirstMode,sizeof(setWirstMode) / sizeof(uint8_t));
	} else if (gnssMode == 9) {
		HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setBikeMode,sizeof(setBikeMode) / sizeof(uint8_t));
	}
}
/*!
 * Parse data to navigation position velocity time solution standard.
 * Look at: 32.17.15.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParsePVTData(GNSS_StateHandle *GNSS) {
	uShort.bytes[0] = GNSS_Handle.i2cWorkingBuffer[10];
	GNSS->yearBytes[0]=GNSS_Handle.i2cWorkingBuffer[10];
	uShort.bytes[1] = GNSS_Handle.i2cWorkingBuffer[11];
	GNSS->yearBytes[1]=GNSS_Handle.i2cWorkingBuffer[11];
	GNSS->year = uShort.uShort;
	GNSS->month = GNSS_Handle.i2cWorkingBuffer[12];
	GNSS->day = GNSS_Handle.i2cWorkingBuffer[13];
	GNSS->hour = GNSS_Handle.i2cWorkingBuffer[14];
	GNSS->min = GNSS_Handle.i2cWorkingBuffer[15];
	GNSS->sec = GNSS_Handle.i2cWorkingBuffer[16];
	GNSS->fixType = GNSS_Handle.i2cWorkingBuffer[26];

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 30];
		GNSS->lonBytes[var]= GNSS_Handle.i2cWorkingBuffer[var + 30];
	}
	GNSS->lon = iLong.iLong;
	GNSS->fLon=(float)iLong.iLong/10000000.0;
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 34];
		GNSS->latBytes[var]=GNSS_Handle.i2cWorkingBuffer[var + 34];
	}
	GNSS->lat = iLong.iLong;
	GNSS->fLat=(float)iLong.iLong/10000000.0;
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 38];
	}
	GNSS->height = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 42];
		GNSS->hMSLBytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 42];
	}
	GNSS->hMSL = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 46];
	}
	GNSS->hAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 50];
	}
	GNSS->vAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 66];
		GNSS->gSpeedBytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 66];
	}
	GNSS->gSpeed = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 70];
	}
	GNSS->headMot = iLong.iLong * 1e-5; // todo I'm not sure this good options.
}

/*!
 * Parse data to UTC time solution standard.
 * Look at: 32.17.30.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseNavigatorData(GNSS_StateHandle *GNSS) {
	uShort.bytes[0] = GNSS_Handle.i2cWorkingBuffer[18];
	uShort.bytes[1] = GNSS_Handle.i2cWorkingBuffer[19];
	GNSS->year = uShort.uShort;
	GNSS->month = GNSS_Handle.i2cWorkingBuffer[20];
	GNSS->day = GNSS_Handle.i2cWorkingBuffer[21];
	GNSS->hour = GNSS_Handle.i2cWorkingBuffer[22];
	GNSS->min = GNSS_Handle.i2cWorkingBuffer[23];
	GNSS->sec = GNSS_Handle.i2cWorkingBuffer[24];
}

/*!
 * Parse data to geodetic position solution standard.
 * Look at: 32.17.14.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParsePOSLLHData(GNSS_StateHandle *GNSS) {
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 10];
	}
	GNSS->lon = iLong.iLong;
	GNSS->fLon=(float)iLong.iLong/10000000.0;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 14];
	}
	GNSS->lat = iLong.iLong;
	GNSS->fLat=(float)iLong.iLong/10000000.0;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 18];
	}
	GNSS->height = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 22];
	}
	GNSS->hMSL = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 26];
	}
	GNSS->hAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.i2cWorkingBuffer[var + 30];
	}
	GNSS->vAcc = uLong.uLong;
}

/*!
 *  Sends the basic configuration: Activation of the UBX standard, change of NMEA version to 4.10 and turn on of the Galileo system.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_LoadConfig(GNSS_StateHandle *GNSS) {
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, configUBX,
			sizeof(configUBX) / sizeof(uint8_t));
	HAL_Delay(250);
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setNMEA410,
			sizeof(setNMEA410) / sizeof(uint8_t));
	HAL_Delay(250);
	HAL_I2C_Master_Transmit_DMA(GNSS->hi2c4,0x42, setGNSS,
			sizeof(setGNSS) / sizeof(uint8_t));
	HAL_Delay(250);
}



/*!
 *  Creates a checksum based on UBX standard.
 * @param class Class value from UBX doc.
 * @param messageID MessageID value from UBX doc.
 * @param dataLength Data length value from UBX doc.
 * @param payload Just payload.
 * @return  Returns checksum.
 */
uint8_t GNSS_Checksum(uint8_t class, uint8_t messageID, uint8_t dataLength,uint8_t *payload) {
//todo: Look at 32.4 UBX Checksum
	return 0;
}
