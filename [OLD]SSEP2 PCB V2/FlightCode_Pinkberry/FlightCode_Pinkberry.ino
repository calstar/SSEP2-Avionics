
#include <Wire.h>
#include <MS5803_01.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SoftWire.h> // software defined i2c pins for 48,47
#include <SparkFun_u-blox_GNSS_v3.h>

#define ARMED_LIGHT //ADD PIN NO.
#define BATTERY_VOLTAGE //ADD PIN NO.

#define SCK  14
#define MISO  16
#define MOSI  15
#define SDCS  12

#define myWire gpsWire // Connect using the Wire1 port. Change this if required
#define gnssAddress 0x42 // T  he default I2C address for GPS is 0x42. Change this if required

struct Readings {
  float press1;
  float press2;
  float temp1;
  float temp2;
  float alt1; //From baro1
  float alt2; //From baro2
  float alt3; //From gps
  float latitude;
  float longditude;
  float accel_ADX; //from ADXL375
  float accel_BNO; //from BNO085
  float battVoltage;
  float pyroState; //state and check might be bools instead... 
  float pyroCheck; 
}

double baro1_baseline;
double baro2_baseline;

MS_5803 baro1 = MS_5803(4096,0x76);
MS_5803 baro2 = MS_5803(4096,0x77);

SFE_UBLOX_GNSS gps;

SPIClass spi = SPIClass(FSPI); // Initialize SD Card with Full SPI Connection

