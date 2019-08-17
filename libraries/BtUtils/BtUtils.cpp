/* -*-C++-*- */

#include "Arduino.h"
#include "BtUtils.h"

/*----------------------------------------------------------------------
 * Initialization.
 ----------------------------------------------------------------------*/

BtUtils::BtUtils(SdFat *sd_in, SFEMP3Shield *MP3player_in) {

  playerStatus = IS_STOPPED;
  lastTrackPlayed = -1;
  lastStartTime = 0;
  currentVolume = 0;
  fadeInTime = 0;
  startDelay = 0;

  sd = sd_in;
  MP3player = MP3player_in;

  pinMode(LED_BUILTIN, OUTPUT);
   
  // Initialize Serial port and wait while it initializes itself
  Serial.begin(57600);
  // while (!Serial) ; {}
  Serial.println("Touch MP3 player with delay");

  // Initialize SD card reader
  if (!sd->begin(SD_SEL, SPI_HALF_SPEED)) sd->initErrorHalt();
  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");

  // Initialze Touch controller
  MPR121.setInterruptPin(MPR121_INT);
  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  // Initialize MP3 player
  byte result = MP3player->begin();
  MP3player->setVolume(0,0);
  if (result != 0) {
    log_action("ERROR: initializing MP3 player, error code: ", (int)result);
   }
}

/*----------------------------------------------------------------------
 * Simple utility functions
 ----------------------------------------------------------------------*/

void BtUtils::log_action(char *msg, int track) {
  Serial.print(msg);
  Serial.println(track);
}

void BtUtils::turnLedOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void BtUtils::turnLedOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

/*----------------------------------------------------------------------
 * Touch system: was a key touched or released?
 ----------------------------------------------------------------------*/

void BtUtils::setTouchReleaseThreshold(int touchThreshold, int releaseThreshold) {
  if (touchThreshold < 1)
    touchThreshold = 1;
  if (releaseThreshold >= touchThreshold)
    releaseThreshold = touchThreshold - 1;
  MPR121.setTouchThreshold(touchThreshold);
  MPR121.setReleaseThreshold(releaseThreshold);
}  

int BtUtils::getPinTouchStatus(int *whichTrack) {

  if (!MPR121.touchStatusChanged()) {
    *whichTrack = -1;
    return TOUCH_NO_CHANGE;
  }
  MPR121.updateTouchData();
  if (MPR121.getNumTouches() <= 1) {    // Ignore when two or more pins touched
    *whichTrack = -1;
    return TOUCH_NO_CHANGE;
  }

  *whichTrack = -1;
  int pinStatus = TOUCH_NO_CHANGE;

  // Loop over pins, find the one that was touched. Note that we don't end
  // the loop even if we find one; we test all of the pins. This seems to
  // be necessary so that isNewTouch() returns correctly the next time we try.
  for (int i = FIRST_PIN; i < LAST_PIN; i++) {
    if (MPR121.isNewTouch(i)) {
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_TOUCH;
      log_action("pin touched: ", i);
      turnLedOn();
    }
    else if (MPR121.isNewRelease(i)){
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_RELEASE;
      log_action("pin released: ", i);
      turnLedOff();
    }
  }
  return pinStatus;
}


/*----------------------------------------------------------------------
 * Volume controls
 ----------------------------------------------------------------------*/

// Set the volume, range is 0 to 100 (percent).
// The MIDI player sets volume in 254 increments (254 is minimum, 0
// is maximum), each step being -2dB, so it's an exponential scale. The
// sqrt() reverses this a bit, making it more linear.

void BtUtils::setVolume(int percent) {
  if (percent > 100)
    percent = 100;
  else if (percent < 0)
    percent = 0;
  float p = (float)percent/100.0;
  p = sqrt(p);
  uint8_t volume = (1.0 - p) * 254.0;
  MP3player->setVolume(volume);
  currentVolume = percent;
  log_action("volume set to: ", percent);
}

void BtUtils::setFadeInTime(int milliseconds) {
  fadeInTime = milliseconds;
}

// This increase the volume automatically ("fade in") based on the
// passing time and the fadeInTime above.

void BtUtils::increaseVolume() {
  if (playerStatus != IS_PLAYING || currentVolume >= 100) {
    return;
  }
  float timeToMax = fadeInTime == 0 ? 1.0 : (float)fadeInTime;
  unsigned long elapsedTime = millis() - lastStartTime;
  int newVolume = int(100.0*(float)elapsedTime/timeToMax);
  if (newVolume > 100) {
    newVolume = 100;
  }
  if (newVolume != currentVolume) {
    setVolume(newVolume);
  }
}

/*----------------------------------------------------------------------
 * Queuing, start, stop, resume of tracks
 ----------------------------------------------------------------------*/

int BtUtils::getPlayerStatus() {
  if (playerStatus == IS_PLAYING && MP3player->isPlaying() != 1) {
    playerStatus = IS_STOPPED;
    log_action("player finished track: ", lastTrackPlayed);
  }
  return playerStatus;
}

int BtUtils::getLastTrackPlayed() {
  return lastTrackPlayed;
}

void BtUtils::queueTrackToStartAfterDelay(int trackNumber) {
  if (MP3player->isPlaying())
    MP3player->stopTrack();
  lastTrackPlayed = trackNumber;
  lastStartTime = millis();
  playerStatus = IS_WAITING;
  log_action("queued track, waiting for timeout, track ", trackNumber);
}

void BtUtils::startTrack(int trackNumber) {
  if (MP3player->isPlaying())
    MP3player->stopTrack();
  MP3player->playTrack(trackNumber);
  lastTrackPlayed = trackNumber;
  lastStartTime = millis();
  playerStatus = IS_PLAYING;
  if (fadeInTime > 0) {
    setVolume(50);	// very quiet start for fade-in
  } else {
    setVolume(100);	// normal is full volume
  }
  log_action("playing track ", trackNumber);
}

void BtUtils::resumeTrack() {
  MP3player->resumeMusic();
  playerStatus = IS_PLAYING;
  log_action("resuming track ", lastTrackPlayed);
}

void BtUtils::pauseTrack() {
  MP3player->pauseMusic();
  playerStatus = IS_PAUSED;
  log_action("paused track ", lastTrackPlayed);
}

void BtUtils::stopTrack() {
  MP3player->pauseMusic();
  playerStatus = IS_PAUSED;
  log_action("paused track ", lastTrackPlayed);
}

void BtUtils::setStartDelay(int milliseconds) {
  startDelay = milliseconds;
}

void BtUtils::startIfTimeoutReached() {
  if (playerStatus != IS_WAITING || startDelay <= 0) {
    return;
  }
  unsigned long elapsedTime = millis() - lastStartTime;
  if (elapsedTime < startDelay) {
    return;
  }
  log_action("wait time (milliseconds) completed: ", startDelay);
  startTrack(lastTrackPlayed);
}

void BtUtils::doTimerTasks() {

  // Every time through the loop, see if it's time to increase the volume
  // or to start a time-delayed track

  startIfTimeoutReached();
  increaseVolume();
}
