//Starts at touch. Stops on release. Resumes in place with next touch at same sensor. 
//The “idle time out” time can be shortened or lengthened. When new sensor is touched, it plays from start. 
//When previous is touched, it plays from start.

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
  
  // This sets an "idle timeout" -- if nothing hapens for this length of time,
  // it clears out the "resume" feature so that the next time you call
  // bt->resume(), it will start the track over instead of resuming.  The
  // timeout is in seconds.

  //bt->startOverAfterNoTouchTime(30);


  // Set the output volume (left and right). This ranges from zero (silent)
  // to 100 (full volume).  The default is 100 (i.e. if you don't call this
  // function at all, the volume will be 100%).

  bt->setVolume(20, 20);


  // Set the fade-in and fade-out times in milliseconds (e.g. 1000 is 1
  // second). When a track is started, the volume is initially zero, and it
  // takes this much time to reach the volume. Note that whatever you set
  // the volume to (see above) is the target that the fade-in is heading
  // for in the specified time.

  // bt->setFadeInTime(1000);    // 1 second fade-in
  // bt->setFadeOutTime(2000);   // 2 second fade-out


  // Set the touch sensitivity. Low values make it very sensitive (i.e. it
  // will trigger a touch even when your hand is nearby), and high valuse
  // make it less sensitive (i.e. you have to actually touch the contact).
  // The first number is touch, the second number is release. Touch <i>must</i>
  // be greater than release.

  bt->setTouchReleaseThreshold(10, 5);
}


void loop() {

  int trackNumber;
  int touchStatus = bt->getPinTouchStatus(&trackNumber);

  // If a new touch is detected:
  //   - if it's the same track as before, resume playing where it left off.
  //   - if it's a different track, start it from the beginning.

  if (touchStatus == NEW_TOUCH) {
    int lastPlayed  = bt->getLastTrackPlayed();
    if (bt->getPlayerStatus() == IS_PAUSED && trackNumber == lastPlayed) {
      bt->resumeTrack();
    } else {
      bt->startTrack(trackNumber);
    }
    bt->turnLedOn();
  }

  // Pause the track as soon as the release is detected.

  else if (touchStatus == NEW_RELEASE) {
    bt->pauseTrack();
    bt->turnLedOff();
  } 

}