void setup() {
  Serial.begin(115200);
  delay(2000);

  // INITIALIZE MS5803 BAROMETER ///////////////////////////
  // If you don't want all the coefficients printed out, set sensor.initializeMS_5803(false).
  baro1.initializeMS_5803(false);
  baro2.initializeMS_5803(false);  
  delay(3000);
  baro1.readSensor();
  baro2.readSensor();
  baro1_baseline = baro1.pressure();
  baro2_baseline = baro2.pressure();
  Serial.println("Baro1 Baseline: " + baro1_baseline);
  Serial.println("Baro2 Baseline: " + baro2_baseline); 


  // INITIALIZE SD CARD /////////////////////////////////
  spi.begin(SCK, MISO, MOSI, SDCS); 
  if (!SD.begin(SDCS,spi,80000000)) {  //Starts SD Card
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType(); //Finds Type of SD Card
   if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }
  

  // INITIALIZE GPS //////////////////////////////////////
  gpsWire.begin(48,47); //I2C (SDA, SCL) 
  while (gps.begin(gpsWire, gnssAddress) == false) //Connect to the u-blox module using custom port and address
  {
    Serial.println(F("u-blox GPS not detected. Retrying..."));
    delay (1000);
  }
  gps.setI2COutput(COM_TYPE_UBX);   


  // INITIALIZE ADXL375 ACCELEROMETER ///////////////////
  // TO DO


  // INITIALIZE BNO085 ACCELEROMETER ///////////////////
  // TO DO


  // INITIALIZE PYRO BTS3080 ///////////////////////////
  // TO DO


  digitalWrite(ARMED_LED, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

}





void writeToSD(){
  //TO DO
}

/*
getBarometer()
Takes sensor reading from all sensors. Prints pressure, temperature, and 
altitude for each.
*/
void getBarometer() {
  baro1.readSensor();
  baro2.readSensor();
  // Show pressure
  Readings.press1 = baro1.pressure();
  Readings.press2 = baro2.pressure();  
  Serial.print("Pressure = ");
  Serial.print(Readings.press1);
  Serial.println(" mbar");
  Serial.print("Pressure2 = ");
  Serial.print(Readings.press2);
  Serial.println(" mbar");
  
  // Show temperature
  Readings.temp1 = baro1.temperature();
  Readings.temp2 = baro2.temperature(); 
  Serial.print("Temperature = ");
  Serial.print(Readings.temp1);
  Serial.println(" C");
  Serial.print("Temperature2 = ");
  Serial.print(Readings.temp2);
  Serial.println(" C");

  Serial.print("Temperature = ");
  Serial.print(Readings.temp1*1.8 + 32);
  Serial.println(" F");
  Serial.print("Temperature2 = ");
  Serial.print(Readings.temp2*1.8 + 32);
  Serial.println(" F");

  // Show altitude above start 
  Readings.alt1 = (44330.0 * (1 - pow(baro1.pressure() / baro1_baseline, 1 / 5.255)));
  Readings.alt2 = (44330.0 * (1 - pow(baro2.pressure() / baro2_baseline, 1 / 5.255)));
  Serial.print("Altitude = ");
  Serial.print(Readings.alt1);
  Serial.println(" m");
  Serial.print("Altitude2 = ");
  Serial.print(Readings.alt2);
  Serial.println(" m");
  
  delay(10); // For readability
}


/*
getGPS()
Display new PVT (position, velocity, time) data when it is received. 
Poll the latest NAV-SAT data, displays the strength of each satellite as a bar chart
*/
void getGPS() {
  if (myGNSS.getPVT() == true)
  // Request (poll) the position, velocity and time (PVT) information.
  // The module only responds when a new position is available. Default is once per second.
  // getPVT() returns true when new data is received.
  {
    int32_t Readings.latitude = myGNSS.getLatitude();
    Serial.print(F("Lat: "));
    Serial.print(Readings.latitude);

    int32_t Readings.longitude = myGNSS.getLongitude();
    Serial.print(F(" Long: "));
    Serial.print(Readings.longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    int32_t Readings.alt3 = myGNSS.getAltitudeMSL(); // Altitude above Mean Sea Level
    Serial.print(F(" Alt: "));
    Serial.print(Readings.alt3);
    Serial.print(F(" (mm)"));

    Serial.println();
  }

  if (myGNSS.getNAVSAT()) 
  // Poll the latest NAV SAT data
  {
    Serial.println();
    // See u-blox_structs.h for the full definition of UBX_NAV_SAT_data_t
    Serial.print(F("New NAV SAT data received. It contains data for "));
    Serial.print(myGNSS.packetUBXNAVSAT->data.header.numSvs);

    if (myGNSS.packetUBXNAVSAT->data.header.numSvs == 1)
        Serial.println(F(" SV."));
    else
        Serial.println(F(" SVs."));

    // Just for giggles, print the signal strength for each SV as a barchart
    for (uint16_t block = 0; block < myGNSS.packetUBXNAVSAT->data.header.numSvs; block++) // For each SV
    {
        switch (myGNSS.packetUBXNAVSAT->data.blocks[block].gnssId) // Print the GNSS ID
        {
        case 0:
            Serial.print(F("GPS     "));
        break;
        case 1:
            Serial.print(F("SBAS    "));
        break;
        case 2:
            Serial.print(F("Galileo "));
        break;
        case 3:
            Serial.print(F("BeiDou  "));
        break;
        case 4:
            Serial.print(F("IMES    "));
        break;
        case 5:
            Serial.print(F("QZSS    "));
        break;
        case 6:
            Serial.print(F("GLONASS "));
        break;
        default:
            Serial.print(F("UNKNOWN "));
        break;      
        }
        
        Serial.print(myGNSS.packetUBXNAVSAT->data.blocks[block].svId); // Print the SV ID
        
        if (myGNSS.packetUBXNAVSAT->data.blocks[block].svId < 10) Serial.print(F("   "));
        else if (myGNSS.packetUBXNAVSAT->data.blocks[block].svId < 100) Serial.print(F("  "));
        else Serial.print(F(" "));

        // Print the signal strength as a bar chart
        for (uint8_t cno = 0; cno < myGNSS.packetUBXNAVSAT->data.blocks[block].cno; cno++)
        Serial.print(F("="));

        Serial.println();
    }
  }
} 


void getAccel_ADX() {
  //TO DO
}


void getAccel_BNO() {
  //TO DO
}


void getBatteryVoltage() {
  Readings.battVoltage = analogRead(BATTERY_VOLTAGE);
  Serial.print("Battery Voltage: ");
  Serial.println(Readings.battVoltage);
}

void getPyroState() {
  //TO DO  
}


void getPyroCheck() {
  //TO DO
}