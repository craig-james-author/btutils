// Touch starts track. When released the track continues until another track is touched. If returned to previous track, it resumes where it left off in place. No time out.
// Needs one second of something at the start of track -- can be silence or a sound such a waves, bell, or something that relates to the content of the track.

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

  // Set the output volume (left and right). This ranges from zero (silent) to
  // 100 (full volume).  The default is 100 (i.e. if you don't call this
  // function at all, the volume will be 100%).

  // bt->setVolume(100, 100);


  // Set the fade-in time in milliseconds (e.g. 1000 is 1 second). When a track is started,
  // the volume is initially zero, and it takes this much time to reach full volume.

  // bt->setFadeInTime(2000);    // 2 second fade-in

  // Same for the fade-out time.

  // bt->setFadeOutTime(2000);   // 2 second fade-out

  // Set the touch sensitivity. Low values make it very sensitive (i.e. it
  // will trigger a touch even when your hand is nearby), and high valuse
  // make it less sensitive (i.e. you have to actually touch the contact).
  // The first number is touch, the second number is release. Touch must
  // be greater than release.

   bt->setTouchReleaseThreshold(10, 8);

  for (int i = 0; i <= LAST_PIN; i++) {
    trackPosition[i] = 0;
  }
}


void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);
  int lastPlayed  = bt->getLastTrackPlayed();
  int currentLocation = bt->getCurrentTrackLocation();
  int playerStatus = bt->getPlayerStatus();

  if (playerStatus == IS_STOPPED) { // Reached the end of a track?
    bt->turnLedOff();
    trackPosition[lastPlayed] = 0;
    bt->stopTrack();
  }
  if (touchStatus == NEW_TOUCH) {
    if (trackNumber != lastPlayed) {
      if (playerStatus == IS_PLAYING) {
	trackPosition[lastPlayed] += currentLocation;
      } else {
	trackPosition[lastPlayed] = 0;
      }
      bt->startTrack(trackNumber, trackPosition[trackNumber]);
      bt->turnLedOn();

      Serial.print("Pause track ");
      Serial.print(lastPlayed);
      Serial.print(" at ");
      Serial.println(trackPosition[lastPlayed]);
      Serial.print("Start track ");
      Serial.print(trackNumber);
      Serial.print(" at ");
      Serial.println(trackPosition[trackNumber]);
    }
  }
  else if (touchStatus == NEW_RELEASE) {
    bt->turnLedOff();
  }

  bt->doTimerTasks();
}
