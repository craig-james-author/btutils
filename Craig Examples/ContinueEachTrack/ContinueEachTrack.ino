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

  bt->setFadeInTime(2000);    // 2 second fade-in

  // Same for the fade-out time.

  bt->setFadeOutTime(2000);   // 2 second fade-out

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
  if (touchStatus == NEW_TOUCH) {
    if (bt->getPlayerStatus() == IS_PAUSED && trackNumber == lastPlayed) {
      bt->resumeTrack();
      Serial.println("Resume track");
    } else {
      bt->startTrack(trackNumber, trackPosition[trackNumber]);
      Serial.print("Start track ");
      Serial.print(trackNumber);
      Serial.print(" at ");
      Serial.println(trackPosition[trackNumber]);
    }
  }

  // Pause the track as soon as the release is detected. Notice that
  // getCurrentTrackLocation() returns the location *difference* from
  // where you last started, not the absolute position, so we have to
  // add it to the last track-position value.

  else if (touchStatus == NEW_RELEASE) {
    trackPosition[lastPlayed] += bt->getCurrentTrackLocation();
    bt->pauseTrack();
    Serial.print("Pause track at: ");
    Serial.println(trackPosition[lastPlayed]);
  } 

  bt->doTimerTasks();
}
