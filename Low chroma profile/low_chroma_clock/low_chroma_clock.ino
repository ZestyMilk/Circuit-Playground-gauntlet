//GPS Clock by Tully Jagoe

#include "Adafruit_CircuitPlayground.h"
#include <SoftwareSerial.h>
#include "Adafruit_GPS.h"
#include <Adafruit_NeoPixel.h>
#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#include <math.h>
#endif

//Neopixels on the clock face
#define NUMPIXELS      16 //number of neopixels in clock face
#define PIN            9  //clock face neopixel data pin

//Neopixels on the Circuit Playground
#define N_PIXELS       9  // Number of pixels in strand
#define LED_PIN        12  // NeoPixel LED strand is connected to this pin

// Marker Neopixels
#define MARKPIN        6

// Light sensor definitions
#define ANALOG_INPUT  A5
#define VALUE_MIN     0
#define VALUE_MAX     5

#define BUTTONL       4
#define BUTTONR       19
#define BUTTONS       21

//Speaker sound duration
#define TONE_DURATION_MS 50 

// From adaFruit NEOPIXEL goggles example: Gamma correction improves appearance of midrange colors
const uint8_t gamma[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
      3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
      7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
     13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
     20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
     30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
     42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
     76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
     97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
    122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
    150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
    182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
    218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255
};

