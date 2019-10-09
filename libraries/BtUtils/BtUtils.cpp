/* -*-C++-*-
+======================================================================
| Copyright (c) 2019, Craig A. James
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

#include "Arduino.h"
#include "BtUtils.h"

/*----------------------------------------------------------------------
 * Initialization.
 ----------------------------------------------------------------------*/

BtUtils::BtUtils(SdFat *sd_in, SFEMP3Shield *MP3player_in) {

  // Note: this can't be created as a static object. See comments below
  // in setup().

  _playerStatus        = IS_STOPPED;
  _lastTrackPlayed     = -1;
  _lastStartTime       = 0;
  _lastStopTime        = 0;
  _startDelay          = 1000;
  _startOverIfIdleTime = -1;
  _lastActionTime      = 0;
  _targetVolume        = 100;
  _actualVolume        = 100;
  _fadeInTime          = 0;
  _fadeOutTime         = 0;
  _thisFadeInTime      = 0;
  _thisFadeOutTime     = 0;
  _lastProximity       = 0.0;
  _proximityPinNumber  = 0;

  _sd = sd_in;
  _MP3player = MP3player_in;
}

BtUtils* BtUtils::setup(SdFat *sd, SFEMP3Shield *MP3player) {

  // Note: it might seem like this could be a static object declared at the
  // program start, but that doesn't work due to some out-of-sequence
  // operations that would occur before the BareTouch board is ready. By
  // creating the BtUtils object dynamically during the Arduino setup()
  // function, we avoid those problems.

  Serial.begin(57600);
  pinMode(LED_BUILTIN, OUTPUT);

  // while (!Serial) ; {} //uncomment when using the serial monitor 
  Serial.println("BtUtils setup");

  if (!sd->begin(SD_SEL, SPI_HALF_SPEED))
    sd->initErrorHalt();

  if (!MPR121.begin(MPR121_ADDR))
    Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);
  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  byte result = MP3player->begin();
  MP3player->setVolume(10,10);
 
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }

  BtUtils* bt = new BtUtils(sd, MP3player);
  return bt;
}

/*----------------------------------------------------------------------
 * Simple utility functions
 ----------------------------------------------------------------------*/

void BtUtils::log_action(char *msg, int track) {
#ifdef DEBUG
  Serial.print(msg);
  Serial.println(track);
#endif
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
  if (touchThreshold > 255)
    touchThreshold = 255;
  if (releaseThreshold >= touchThreshold)
    releaseThreshold = touchThreshold - 1;
  if (releaseThreshold < 0)
    releaseThreshold = 0;

  MPR121.setTouchThreshold(touchThreshold);
  MPR121.setReleaseThreshold(releaseThreshold);
  LOG_ACTION("Touch threshold: ", touchThreshold);
  LOG_ACTION("Release threshold: ", releaseThreshold);
}  

int BtUtils::getPinTouchStatus(int *whichTrack) {

  *whichTrack = -1;
  int pinStatus = TOUCH_NO_CHANGE;

  if (!MPR121.touchStatusChanged())
    return pinStatus;

  MPR121.updateTouchData();
  if (MPR121.getNumTouches() > 1)     // Ignore when two or more pins touched
    return pinStatus;

  // Loop over pins, find the one that was touched. Note that we don't end
  // the loop even if we find one; we test all of the pins. This seems to
  // be necessary so that isNewTouch() returns correctly the next time we try.
  for (int i = FIRST_PIN; i <= LAST_PIN; i++) {
    if (MPR121.isNewTouch(i)) {
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_TOUCH;
      LOG_ACTION("pin touched: ", i);
      turnLedOn();
    }
    else if (MPR121.isNewRelease(i)){
      *whichTrack = i - FIRST_PIN;
      pinStatus = NEW_RELEASE;
      LOG_ACTION("pin released: ", i);
      turnLedOff();
    }
  }
  return pinStatus;
}

/*----------------------------------------------------------------------
 * Proximity sensor.
 ----------------------------------------------------------------------*/

void BtUtils::setProximitySensingMode() {

  // This is based on the "prox_volume.ino" example supplied with the
  // TouchBoard.  Slow down some of the MPR121 baseline filtering to avoid
  // filtering out slow hand movements. There is no explanation of these in
  // the example, so they're just copied here verbatim.

  MPR121.setRegister(MPR121_NHDF, 0x01); // noise half delta (falling)
  MPR121.setRegister(MPR121_FDLF, 0x3F); // filter delay limit (falling)   
}


#define LOW_DIFF 0
#define HIGH_DIFF 50
#define filterWeight 0.3f // 0.0f to 1.0f - higher value = more smoothing

