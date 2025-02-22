#include <Wire.h>
#include <Adafruit_INA260.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCurrentCharacteristic = NULL;
BLECharacteristic *pBusVoltageCharacteristic = NULL;
BLECharacteristic *pContVoltageCharacteristic = NULL;
BLECharacteristic *pPowerCharacteristic = NULL;
BLECharacteristic *pArmedCharacteristic = NULL;

// Create an INA260 instance
#define NUM_LEDS 1
#define LED_PIN 5
#define SDA 6
#define SCL 7
#define SWITCH_EN 4
#define CONT_ADC 3
#define CONT_EN 10
Adafruit_INA260 ina260;

bool deviceArmed = false;     // Initial device state
bool deviceConnected = false;  // Set connection flag
#define SERVICE_UUID "8fc39c4d-8122-45c9-9971-d74f0a238f38"
#define CURRENT_CHARACTERISTIC_UUID "e98d3cd5-d888-4f07-80ad-279b8f7d16a2"
#define BUS_VOLTAGE_CHARACTERISTIC_UUID "71f99001-b3e0-433c-947f-b55ac1dd8d8c"

#define CONT_VOLTAGE_CHARACTERISTIC_UUID "b8e28bee-cdac-44c3-b472-da95392612bb"
#define POWER_CHARACTERISTIC_UUID "08f68bcd-8e15-4142-a76c-654f547ccaf6"
#define ARMED_CHARACTERISTIC_UUID "ca2e52de-27b2-422f-8cd9-4683328c4898"
#define RESPONSE_CHARACTERISTIC_UUID "d66eb26e-1430-4d60-8052-a65cb4f1ab20"
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    //do something on connect?
    deviceConnected = true;  // Set connection flag
  };

  void onDisconnect(BLEServer *pServer) {
    //do something like sleep on disconnect
    deviceConnected = false;  // Set connection flag
     pServer->startAdvertising(); // restart advertising after disconnecting
  }
};

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
void setup() {
  // Start the serial communication
  Serial.begin(115200);
  strip.begin();
  strip.show();
  delay(500);
  pinMode(CONT_EN, OUTPUT);
  pinMode(SWITCH_EN, OUTPUT);
  digitalWrite(CONT_EN, LOW);
  digitalWrite(SWITCH_EN, LOW);

  // Create BLE Device
  BLEDevice::init("STAR BLE Switch 4");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  //pService->setName("Bluetooth Switch Info");

  // Create Characteristics
  pCurrentCharacteristic = pService->createCharacteristic(
    CURRENT_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCurrentCharacteristic->addDescriptor(new BLE2902());
  //pCurrentCharacteristic->setName("Current Value");

  pBusVoltageCharacteristic = pService->createCharacteristic(
    BUS_VOLTAGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pBusVoltageCharacteristic->addDescriptor(new BLE2902());
  //pBusVoltageCharacteristic->setName("Bus Voltage");

  pContVoltageCharacteristic = pService->createCharacteristic(
    CONT_VOLTAGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pContVoltageCharacteristic->addDescriptor(new BLE2902());
  //pContVoltageCharacteristic->setName("Continuity Voltage");

  pPowerCharacteristic = pService->createCharacteristic(
    POWER_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pPowerCharacteristic->addDescriptor(new BLE2902());
 //pPowerCharacteristic->setName("Power Value");
  pArmedCharacteristic = pService->createCharacteristic(
    ARMED_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  //pArmedCharacteristic->addDescriptor(new BLE2902());
 //pArmedCharacteristic->setName("Armed State");
  /*pResponseCharacteristic = pService->createCharacteristic(
    RESPONSE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pResponseCharacteristic->addDescriptor(new BLE2902());*/
  // Start BLE Services
  pService->start();
    // Set the desired connection interval (in milliseconds)

  // Start Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

pAdvertising->setMinPreferred(0x06);
pAdvertising->setMaxPreferred(0x12);

  pAdvertising->start();
  // Initialize I2C on GPIO6 (SDA) and GPIO7 (SCL)
  Wire.begin(SDA, SCL);

  // Initialize INA260
  ina260.begin();


  Serial.println("INA260 Found!");
}

void loop() {

  /* Serial.print("Current: ");
  Serial.print(ina260.readCurrent());
  Serial.println(" mA");*/
  float currentValue = ina260.readCurrent();

  uint8_t currentBytes[4];
  memcpy(currentBytes, &currentValue, sizeof(currentValue));
  pCurrentCharacteristic->setValue(currentBytes, sizeof(currentBytes));
  pCurrentCharacteristic->notify();

  /* Serial.print("Bus Voltage: ");
  Serial.print(ina260.readBusVoltage());
  Serial.println(" mV");*/
  float busVoltageValue = ina260.readBusVoltage();
  uint8_t busVoltageBytes[4];
  memcpy(busVoltageBytes, &busVoltageValue, sizeof(busVoltageValue));
  pBusVoltageCharacteristic->setValue(busVoltageBytes, sizeof(busVoltageBytes));
  pBusVoltageCharacteristic->notify();

  /*Serial.print("Power: ");
  Serial.print(ina260.readPower());
  Serial.println(" mW");
  float powerValue = ina260.readPower();
  uint8_t powerBytes[4];
  memcpy(powerBytes, &powerValue, sizeof(powerValue));
  pPowerCharacteristic->setValue(powerBytes, sizeof(powerBytes));
  pPowerCharacteristic->notify();*/

  int adcValue = analogRead(CONT_ADC);
  uint8_t contVoltageBytes[4];
  memcpy(contVoltageBytes, &adcValue, sizeof(adcValue));
  pContVoltageCharacteristic->setValue(contVoltageBytes, sizeof(contVoltageBytes));
  pContVoltageCharacteristic->notify();

  // Update armed characteristic value



if (pArmedCharacteristic->getValue().length() >= 3) {
 String value = pArmedCharacteristic->getValue();
if (value[0] == 0x61 && value[1] == 0x72 && value[2] == 0x6D && deviceArmed == false)  {
      deviceArmed = true;
      
      digitalWrite(SWITCH_EN,HIGH);
      digitalWrite(CONT_EN,LOW);
      Serial.println("Device armed");
      
      strip.setPixelColor(0, strip.Color(0, 255, 0));  // Green
      //strip.show();

      String armedValue = "armed";
      
      pPowerCharacteristic->setValue(armedValue);
      pPowerCharacteristic->notify();
    } else if (value[0] == 0x6F && value[1] == 0x66 && value[2] == 0x66 && deviceArmed == true)  {
      deviceArmed = false;
      
      digitalWrite(SWITCH_EN,LOW);
      digitalWrite(CONT_EN,HIGH);
      Serial.println("Device disarmed");
      strip.setPixelColor(0, strip.Color(255, 0, 0));  // Red
      //strip.show();
      String armedValue = "disarmed";
      
      pPowerCharacteristic->setValue(armedValue);
      pPowerCharacteristic->notify();
    }
  }
}
