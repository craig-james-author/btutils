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

  // Set the fade-in time in milliseconds (e.g. 1000 is 1 second). When a track is started,
  // the volume is initially zero, and it takes this much time to reach full volume.

  bt->setFadeInTime(2000);    // 2 second fade-in

  // Same for the fade-out time.

  bt->setFadeOutTime(1000);   // 1 second fade-out
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

  bt->doTimerTasks();
}
