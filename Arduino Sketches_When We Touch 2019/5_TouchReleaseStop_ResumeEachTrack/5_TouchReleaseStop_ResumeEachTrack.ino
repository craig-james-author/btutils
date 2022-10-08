// Touching the sensor starts the track. Releasing it stops it. Touching the same sensor, resumes
// where stopped. Touching a new, starts it. Touching previous resumes in place.  Needs one second
// of something at the start of track -- can be silence or a sound such a waves, bell, or something
// that relates to the content of the track.

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

uint32_t trackPosition[12];

void setup() {
  bt = BtUtils::setup(&sd, &MP3player);

  // Set the fade-in time in milliseconds (e.g. 1000 is 1 second). When a track is started,
  // the volume is initially zero, and it takes this much time to reach full volume.

  //Set the fade in time here (uncomment first to make to enable)
  // bt->setFadeInTime(2000);    // 2 second fade-in

  // Set the fade-out time here (uncomment first to enable):
  // bt->setFadeOutTime(2000);   // 2 second fade-out

  // Set the output volume (left and right). This ranges from zero (silent) to
  // 100 (full volume).  The default is 100 (i.e. if you don't call this
  // function at all, the volume will be 100%).

  bt->setVolume(50, 50);

  // Set the touch sensitivity. Low values make it very sensitive (i.e. it
  // will trigger a touch even when your hand is nearby), and high valuse
  // make it less sensitive (i.e. you have to actually touch the contact).
  // The first number is touch, the second number is release. Touch <i>must</i>
  // be greater than release.

  bt->setTouchReleaseThreshold(5,2);

  
  for (int i = 0; i <= LAST_PIN; i++) {
    trackPosition[i] = 0;
  }
}


void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // If a new touch is detected:
  //   - if it's the same track as before, resume playing where it left off.
  //   - if it's a different track, start it from the beginning.

  int lastPlayed  = bt->getLastTrackPlayed();
  int currentLocation = bt->getCurrentTrackLocation();
  int playerStatus = bt->getPlayerStatus();
  if (touchStatus == NEW_TOUCH) {
    if (playerStatus == IS_PAUSED && trackNumber == lastPlayed) {
      bt->resumeTrack();
      //bt->log_action("Resume track:", lastPlayed);
    } else {
      if (playerStatus == IS_PLAYING) {
        trackPosition[lastPlayed] += currentLocation;
      } else if (playerStatus == IS_STOPPED) {
	trackPosition[lastPlayed] = 0;
      }
      bt->startTrack(trackNumber, trackPosition[trackNumber]);
      //bt->log_action("Continue track: ", trackNumber);
    }
    bt->turnLedOn();
  }

  // Pause the track if a release is detected. Notice that
  // getCurrentTrackLocation() returns the location difference from where
  // you last started, not the absolute position, so we have to add it to
  // the last track-position value.

  else if (touchStatus == NEW_RELEASE) {
    if (playerStatus == IS_STOPPED) {
      trackPosition[lastPlayed] = 0;
      bt->stopTrack();
      //bt->log_action("End of track, stopped: ", lastPlayed);
    } else {
      trackPosition[lastPlayed] += currentLocation;
      bt->pauseTrack();
      //bt->log_action("Pause track: ", lastPlayed);
    }
    bt->turnLedOff();
  }

  // This last sectoin turns the LED off if the end of the track is reached
  // while the pin is still being touched.
  else if (playerStatus == IS_STOPPED) {
    bt->turnLedOff();
  }
  bt->doTimerTasks();
}
