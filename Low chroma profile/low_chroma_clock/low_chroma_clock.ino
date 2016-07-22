//GPS Clock by Tully Jagoe

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

#include "Adafruit_CircuitPlayground.h"


// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            9
#define MARKPIN        6

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16

#define N_PIXELS  9  // Number of pixels in strand
#define MIC_PIN   A4  // Microphone is attached to this analog pin
#define LED_PIN    12  // NeoPixel LED strand is connected to this pin
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG 24 //Time of pause before peak dot falls
#define PEAK_FALL 4 //Rate of falling peak dot
#define INPUT_FLOOR 10 //Lower range of analogRead input
#define INPUT_CEILING 300 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)

#define TONE_DURATION_MS 50
 
 
 
byte peak = 16;      // Peak level of column; used for falling dots
unsigned int sample;
 
byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot
 


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
//   https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
// This value, -7, will set the time to UTC-7 or Pacific Standard Time during
// daylight savings time.
#define HOUR_OFFSET       +10

//SoftwareSerial gpsSerial(4, 3);  // software serial connection with RX = pin 4 and TX = pin 3.

Adafruit_GPS gps(&Serial1);      //hardware serial

//uint32_t milli_color   = pixels.Color ( 120, 70, 200); //pale purple millisecond pulse
uint32_t milli_color   = pixels.Color ( 120, 120, 150); 
uint32_t hour_color    = pixels.Color ( 100, 100, 120);
uint32_t minutes_color = pixels.Color ( 70, 70, 100);
uint32_t second_color  = pixels.Color ( 30, 30, 50);
uint32_t marker_color  = pixels.Color ( 70, 70, 100);
uint32_t ring_color    = pixels.Color ( 10, 10, 15);
uint32_t strip_color   = pixels.Color ( 0, 0, 0);
uint32_t cylon_color   = pixels.Color ( 10, 10, 25);

uint32_t off_color     = pixels.Color ( 0, 0, 0);

bool hashadlock= false;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

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

  
  CircuitPlayground.playTone(320, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(494, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(320, TONE_DURATION_MS);
  delay(100);
  CircuitPlayground.playTone(494, TONE_DURATION_MS);
  
  
}

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

  //Display code
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  const long interval = 20;       //change the speed of the cylon here
  if (currentMillis - previousMillis >= interval) {
    //previousMillis = currentMillis;
    previousMillis += interval;

    //if (gps.fixquality !=0){
    //if (gps.fix){
    if (hashadlock){
        drawclock(); //shows the clock, as long as GPS is locked, else cylon
    }
    else {
      drawclock();
      cylon();
    }
    gammacorrect(); //correct brightness

    //if we're not inhibited, refresh the display
    //if(dispInhibit == false) pixels.show(); // This sends the updated pixel color to the hardware.
    pixels.show();
    markers.show();
    ring.show();
    strip.show();
  }
}

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

void clearstrand(){
  //Sets all neopixels blank
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, off_color);
    markers.setPixelColor(i, marker_color);
    ring.setPixelColor(i, ring_color);
    strip.setPixelColor(i, strip_color);
  }
}

void clearstrand2(){
  //Sets all neopixels blank
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, off_color);
    markers.setPixelColor(i, off_color);
    ring.setPixelColor(i, off_color);
    strip.setPixelColor(i, off_color);
  }
}

void cylon(){
  static int i=0; //first cylon led
  static int o=1; //second cylon led, change value for starting distance.
  static int p=2; //second cylon led, change value for starting distance.
  int j=0;
  int k=0;
  int l=0;

  clearstrand2();
  if (i>=9){
    j=17-i;
  }else{
    j=i;
  }
  if (o>=9){
    k=17-o;
  }else{
    k=o;
  }
  if (p>=9){
    l=17-p;
  }else{
    l=p;
  }
  strip.setPixelColor(j, cylon_color);
  strip.setPixelColor(k, cylon_color);
  strip.setPixelColor(l, cylon_color);
  //pixels.setPixelColor(j, pixels.Color(random(0,255),random(0,255),random(0,255)));    //randomises colour every time it moves to the next pixel
  //pixels.setPixelColor(j, pixels.Color(random(100,200),0,random(200,255)));    //random shades of blue, pink, and purple
  i++;
  if (i==18){
    i=0;
  }
  o++;
  if (o==18){
    o=0;
  }
  p++;
  if (p==18){
    p=0;
  }

  //every second, a pulse crosses the whole strip end to end  
  static int z=0;
  static int x=0;
  int minutes = gps.minute;
  int seconds = gps.seconds;
  if (z!=10){
    ring.setPixelColor(z, 10, 0, 0); //pulse colour    
    z++;
  }
  if (x!=seconds){
    CircuitPlayground.playTone(320, TONE_DURATION_MS);
    z=0;
    x=seconds;
    
  }
}

void drawclock(){
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

  

  //every second, a pulse crosses the whole strip end to end  
  static int o=0;
  static int d=0;
  if (o!=10){
    ring.setPixelColor(o, milli_color); //pulse colour    
    o++;
  }
  if (d!=seconds){
    o=0;
    d=seconds;
    
  }  
}

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
