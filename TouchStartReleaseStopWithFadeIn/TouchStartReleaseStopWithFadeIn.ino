/* -*-C++-*- */

#include "BtUtils.h"
#include <MPR121.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h> 
#include <SFEMP3Shield.h>

SdFat sd;
SFEMP3Shield MP3player;

BtUtils bt(&sd, &MP3player);

void setup() {
  bt.setFadeInTime(1000);	// 1 second (in milliseconds)
}

void loop() {

  int trackNumber;
  int touchStatus = bt.getPinTouchStatus(&trackNumber);

  if (touchStatus = NEW_TOUCH) {
    bt.startTrack(trackNumber);
  }
  else if (touchStatus == NEW_RELEASE) {
    bt.stopTrack();
  } 

  bt.doTimerTasks();
}