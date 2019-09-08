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

  // Set the delay time in milliseconds (e.g. 1000 is 1 second). When a track is started,
  // the actual sound won't start until this time has elapsed.

  bt->setStartDelay(1500);
}

void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);
  int lastPlayed  = bt->getLastTrackPlayed();

  if (touchStatus == NEW_TOUCH) {

    int playerStatus = bt->getPlayerStatus();

    // If the player is already playing, pause the track. Otherwise, queue the track,
    // which will cause it to play after the time set in setStartDelay(), above.

    if (playerStatus == IS_PLAYING) {
      if (lastPlayed == trackNumber) {
        bt->pauseTrack();
      } else {
        bt->queueTrackToStartAfterDelay(trackNumber);
      }
    }

    // If the track is paused, resume playing. If this is the same track
    // that was playing before, resume playing it; if it's a different
    // track than the one that was playing, queue it for delayed start. (Note
    // that the track resumes immediately, not after a delay. Only a newly
    // selected track is queued for delay.)

    else if (playerStatus == IS_PAUSED) {
      if (trackNumber == lastPlayed) {
        bt->resumeTrack();
      } else {
        bt->queueTrackToStartAfterDelay(trackNumber);
      }
    }
    else if (playerStatus == IS_STOPPED) {
      bt->queueTrackToStartAfterDelay(trackNumber);
    }     
  }

  // When using delays and/or fade in, this must be at the end of the loop() function.
  bt->doTimerTasks();
}
