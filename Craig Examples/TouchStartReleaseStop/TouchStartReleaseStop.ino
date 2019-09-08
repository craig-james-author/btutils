#include "Compiler_Errors.h"
#include "BtUtils.h"
#include <MPR121.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h> 
#include <SFEMP3Shield.h>

SdFat sd;
SFEMP3Shield MP3player;

BtUtils *bt;

void setup() {
  bt = BtUtils::setup(&sd, &MP3player);
}

void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // Plays while being touched, stops when released. Each track starts at the beginning.

  if (touchStatus == NEW_TOUCH) {
    bt->startTrack(trackNumber);
  }
  else if (touchStatus == NEW_RELEASE) {
    bt->stopTrack();
  } 
}
