#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <gfxfont.h>

Adafruit_7segment matrix = Adafruit_7segment();

SoftwareSerial BTserial(11, 10); // RX | TX

int TSL230_Pin = 2; //TSL230 output
//int TSL230_s0 = 3; //TSL230 sensitivity setting 1
//int TSL230_s1 = 5; //TSL230 sensitivity setting 2

int TSL230_samples =100; //higher = slower but more stable and accurate

void setupTSL230();

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(){
  matrix.begin(0x70);
  //To display '----' when microontroller is turned on
  matrix.writeDigitRaw(0, 0b01000000); //-
  matrix.writeDigitRaw(1, 0b01000000); //-
  matrix.writeDigitRaw(3, 0b01000000); //- NOTE: Location #2 is for colon dots
  matrix.writeDigitRaw(4, 0b01000000); //-
  matrix.writeDisplay();

  //Serial.begin(115200);
  Serial.begin(9600);
  setupTSL230();

  Serial.println("Check BT");
  BTserial.begin(9600);
  delay(1000);
  BTserial.print("AT");
  delay(1000);
  while(BTserial.available()) {
    int ch = BTserial.read();
    Serial.write(ch);
  }
#ifndef __AVR_ATtiny85__
#endif

  
 
  pinMode (6, INPUT);

 //Serial.begin(19200);
  //Serial.begin(9600); 
  attachInterrupt(digitalPinToInterrupt(2), turbidityhandler, RISING); 
    
}

// variables needed to calculate frequency, ISR
// period of pulse accumulation and serial output, milliseconds
#define MinDuration 100
#define MaxDuration 5000
long previousMillis = 0; // will store last time of the cycle end
volatile unsigned long duration=0; // accumulates pulse width
volatile unsigned int pulsecount=0;
volatile unsigned long previousMicros=0;

// 
int flag = 0;
int blinkcount = 0;
//float readTSL230(int samples);
float loop2();

void loop()
{
  if (flag == 1)
  {
    if(blinkcount < 5) {
      //To display 'CALC' when measurement in progress i.e. switch is pressed
      matrix.writeDigitRaw(0, 0x39); //C
      matrix.writeDigitRaw(1, 0x77); //A
      matrix.writeDigitRaw(3, 0x38); //L; NOTE: Location #2 is for colon dots
      matrix.writeDigitRaw(4, 0x39); //C
      matrix.writeDisplay();
    }
    else {
      matrix.writeDigitRaw(0, 0); //C
      matrix.writeDigitRaw(1, 0); //A
      matrix.writeDigitRaw(3, 0); //L; NOTE: Location #2 is for colon dots
      matrix.writeDigitRaw(4, 0); //C
      matrix.writeDisplay();
    }
    blinkcount ++;
    if(blinkcount >= 10)
      blinkcount = 0;
      
         float freq = loop2();

    Serial.print("FREQ : ");
    Serial.println(freq);
    if(freq > 0) {
      // calculate NTU from raw frequency data and send through bluetooth
      //float ntu = 2E-6*freq*freq*freq*freq - 0.0007*freq*freq*freq + 0.0864*freq*freq - 0.5248*freq + 0.2791  ;  // 4/30/2018 equation drived from best fit curve poly4 of NTU vs frequency
      //float ntu = 0.0503*freq - 8.4385; //from 4/25/2019
      //float ntu= 7E-07*freq*freq*freq - 0.0003*freq*freq + 0.104*freq + 4.5299; //meter 1 cal eqn 6/3/19 
      float ntu= 3E-05*freq*freq*freq - 0.0073*freq*freq + 1.0675*freq - 24.869; //meter 2 cal eqn 6/3/19
      Serial.print("Frequency: ");
      Serial.print(freq);
      Serial.print(", ");
      Serial.print("NTU: ");
      Serial.println(ntu);
      
      //Send data to phone via HM-10 bluetooth module
     
      BTserial.print(freq);
      BTserial.print(",");
      BTserial.println(ntu);
      if(ntu > 10000)
        matrix.print(9999);
      else
        matrix.print(ntu);
      matrix.writeDisplay();
      delay (20);
      flag = 0;
      Serial.println("Measure: flag -> 2");
    }
  }
  pinMode(6, INPUT);
  int btn = digitalRead(6); //assigning pin 6 of arduino as pushbutton switch input
 
  if (btn == HIGH && flag == 0)
  {
    flag = 1;
    pulsecount = 0;
    duration = 0;
    blinkcount = 0;
      }
    delay(100);
}

void setupTSL230(){
  //pinMode(TSL230_s0, OUTPUT); 
  //pinMode(TSL230_s1, OUTPUT); 

  //configure sensitivity - Can set to
  //S1 LOW  | S0 HIGH: low
  //S1 HIGH | S0 LOW:  med
  //S1 HIGH | S0 HIGH: high

  //digitalWrite(TSL230_s1, HIGH);
  //digitalWrite(TSL230_s0, LOW);
}

float loop2()
{
  unsigned long currentMillis = millis();
    Serial.print(" "); // separator!
    Serial.print(currentMillis);
    Serial.print(" "); // separator!
    Serial.print(previousMillis);
  if (currentMillis - previousMillis >= MinDuration) 
  {
    previousMillis = currentMillis;   
    // need to bufferize to avoid glitches
    unsigned long _duration = duration;
    unsigned long _pulsecount = pulsecount;
    Serial.print(" "); // separator!
    Serial.print(_duration);
    Serial.print(" "); // separator!
    Serial.print(_pulsecount);
    if(_pulsecount > TSL230_samples || currentMillis - previousMillis > MaxDuration) {
      duration = 0; // clear counters
      pulsecount = 0;
      float freq = 1e6 / float(_duration);
      freq *= _pulsecount; // calculate F
      return freq;
    }
    else
      return -1;     
  }
  else
    return -1;
}

void turbidityhandler() // interrupt handler
{
  unsigned long currentMicros = micros();
  duration += currentMicros - previousMicros;
  previousMicros = currentMicros;
  pulsecount++;
}






