#include <TMC2209.h>

#include <HardwareSerial.h>
// This example will not work on Arduino boards without HardwareSerial ports,
// such as the Uno, Nano, and Mini.
//
// See this reference for more details:
// https://www.arduino.cc/reference/en/language/functions/communication/serial/
#define UART_TX_PIN 17
#define UART_RX_PIN 18

HardwareSerial serial_stream(3);  // Use UART3 (Serial3) and specify custom pins


const long SERIAL_BAUD_RATE = 115200;
const int DELAY = 200;
// current values may need to be reduced to prevent overheating depending on
// specific motor and power supply voltage
const uint8_t RUN_CURRENT_PERCENT = 100;  
const int32_t VELOCITY = 20000;
const uint8_t STALL_GUARD_THRESHOLD = 20;


// Instantiate TMC2209
TMC2209 stepper_driver;


void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

  serial_stream.begin(SERIAL_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);  // Initialize Serial3 with custom pins

  stepper_driver.setup(serial_stream);

  stepper_driver.setRunCurrent(RUN_CURRENT_PERCENT);
  stepper_driver.setStallGuardThreshold(STALL_GUARD_THRESHOLD);
  stepper_driver.enable();
  stepper_driver.moveAtVelocity(VELOCITY);
}

void loop()
{
  if (not stepper_driver.isSetupAndCommunicating())
  {
    Serial.println("Stepper driver not setup and communicating!");
    return;
  }

  Serial.print("run_current_percent = ");
  Serial.println(RUN_CURRENT_PERCENT);

  Serial.print("stall_guard_threshold = ");
  Serial.println(STALL_GUARD_THRESHOLD);

  uint16_t stall_guard_result = stepper_driver.getStallGuardResult();
  Serial.print("stall_guard_result = ");
  Serial.println(stall_guard_result);

  Serial.println();
  delay(DELAY);

}
