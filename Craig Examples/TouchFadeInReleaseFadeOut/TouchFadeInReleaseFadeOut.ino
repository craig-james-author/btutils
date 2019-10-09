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

  bt->setFadeOutTime(2000);   // 2 second fade-out
}


void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // If a new touch is detected:
  //   - if it's the same track as before, resume playing where it left off.
  //   - if it's a different track, start it from the beginning.

  if (touchStatus == NEW_TOUCH) {
    int lastPlayed  = bt->getLastTrackPlayed();
    if (bt->getPlayerStatus() == IS_PAUSED && trackNumber == lastPlayed) {
      bt->resumeTrack();
    } else {
      bt->startTrack(trackNumber);
    }
  }

  // Pause the track as soon as the release is detected.

  else if (touchStatus == NEW_RELEASE) {
    bt->pauseTrack();
  } 

  bt->doTimerTasks();
}
