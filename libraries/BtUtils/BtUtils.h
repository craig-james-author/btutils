/* -*-C++-*-
+======================================================================
| Copyright (c) 2019-2022, Craig A. James
|
| Permission is hereby granted, free of charge, to any person obtaining a
| copy of this software and associated documentation files (the
| "Software"), to deal in the Software without restriction, including
| without limitation the rights to use, copy, modify, merge, publish,
| distribute, sublicense, and/or sell copies of the Software, and to permit
| persons to whom the Software is furnished to do so, subject to the
| following conditions:
|
| The above copyright notice and this permission notice shall be included
| in all copies or substantial portions of the Software.
|
| THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
| OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
| MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
| NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
| DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
| OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
| USE OR OTHER DEALINGS IN THE SOFTWARE.
+======================================================================
*/

#ifndef BtUtils_h
#define BtUtils_h 1

#include "BtUtils.h"

// touch includes
#include <MPR121.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
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
// #define DEBUG 1
#ifdef DEBUG
#define LOG_ACTION _log_action
#define SERIAL_PRINT(x) Serial.print(x)
#define SERIAL_PRINTLN(x) Serial.println(x)
#else
#define LOG_ACTION(A,B)
#define SERIAL_PRINT(x)
#define SERIAL_PRINTLN(x)
#endif

// Disable certain unneeded features to save space

#define BTUTILS_ENABLE_FADES 1
#define BTUTILS_ENABLE_START_AFTER_DELAY 1

class BtUtils
{
 public:

  BtUtils(SdFat*, SFEMP3Shield*);
  static BtUtils* setup(SdFat*, SFEMP3Shield*);
  static void turnLedOn();
  static void turnLedOff();
  void doTimerTasks();

  int  getPinTouchStatus(int *whichPinChanged);
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

#ifdef DEBUG
  static void _log_action(const char *msg, int track);
#endif

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

  // Touch pins: what was the last one touched?
  int _lastPinTouched;

  // Proximity detection and smoothing
  float _lastProximity;
  float _proximityMultiplier;

  SdFat *_sd;
  SFEMP3Shield *_MP3player;

  uint8_t _volumePctToByte(int percent);
  void _setVolume(int leftPercent, int rightPercent);
  void _setActualVolume(int percent);
  int  _calculateFadeTime(bool goingUp);
  void _doVolumeFadeInAndOut();
  void _startTrackIfStartDelayReached();
};

#endif
