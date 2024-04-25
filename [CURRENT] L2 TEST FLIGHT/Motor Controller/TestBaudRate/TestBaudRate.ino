#include <TMC2209.h>

// This example will not work on Arduino boards without HardwareSerial ports,
// such as the Uno, Nano, and Mini.
//
// See this reference for more details:
// https://www.arduino.cc/reference/en/language/functions/communication/serial/

HardwareSerial & serial_stream = Serial1;

const long SERIAL_BAUD_RATE = 115200;
const long SERIAL1_BAUD_RATE_COUNT = 10;
const long SERIAL1_BAUD_RATES[SERIAL1_BAUD_RATE_COUNT] =
{
  500000,
  250000,
  115200,
  57600,
  38400,
  31250,
  28800,
  19200,
  14400,
  9600
};
const uint8_t SUCCESSIVE_OPERATION_COUNT = 3;
const int DELAY = 2000;

// Instantiate TMC2209
TMC2209 stepper_driver;
uint8_t serial1_baud_rate_index = 0;
const int RX_PIN = 17;
const int TX_PIN = 18;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(13,OUTPUT);
  pinMode(14,OUTPUT);
  digitalWrite(13, LOW);
    digitalWrite(14, LOW);


}

void loop()
{
  long serial1_baud_rate = SERIAL1_BAUD_RATES[serial1_baud_rate_index++];
  stepper_driver.setup(serial_stream,serial1_baud_rate, TMC2209::SERIAL_ADDRESS_3, RX_PIN, TX_PIN);
  if (serial1_baud_rate_index == SERIAL1_BAUD_RATE_COUNT)
  {
    serial1_baud_rate_index = 0;
  }

  bool test_further = false;

  Serial.println("*************************");
  Serial.print("serial1_baud_rate = ");
  Serial.println(serial1_baud_rate);

  if (stepper_driver.isSetupAndCommunicating())
  {
    Serial.println("Stepper driver setup and communicating!");
    test_further = true;
  }
  else
  {
    Serial.println("Stepper driver not setup and communicating!");
  }

  if (test_further)
  {
    uint32_t microstep_sum = 0;
    for (uint8_t i=0; i<SUCCESSIVE_OPERATION_COUNT; ++i)
    {
      microstep_sum += stepper_driver.getMicrostepsPerStep();
    }
    if (microstep_sum > 0)
    {
      Serial.println("Successive read test passed!");
    }
    else
    {
      Serial.println("Successive read test failed!");
    }
    uint8_t itc_begin = stepper_driver.getInterfaceTransmissionCounter();
    for (uint8_t i=0; i<SUCCESSIVE_OPERATION_COUNT; ++i)
    {
      stepper_driver.disable();
    }
    uint8_t itc_end = stepper_driver.getInterfaceTransmissionCounter();
    if (itc_begin != itc_end)
    {
      Serial.println("Successive write test passed!");
    }
    else
    {
      Serial.println("Successive write test failed!");
    }
  }

  Serial.println("*************************");
  Serial.println();
  delay(DELAY);
}
