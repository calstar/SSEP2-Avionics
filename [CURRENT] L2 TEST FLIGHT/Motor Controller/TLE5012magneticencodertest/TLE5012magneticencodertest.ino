#include <SPI.h>
int angleValue_command = 0x8021; // Register command für Anglevalue read
int ss = 11;    //Chipselect pin
#define SCK 36
#define MISO 37
#define MOSI 35

void setup()
{
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // CLOCK speed einstellen 16MHZ/2 -> 8 MHZ
  Serial.begin(115200);
  pinMode(ss, OUTPUT);    //Chip select as output
  pinMode(SCK, OUTPUT);   //Clock pin as output
  pinMode(MISO, INPUT);   //Set MISO to input
  pinMode(MOSI, OUTPUT);  //Set MOSI to output
}
void loop()
{
  int miso_data = spi_read(angleValue_command);
  if (miso_data < 0) miso_data = abs(miso_data);
  
  // Scale and map the raw angle value to the 0-360 range
  int scaledAngle = map(miso_data, 16384, 0, 0, 360);
  
  //int angle = miso_data * (360/(pow(2, 15)));
  Serial.println(scaledAngle);
}
int spi_read (int read_command)
{
  int data1 = 0;
  int data2 = 0;
  int safety = 0;

  digitalWrite(ss, LOW);      // Chipselect auf low
/*------------ Send request to the slave ------------------------
    Send 16-bit command */
  pinMode(MOSI, OUTPUT);
  SPI.transfer((byte)(read_command >> 8));    // 8 High-Bits to the Slave sent
  delay(0.2);
  SPI.transfer((byte)(read_command & 0x00FF)); // 8 Low-Bits to the Slave sent
  pinMode(MOSI, INPUT);                        // MOSI Pin Switch to high impedance
  delay(0.2);
  /* ende 16 Bit Commands */
  /* 16Bit Data 1 received */

  /*-----------------Response from slave----------------------*/
  data1 = SPI.transfer(0);

  // Für den Fall wenn der Sensor einen 16 Bit Wert zurücksendet ...
  data1 <<= 8;
  data1 |= SPI.transfer(0);

  /* 16Bit Data 2 received */
  data2 = SPI.transfer(0); // Response from slave

  // In case the sensor sends back a 16 bit value...
  data2 <<= 8;
  data2 |= SPI.transfer(0);

  /* 16Bit safety received */
  safety = SPI.transfer(0); // Response from slave
  // In case the sensor sends back a 16 bit value...
  safety <<= 8;
  safety |= SPI.transfer(0);
  int miso = data1;
  digitalWrite(ss, HIGH);     // Chipselect on high
  return miso;
}