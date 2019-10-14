/* -*-C-*- */

#ifndef BtUtils_h
#define BtUtils_h 1

#include "BtUtils.h"
// #include "math.h"

// touch includes
#include <MPR121.h>
// #include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
// #include <SPI.h>
// #include <SdFat.h>
// #include <FreeStack.h> 
#include <SFEMP3Shield.h>

// TouchBoard definitions
#define FIRST_PIN  0
#define LAST_PIN  11
#define NUM_PINS  12

// Music playback state
#define IS_STOPPED 0
#define IS_PLAYING 1
#define IS_PAUSED  2
#define IS_WAITING 3

// Touches to the electrodes
#define TOUCH_NO_CHANGE 0
#define NEW_TOUCH 1
#define NEW_RELEASE 2

// Debugging: enable/disable logging
#ifdef DEBUG
#define LOG_ACTION log_action
#else
#define LOG_ACTION(A,B)
#endif

#define BTUTILS_ENABLE_FADES 1

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
  void setVolume(int leftPercent, int rightPercent);
  void setFadeInTime(int milliseconds);
  void setFadeOutTime(int milliseconds);

  int  getPlayerStatus();
  int  getLastTrackPlayed();
  uint32_t getCurrentTrackLocation();

  void startTrack(int trackNumber, uint32_t location = 0);
  void resumeTrack();
  void pauseTrack();
  void stopTrack();

  void startOverAfterNoTouchTime(int seconds);

  void setStartDelay(int milliseconds);
  void queueTrackToStartAfterDelay(int trackNumber);

  void setProximitySensingMode();
  int getProximityPercent(int pinNumber);
  int setProximityMultiplier(float multiplier);

 private:

  // MP3 player status
  int _playerStatus;
  int _lastTrackPlayed;
  unsigned long _lastStartTime;
  unsigned long _lastStopTime;
  unsigned long _startDelay;
  unsigned long _lastActionTime;
  unsigned long _startOverIfIdleTime;

  // Volume control
  int _targetVolume;
  int _actualVolume;
  int _fadeInTime;
  int _fadeOutTime;
  int _thisFadeInTime;
  int _thisFadeOutTime;

  // Touch pins status
  bool _pinIsTouched[NUM_PINS];
  bool _lastPinTouched;


  // Proximity detection and smoothing
  float _lastProximity;
  float _proximityMultiplier;

  SdFat *_sd;
  SFEMP3Shield *_MP3player;

  uint8_t _volumePctToByte(int percent);
  void _setActualVolume(int percent);
  int  _calculateFadeTime(bool goingUp);
  void _doVolumeFadeInAndOut();
  void _startTrackIfStartDelayReached();
};

#endif
