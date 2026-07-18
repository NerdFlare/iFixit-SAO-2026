/*
             *************             
         *********************         
       *************************       
     *****************************     
   *********************************   
  *********   ***********   *********  
 *********       *****       ********* 
 **********                 ********** 
************               ************
**************           **************
**************           **************
************               ************
 **********                 ********** 
 *********       *****       ********* 
  *********   **********    *********  
   *********************************   
     *****************************     
       *************************       
         *********************         
             *************
            iFixit SAO 2026
              Version 0.1
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <tinyNeoPixel.h>

//Lightning Bolt
const int D1 = PIN_PB0; //Top
const int D2 = PIN_PB1; //Bottom
//Soldering iron tip
const int D3 = PIN_PA4;
//Button
const int BTN = PIN_PA1;
//Soldering Game
const int R5 = PIN_PA6;
const int R7 = PIN_PA7;
const int R9 = PIN_PB3;
const int R11 = PIN_PB2;
//Neopixels
const int NEOPIXEL = PIN_PA5;
const int PIXEL_COUNT = 7;
tinyNeoPixel strip = tinyNeoPixel(PIXEL_COUNT, NEOPIXEL, NEO_GRB);

//Animation modes
int num_modes_addr = 255;
int NUM_MODES = EEPROM.read(num_modes_addr);
int mode_addr = 254;
int MODE = EEPROM.read(mode_addr);

//Button pressed
bool pressed = true;

//fire aniamtion variables
unsigned long lastNeopixelUpdate = 0;
const int frameDelay = 40; // ms between animation frames
uint8_t heat[PIXEL_COUNT];

//rainbow animation
int rainbox_index = 0;

//D3 LED Tip
unsigned long lastD3Update = 0;
int brightness = 0;  // how bright the LED is
int fadeAmount = 1;  // how fast do we fade

//D1, D2 Lightning animation variables
enum State {
  WAITING,
  TOP_ON,
  BOTTOM_ON,
  TOP_DIM,
  FLASH,
  IMPACT,
  FADE_OUT,
  FLICKER
};
State state = WAITING;
unsigned long stateStart = 0;
unsigned long waitTime = 1000;
int fadeValue = 255;

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Converts a heat value (0-255) to a blue flame color
uint32_t propane(uint8_t temperature) {

  // Scale temperature
  uint8_t t = (temperature * 255) / 255;

  // Very cool = off/dim blue
  if (t < 60) {
    return strip.Color(0, 0, t * 2);
  }

  // Medium = deep blue
  else if (t < 140) {
    uint8_t b = 150 + (t - 60);
    uint8_t g = (t - 60) / 3;
    return strip.Color(0, g, min(255, b));
  }

  // Hot = cyan/blue-white core
  else if (t < 220) {
    uint8_t g = 80 + (t - 140);
    uint8_t b = 255;
    uint8_t r = (t - 140) / 8;
    return strip.Color(r, min(255, g), b);
  }

  // Hottest = small white core
  else {
    uint8_t w = t - 220;
    return strip.Color(40 + w, 180 + w, 255);
  }
}

// Converts a heat value (0-255) to a yellow flame color
uint32_t candle(uint8_t temperature) {

  uint8_t t192 = (temperature * 191) / 255;

  uint8_t heatramp = (t192 & 0x3F) << 2;

  // Cool -> red
  if (t192 > 0x80) {
    // Hot -> red/yellow
    return strip.Color(255, 50 + (heatramp/3), 0);
  }
  else if (t192 > 0x40) {
    // Medium -> orange/yellow
    return strip.Color(255, heatramp + 40, 0);
  }
  else {
    // Low -> dark red
    return strip.Color(heatramp, 0, 0);
  }
}

void updateFire(bool which) {

  // Step 1: Cool down every cell a little
  for (int i = 0; i < PIXEL_COUNT; i++) {
    int cooldown = random(0, 25);
    heat[i] = max(0, heat[i] - cooldown);
  }

  // Step 2: Heat drifts upward
  for (int i = PIXEL_COUNT - 1; i >= 2; i--) {
    heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
  }

  // Step 3: Random sparks near the "hot" end
  if (random(255) < 160) {
    int y = random(2); // ignite near the base
    heat[y] = min(255, heat[y] + random(160, 255));
  }

  // Step 4: Convert heat to LED colors
  for (int i = 0; i < PIXEL_COUNT; i++) {
    if(which){
      strip.setPixelColor(i, propane(heat[i]));
    }else{
      strip.setPixelColor(i, candle(heat[i]));
    }
  }

  strip.show();
}


void setup() {

  pinMode(PIN_PB0, OUTPUT);
  pinMode(PIN_PB1, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(NEOPIXEL, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(R5, INPUT_PULLUP);
  pinMode(R7, INPUT_PULLUP);
  pinMode(R9, INPUT_PULLUP);
  pinMode(R11, INPUT_PULLUP);
        
  strip.setBrightness(25);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0,0,0);
  }
  strip.show(); // Initialize all neopixels to 'off'

  if (NUM_MODES > 4){ NUM_MODES = 3;}
  if (MODE > NUM_MODES){ MODE = 0;}

  analogWrite(D1, 0);
  analogWrite(D2, 0);
  analogWrite(D3, 0);

}

void loop() {
  
  unsigned long now = millis();

  //Soldering Game (enable bonus mode)
  if(NUM_MODES == 3 && digitalRead(R5) == LOW && digitalRead(R7) == LOW && digitalRead(R9) == LOW && digitalRead(R11) == LOW){
    NUM_MODES = 4;
    MODE = 3;
    EEPROM.update(num_modes_addr,NUM_MODES);
  }

  //Handle Button
  if (digitalRead(BTN) == LOW && !pressed){
    pressed = true;
    MODE = (MODE + 1) % NUM_MODES;
    EEPROM.update(mode_addr,MODE);
    delay(20);     // Short delay to debounce button.
  }
  if (digitalRead(BTN) == HIGH){
    pressed = false;
    delay(20);
  }

  //Fade Red LED
  if(now - lastD3Update >= 10 && MODE != 2){
    lastD3Update = now;
    analogWrite(D3, brightness);
    brightness = brightness + fadeAmount;
    if (brightness <= 0 || brightness >= 128) {
      fadeAmount = -fadeAmount;
    }
  }else{
    analogWrite(D3, 0);
  }

  //Animate Neopixels
  switch (MODE){
    case 0:
    //Candle Animation
      if (now - lastNeopixelUpdate >= frameDelay) {
        lastNeopixelUpdate = now;
        updateFire(false);
      }
      break;
    case 1:
    //Propane Animation
      if (now - lastNeopixelUpdate >= frameDelay) {
        lastNeopixelUpdate = now;
        updateFire(true);
      }
      break;
    case 2:
    //Iron Off
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0);
      }
      strip.show();
      break;
    case 3:
    //Rainbox Animation (BONUS)
      if (now - lastNeopixelUpdate >= 10){
        lastNeopixelUpdate = now;
        for (int i = strip.numPixels() - 1; i >= 0; i--) {
          strip.setPixelColor(i, Wheel(((i * 64) + rainbox_index) & 255));
          rainbox_index = (rainbox_index+1)%256;
        }
        strip.show();
      }
      break;
  }

  //Animate Lightening Bolt
  switch (state) {
    case WAITING:
      if (now - stateStart >= waitTime) {
        analogWrite(D1, 255);
        analogWrite(D2, 0);
        state = TOP_ON;
        stateStart = now;
      }
      break;
    case TOP_ON:
      // Slower initial buildup
      if (now - stateStart >= 120) {
        analogWrite(D2, 255);
        state = BOTTOM_ON;
        stateStart = now;
      }
      break;
    case BOTTOM_ON:
      if (now - stateStart >= 150) {
        analogWrite(D1, 80);
        state = TOP_DIM;
        stateStart = now;
      }
      break;
    case TOP_DIM:
      if (now - stateStart >= 120) {
        analogWrite(D1, 255);
        analogWrite(D2, 255);
        state = FLASH;
        stateStart = now;
      }
      break;
    case FLASH:
      if (now - stateStart >= 80) {
        analogWrite(D1, 0);
        analogWrite(D2, 255);
        state = IMPACT;
        stateStart = now;
      }
      break;
    case IMPACT:
      // Hold the bottom glow longer
      if (now - stateStart >= 350) {
        fadeValue = 255;
        state = FADE_OUT;
        stateStart = now;
      }
      break;
    case FADE_OUT:
      // Gradual fade every 20ms
      if (now - stateStart >= 20) {
        fadeValue -= 15;
        if (fadeValue < 0) {
          fadeValue = 0;
        }
        analogWrite(D2, fadeValue);
        stateStart = now;
        if (fadeValue == 0) {
          // Optional secondary flicker
          if (random(100) < 40) {
            analogWrite(D1, 120);
            analogWrite(D2, 200);
            state = FLICKER;
            stateStart = now;
          } else {
            state = WAITING;
            stateStart = now;
            waitTime = random(2000, 5000);
          }
        }
      }
      break;
    case FLICKER:
      if (now - stateStart >= 120) {
        analogWrite(D1, 0);
        analogWrite(D2, 0);
        state = WAITING;
        stateStart = now;
        waitTime = random(2000, 5000);
      }
      break;
  }
}

// <3 znjp 