int BtUtils::getProximityPercent(int pinNumber) {

  MPR121.updateAll();

  // read the difference between the measured baseline and the measured continuous data
  int reading = MPR121.getBaselineData(pinNumber)-MPR121.getFilteredData(pinNumber);

  // constrain the reading between our low and high mapping values
  unsigned int prox = constrain(reading, LOW_DIFF, HIGH_DIFF);
  
  // implement a simple (IIR lowpass) smoothing filter
  _lastProximity = (filterWeight*_lastProximity) + ((1-filterWeight)*(float)prox);

  // map the LOW_DIFF..HIGH_DIFF range to 0..100 (percentage)
  int thisProximity = map(_lastProximity, LOW_DIFF, HIGH_DIFF, 0, 100);

  return thisProximity;
}


/*----------------------------------------------------------------------
 * Volume controls
 ----------------------------------------------------------------------*/

// Convert the volume, range is 0 to 100 (percent).  The MIDI player sets
// volume in 254 increments (254 is minimum, 0 is maximum), each step being
// -2dB, so it's an exponential scale. The heurist 1/10x^1.5 function below
// (offset and scaled so that zero is zero and 100 is max) reverses this a
// bit, making it sound more linear.

uint8_t BtUtils::_volumePctToByte(int percent) {
  if (percent > 100)	// max volume
    percent = 100;
  else if (percent < 0) // min volume
    percent = 0;
  float p = (float)percent/100.0;
  p = (1 - (1/(pow(10.0*(p+0.1), 1.5))))/0.974;
  uint8_t b = ((1.0 - p) * 254.0);
  LOG_ACTION("_volumePctToByte percent: ", percent);
  LOG_ACTION("_volumePctToByte byte:    ", b);
  return b;
}

void BtUtils::_setActualVolume(int percent) {
  uint8_t volume = _volumePctToByte(percent);
  _MP3player->setVolume(volume);
  _actualVolume = percent;
}

void BtUtils::setVolume(int leftPercent, int rightPercent) {
  LOG_ACTION("set volume set: ", leftPercent);
  _targetVolume = leftPercent;
  _setActualVolume(leftPercent);
}

void BtUtils::setVolume(int percent) {
  setVolume(percent, percent);
}

void BtUtils::setFadeInTime(int milliseconds) {
#ifdef BTUTILS_ENABLE_FADES
  _fadeInTime = milliseconds;
#endif
}

void BtUtils::setFadeOutTime(int milliseconds) {
#ifdef BTUTILS_ENABLE_FADES
  _fadeOutTime = milliseconds;
#endif
}

int BtUtils::_calculateFadeTime(bool goingUp) {
#ifdef BTUTILS_ENABLE_FADES
  int deltaVolume;
  int fadeTime;
  if (goingUp) {
    deltaVolume = 100 - _actualVolume;
    fadeTime = _fadeInTime;
  } else {
    deltaVolume = _actualVolume;
    fadeTime = _fadeOutTime;
  }
  int time = (int)(0.499 + (float)fadeTime * (float)deltaVolume/100.0);
  return time;
#else
  return 0;
#endif
}


void BtUtils::_doVolumeFadeInAndOut() {

#ifdef BTUTILS_ENABLE_FADES

  // Do fade-in?

  if (_lastStartTime > 0 && _fadeInTime != 0 && _playerStatus == IS_PLAYING && _actualVolume < _targetVolume) {

    // Calculate the target volume based on how much elapsed time since the track started playing.
    // But if _thisFadeInTime is different that _fadeInTime, it means we started with non-zero
    // volume, so push the elapsed time forward by difference of the two.

    unsigned long elapsedTime = millis() - _lastStartTime;
    if (_fadeInTime != _thisFadeInTime) {
      elapsedTime += _fadeInTime - _thisFadeInTime;	// push elapsed time forward to adjust for non-zero start volume
    }
    
    int newVolumePercent = int((float)_targetVolume*(float)elapsedTime/(float)_fadeInTime);
    
    // Time to increase volume?
    
    if (newVolumePercent != _actualVolume) {
      if (newVolumePercent >= _targetVolume) {
	newVolumePercent = _targetVolume;
      }
      _setActualVolume(newVolumePercent);
    }
  }

  // Else -- do fade-out?

  else if (_lastStopTime > 0 && (_playerStatus == IS_STOPPED || _playerStatus == IS_PAUSED)
	   && _fadeOutTime != 0 && _actualVolume > 0) {

    // Calculate the target volume based on how much elapsed time since the track stopped playing.
    // But if _thisFadeOutTime is different than _fadeOutTime, it means we started with volume less
    // than 100%, so push elapsed time forward by the difference of the two.

    unsigned long elapsedTime = millis() - _lastStopTime;
    if (_fadeOutTime != _thisFadeOutTime) {
      elapsedTime += _fadeOutTime - _thisFadeOutTime;
    }

    int newVolumePercent = _targetVolume - int((float)(_targetVolume)*(float)elapsedTime/(float)_fadeOutTime);

    // Time to decrease volume?

    if (newVolumePercent != _actualVolume) {
      if (newVolumePercent < 0) {
	newVolumePercent = 0;
	if (_playerStatus == IS_PAUSED) {
	  _MP3player->pauseMusic();
	  LOG_ACTION("fade-out done, music paused, track ", _lastTrackPlayed);
	} else {
	  _MP3player->stopTrack();
	}
      }
      _setActualVolume(newVolumePercent);
    }
  }
#else
  _setActualVolume(_targetVolume);
#endif
}

