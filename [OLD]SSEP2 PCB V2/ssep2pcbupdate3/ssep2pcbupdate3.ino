#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>  //Needed for I2C
//#include <SoftWire.h> // software defined i2c pins for 48,47
#include <MS5803_01.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL375.h>
#define BNO08X_CS 14
#define BNO08X_INT 42
#define BNO08X_RESET 41

#define SCK 12
#define MISO 13
#define MOSI 11
#define SDCS 10  // SD Card Definitions and Library
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

#include <SparkFun_u-blox_GNSS_v3.h>  //http://librarymanager/All#SparkFun_u-blox_GNSS_v3 GPS Library

SFE_UBLOX_GNSS myGNSS;  // Initialize GPS
Adafruit_ADXL375 accel = Adafruit_ADXL375(12345);

#define myWire Wire1      // Connect using the Wire1 port. Change this if required
#define gnssAddress 0x42  // T  he default I2C address for GPS is 0x42. Change this if required

double pressure_baseline;
MS_5803 sensor = MS_5803(4096, 0x77);

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

SPIClass spi = SPIClass(FSPI);  // Initialize SD Card with Full SPI Connection

void setup() {
  Serial.begin(115200);              //enable Serial communication for sending data to computer
  spi.begin(SCK, MISO, MOSI, SDCS);  //Enables SPI Communication for SD Card
  myWire.begin();                    //I2C (SDA, SCL) (3,4)

  myGNSS.setPacketCfgPayloadSize(UBX_NAV_SAT_MAX_LEN);  // Allocate extra RAM to store the full NAV SAT data
  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial
  while (myGNSS.begin(myWire, gnssAddress) == false)  //Connect to the u-blox module using custom port and address
  {
    Serial.println(F("u-blox GPS not detected. Retrying..."));
    delay(1000);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX);  //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.setI2CpollingWait(25);       // Set i2cPollingWait to 25ms
  if (!bno08x.begin_I2C()) {
    // if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte
    // UART buffer! if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
  }
  

  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }
  setReports();

if(!accel.begin())
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no ADXL375 detected ... Check your wiring!");
    while(1) {
      delay(10);
    }
  }
  accel.printSensorDetails();
  Serial.print(accel.getDataRate());
  Serial.println("");
  accel.setTrimOffsets(0, 0, 0);
  Serial.println("Hold accelerometer flat to set offsets to 0, 0, and -1g...");
  delay(5000);
  int16_t x, y, z;
  x = accel.getX();
  y = accel.getY();
  z = accel.getZ();
  Serial.print("Raw X: "); Serial.print(x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(z); Serial.print("  ");Serial.println(" counts");

  // the trim offsets are in 'multiples' of 4, we want to round, so we add 2
  accel.setTrimOffsets(-(x+2)/4, 
                       -(y+2)/4, 
                       -(z-20+2)/4);  // Z should be '20' at 1g (49mg per bit)
  
  int8_t x_offset, y_offset, z_offset;
  accel.getTrimOffsets(&x_offset, &y_offset, &z_offset);
  Serial.print("Current trim offsets: ");
  Serial.print(x_offset);  Serial.print(", ");
  Serial.print(y_offset);  Serial.print(", ");
  Serial.println(z_offset);

  Serial.println();

  if (!SD.begin(SDCS, spi, 80000000)) {  //Starts SD Card
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();  //Finds Type of SD Card
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);  //Finds SD Card Size
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  uint64_t FreeBytes = SD.totalBytes() / (1024 * 1024) - SD.usedBytes() / (1024 * 1024);
  Serial.printf("Free space: %lluMB\n", FreeBytes);


  //writeFile(SD, "/count.txt", "Counting Test:\n");  //write a test file to SD card
  listDir(SD, "/", 0);
  createDir(SD, "/mydir");
  listDir(SD, "/", 0);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");

  delay(2000);
  sensor.initializeMS_5803(false);
  delay(3000);
  sensor.readSensor();
  pressure_baseline = sensor.pressure();
  Serial.println(pressure_baseline);
}

