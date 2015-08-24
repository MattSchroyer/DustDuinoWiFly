// DUSTDUINO WIFLY SKETCH
// Version 4
// By Matthew Schroyer
// 8/24/2015


// REMEMBER TO SET THEESE TO YOUR OWN PROJECT
//
#define APIKEY         "??????" // your xively api key
#define FEEDID         01234567890 // your feed ID
#define USERAGENT      "DustDuino" // user agent is the project name
//
//

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

const char server[] = "api.xively.com";

void setup(){
  Serial.begin(9600);
  wifly.begin(&Serial, NULL);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  wdt_enable(WDTO_8S);
  starttime = millis();
}

void loop(){
  
  digitalWrite(7) = HIGH;
  
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

    
    if ((millis() - starttime) > sampletime_ms) {
      
      ratioP1 = durationP1/(sampletime_ms*10.0);  // Integer percentage 0=>100
      ratioP2 = durationP2/(sampletime_ms*10.0);
      countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
      countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
      int PM10count = countP2;
      int PM25count = countP1 - countP2;
      
      if(PM25count < 0){
        PM25count = 0;
      }
      
      // converts particle counts into concentration
      // first, PM10 conversion
      double r10 = 2.6*pow(10,-6);
      double pi = 3.14159;
      double vol10 = (4/3)*pi*pow(r10,3);
      double density = 1.65*pow(10,12);
      double mass10 = density*vol10;
      double K = 3531.5;
      float concLarge = (PM10count)*K*mass10;
      // next, PM2.5 conversion
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
  
  // Makes a HTTP connection to the server and sends data:
void sendData(int PM10Conc, int PM25Conc, int PM10count, int PM25count) {

  // Forms the data into a message to send to Xively:
  
    String DuinoData;
    DuinoData = "PM10,";
    DuinoData = DuinoData + PM10Conc;
    DuinoData = DuinoData + '\r';
    DuinoData = DuinoData + '\n';
    DuinoData = DuinoData + "PM25,";
    DuinoData = DuinoData + PM25Conc;
    DuinoData = DuinoData + '\r';
    DuinoData = DuinoData + '\n';
    DuinoData = DuinoData + "PM10count,";
    DuinoData = DuinoData + PM10count;
    DuinoData = DuinoData + '\r';
    DuinoData = DuinoData + '\n';
    DuinoData = DuinoData + "PM25count,";
    DuinoData = DuinoData + PM25count;
    DuinoData = DuinoData + '\r';
    DuinoData = DuinoData + '\n';
                        
    wifly.open(server, 80);
    wifly.print("PUT /v2/feeds/");
    wifly.print(FEEDID);
    wifly.println(".csv HTTP/1.1");
    wifly.println("Host: api.xively.com");
    wifly.print("X-ApiKey: ");
    wifly.println(APIKEY);
    wifly.print("User-Agent: ");
    wifly.println(USERAGENT);
    wifly.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    int messageLength = DuinoData.length();
    wifly.println(messageLength);

    // last pieces of the HTTP PUT request:
    wifly.println("Content-Type: text/csv");
    wifly.println("Connection: close");
    wifly.println();                  

    // here's the actual content of the PUT request:
    wifly.print(DuinoData);
    wifly.close();
}
  
  int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten, 
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

/* Connect the WiFly serial to the serial monitor. */
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
