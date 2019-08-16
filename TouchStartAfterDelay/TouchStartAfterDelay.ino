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

  if (touchStatus != TOUCH_NO_CHANGE) {

    if (touchStatus = NEW_TOUCH) {

      bt.log_action("pin touched: ", trackNumber);
      bt.turnLedOn();

      int playerStatus = bt.getPlayerStatus();

      if (playerStatus == IS_PLAYING) {
	if (lastPlayed == trackNumber) {
	  bt.pauseTrack(trackNumber);
	} else {
	  bt.queueTrackToStartAfterDelay(trackNumber);
	}
      }
      else if (playerStatus == IS_PAUSED) {
	if (trackNumber == lastPlayed) {
	  bt.resumeTrack(trackNumber);
	} else {
	  bt.queueTrackToStartAfterDelay(trackNumber);
	}
      }
      else if (playerStatus == IS_STOPPED) {
	bt.queueTrackToStartAfterDelay(trackNumber);
      }     
    }
    else if (touchStatus == NEW_RELEASE) {
      bt.log_action("pin released: ", trackNumber);
      bt.turnLedOff();
    } 
  }

  bt.doLoopTasks();

}
