// DUSTDUINO v1.0
// Released 18 October 2013
// Last Updated 30 August 2015
//
// This software is released as-is, without warranty,
// under a Creative Commons Attribution-ShareAlike
// 3.0 Unported license. For more information about
// this license, visit:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Written by Matthew Schroyer, except where specified.
// For more information on building a DustDuino, visit:
// http://www.mentalmunition.com/2013/10/measure-air-pollution-with-dustduino-of.html

/*888888888888888888888888888888888888888888888888888888888888888888888888888888888888*/

// WiFlyHQ Library written by Harlequin-Tech.
// Available at: https://github.com/harlequin-tech/WiFlyHQ
#include <WiFlyHQ.h>
#include <avr/wdt.h>

unsigned long starttime;

unsigned long triggerOnP1;
unsigned long triggerOffP1;
unsigned long pulseLengthP1;
unsigned long durationP1;
boolean valP1 = HIGH;
boolean triggerP1 = false;

unsigned long triggerOnP2;
unsigned long triggerOffP2;
unsigned long pulseLengthP2;
unsigned long durationP2;
boolean valP2 = HIGH;
boolean triggerP2 = false;

float ratioP1 = 0;
float ratioP2 = 0;
unsigned long sampletime_ms = 30000;
float countP1;
float countP2;

WiFly wifly;
void terminal();

const char ARDUINO_KEY[] = "YOUR-API-KEY-RECEIVED-BY-EMAIL";
const char server[] = "api.opendustmap.com";

void setup(){
  Serial.begin(9600);
  wifly.begin(&Serial, NULL);
  pinMode(3, INPUT);
  pinMode(2, INPUT);
  wdt_enable(WDTO_8S);
  starttime = millis();
}

void loop(){
  
  digitalWrite(7, HIGH);
  
  valP1 = digitalRead(3);
  valP2 = digitalRead(2);
  
  if(valP1 == LOW && triggerP1 == false){
    triggerP1 = true;
    triggerOnP1 = micros();
  }
  
  if (valP1 == HIGH && triggerP1 == true){
      triggerOffP1 = micros();
      pulseLengthP1 = triggerOffP1 - triggerOnP1;
      durationP1 = durationP1 + pulseLengthP1;
      triggerP1 = false;
  }
  
    if(valP2 == LOW && triggerP2 == false){
    triggerP2 = true;
    triggerOnP2 = micros();
  }
  
    if (valP2 == HIGH && triggerP2 == true){
      triggerOffP2 = micros();
      pulseLengthP2 = triggerOffP2 - triggerOnP2;
      durationP2 = durationP2 + pulseLengthP2;
      triggerP2 = false;
  }
  
    wdt_reset();

    // Function creates particle count and mass concentration
    // from PPD-42 low pulse occupancy (LPO).
    if ((millis() - starttime) > sampletime_ms) {
      
      // Generates PM10 and PM2.5 count from LPO.
      // Derived from code created by Chris Nafis
      // http://www.howmuchsnow.com/arduino/airquality/grovedust/
      
      ratioP1 = durationP1/(sampletime_ms*10.0);
      ratioP2 = durationP2/(sampletime_ms*10.0);
      countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
      countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
      float PM10count = countP2;
      float PM25count = countP1 - countP2;
      
      // Assues density, shape, and size of dust
      // to estimate mass concentration from particle
      // count. This method was described in a 2009
      // paper by Uva, M., Falcone, R., McClellan, A.,
      // and Ostapowicz, E.
      // http://wireless.ece.drexel.edu/research/sd_air_quality.pdf
      
      // begins PM10 mass concentration algorithm
      double r10 = 2.6*pow(10,-6);
      double pi = 3.14159;
      double vol10 = (4/3)*pi*pow(r10,3);
      double density = 1.65*pow(10,12);
      double mass10 = density*vol10;
      double K = 3531.5;
      float concLarge = (PM10count)*K*mass10;
      
      // next, PM2.5 mass concentration algorithm
      double r25 = 0.44*pow(10,-6);
      double vol25 = (4/3)*pi*pow(r25,3);
      double mass25 = density*vol25;
      float concSmall = (PM25count)*K*mass25;
      
      sendData(concLarge, concSmall, PM10count, PM25count);
      
      durationP1 = 0;
      durationP2 = 0;
      starttime = millis();
      wdt_reset();
    }
  }
  
    // WiFly HTTP connection to the Xively server.
    // Patchube client originally written 15 March 2010 by Tom Igoe,
    // Usman Haque, and Joe Saavedra at http://arduino.cc/en/Tutorial/PachubeCient
    // Modified to work with WiFly RN-XV and the OpenDustMap.org REST API as
    // created by the Earth Journalism Network and Development Seed https://developmentseed.org/

    void sendData(int PM10Conc, int PM25Conc, int PM10count, int PM25count) {
    wifly.open(server, 80);
    wifly.println("POST /api/v1/readings/ HTTP/1.1");
    wifly.print("Host: "); wifly.println(server);
    wifly.print("Authorization: Token ");wifly.println(ARDUINO_KEY);
    wifly.println("User-Agent: Arduino/1.0");
    wifly.print("Accept: *");wifly.print("/");wifly.println("*");
    wifly.println("Referer:");
    wifly.print("Content-Length: ");

    // Calculates the length of the sensor reading in bytes:
    int thisLength = 33 + getLength(PM10Conc) + getLength(PM25Conc) + getLength(PM10count) + getLength(PM25count);
    wifly.println(thisLength);

    // last pieces of the HTTP POST request:
    wifly.println("Content-Type: application/x-www-form-urlencoded");
    wifly.println("Connection: close");
    wifly.println();

    // here's the actual content of the POST request:
    wifly.print("pm10=");
    wifly.print(PM10Conc);
    wifly.print("&pm25=");
    wifly.print(PM25Conc);
    wifly.print("&pm10count=");
    wifly.print(PM10count);
    wifly.print("&pm25count=");
    wifly.println(PM25count);
    wifly.close();
    
    // this will reboot the RN171 module after every message sent
    // note: may cause full reset of Arduino if shield is used
    // Added 8/20/2015
    wifly.reboot();
}
  
  // This function also is derived from
  // the Patchube client sketch. Returns
  // number of digits, which Xively needs
  // to post data correctly.
  int getLength(int someValue) {
  
  int digits = 1;
  if (someValue < 0) {
    someValue = someValue * -1;
    digits = 2;
  }
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  return digits;
}

// This function connects the WiFly to serial.
// Developed by Harlequin-Tech for the WiFlyHQ library.
// https://github.com/harlequin-tech/WiFlyHQ

void terminal()
{
    while (1) {
	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}


	if (Serial.available() > 0) {
	    wifly.write(Serial.read());
	}
    }
}
