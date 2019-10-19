//Good for testing sensor connection. Touch it starts, touch it stops.

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

  // Set the output volume (left and right). This ranges from zero (silent) to
  // 100 (full volume).  The default is 100 (i.e. if you don't call this
  // function at all, the volume will be 100%).

  bt->setVolume(100, 100);


  // Set the fade-in and fade-out times in milliseconds (e.g. 1000 is 1
  // second). When a track is started, the volume is initially zero, and it
  // takes this much time to reach the volume. Note that whatever you set
  // the volume to (see above) is the target that the fade-in is heading
  // for in the specified time.

  bt->setFadeInTime(3000);    // 1 second fade-in
  bt->setFadeOutTime(2000);   // 2 second fade-out


  // Set the touch sensitivity. Low values make it very sensitive (i.e. it
  // will trigger a touch even when your hand is nearby), and high valuse
  // make it less sensitive (i.e. you have to actually touch the contact).
  // The first number is touch, the second number is release. Touch <i>must</i>
  // be greater than release.

  // bt->setTouchReleaseThreshold(40, 20);
}

void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // This is simple: touches alternately start and stop the track playing.
  // Each track starts from the beginning each time. Note that if a track
  // is playing, any touch on any pin will stop it; it doesn't have to be
  // the same pin that started it.

  if (touchStatus == NEW_TOUCH) {
    if (bt->getPlayerStatus() == IS_PLAYING) {
      bt->stopTrack();
      bt->turnLedOff();
    } else {
      bt->startTrack(trackNumber);
      bt->turnLedOn();
    }
  }

  bt->doTimerTasks();
}
