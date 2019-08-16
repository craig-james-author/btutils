/* -*-C++-*- */

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

BtUtils bt(&sd, &MP3player);

void setup() {
}

void loop() {

  int trackNumber;
  int touchStatus = bt.getPinTouchStatus(&trackNumber);
  int lastPlayed  = bt.getLastTrackPlayed();

  if (touchStatus = NEW_TOUCH) {

    int playerStatus = bt.getPlayerStatus();

    if (playerStatus == IS_PLAYING) {
      if (lastPlayed == trackNumber) {
	bt.pauseTrack();
      } else {
	bt.queueTrackToStartAfterDelay(trackNumber);
      }
    }
    else if (playerStatus == IS_PAUSED) {
      if (trackNumber == lastPlayed) {
	bt.resumeTrack();
      } else {
	bt.queueTrackToStartAfterDelay(trackNumber);
      }
    }
    else if (playerStatus == IS_STOPPED) {
      bt.queueTrackToStartAfterDelay(trackNumber);
    }     
  }

  // We're using delays and fade in, so must call the timer tasks
  bt.doTimerTasks();

}
