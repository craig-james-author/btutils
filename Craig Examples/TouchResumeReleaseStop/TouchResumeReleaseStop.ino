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
  
  // This sets an "idle timeout" -- if nothing hapens for this length of time, it clears
  // out the resume feature so that the next time you call bt->resume(), it will start the
  // track over instead.  The timeout is in seconds.
  bt->startOverAfterNoTouchTime(10);

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

}
