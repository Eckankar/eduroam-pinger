#define ESP8266_USE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <SPI.h>
// http://docs.iteadstudio.com/ITEADLIB_Arduino_WeeESP8266/index.html

// Connection (esp8266):
// 3.3V: VCC, CH_PD, 2, 0
// GND: GND, 15
// Arduino 2 -> voltage splitter -> RXD
// Arduino 3 <- TXD

SoftwareSerial wifiSerial(3, 2); /* rx=3, tx=2 */
ESP8266 wifi(wifiSerial);

#define RESET_PIN 5

#include "credentials.h"
#define DEBUG 1

uint8_t buf[500];

void setup() {  
  Serial.begin(9600);
  wifiSerial.begin(9600);    
 
  pinMode(RESET_PIN, OUTPUT);
  reset();
}

int con_failures = 0;

void loop() {
  if (!wifi.kick()) reset();
  Serial.println(F("Pinging..."));
  if (getRequest(F("arduino.coq.dk"), 80, F("/eduroam/ping"), buf, sizeof(buf))) {
    Serial.println("OK");
    con_failures = 0;
  } else {
    Serial.println("Failed :(");
    if (con_failures++ > 5) {
      delay((con_failures - 5) * 1000 * 5);
      reset();
    }
  }
}

bool getRequest(String host, int port, String path, uint8_t *resBuf, int resBufSize) {
  String request = "GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nUser-Agent: arduino:eduroamcheck:1.0\r\n\r\n";
  
  if (! wifi.createTCP(host, port)) {
    Serial.print(F("Unable to connect to "));
    Serial.println(host);
    return false;
  }
  
  if (! wifi.send((const uint8_t *) request.c_str(), request.length())) {
    Serial.println(F("Unable to send data."));
    return false; 
  }
  
  /*int received;
  do {
    received = wifi.recv(resBuf, resBufSize-1, 2000);
    resBuf += received;
    resBufSize -= received;
  } while (received > 0 && resBufSize > 1);
  resBuf[0] = '\0';
  */
  if (! wifi.releaseTCP()) {
    Serial.println(F("Unable to release TCP connection."));
  }
  
  return true;
}

void hang(const __FlashStringHelper *message) {
  Serial.print(F("Error: "));
  Serial.println(message);
  delay(10000);
}

void reset() {
  digitalWrite(RESET_PIN,LOW);
  delay(100);
  digitalWrite(RESET_PIN,HIGH);
  delay(500);
  
  Serial.println(F("== boot =="));
  Serial.println(F("Set to station mode."));
  if (! wifi.setOprToStation()) return hang(F("Could not change to station mode."));
  Serial.println(F("Joining AP"));
  if (! wifi.joinAP(SSID, PASSWORD)) return hang(F("Could not connect to access point."));
  Serial.print(F("Getting IP... "));
  String ip = wifi.getLocalIP();
  Serial.println(ip);
  Serial.println(F("Disabling multiplexing..."));
  delay(10);
  if (! wifi.disableMUX()) return hang(F("Could not disable multiplexing."));
  Serial.println(F("Initialization complete."));
}

String sendData(String command, const int timeout)
{
    String response = "";
    
    if (DEBUG) {
        Serial.print("C> " + command + "\r\n");
    }   
    wifiSerial.print(command + "\r\n"); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(wifiSerial.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = wifiSerial.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(DEBUG) {
      Serial.print("S> " + response);
    }
    
    return response;
}
