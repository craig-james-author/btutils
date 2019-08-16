/* -*-C++-*- */

// compiler error handling
#include "Compiler_Errors.h"
#include "math.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// TouchBoard definitions
#define FIRST_PIN  0
#define LAST_PIN  11

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h> 
#include <SFEMP3Shield.h>

// SD card (file system) controller
SdFat sd;

// mp3 player controller
SFEMP3Shield MP3player;

// Music playback state
#define IS_STOPPED 0
#define IS_PLAYING 1
#define IS_PAUSED  2
#define IS_WAITING 3
int playerStatus = IS_STOPPED;
int lastPlayed = -1;
unsigned long lastStartTime = 0;
int currentVolume = 0;

/*----------------------------------------------------------------------
 * How long to wait after touch to start playing, and how long to reach
 * full volume after that (both in milliseconds).
 ----------------------------------------------------------------------*/

#define DELAY_START 1000
#define TIME_TO_MAX_VOLUME 1000


/*----------------------------------------------------------------------
 * Initialization.
 ----------------------------------------------------------------------*/

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  // Initialize Serial port and wait while it initializes itself
  Serial.begin(57600);
  // while (!Serial) ; {}
  Serial.println("Touch MP3 player with delay");

  // Initialize SD card reader
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");

  // Initialze Touch controller
  MPR121.setInterruptPin(MPR121_INT);
  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  // Initialize MP3 player
  byte result = MP3player.begin();
  MP3player.setVolume(0,0);
  if (result != 0) {
    log_action("ERROR: initializing MP3 player, error code: ", (int)result);
   }
}

void log_action(char *msg, int track) {
  Serial.print(msg);
  Serial.println(track);
}

/*----------------------------------------------------------------------
 * Touch system: was a key touched or released?
 ----------------------------------------------------------------------*/

#define NO_CHANGE 0
#define NEW_TOUCH 1
#define NEW_RELEASE 2


int getPinTouchStatus(int *whichTrack) {

  if (!MPR121.touchStatusChanged()) {
    *whichTrack = -1;
    return NO_CHANGE;
  }
  MPR121.updateTouchData();
  if (MPR121.getNumTouches() <= 1) {    // Ignore when two or more pins touched
    *whichTrack = -1;
    return NO_CHANGE;
  }

  *whichTrack = -1;
  int pinStatus = NO_CHANGE;

  // Loop over pins, find the one that was touched. Note that we don't end
  // the loop even if we find one; we test all of the pins. This seems to
  // be necessary so that isNewTouch() returns correctly the next time we try.
  for (int i = FIRST_PIN; i < LAST_PIN; i++) {
    if (MPR121.isNewTouch(i)) {
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_TOUCH;
    }
    else if (MPR121.isNewRelease(i)){
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_RELEASE;
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

void setVolume(int percent) {
  if (percent > 100)
    percent = 100;
  else if (percent < 0)
    percent = 0;
  float p = (float)percent/100.0;
  p = sqrt(p);
  uint8_t volume = (1.0 - p) * 254.0;
  MP3player.setVolume(volume);
  currentVolume = percent;
  log_action("volume set to: ", percent);
}

// This increase the volume automatically ("fade in") based on the
// passing time and the TIME_TO_MAX_VOLUME declared above.

void increaseVolume() {
  if (playerStatus != IS_PLAYING || currentVolume >= 100) {
    return;
  }
  float timeToMax = TIME_TO_MAX_VOLUME == 0 ? 1.0 : (float)TIME_TO_MAX_VOLUME;
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

void queueTrackToStartAfterDelay(int trackNumber) {
  if (MP3player.isPlaying())
    MP3player.stopTrack();
  lastPlayed = trackNumber;
  lastStartTime = millis();
  playerStatus = IS_WAITING;
  log_action("queued track, waiting for timeout, track ", trackNumber);
}

void startTrack(int trackNumber) {
  if (MP3player.isPlaying())
    MP3player.stopTrack();
  MP3player.playTrack(trackNumber);
  lastPlayed = trackNumber;
  lastStartTime = millis();
  playerStatus = IS_PLAYING;
  setVolume(50);
  log_action("playing track ", trackNumber);
}

void resumeTrack(int trackNumber) {
  MP3player.resumeMusic();
  playerStatus = IS_PLAYING;
  log_action("resuming track ", trackNumber);
}

void pauseTrack(int trackNumber) {
  MP3player.pauseMusic();
  playerStatus = IS_PAUSED;
  log_action("paused track ", trackNumber);
}

void startIfTimeoutReached() {
  if (playerStatus != IS_WAITING) {
    return;
  }
  unsigned long elapsedTime = millis() - lastStartTime;
  if (elapsedTime < DELAY_START) {
    return;
  }
  log_action("wait time (milliseconds) completed: ", DELAY_START);
  startTrack(lastPlayed);
}


/*----------------------------------------------------------------------
 * The main program loop
----------------------------------------------------------------------*/

void loop() {

  int trackNumber;
  int touchStatus = getPinTouchStatus(&trackNumber);

  if (touchStatus != NO_CHANGE) {

    // If the player happened to finish a track but our status is
    // IS_PLAYING, update it to IS_STOPPED.

    if (playerStatus == IS_PLAYING && MP3player.isPlaying() != 1) {
      playerStatus = IS_STOPPED;
      log_action("player finished track: ", lastPlayed);
    }

    if (touchStatus = NEW_TOUCH) {

      log_action("pin touched: ", trackNumber);
      digitalWrite(LED_BUILTIN, HIGH);

      if (playerStatus == IS_PLAYING) {
	if (lastPlayed == trackNumber) {
	  pauseTrack(trackNumber);
	} else {
	  queueTrackToStartAfterDelay(trackNumber);
	}
      }
      else if (playerStatus == IS_PAUSED) {
	if (trackNumber == lastPlayed) {
	  resumeTrack(trackNumber);
	} else {
	  queueTrackToStartAfterDelay(trackNumber);
	}
      }
      else if (playerStatus == IS_STOPPED) {
	queueTrackToStartAfterDelay(trackNumber);
      }     
    }
    else if (touchStatus == NEW_RELEASE) {
      log_action("pin released: ", trackNumber);
      digitalWrite(LED_BUILTIN, LOW);
    } 
  }

  // Every time through the loop, see if it's time to increase the volume
  // or to start a time-delayed track
  startIfTimeoutReached();
  increaseVolume();

}
