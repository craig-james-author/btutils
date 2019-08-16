// -*- C++ -*-

// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h> 
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int currentlyPlaying = -1;

// See getTouchAction() and related, below

int lastTouchArray[12];
unsigned long lastTouchTime = 0;
int lastTouch = -1;

#define NO_CHANGE  0x0100
#define SLIDE_UP   0x0200
#define SLIDE_DOWN 0x0400
#define KEY_DOWN   0x0800
#define KEY_UP     0x1000

#define IS_NO_CHANGE(tidx)        (tidx & NO_CHANGE)
#define IS_TOUCH_SLIDE_UP(tidx)   (tidx & SLIDE_UP)
#define IS_TOUCH_SLIDE_DOWN(tidx) (tidx & SLIDE_DOWN)
#define IS_TOUCH_START(tidx)      (tidx & KEY_DOWN)
#define IS_TOUCH_STOP(tidx)       (tidx & KEY_UP)
#define WHICH_PIN(tidx)           (tidx & 0xFF)

// touch behaviour definitions
#define firstPin 0
#define lastPin 11

// sd card instantiation
SdFat sd;

void setup() {  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) ; {} //uncomment when using the serial monitor 
  Serial.println("Craig's experimental frequency slider");

  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  result = MP3player.begin();
  MP3player.setVolume(10,10);
 
  if (result != 0)  {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }
   
  for (int i = 0; i < 12; i++) {
    lastTouchArray[i] = 0;
  }

  Serial.println("Setup finished");
}

void loop() {
  readTouchInputs();
}

void readTouchInputs() {
    
  int result = getTouchAction();
  if (IS_NO_CHANGE(result)) {
    return;
  }

  int pin;
  if (IS_TOUCH_START(result)) {

    pin = WHICH_PIN(result);
        
    Serial.print("pin ");
    Serial.print(pin);
    Serial.println(" was just touched");
    digitalWrite(LED_BUILTIN, HIGH);
            
  } else if (IS_TOUCH_STOP(result)) {

    pin = WHICH_PIN(result);
    //    Serial.print("pin ");
    //    Serial.print(pin);
    //    Serial.println(" is no longer being touched");
    digitalWrite(LED_BUILTIN, LOW);

  } else if (IS_TOUCH_SLIDE_UP(result)) {
    
    pin = WHICH_PIN(result);
    Serial.print("pin ");
    Serial.print(pin);
    Serial.println(" slide up");

  } else if (IS_TOUCH_SLIDE_DOWN(result)) {

    pin = WHICH_PIN(result);
    Serial.print("pin ");
    Serial.print(pin);
    Serial.println(" slide down");
  }
  
}

byte getTouchArray(int *touchArray) {

  if (!MPR121.touchStatusChanged()) {
    return 0;
  }
  MPR121.updateTouchData();

  byte changed = 0;
  for (int i = 0; i < 12; i++) {
    if (MPR121.isNewTouch(i)) {
      touchArray[i] = 1;
      //      Serial.print("pin touch ");
      //      Serial.println(i);
      changed += lastTouchArray[i] != 1 ? 1 : 0;
    } else if (MPR121.isNewRelease(i)) {
      touchArray[i] = -1;
      //      Serial.print("pin release ");
      //      Serial.println(i);
      changed += lastTouchArray[i] != -1 ? 1 : 0;
    } else {
      touchArray[i] = 0;
    }
  }
  //  Serial.print("changed:");
  //  Serial.println(changed);

  //  Serial.print("old: ");
  //  printTouchArray(lastTouchArray);
  //  Serial.print("new: ");
  //  printTouchArray(touchArray);

  return changed;
}

void saveAsLastTouchArray(int *touchArray) {
  for (int i = 0; i < 12; i++) {
    lastTouchArray[i] = touchArray[i];
  }
}

void printTouchArray(int *touchArray) {
  for (int i = 0; i < 12; i++) {
    Serial.print(touchArray[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

int getTouchAction() {

  int touchArray[12];

  byte changed = getTouchArray(touchArray);
  if (!changed) {
    return NO_CHANGE;
  }

  int touchAction = NO_CHANGE;

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastTouchTime;

  for (int i = 0; i < 12; i++) {
    if (touchArray[i] != lastTouchArray[i]) {
      if (touchArray[i] == 1) {
	lastTouchTime = currentTime;
	if (elapsedTime <= 1000) {
	  if (i >= lastTouch) {
	    touchAction = (SLIDE_UP | i);
	  } else {
	    touchAction = (SLIDE_DOWN | i);
	  }
	  lastTouch = i;
	}
      } else {
	lastTouch = -1;
      }
      if (touchAction == NO_CHANGE) {
	if (touchArray[i] == 1) {
	  touchAction =  KEY_DOWN | i;
	  lastTouch = i;
	} else {
	  touchAction = KEY_UP | i;
	  lastTouch = -1;
	}
      }
      break;
    }
  }

  if (changed) {
    saveAsLastTouchArray(touchArray);
  }

  return touchAction;
}
