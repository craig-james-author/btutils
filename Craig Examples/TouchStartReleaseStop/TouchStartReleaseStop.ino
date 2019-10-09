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

  // Set the touch sensitivity. Low values make it very sensitive (i.e. it
  // will trigger a touch even when your hand is nearby), and high valuse
  // make it less sensitive (i.e. you have to actually touch the contact).
  // The first number is touch, the second number is release. Touch <i>must</i>
  // be greater than release.

  bt->setTouchReleaseThreshold(10, 5);

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