uint16_t light = analogRead(ANALOG_INPUT);

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel markers = Adafruit_NeoPixel(3, MARKPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ring = Adafruit_NeoPixel(10, 17, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// Offset the hours from UTC (universal time) to your local time by changing
// this value.  The GPS time will be in UTC so lookup the offset for your
// local time from a site like:
// https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
// This value, -7, will set the time to UTC-7 or Pacific Standard Time during
// daylight savings time.
#define HOUR_OFFSET       +10

Adafruit_GPS gps(&Serial1);      //hardware serial

uint32_t milli_color   = pixels.Color ( 80, 80, 200); 

uint32_t marker_color  = pixels.Color ( 10, 10, 20);
uint32_t ring_color    = pixels.Color ( 1, 1, 2);
uint32_t vu_color      = pixels.Color ( 10, 0, 0);
uint32_t strip_color   = pixels.Color ( 0, 0, 0);
uint32_t cylon_color   = pixels.Color ( 10, 10, 25);
uint32_t off_color     = pixels.Color ( 0, 0, 0);

const uint8_t spSTART[]     PROGMEM = {0x08,0xD8,0x50,0x05,0x01,0x5F,0xB1,0x12,0xE0,0x33,0x55,0x06,0x7C,0xEE,0x66,0x80,0x6F,0x54,0x19,0xF0,0x65,0x18,0x07,0x10,0x28,0x94,0xBB,0x8B,0xC8,0x06,0x03,0x6C,0x96,0x36,0x9C,0xAA,0xD3,0x5D,0xBC,0xD3,0xF1,0x8A,0x52,0x6F,0xE9,0x28,0x27,0x28,0xF1,0x32,0x24,0x3C,0x9D,0x28,0xC7,0x4B,0x97,0xF0,0x74,0xD2,0x9C,0xAE,0x4D,0x23,0xE3,0x2D,0x56,0x95,0xF3,0xB6,0x85,0x76,0x5A,0x5D,0x2E,0xDB,0x1A,0xB6,0x6A,0x8C,0xB9,0x4C,0xAA,0xEB,0xC2,0x36,0x95,0x18,0x15,0x2A,0x0B,0xCB,0x54,0x83,0x8E,0x9B,0xB4,0x24,0x93,0x50,0xEE,0x2E,0x22,0x1B,0x00,0x18,0x70,0x94,0x3A,0x03,0x5E,0x66,0x53,0xC0,0x51,0x96,0x04,0xD8,0x42,0x02,0x01,0x8D,0xBA,0xFD,0x05,0x79,0x15,0x65};
const uint8_t spPROGRAMME[] PROGMEM = {0x0C,0x48,0x2E,0x95,0x03,0x2E,0x0A,0x8D,0x3D,0xDA,0x13,0xD5,0x68,0xF9,0xA9,0x73,0x68,0xC7,0x94,0x13,0x94,0x62,0xA9,0xC6,0xAB,0x4F,0x50,0x63,0x84,0x19,0xAD,0x3E,0x41,0x0B,0xEE,0x6E,0xB8,0xE9,0x04,0xD5,0x69,0x84,0xF0,0xAB,0x11,0x66,0xE7,0x66,0x22,0xAF,0x5C,0x14,0xA3,0x2B,0xAB,0x2E,0x34,0xA9,0x54,0x19,0x1D,0x96,0x4A,0xE5,0x3E,0xAB,0xB8,0x44,0x9C,0x50,0xA7,0x66,0x21,0xA4,0x71,0x52,0x95,0x33,0x17,0x66,0xA7,0x1D,0x75,0x69,0x1A,0x94,0xE9,0x75,0xB5,0xA5,0xA9,0x9B,0xE7,0x9A,0x33,0xB4,0xD8,0xE1,0xC1,0x5D,0xCE,0xD8,0xFC,0x44,0x24,0x75,0x3E,0x53,0x8B,0xE5,0x19,0xBC,0x78,0xCC,0x35,0x76,0x64,0xF2,0xED,0x36,0xD7,0x54,0x51,0xCE,0xB3,0xEB,0x1C,0xE6,0xEC,0x34,0x0C,0xAC,0x89,0x9F,0xED,0xF2,0x45};

bool hashadlock= true;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  delay(500);
  // Setup function runs once at startup to initialize the display and GPS.

  // Setup the GPS using a 9600 baud connection (the default for most
  // GPS modules).
  gps.begin(9600);

  // Configure GPS to onlu output minimum data (location, time, fix).
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // Use a 1 hz, once a second, update rate.
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  
  // Enable the interrupt to parse GPS data.
  enableGPSInterrupt();
  
  //Initialise the NeoPixel library (watch this doesn't disable interrupts)
  pixels.begin();
  markers.begin();
  ring.begin();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  CircuitPlayground.speaker.say(spPROGRAMME);
  CircuitPlayground.speaker.say(spSTART);
  
  /*
  CircuitPlayground.playTone(320, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(494, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(320, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(494, TONE_DURATION_MS);
  */
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void loop() {

  //uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  //if (len != 0) return;
  
  // Loop function runs over and over again to implement the clock logic.

  //GPS code
  // Check if GPS has new data and parse it.
  if (gps.newNMEAreceived()) {
    if (gps.parse(gps.lastNMEA())){
      Serial.println("");
      Serial.println ("----GPS Parse OK");
      if (gps.fix){
        hashadlock=true;
      }
    }
  }

  

    //if (gps.fixquality !=0){
    //if (gps.fix){
    if (hashadlock){
        drawclock(); //shows the clock, as long as GPS is locked, else cylon
    }
    else {
      drawclock();
    }
    gammacorrect(); //correct brightness
    //vumeter();
    

    //if we're not inhibited, refresh the display
    //if(dispInhibit == false) pixels.show(); // This sends the updated pixel color to the hardware.
    pixels.show();
    markers.show();
    ring.show();
    strip.show();
  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

SIGNAL(TIMER0_COMPA_vect) {
  // Use a timer interrupt once a millisecond to check for new GPS data.
  // This piggybacks on Arduino's internal clock timer for the millis()
  // function.
  gps.read();
}

void enableGPSInterrupt() {
  // Function to enable the timer interrupt that will parse GPS data.
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function above
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void drawclock(){
  
  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light)+1);
  if (light > 50){
    light = 50;
  }

  uint32_t hour_color    = pixels.Color ( 65+(light/2), 65+(light/2), 85+(light/2));
  uint32_t minutes_color = pixels.Color ( 35+(light/2), 35+(light/2), 55+(light/2));
  uint32_t second_color  = pixels.Color ( 30, 30, 50);
  
  // Grab the current hours, minutes, seconds from the GPS.
  // This will only be set once the GPS has a fix!  Make sure to add
  // a coin cell battery so the GPS will save the time between power-up/down.
  int hours = gps.hour;
  // Handle when UTC + offset wraps around to a negative or > 23 value.
  if (hours < 0) {
    hours = 24+hours;
  }
  if (hours > 23) {
    hours = 24-hours;
  }
  hours = hours + HOUR_OFFSET;
  if (TIME_24_HOUR == false){ //set 12 hour time
    if (hours > 12) {
      hours = (hours-12);
      hours %= 12;
    }
  }
  
  int minutes = gps.minute;
  int seconds = gps.seconds;
  
  clearstrand();
  //hours goes form led0 to led11, 1 hour per led
  int hoursled = hours;
  add_color(hoursled, hour_color);
  
  //mins goes from led0 to led11, 5 minutes per led
  int minutesled = minutes/5;
  add_color(minutesled, minutes_color);
  int minutes2led = (minutes/5)+1;
  add_color(minutes2led, minutes_color);
  
  //seconds goes from led0 to led11
  int secondsled = seconds/5;
  add_color(secondsled, second_color);

  
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  const long interval = 20;

  static unsigned long previousMillis2 = 0;
  const long interval2 = 1000;
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis += interval; 
    static int o=0;
    static int d=0;
    if (o!=10){
      ring.setPixelColor(o, light, light, light); //pulse colour    
      o++;
    }
    if (d!=seconds){
      o=0;
      d=seconds;
    }
  }
  buttoncheck();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void datashow(){

  // Light sensor values
  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light/10)+1);
  if (light > 50){
    light = 50;
  }

  static unsigned long previousMillis = 0;
  static unsigned long previousMillis2 = 0;
  unsigned long currentMillis = millis();
  const long interval = 100;
  const long interval2 = 0;
  int u=(random(0,3));
  int i=(random(0,3));
  int o=(random(0,3));
  int p=(random(0,3));
  
  if (currentMillis - previousMillis2 >= interval2) {
  previousMillis2 += interval2;
  strip.setPixelColor(4, light, light, (light*2));
  }
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis += interval;

    strip.setPixelColor(0, u*light, u*light, (u*light)*2);
    strip.setPixelColor(1, i*light, i*light, (i*light)*2);
    strip.setPixelColor(2, o*light, o*light, (o*light)*2);
    strip.setPixelColor(3, p*light, p*light, (p*light)*2);
    strip.setPixelColor(5, p*light, p*light, (p*light)*2);
    strip.setPixelColor(6, o*light, o*light, (o*light)*2);
    strip.setPixelColor(7, i*light, i*light, (i*light)*2);
    strip.setPixelColor(8, u*light, u*light, (u*light)*2);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void buttoncheck(){
  //Button values
  uint16_t buttonL = digitalRead(BUTTONL);
  uint16_t buttonR = digitalRead(BUTTONR);
  uint16_t buttonS = digitalRead(BUTTONS);
  uint16_t buttonstate = 1;

  if (buttonS == HIGH){
      warning();
  }else{
      datashow();
  }
  
  if (buttonL == HIGH){
      torch();
  }else{
      datashow();
  }
  
  if (buttonR == HIGH){
      alarm();
  }else{
      datashow();
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void torch(){
  strip.setPixelColor(0, 50, 50, 120);
  strip.setPixelColor(1, 50, 50, 120);
  strip.setPixelColor(2, 50, 50, 120);
  strip.setPixelColor(3, 50, 50, 120);
  strip.setPixelColor(4, 50, 50, 120);
  strip.setPixelColor(5, 50, 50, 120);
  strip.setPixelColor(6, 50, 50, 120);
  strip.setPixelColor(7, 50, 50, 120);
  strip.setPixelColor(8, 50, 50, 120);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void alarm() {
  static unsigned long previousMillis = 0;
  static unsigned long previousMillis2 = 200;
  unsigned long currentMillis = millis();
  const long interval = 1300;
  const long interval2 = 1300;

  if (currentMillis - previousMillis >= interval) {
    previousMillis += interval;
    for(int j = 0; j < 150 ; j++){
       for(uint16_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(gamma[j],gamma[j],0) );
          }
        strip.show();
        CircuitPlayground.playTone(494, TONE_DURATION_MS);
    }
    for(int j = 20; j >= 0 ; j--){
        for(uint16_t i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(gamma[j],gamma[j],0) );
          }
        strip.show();
    }
  }
  if (currentMillis - previousMillis2 >= interval2) {
    previousMillis2 += interval2;
    for(int j = 0; j < 150 ; j++){
       for(uint16_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(gamma[j],gamma[j],0) );
          }
        strip.show();
        CircuitPlayground.playTone(494, TONE_DURATION_MS);
   }
    for(int j = 20; j >= 0 ; j--){
        for(uint16_t i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(gamma[j],gamma[j],0) );
          }
        strip.show();
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void warning() {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();
      CircuitPlayground.playTone(320, TONE_DURATION_MS);
      
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 100 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(100 - WheelPos, 0, 0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, 0, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos, 0, 0);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


void clearstrand(){
  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light/15)+1);
  if (light > 10){
    light = 10;
  }
  
  //Sets all neopixels blank
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, 0, 0, 27-light);
    markers.setPixelColor(i, light*4, light*4, light*8);
    ring.setPixelColor(i, light/5, light/5, light/3+1);
    strip.setPixelColor(i, strip_color);
  }
}



