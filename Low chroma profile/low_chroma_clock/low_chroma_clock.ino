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
#define NUMPIXELS      16
#define PIN            9

//Neopixels on the Circuit Playground
#define N_PIXELS       9  // Number of pixels in strand
#define LED_PIN        12  // NeoPixel LED strand is connected to this pin

// Marker Neopixels
#define MARKPIN        6

// Light sensor definitions
#define ANALOG_INPUT  A5
#define VALUE_MIN     0
#define VALUE_MAX     5

/////////////////////////////////
#define MIC_PIN        A4
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG 24 //Time of pause before peak dot falls
#define PEAK_FALL 4 //Rate of falling peak dot
#define INPUT_FLOOR 10 //Lower range of analogRead input
#define INPUT_CEILING 300 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)

byte peak = 16;      // Peak level of column; used for falling dots
unsigned int sample;
byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot
////////////////////////////////


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
uint32_t hour_color    = pixels.Color ( 100, 100, 120);
uint32_t minutes_color = pixels.Color ( 70, 70, 100);
uint32_t second_color  = pixels.Color ( 30, 30, 50);
uint32_t marker_color  = pixels.Color ( 10, 10, 20);
uint32_t ring_color    = pixels.Color ( 1, 1, 2);
uint32_t strip_color   = pixels.Color ( 0, 0, 0);
uint32_t cylon_color   = pixels.Color ( 10, 10, 25);

uint32_t off_color     = pixels.Color ( 0, 0, 0);

bool hashadlock= true;

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
    vumeter();
    

    //if we're not inhibited, refresh the display
    //if(dispInhibit == false) pixels.show(); // This sends the updated pixel color to the hardware.
    pixels.show();
    markers.show();
    ring.show();
    strip.show();
  
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
  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light/15)+1);
  
  //Sets all neopixels blank
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, off_color);
    markers.setPixelColor(i, light*4, light*4, light*8);
    ring.setPixelColor(i, light/5, light/5, light/3);
    strip.setPixelColor(i, strip_color);
  }
}

/*
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
    //CircuitPlayground.playTone(320, TONE_DURATION_MS);
    z=0;
    x=seconds;
    
  }
}
*/

void drawclock(){

  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light/15)+1);
  
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
  if (currentMillis - previousMillis >= interval) {
    previousMillis += interval; 
    static int o=0;
    static int d=0;
    if (o!=10){
      ring_color(o, light*10, light*10, light*20); //pulse colour    
      o++;
    }
    if (d!=seconds){
      o=0;
      d=seconds;
    }
  }
  datashow();
   
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

void ring_color (uint8_t position2, uint32_t color)
{
  uint32_t blended_color2 = blend (ring.getPixelColor (position2), color);
 
  /* Gamma mapping */
  uint8_t r,b,g;
 
  r = (uint8_t)(blended_color2 >> 16),
  g = (uint8_t)(blended_color2 >>  8),
  b = (uint8_t)(blended_color2 >>  0);
 
  ring.setPixelColor (position2, blended_color);
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

void datashow(){

  // Light sensor values
  uint16_t light = analogRead(ANALOG_INPUT);
  light = ((light/5)+1);

  //Button values
  uint16_t buttonL = digitalRead(BUTTONL);
  uint16_t buttonR = digitalRead(BUTTONR);
  uint16_t buttonS = digitalRead(BUTTONS);
  uint16_t buttonstate = 1;
  
  static unsigned long previousMillis = 0;
  static unsigned long previousMillis2 = 0;
  unsigned long currentMillis = millis();
  const long interval = 100;
  const long interval2 = 0;
  int u=(random(0,3));
  int i=(random(0,3));
  int o=(random(0,3));
  int p=(random(0,3));
  
  if (buttonS == HIGH){
        strip.setPixelColor(0, 255, 255, 255);
        strip.setPixelColor(1, 255, 255, 255);
        strip.setPixelColor(2, 255, 255, 255);
        strip.setPixelColor(3, 255, 255, 255);
        strip.setPixelColor(4, 255, 255, 255);
        strip.setPixelColor(5, 255, 255, 255);
        strip.setPixelColor(6, 255, 255, 255);
        strip.setPixelColor(7, 255, 255, 255);
        strip.setPixelColor(8, 255, 255, 255);
      }else{
      
    if (currentMillis - previousMillis2 >= interval2) {
    previousMillis2 += interval2;
    strip.setPixelColor(4, 15, 15, 30);
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
  if (buttonL == HIGH){
        strip.setPixelColor(3, 0, 0, 10);
        strip.setPixelColor(5, 0, 0, 0);
        buttonstate == 3;
      }
  if (buttonR == HIGH){
        strip.setPixelColor(3, 0, 0, 0);
        strip.setPixelColor(5, 0, 0, 10);
        buttonstate == 1;
      }
  //strip.show();
  }
}

void vumeter(){
  unsigned long startMillis= millis();  // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level
 
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int c, y;
 
 
  // collect data for length of sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
 
  // Serial.println(peakToPeak);
 
 
  //Fill the ring with rainbow gradient
  for (int i=0;i<=ring.numPixels()-1;i++){
    ring_color(i, 10, 0, 0);
  }
 
 
  //Scale the input logarithmically instead of linearly
  c = fscale(INPUT_FLOOR, INPUT_CEILING, ring.numPixels(), 0, peakToPeak, 2);
 
  
 
 
  if(c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if (c <= ring.numPixels()) { // Fill partial column with off pixels
    drawLine(ring.numPixels(), ring.numPixels()-c, ring.Color(0, 0, 0));
  }
 
  // Set the peak dot to match the rainbow gradient
  y = ring.numPixels() - peak;
  
  ring_color(y-1,0, 0, 0);
 
  ring.show();
 
  // Frame based peak dot animation
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate 
      peak++;
      dotCount = 0;
    }
  } 
  else {
    dotHangCount++; 
  }
}
 
//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    ring_color(i, c);
  }
}
 
 
float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){
 
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;
 
 
  // condition curve parameter
  // limit range
 
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;
 
  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function
 
  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution  
   Serial.println(); 
   */
 
  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }
 
  // Zero Refference the values
  OriginalRange = originalMax - originalMin;
 
  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }
 
  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float
 
  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }
 
  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
 
  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }
 
  return rangedValue;
}


