#include <max7301.h> //SPI IO Expander Library
//https://max7301.readthedocs.io/en/latest/usage.html
// look into transition detection and getPinMode() in future
#include <WiFi.h>

const char* ssid     = "CUM PCB";
const char* password = "ssepcum6969"; //must be 7+ characters
#include "FS.h"
#include "SPI.h"

// Load Wi-Fi library
#include <WiFi.h>

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

WiFiServer server(80);
String header;
MAX7301 max7301(14,15,16,45,true); //Initialize IO Expander (CLK, MOSI, MISO, CS, true)
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;
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



    Serial.println("\n[*] Creating AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
delay(50);
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /ignite") >= 0) {
              Serial.println("PYRO Channel 1 Triggered on");
              output26State = "on";
              Serial.println("outputting on");
             max7301.digitalWrite(28, HIGH); //test that pin 22(bottom right most) is working correctly
             delay(4000);  
             Serial.println("outputting off");
              max7301.digitalWrite(28, LOW);   
             output26State = "off"; 
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("PYRO Channel 2 Triggered on");
              output27State = "on";
              Serial.println("outputting on");
             max7301.digitalWrite(27, HIGH); //test that pin 22(bottom right most) is working correctly
             delay(4000);  
             Serial.println("outputting off");
              max7301.digitalWrite(27, LOW);
              
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #ff0000; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>STAR CUM PCB</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>Pyro Channel 1 </p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/ignite\"><button class=\"button\">IGNITE</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
           /*    
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>Pyro Channel 2 - " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            } */
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}