///////////////// BORING SUPPORTING STUFF /////////////////

//copied from NeoPixel ring clock face by Kevin ALford and modified by Becky Stern for Adafruit Industries
void add_color (uint8_t position, uint32_t color)
{
  uint32_t blended_color = blend (pixels.getPixelColor (position), color);
 
  /* Gamma mapping */
  uint8_t r,b,g;
 
  r = (uint8_t)(blended_color >> 16),
  g = (uint8_t)(blended_color >>  8),
  b = (uint8_t)(blended_color >>  0);
 
  pixels.setPixelColor (position, blended_color);
}

uint32_t blend (uint32_t color1, uint32_t color2)
{
  uint8_t r1,g1,b1;
  uint8_t r2,g2,b2;
  uint8_t r3,g3,b3;
 
  r1 = (uint8_t)(color1 >> 16),
  g1 = (uint8_t)(color1 >>  8),
  b1 = (uint8_t)(color1 >>  0);
 
  r2 = (uint8_t)(color2 >> 16),
  g2 = (uint8_t)(color2 >>  8),
  b2 = (uint8_t)(color2 >>  0);
 
 
  return pixels.Color (constrain (r1+r2, 0, 255), constrain (g1+g2, 0, 255), constrain (b1+b2, 0, 255));
}

void gammacorrect(){
  uint8_t r,b,g;
  for(int i=0; i<NUMPIXELS; i++){ //increment through all pixels
    uint32_t raw_color= pixels.getPixelColor(i); //fetch color
    r = (uint8_t)(raw_color >> 16); //extracting colour values from the uint32
    g = (uint8_t)(raw_color >>  8);
    b = (uint8_t)(raw_color >>  0);
    r = gamma[r]; //select pixel intensity from colour value using gamma chart
    g = gamma[g];
    b = gamma[b];
    pixels.setPixelColor (i, pixels.Color(r,g,b)); //jam it back in
  
  }
}
