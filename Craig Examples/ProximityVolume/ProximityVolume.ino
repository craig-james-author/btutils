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

  // Set up the TouchBoard for proximity sensing. Only one pin is used for
  // proximity sensing; this specifies which pin to use.

  bt->setProximitySensingPinNumber(0);    // pin zero

  // Set the volume to zero initially so that nothing sounds until
  // your hand gets near the proximity pin (pin zero that we set above).

  bt->setVolume(0);

}

void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // Touches on pins zero are ignored (we're using it for proximity
  // sensing). Touches on pins 1 through 11 alternately start and stop the
  // track playing, just like the TouchStartTouchStop example. Note that
  // you may not hear anything initially; sound will become apparent as
  // your hand gets near the proximity sensing pin.

  if (touchStatus == NEW_TOUCH) {
    if (trackNumber != 0) {     // ignore track zero, we're using it for proximity sensing.
      if (bt->getPlayerStatus() == IS_PLAYING) {
        bt->stopTrack();
      } else {
        bt->startTrack(trackNumber);
      }
    }
  }

  // Each time through the looop, check the proximity, and set the volume
  // accordingly. The proximity returns 0 (far away) to 100 (very close),
  // so we just use this for the volume, which is also 0 (off) to 100 (full
  // volume). The sensor's concept of "far" seems to be anything more than
  // 1/2 inch or so, so your finger has to get pretty close before you hear
  // anything.

  if (bt->getPlayerStatus() == IS_PLAYING) {
    int proximity = bt->getProximityPercent();
    bt->setVolume(proximity);
  }

}
