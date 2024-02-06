#include <max7301.h> //SPI IO Expander Library
//https://max7301.readthedocs.io/en/latest/usage.html
// look into transition detection and getPinMode() in future

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h> //Needed for I2C
#include <SoftWire.h> // software defined i2c pins for 48,47
#define SCK  14
#define MISO  16
#define MOSI  15
#define SDCS  12 // SD Card Definitions and Library
// look into not enough file space alerts and directory
//from SD card
/* listDir(SD, "/", 0);
  createDir(SD, "/mydir");
  removeDir(SD, "/mydir");
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  deleteFile(SD, "/foo.txt");
  renameFile(SD, "/hello.txt", "/foo.txt");
  readFile(SD, "/foo.txt"); 
*/

#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3 GPS Library

SFE_UBLOX_GNSS myGNSS; // Initialize GPS

#define myWire Wire1 // Connect using the Wire1 port. Change this if required
#define gnssAddress 0x42 // T  he default I2C address for GPS is 0x42. Change this if required


MAX7301 max7301(14,15,16,45,true); //Initialize IO Expander (CLK, MOSI, MISO, CS, true)

SPIClass spi = SPIClass(FSPI); // Initialize SD Card with Full SPI Connection

void setup() {
  max7301.enable(); //Enable and configure pins on IO expander
  max7301.pinMode(4, GPIO_INPUT);
  max7301.pinMode(5, GPIO_INPUT);
  max7301.pinMode(6, GPIO_INPUT);
  max7301.pinMode(7, GPIO_INPUT);
  max7301.pinMode(8, GPIO_INPUT);
  max7301.pinMode(9, GPIO_INPUT);
  max7301.pinMode(10, GPIO_INPUT);
  max7301.pinMode(11, GPIO_INPUT);
  max7301.pinMode(12, GPIO_INPUT);
  max7301.pinMode(13, GPIO_INPUT);
  max7301.pinMode(14, GPIO_INPUT);
  max7301.pinMode(15, GPIO_INPUT);
  max7301.pinMode(16, GPIO_INPUT);
  max7301.pinMode(17, GPIO_OUTPUT);
  max7301.pinMode(18, GPIO_OUTPUT);
  max7301.pinMode(21, GPIO_OUTPUT);
  max7301.pinMode(22, GPIO_OUTPUT);
  max7301.pinMode(23, GPIO_OUTPUT);
  max7301.pinMode(24, GPIO_OUTPUT);
  max7301.pinMode(25, GPIO_OUTPUT);
  max7301.pinMode(26, GPIO_OUTPUT);
  max7301.pinMode(27, GPIO_OUTPUT);
  max7301.pinMode(28, GPIO_OUTPUT);
  max7301.pinMode(29, GPIO_INPUT); 
  max7301.pinMode(30, GPIO_OUTPUT);
  max7301.pinMode(31, GPIO_OUTPUT);
  Serial.begin(115200); //enable Serial communication for sending data to computer
  spi.begin(SCK, MISO, MOSI, SDCS); //Enables SPI Communication for SD Card
  myWire.begin(48,47); //I2C (SDA, SCL)

  myGNSS.setPacketCfgPayloadSize(UBX_NAV_SAT_MAX_LEN); // Allocate extra RAM to store the full NAV SAT data
  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial
  while (myGNSS.begin(myWire, gnssAddress) == false) //Connect to the u-blox module using custom port and address
  {
    Serial.println(F("u-blox GPS not detected. Retrying..."));
    delay (1000);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  

  if (!SD.begin(SDCS,spi,80000000)) {  //Starts SD Card
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType(); //Finds Type of SD Card
   if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024); //Finds SD Card Size
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  uint64_t FreeBytes =SD.totalBytes() / (1024 * 1024) - SD.usedBytes() / (1024 * 1024);
  Serial.printf("Free space: %lluMB\n", FreeBytes);


  //writeFile(SD, "/count.txt", "Counting Test:\n");  //write a test file to SD card
 
}

void loop() {
  
  max7301.digitalWrite(22, HIGH); //test that pin 22(bottom right most) is working correctly
  //delay(1000);  
  max7301.digitalWrite(22, LOW);   
  byte SDDET = max7301.digitalRead(29);
  Serial.println(SDDET); // prints 1 if NO SD card, 0 if is SD card
  //delay(1000);   
  //counter++;
  appendFile(SD, "/count.txt", "test1");


  // Request (poll) the position, velocity and time (PVT) information.
  // The module only responds when a new position is available. Default is once per second.
  // getPVT() returns true when new data is received.
  if (myGNSS.getPVT() == true)
  {
    int32_t latitude = myGNSS.getLatitude();
    Serial.print(F("Lat: "));
    Serial.print(latitude);

    int32_t longitude = myGNSS.getLongitude();
    Serial.print(F(" Long: "));
    Serial.print(longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    int32_t altitude = myGNSS.getAltitudeMSL(); // Altitude above Mean Sea Level
    Serial.print(F(" Alt: "));
    Serial.print(altitude);
    Serial.print(F(" (mm)"));

    Serial.println();
  }
  if (myGNSS.getNAVSAT()) // Poll the latest NAV SAT data
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


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