void loop() {
  sensor.readSensor();
  Serial.print("Pressure = ");
  Serial.print(sensor.pressure());
  Serial.println(" mbar");

  // Show temperature
  Serial.print("Temperature = ");
  Serial.print(sensor.temperature());
  Serial.println(" C");


  Serial.print("Altitude = ");
  Serial.print((44330.0 * (1 - pow(sensor.pressure() / pressure_baseline, 1 / 5.255))));
  Serial.println(" m AGL");


  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }
  switch (sensorValue.sensorId) {

    case SH2_ACCELEROMETER:
      Serial.print("Accelerometer - x: ");
      Serial.print(sensorValue.un.accelerometer.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.accelerometer.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.accelerometer.z);
      break;
    case SH2_GYROSCOPE_CALIBRATED:
      Serial.print("Gyro - x: ");
      Serial.print(sensorValue.un.gyroscope.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.gyroscope.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.gyroscope.z);
      break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
      Serial.print("Magnetic Field - x: ");
      Serial.print(sensorValue.un.magneticField.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.magneticField.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.magneticField.z);
      break;
    case SH2_LINEAR_ACCELERATION:
      Serial.print("Linear Acceration - x: ");
      Serial.print(sensorValue.un.linearAcceleration.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.linearAcceleration.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.linearAcceleration.z);
      break;
    case SH2_GRAVITY:
      Serial.print("Gravity - x: ");
      Serial.print(sensorValue.un.gravity.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.gravity.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.gravity.z);
      break;
    case SH2_ROTATION_VECTOR:
      Serial.print("Rotation Vector - r: ");
      Serial.print(sensorValue.un.rotationVector.real);
      Serial.print(" i: ");
      Serial.print(sensorValue.un.rotationVector.i);
      Serial.print(" j: ");
      Serial.print(sensorValue.un.rotationVector.j);
      Serial.print(" k: ");
      Serial.print(sensorValue.un.rotationVector.k);
      break;
    case SH2_GEOMAGNETIC_ROTATION_VECTOR:
      Serial.print("Geo-Magnetic Rotation Vector - r: ");
      Serial.print(sensorValue.un.geoMagRotationVector.real);
      Serial.print(" i: ");
      Serial.print(sensorValue.un.geoMagRotationVector.i);
      Serial.print(" j: ");
      Serial.print(sensorValue.un.geoMagRotationVector.j);
      Serial.print(" k: ");
      Serial.print(sensorValue.un.geoMagRotationVector.k);
      break;

    case SH2_GAME_ROTATION_VECTOR:
      Serial.print("Game Rotation Vector - r: ");
      Serial.print(sensorValue.un.gameRotationVector.real);
      Serial.print(" i: ");
      Serial.print(sensorValue.un.gameRotationVector.i);
      Serial.print(" j: ");
      Serial.print(sensorValue.un.gameRotationVector.j);
      Serial.print(" k: ");
      Serial.print(sensorValue.un.gameRotationVector.k);
      break;

    case SH2_STEP_COUNTER:
      Serial.print("Step Counter - steps: ");
      Serial.print(sensorValue.un.stepCounter.steps);
      Serial.print(" latency: ");
      Serial.print(sensorValue.un.stepCounter.latency);
      break;

    case SH2_STABILITY_CLASSIFIER:
      {
        Serial.print("Stability Classification: ");
        sh2_StabilityClassifier_t stability = sensorValue.un.stabilityClassifier;
        switch (stability.classification) {
          case STABILITY_CLASSIFIER_UNKNOWN:
            Serial.println("Unknown");
            break;
          case STABILITY_CLASSIFIER_ON_TABLE:
            Serial.println("On Table");
            break;
          case STABILITY_CLASSIFIER_STATIONARY:
            Serial.println("Stationary");
            break;
          case STABILITY_CLASSIFIER_STABLE:
            Serial.println("Stable");
            break;
          case STABILITY_CLASSIFIER_MOTION:
            Serial.println("In Motion");
            break;
        }
        Serial.print(stability.classification);
        break;
      }

    case SH2_RAW_ACCELEROMETER:
      Serial.print("Raw Accelerometer - x: ");
      Serial.print(sensorValue.un.rawAccelerometer.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.rawAccelerometer.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.rawAccelerometer.z);
      break;
    case SH2_RAW_GYROSCOPE:
      Serial.print("Raw Gyro - x: ");
      Serial.print(sensorValue.un.rawGyroscope.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.rawGyroscope.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.rawGyroscope.z);
      break;
    case SH2_RAW_MAGNETOMETER:
      Serial.print("Raw Magnetic Field - x: ");
      Serial.print(sensorValue.un.rawMagnetometer.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.rawMagnetometer.y);
      Serial.print(" z: ");
      Serial.print(sensorValue.un.rawMagnetometer.z);
      break;

    case SH2_SHAKE_DETECTOR:
      {
        Serial.print("Shake Detector - shake detected on axis: ");
        sh2_ShakeDetector_t detection = sensorValue.un.shakeDetector;
        switch (detection.shake) {
          case SHAKE_X:
            Serial.println("X");
            break;
          case SHAKE_Y:
            Serial.println("Y");
            break;
          case SHAKE_Z:
            Serial.println("Z");
            break;
          default:
            Serial.println("None");
            break;
        }
        Serial.print(detection.shake);
      }

    case SH2_PERSONAL_ACTIVITY_CLASSIFIER:
      {

        sh2_PersonalActivityClassifier_t activity =
          sensorValue.un.personalActivityClassifier;
        Serial.print("Activity classification - Most likely: ");
        printActivity(activity.mostLikelyState);
        Serial.println("");
        Serial.print(activity.mostLikelyState);

        Serial.println("Confidences:");
        // if PAC_OPTION_COUNT is ever > 10, we'll need to
        // care about page
        for (uint8_t i = 0; i < PAC_OPTION_COUNT; i++) {
          Serial.print("\t");
          printActivity(i);
          Serial.print(": ");
          Serial.print(activity.confidence[i]);
        }
      }
  }

  //delay(1000);
  //counter++;
  appendFile(SD, "/count.txt", "test1");

  sensors_event_t event;
  accel.getEvent(&event);

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");
  Serial.println("m/s^2 ");
  
  Serial.print("Raw X: "); Serial.print(accel.getX()); Serial.print("  ");
  Serial.print("Y: "); Serial.print(accel.getY()); Serial.print("  ");
  Serial.print("Z: "); Serial.print(accel.getZ()); Serial.print("  ");
  Serial.println(" counts");
  Serial.println();
  delay(10);

  myGNSS.checkUblox();      // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks();  // Check if any callbacks are waiting to be processed.
  // Request (poll) the position, velocity and time (PVT) information.
  // The module only responds when a new position is available. Default is once per second.
  // getPVT() returns true when new data is received.
  if (myGNSS.getPVT() == true) {
    int32_t latitude = myGNSS.getLatitude();
    Serial.print(F("Lat: "));
    Serial.print(latitude);

    int32_t longitude = myGNSS.getLongitude();
    Serial.print(F(" Long: "));
    Serial.print(longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    int32_t altitude = myGNSS.getAltitudeMSL();  // Altitude above Mean Sea Level
    Serial.print(F(" Alt ASL: "));
    Serial.print(altitude);
    Serial.print(F(" (mm)"));

    Serial.println();

    byte fixType = myGNSS.getFixType();
    Serial.print(F(" Fix: "));
    if (fixType == 0) Serial.print(F("No fix"));
    else if (fixType == 1) Serial.print(F("Dead reckoning"));
    else if (fixType == 2) Serial.print(F("2D"));
    else if (fixType == 3) Serial.print(F("3D"));
    else if (fixType == 4) Serial.print(F("GNSS + Dead reckoning"));
    else if (fixType == 5) Serial.print(F("Time only"));
  }
  if (myGNSS.getNAVSAT())  // Poll the latest NAV SAT data
  {
    Serial.println();

    // See u-blox_structs.h for the full definition of UBX_NAV_SAT_data_t
    Serial.print(F("New NAV SAT data received. It contains data for "));
    Serial.print(myGNSS.packetUBXNAVSAT->data.header.numSvs);
    if (myGNSS.packetUBXNAVSAT->data.header.numSvs == 1)
      Serial.println(F(" SV."));
    else
      Serial.println(F(" SVs."));

    // print the signal strength for each SV as a barchart
    for (uint16_t block = 0; block < myGNSS.packetUBXNAVSAT->data.header.numSvs; block++)  // For each SV
    {
      switch (myGNSS.packetUBXNAVSAT->data.blocks[block].gnssId)  // Print the GNSS ID
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

      Serial.print(myGNSS.packetUBXNAVSAT->data.blocks[block].svId);  // Print the SV ID

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


void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
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

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  }
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
    Serial.println("Could not enable gyroscope");
  }
  if (!bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED)) {
    Serial.println("Could not enable magnetic field calibrated");
  }
  if (!bno08x.enableReport(SH2_LINEAR_ACCELERATION)) {
    Serial.println("Could not enable linear acceleration");
  }
  if (!bno08x.enableReport(SH2_GRAVITY)) {
    Serial.println("Could not enable gravity vector");
  }
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
  if (!bno08x.enableReport(SH2_GEOMAGNETIC_ROTATION_VECTOR)) {
    Serial.println("Could not enable geomagnetic rotation vector");
  }
  if (!bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game rotation vector");
  }
  if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
    Serial.println("Could not enable step counter");
  }
  if (!bno08x.enableReport(SH2_STABILITY_CLASSIFIER)) {
    Serial.println("Could not enable stability classifier");
  }
  if (!bno08x.enableReport(SH2_RAW_ACCELEROMETER)) {
    Serial.println("Could not enable raw accelerometer");
  }
  if (!bno08x.enableReport(SH2_RAW_GYROSCOPE)) {
    Serial.println("Could not enable raw gyroscope");
  }
  if (!bno08x.enableReport(SH2_RAW_MAGNETOMETER)) {
    Serial.println("Could not enable raw magnetometer");
  }
  if (!bno08x.enableReport(SH2_SHAKE_DETECTOR)) {
    Serial.println("Could not enable shake detector");
  }
  if (!bno08x.enableReport(SH2_PERSONAL_ACTIVITY_CLASSIFIER)) {
    Serial.println("Could not enable personal activity classifier");
  }
}

void printActivity(uint8_t activity_id) {
  switch (activity_id) {
    case PAC_UNKNOWN:
      Serial.print("Unknown");
      break;
    case PAC_IN_VEHICLE:
      Serial.print("In Vehicle");
      break;
    case PAC_ON_BICYCLE:
      Serial.print("On Bicycle");
      break;
    case PAC_ON_FOOT:
      Serial.print("On Foot");
      break;
    case PAC_STILL:
      Serial.print("Still");
      break;
    case PAC_TILTING:
      Serial.print("Tilting");
      break;
    case PAC_WALKING:
      Serial.print("Walking");
      break;
    case PAC_RUNNING:
      Serial.print("Running");
      break;
    case PAC_ON_STAIRS:
      Serial.print("On Stairs");
      break;
    default:
      Serial.print("NOT LISTED");
  }
  Serial.print(" (");
  Serial.print(activity_id);
  Serial.print(")");
}