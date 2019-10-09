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

  // Set up the TouchBoard for proximity sensing. This changes the sensitivity
  // of the board and changes its filtering characteristics so that slow hand
  // movements are detected correctly.

  bt->setProximitySensingMode();

  // Set the volume to zero initially so that nothing sounds until
  // your hand gets near the proximity pin (pin zero that we set above).

  bt->setVolume(0);

}

void loop() {


  int highestProximity = 0;
  int highestProximityPin = -1;


  // Find the pin with the highest proximity reading.
  for (int pin = FIRST_PIN; pin <= LAST_PIN; pin++) {
    int proximity = bt->getProximityPercent(pin);
    if (proximity > highestProximity) {
      highestProximity = proximity;
      highestProximityPin = pin;
    }
  }

  // What's currently going on? (IS_PLAYING, IS_PAUSED, or IS_STOPPED)
  int playerStatus = bt->getPlayerStatus();

  // Which track was last played?
  int lastTrack = bt->getLastTrackPlayed();

  // If nothing is near (proximity is zero) and a track is playing, pause it.
  if (highestProximity == 0) {
    if (playerStatus == IS_PLAYING) {
      BtUtils::log_action("pause: ", lastTrack);
      bt->pauseTrack();
    }
    bt->setVolume(0);
  }

  // else -- proximity was detected
  else {

    // Set the volume. The proximity is in percentage 0-100, and the volume is
    // also 0-100, so we can just set the volume to the proximity number.
    bt->setVolume(highestProximity);
    BtUtils::log_action("setVolume: ", highestProximity);

    // If it's already playing but this is a different pin, switch tracks.
    // If it's the same pin, we don't have to do anything.
    if (playerStatus == IS_PLAYING) {
      if (highestProximityPin != lastTrack) {
	bt->startTrack(highestProximityPin);
      }
    }

    else if (playerStatus == IS_PAUSED) {

      // If it's paused and this is the same track, resume playing
      if (highestProximityPin == lastTrack) {
	bt->resumeTrack();
      }

      // If it's paused and this is a different track, switch to the new track
      else {
	bt->startTrack(highestProximityPin);
      }

    }

    // If it's currently stopped, start this track
    else if (playerStatus == IS_STOPPED) {
      bt->startTrack(highestProximityPin);
    }

  }

}