/*----------------------------------------------------------------------
 * Queuing, start, stop, resume of tracks
 ----------------------------------------------------------------------*/

int BtUtils::getPlayerStatus() {
  if ((_playerStatus == IS_PLAYING || _playerStatus == IS_PAUSED) && _MP3player->isPlaying() != 1) {
    _playerStatus = IS_STOPPED;
    LOG_ACTION("player finished track: ", _lastTrackPlayed);
  }
  return _playerStatus;
}

int BtUtils::getLastTrackPlayed() {
  return _lastTrackPlayed;
}

void BtUtils::queueTrackToStartAfterDelay(int trackNumber) {
  LOG_ACTION("queue track, waiting for timeout, track ", trackNumber);
  if (_MP3player->isPlaying()) {
    _MP3player->stopTrack();
  }
  _lastTrackPlayed = trackNumber;
  _lastStartTime = millis();
  _lastStopTime = 0;
  _lastActionTime = _lastStartTime;
  _playerStatus = IS_WAITING;
}

void BtUtils::startTrack(int trackNumber) {
  LOG_ACTION("start track ", trackNumber);
  if (_fadeInTime > 0) {
    _setActualVolume(0);       // fade-in: start with zero
    _thisFadeInTime = _fadeInTime;
  } else {
    setVolume(_targetVolume);   // normal: start with full requested volume
  }
  if (_MP3player->isPlaying()) {
    _MP3player->stopTrack();
  }
  _MP3player->playTrack(trackNumber);
  _lastTrackPlayed = trackNumber;
  _lastStartTime = millis();
  _lastStopTime = 0;
  _lastActionTime = _lastStartTime;
  _playerStatus = IS_PLAYING;
}

void BtUtils::resumeTrack() {
  LOG_ACTION("resume track ", _lastTrackPlayed);
  if (_lastTrackPlayed < 0) {
    startTrack(0);
  } else if (_lastActionTime > 0 && _startOverIfIdleTime > 0) {
    unsigned long lastActionElapsed = millis() - _lastActionTime;
    if (lastActionElapsed >= _startOverIfIdleTime) {
      LOG_ACTION("startOverIfIdleTime exceeded, restarting track: ", _lastTrackPlayed);
      startTrack(_lastTrackPlayed);
      return;
    }
  }
  LOG_ACTION("resume track: resume player, ", _lastTrackPlayed);
  _MP3player->resumeMusic();
  _playerStatus = IS_PLAYING;
  _thisFadeInTime = _calculateFadeTime(true);
  _lastStartTime = millis();
  _lastStopTime = 0;
  _lastActionTime = millis();
}

void BtUtils::pauseTrack() {
  LOG_ACTION("pause track ", _lastTrackPlayed);
  if (_fadeOutTime == 0) {
    _MP3player->pauseMusic();
  } else {
    _thisFadeOutTime = _calculateFadeTime(false);
    _lastStopTime = millis();
    _lastStartTime = 0;
  }
  _lastActionTime = millis();
  _playerStatus = IS_PAUSED;
}

void BtUtils::stopTrack() {
  LOG_ACTION("stop track ", _lastTrackPlayed);
  _playerStatus = IS_STOPPED;
  _lastTrackPlayed = -1;
  if (_fadeOutTime > 0) {
    _thisFadeOutTime = _calculateFadeTime(false);
    _lastStopTime = millis();
    _lastStartTime = 0;
  } else {
    _MP3player->stopTrack();
  }
  _lastActionTime = _lastStopTime;
}

void BtUtils::startOverAfterNoTouchTime(int seconds) {
  LOG_ACTION("startOverAfterNoTouchTime = ", seconds);
  if (seconds < 0) {
    _startOverIfIdleTime = -1;
  } else {
    _startOverIfIdleTime = (unsigned long)seconds * 1000;
  }
}


void BtUtils::setStartDelay(int milliseconds) {
  _startDelay = milliseconds;
}

void BtUtils::_startTrackIfStartDelayReached() {
  if (_playerStatus != IS_WAITING || _startDelay <= 0) {
    return;
  }
  unsigned long elapsedTime = millis() - _lastStartTime;
  if (elapsedTime < _startDelay) {
    return;
  }
  LOG_ACTION("wait time (milliseconds) completed: ", _startDelay);
  startTrack(_lastTrackPlayed);
}

void BtUtils::doTimerTasks() {

  // Every time through the loop, see if it's time to increase/decrease the volume
  // or to start a time-delayed track

  _startTrackIfStartDelayReached();
  _doVolumeFadeInAndOut();
}
