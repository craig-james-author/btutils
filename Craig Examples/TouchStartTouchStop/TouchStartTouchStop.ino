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

  // This is simple: touches alternately start and stop the track playing.
  // Each track starts from the beginning each time. Note that if a track
  // is playing, any touch on any pin will stop it; it doesn't have to be
  // the same pin that started it.

  if (touchStatus == NEW_TOUCH) {
    if (bt->getPlayerStatus() == IS_PLAYING) {
      bt->stopTrack();
    } else {
      bt->startTrack(trackNumber);
    }
  }
}
