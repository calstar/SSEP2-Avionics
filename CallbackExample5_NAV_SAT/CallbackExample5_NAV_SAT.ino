/*
  Configuring the GNSS to automatically send NAV SAT reports over I2C and display them using a callback
  By: Paul Clark
  SparkFun Electronics
  Date: December 1st, 2021
  License: MIT. See license file for more information.

  This example shows how to configure the u-blox GNSS to send NAV SAT reports automatically
  and access the data via a callback. No more polling!

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136

  Hardware Connections:
  Plug a Qwiic cable into the GPS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3
SFE_UBLOX_GNSS myGNSS;
unsigned long lastTime = 0;

// Callback: newNAVSAT will be called when new NAV SAT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_SAT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoNAVSATcallback
//        /             _____  This _must_ be UBX_NAV_SAT_data_t
//        |            /                _____ You can use any name you like for the struct
//        |            |               /
//        |            |               |
void newNAVSAT(UBX_NAV_SAT_data_t *ubxDataStruct)
{
  Serial.println();

  Serial.print(F("New NAV SAT data received. It contains data for "));
  Serial.print(ubxDataStruct->header.numSvs);
  if (ubxDataStruct->header.numSvs == 1)
    Serial.println(F(" SV."));
  else
    Serial.println(F(" SVs."));

  // Just for giggles, print the signal strength for each SV as a barchart
  for (uint16_t block = 0; block < ubxDataStruct->header.numSvs; block++) // For each SV
  {
    switch (ubxDataStruct->blocks[block].gnssId) // Print the GNSS ID
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
    
    Serial.print(ubxDataStruct->blocks[block].svId); // Print the SV ID
    
    if (ubxDataStruct->blocks[block].svId < 10) Serial.print(F("   "));
    else if (ubxDataStruct->blocks[block].svId < 100) Serial.print(F("  "));
    else Serial.print(F(" "));

    // Print the signal strength as a bar chart
    for (uint8_t cno = 0; cno < ubxDataStruct->blocks[block].cno; cno++)
      Serial.print(F("="));

    Serial.println();
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for user to open terminal
  Serial.println("SparkFun u-blox Example");

  Wire.begin(48,47);

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR

  myGNSS.setNavigationFrequency(1); //Produce one solution per second

  myGNSS.setAutoNAVSATcallbackPtr(&newNAVSAT); // Enable automatic NAV SAT messages with callback to newNAVSAT
uint16_t measRate = myGNSS.getMeasurementRate(); //Get the measurement rate of this module
  Serial.print("Current measurement interval (ms): ");
  Serial.println(measRate);

  uint16_t navRate = myGNSS.getNavigationRate(); //Get the navigation rate of this module
  Serial.print("Current navigation ratio (cycles): ");
  Serial.println(navRate);

  // The measurement rate is the elapsed time between GNSS measurements, which defines the rate
  // e.g. 100 ms => 10 Hz, 1000 ms => 1 Hz, 10000 ms => 0.1 Hz.
  // Let's set the measurement rate (interval) to 5 seconds = 5000 milliseconds
  if (myGNSS.setMeasurementRate(1000, VAL_LAYER_RAM) == false) // Change the rate in RAM only - don't save to BBR
  {
    Serial.println(F("Could not set the measurement rate. Freezing."));
    while (1);
  }

  // setMeasurementRate will set i2cPollingWait to a quarter of the interval
  // Let's override that so we can poll the module more frequently and avoid timeouts
  myGNSS.setI2CpollingWait(25); // Set i2cPollingWait to 25ms

  // The navigation rate is the ratio between the number of measurements and the number of navigation solutions
  // e.g. 5 means five measurements for every navigation solution. Maximum value is 127
  // Let's set the navigation rate (ratio) to 12 to produce a solution every minute
  if (myGNSS.setNavigationRate(1, VAL_LAYER_RAM) == false) // Change the rate in RAM only - don't save to BBR
  {
    Serial.println(F("Could not set the navigation rate. Freezing."));
    while (1);
  }

  // Read and print the updated measurement rate and navigation rate

  measRate = myGNSS.getMeasurementRate(); //Get the measurement rate of this module
  Serial.print("New measurement interval (ms): ");
  Serial.println(measRate);

  navRate = myGNSS.getNavigationRate(); //Get the navigation rate of this module
  Serial.print("New navigation ratio (cycles): ");
  Serial.println(navRate);

  Serial.print("PVT data will be sent every ");
  Serial.print(measRate * navRate / 1000);
  Serial.println(" seconds");

  if ((measRate * navRate / 1000) == 60)
  {
    Serial.println(F("Fun fact: GPS time does not include the 18 leap seconds since 1980."));
    Serial.println(F("PVT data will be sent at the 42 second mark."));
  }

  lastTime = millis();

}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
   if (myGNSS.getPVT()) //Check for new Position, Velocity, Time data. getPVT returns true if new data is available.
  {    
      long latitude = myGNSS.getLatitude();
      Serial.print(F("Lat: "));
      Serial.print(latitude);

      long longitude = myGNSS.getLongitude();
      Serial.print(F(" Long: "));
      Serial.print(longitude);

      //Calculate the interval since the last message
      Serial.print(F(" Interval: "));
      Serial.print(((float)(millis() - lastTime)) / 1000.0, 2);
      Serial.print(F("s"));

      long altitude = myGNSS.getAltitude();
    Serial.print(F(" Alt: "));
    Serial.print(altitude);

      byte fixType = myGNSS.getFixType();
    Serial.print(F(" Fix: "));
    if(fixType == 0) Serial.print(F("No fix"));
    else if(fixType == 1) Serial.print(F("Dead reckoning"));
    else if(fixType == 2) Serial.print(F("2D"));
    else if(fixType == 3) Serial.print(F("3D"));
    else if(fixType == 4) Serial.print(F("GNSS + Dead reckoning"));
    else if(fixType == 5) Serial.print(F("Time only"));

      Serial.println();

      lastTime = millis(); //Update lastTime
  }
}
