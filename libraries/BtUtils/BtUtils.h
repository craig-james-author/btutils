/* -*-C-*- */

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
  static BtUtils* setup(SdFat*, SFEMP3Shield*);
  static void log_action(char *msg, int track);
  static void turnLedOn();
  static void turnLedOff();
  void doTimerTasks();

  int  getPinTouchStatus(int *whichTrack);
  void setTouchReleaseThreshold(int touchThreshold, int releaseThreshold);

  void setVolume(int percent);
  void setFadeInTime(int milliseconds);
  void setFadeOutTime(int milliseconds);

  int  getPlayerStatus();
  int  getLastTrackPlayed();

  void startTrack(int trackNumber);
  void resumeTrack();
  void pauseTrack();
  void stopTrack();

  void startOverAfterNoTouchTime(int seconds);

  void setStartDelay(int milliseconds);
  void queueTrackToStartAfterDelay(int trackNumber);

  void setProximitySensingMode();
  int getProximityPercent(int pinNumber);

 private:
  int _playerStatus;
  int _lastTrackPlayed;
  unsigned long _lastStartTime;
  unsigned long _lastStopTime;
  unsigned long _startDelay;
  unsigned long _lastActionTime;
  unsigned long _startOverIfIdleTime;
  int _targetVolume;
  int _actualVolume;
  int _fadeInTime;
  int _fadeOutTime;
  float _lastProximity;
  int _proximityPinNumber;
  SdFat *_sd;
  SFEMP3Shield *_MP3player;

  uint8_t _volumnPctToByte(int percent);
  void _setActualVolume(int percent);
  void _doVolumeFadeIn();
  void _doVolumeFadeOut();
  void _startTrackIfStartDelayReached();
};

#endif
