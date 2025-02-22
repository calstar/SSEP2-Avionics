#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pSensorCharacteristic = NULL;
BLECharacteristic *pCommandCharacteristic = NULL;

bool deviceArmed = false; // Initial device state

#define SERVICE_UUID           "544e9751-c449-4008-adf5-8d8e4e538b9b"
#define SENSOR_CHARACTERISTIC_UUID "268f6592-bf07-47e3-838b-19a17ea3a003"
#define COMMAND_CHARACTERISTIC_UUID "01bf7531-87e0-46fe-be41-36c7ba9e98f5"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceArmed = false; // Reset device state on connection
    };

    void onDisconnect(BLEServer* pServer) {
      deviceArmed = false; // Reset device state on disconnection
    }
};

void setup() {
  Serial.begin(115200);

  // Create BLE Device
  BLEDevice::init("STAR BLE Switch 1");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create Sensor Characteristic
  pSensorCharacteristic = pService->createCharacteristic(
                          SENSOR_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

  pSensorCharacteristic->addDescriptor(new BLE2902());

  // Create Command Characteristic
  pCommandCharacteristic = pService->createCharacteristic(
                             COMMAND_CHARACTERISTIC_UUID,
                             BLECharacteristic::PROPERTY_WRITE
                           );

  // Start BLE Services
  pService->start();

  // Start Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

void loop() {
  // Read sensor data and update characteristic value
  uint8_t sensorData[] = {/* Replace with your sensor data */};
  pSensorCharacteristic->setValue(sensorData, sizeof(sensorData));
  pSensorCharacteristic->notify();

  // Check for incoming command
  if (pCommandCharacteristic->getValue().length() > 0) {
    uint8_t command = pCommandCharacteristic->getValue()[0];
    if (command == 0x69) {
      deviceArmed = true;
      Serial.println("Device armed");
      strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
      strip.show();
    } else if (command == 0x42) {
      deviceArmed = false;
      Serial.println("Device disarmed");
      strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
      strip.show();
    }
    pCommandCharacteristic->setValue("");
  }

  delay(1000); // Adjust delay as needed
}