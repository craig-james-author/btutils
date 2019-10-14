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

  // bt->setFadeInTime(2000);    // 2 second fade-in

  // Same for the fade-out time.

  // bt->setFadeOutTime(2000);   // 2 second fade-out

  for (int i = 0; i <= LAST_PIN; i++) {
    trackPosition[i] = 0;
  }
}


void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  int lastPlayed  = bt->getLastTrackPlayed();
  if (touchStatus == NEW_TOUCH) {
    int currentLocation = bt->getCurrentTrackLocation();
    if (currentLocation == 0) {
      trackPosition[lastPlayed] = 0;
    } else {
      trackPosition[lastPlayed] += currentLocation;
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
  else if (touchStatus == NEW_RELEASE) {
    bt->turnLedOff();
  }

  bt->doTimerTasks();
}
