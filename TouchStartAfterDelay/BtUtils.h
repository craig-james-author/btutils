/* -*-C-*-
   BtUtils.h - Library to wrap BareTouch board utilities into
   a simple library.
 */

#ifndef BtUtils_h
#define BtUtils_h 1

#include "BtUtils.h"
#include "math.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h> 
#include <SFEMP3Shield.h>

// TouchBoard definitions
#define FIRST_PIN  0
#define LAST_PIN  11

// Music playback state
#define IS_STOPPED 0
#define IS_PLAYING 1
#define IS_PAUSED  2
#define IS_WAITING 3

// Touches to the electrodes
#define TOUCH_NO_CHANGE 0
#define NEW_TOUCH 1
#define NEW_RELEASE 2

class BtUtils
{
 public:

  BtUtils(SdFat*, SFEMP3Shield*);

  void log_action(char *msg, int track);
  void turnLedOn();
  void turnLedOff();
  int  getPinTouchStatus(int *whichTrack);
  void doLoopTasks();

  void setVolume(int percent);
  void setFadeInTime(int milliseconds);

  int  getPlayerStatus();
  int  getLastTrackPlayed();
  void queueTrackToStartAfterDelay(int trackNumber);
  void startTrack(int trackNumber);
  void resumeTrack(int trackNumber);
  void pauseTrack(int trackNumber);
  void setStartDelay(int milliseconds);

 private:
  int playerStatus;
  int lastPlayed;
  unsigned long lastStartTime;
  unsigned long startDelay;
  int currentVolume;
  int fadeInTime;

  SdFat *sd;
  SFEMP3Shield *MP3player;

  void increaseVolume();
  void startIfTimeoutReached();
};

#endif
