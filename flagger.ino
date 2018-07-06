/**
 * Copyright (c) 2018 Inversebit
 * 
 * This code is freed under the MIT License.
 * Full license text: https://opensource.org/licenses/MIT
 * 
 */

 /* IMPORTS */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

extern "C" {
  #include "user_interface.h"
}

/* CONSTANTS DEFINITIONS */
const unsigned short rled = 16;
const unsigned short gled = 5;
const unsigned short servo_pin = 4;

const unsigned short MAX_ANGLE = 95;
const unsigned short MIN_ANGLE = 5;
const unsigned int RISEN_TIME = 10000;

const char* ssid = "ssid";
const char* password = "passwd";

/* GLOBAL VARS */
Servo myservo;

//0->Down; 1->Up
unsigned short flag_pos;
unsigned short rise_flag;
unsigned short lower_flag;
unsigned short flag_angle;
unsigned short last_angle;
unsigned long timeout;

//Server vars
ESP8266WebServer server(80);

/* SETUP FUNCTIONS */
//SERVER FUNCTIONS
void handleFlagReq() 
{
  if(flag_pos == 0){
    rise_flag = 1;
  }
  
  server.send(200, "text/plain", "");
}

void handleNotFound()
{
  server.send(404, "text/plain", "");
}

void setupServer()
{
  server.on("/flag", handleFlagReq);
  server.onNotFound(handleNotFound);
  server.begin();
}

//OTHER SETUP
void setupPins()
{
  pinMode(gled, OUTPUT);
  pinMode(rled, OUTPUT);
  pinMode(servo_pin, OUTPUT);
  myservo.attach(servo_pin);
}

void setupWiFiConn()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(gled, HIGH);
    digitalWrite(rled, HIGH);
    delay(250);
    digitalWrite(gled, LOW);
    digitalWrite(rled, LOW);
    delay(150);
  }

  IPAddress ip = WiFi.localIP();
  int lastOctet = ip[3];
  
  blinkLED(rled, 1, 2000);

  do{          
      int blinky = lastOctet % 10;
      lastOctet /= 10;
          
      blinkLED(gled, blinky, 400);
      blinkLED(rled, 1, 500);
  }while(lastOctet);  
}

void user_init(){
  myservo.write(MIN_ANGLE);
  
  flag_angle = MIN_ANGLE;
  last_angle = flag_angle;
  flag_pos = 0;
  rise_flag = 0;
  lower_flag = 0;
  timeout = 0;
}

//*THE* SETUP
void setup() {
  setupPins();
  setupWiFiConn();
  setupServer();
  user_init();
}


void blinkLED(int ledPin, int times, int blinkDuration)
{
  for(int i = 0; i < times; i++)
  {
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(blinkDuration);
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(blinkDuration);
  }
}

/* (THE REST OF THE) FUNCTIONS */
void planning()
{
  //SANITY CHECK
  if(rise_flag == 1 && lower_flag == 1){
    user_init();
    return;
  }
  
  //#0 Waiting for command
  if(flag_pos == 0 && rise_flag == 0 && lower_flag == 0){
    return;
  }
  
  //#1 RISE FLAG
  if(flag_pos == 0 && rise_flag == 1 && lower_flag == 0){
    if(flag_angle < MAX_ANGLE){
      flag_angle++;
    }
    else{
      flag_pos = 1;
      rise_flag = 0;
      timeout = millis();
    }
  }
  
  //#2 FLAG UP
  if(flag_pos == 1 && rise_flag == 0 && lower_flag == 0){
    int tr = timeout + RISEN_TIME;
    int mi = millis();
    if(tr < mi){
      lower_flag = 1;
    }
  }
  
  //#3 LOWERING FLAG
  if(flag_pos == 1 && rise_flag == 0 && lower_flag == 1){
    if(flag_angle > MIN_ANGLE){
      flag_angle--;
    }
    else{
      flag_pos = 0;
      lower_flag = 0;
    }
  }
}

void acting(){
  if(flag_angle != last_angle)
  {
    myservo.write(flag_angle);
    last_angle = flag_angle;
  }  
}

void loop() 
{
  server.handleClient();

  planning();
  acting();
   
  yield();
}
