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
  _lastActionTime      = 0;
  _startOverIfIdleTime = -1;

  _targetVolume        = 100;
  _actualVolume        = 100;
  _fadeInTime          = 0;
  _fadeOutTime         = 0;
  _thisFadeInTime      = 0;
  _thisFadeOutTime     = 0;

  _lastPinTouched = -1;

  _lastProximity       = 0.0;
  _proximityMultiplier = 1.3;

  _sd = sd_in;
  _MP3player = MP3player_in;
}

BtUtils* BtUtils::setup(SdFat *sd, SFEMP3Shield *MP3player) {

  // Note: it might seem like this could be a static object declared at the
  // program start, but that doesn't work due to some out-of-sequence
  // operations that would occur before the BareTouch board is ready. By
  // creating the BtUtils object dynamically during the Arduino setup()
  // function, we avoid those problems.

  pinMode(LED_BUILTIN, OUTPUT);

  unsigned long start_millis = millis();
  Serial.begin(57600);
//   while (!Serial && ((millis() - start_millis) < 2000)) ; {}
//   delay(250);		// bug: without this delay and println('-----'), won't print the "Setup" message
//   Serial.println("-------");
//   Serial.println("Setup");

  if (!sd->begin(SD_SEL, SPI_HALF_SPEED))
    sd->initErrorHalt();

  if (!MPR121.begin(MPR121_ADDR))
    SERIAL_PRINTLN("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);
  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  byte result = MP3player->begin();
  MP3player->setVolume(10,10);
 
  if(result != 0) {
    SERIAL_PRINT("Error code: ");
    SERIAL_PRINT(result);
    SERIAL_PRINTLN(" when trying to start MP3 player");
   }

  BtUtils* bt = new BtUtils(sd, MP3player);
  return bt;
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

int BtUtils::getPinTouchStatus(int *whichPinChanged) {

  *whichPinChanged = -1;
  bool pinIsTouched[NUM_PINS];

  if (!MPR121.touchStatusChanged())
    return TOUCH_NO_CHANGE;

  MPR121.updateTouchData();

  // Loop over pins, find the status of each. Note that it seems to be
  // necessary to check every pin every time anyway so that isNewTouch()
  // returns correctly the next time we try.
  // Serial.print("_lastPinTouched: ");
  // Serial.print(_lastPinTouched);
  // Serial.print(", ");
  Serial.print("pins: ");
  unsigned char numPinsTouched = 0;
  for (unsigned char i = FIRST_PIN; i <= LAST_PIN; i++) {
    pinIsTouched[i] = MPR121.getTouchData(i);
    if (pinIsTouched[i]) {
      Serial.print(i);
      numPinsTouched++;
    } else {
      Serial.print((i > 9) ? "  " : " ");
    }
    Serial.print(" ");
  }
  
  // If last status says no pin was touched
  //   if no pins are touched now
  //     - status is TOUCH_NO_CHANGE
  //   else
  //     - status is NEW_TOUCH
  //     - *whichPin is lowest-numbered pin 
  // else (last status says a pin was being touched)
  //   if the last pin touched is still touched
  //     - status is TOUCH_NO_CHANGE
  //   else (the last pin touched isn't any more)
  //     - If no other pins are touched
  //          - status NEW_RELEASE on that pin
  //          - lastPinTouched is -1
  //     - else (another pin is being touched)
  //        - status is NEW_TOUCH on that pin
  //        - lastPinTouched is that pin
  //
  // record new pin state
  // return status and *whichPinChanged

  int touchStatus;
  if (_lastPinTouched < 0) {	// Last time through, no pin was touched, so this is new
    if (numPinsTouched == 0) {
      touchStatus = TOUCH_NO_CHANGE;
    }
    else {
      touchStatus = NEW_TOUCH;
      for (unsigned char i = FIRST_PIN; i <= LAST_PIN; i++) {
	if (pinIsTouched[i]) {
	  *whichPinChanged = i;
	  break;
	}
      }
    }
  } else {  // Last time through, a pin was touched, so check for changes
    if (pinIsTouched[_lastPinTouched]) {	// Same pin stil being touched?
      touchStatus = TOUCH_NO_CHANGE;
    } else {					// Last pin touched has been released
      if (numPinsTouched == 0) {
	touchStatus = NEW_RELEASE;
	*whichPinChanged = _lastPinTouched;
      } else {
	touchStatus = NEW_TOUCH;
	for (unsigned char i = FIRST_PIN; i <= LAST_PIN; i++) {
	  if (pinIsTouched[i]) {
	    *whichPinChanged = i;
	    break;
	  }
	}
      }
    }
  }

  if (touchStatus == NEW_TOUCH) {
    _lastPinTouched = *whichPinChanged;
  } else if (touchStatus == NEW_RELEASE) {
    _lastPinTouched = -1;
  }
  
  Serial.print((touchStatus == TOUCH_NO_CHANGE ? "No Change " : (touchStatus == NEW_TOUCH ? "Touch " : "Release ")));
  if (*whichPinChanged >= 0) {
    Serial.print(*whichPinChanged);
  }
  Serial.println("");

  return touchStatus;
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

  return (int)((float)thisProximity*_proximityMultiplier);
}

int BtUtils::setProximityMultiplier(float multiplier) {
  _proximityMultiplier = multiplier;
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

void BtUtils::_setVolume(int leftPercent, int rightPercent) {
  _targetVolume = leftPercent;
  _setActualVolume(leftPercent);
}

void BtUtils::setVolume(int leftPercent, int rightPercent) {
  log_action("set volume percent: ", leftPercent);
  _setVolume(leftPercent, rightPercent);
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
      log_action("Set volume: ", newVolumePercent);
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
      log_action("Set volume: ", newVolumePercent);
      if (newVolumePercent <= 0) {
	newVolumePercent = 0;
	if (_playerStatus == IS_PAUSED) {
	  _MP3player->pauseMusic();
	  log_action("fade-out done, track paused: ", _lastTrackPlayed);
	} else {
	  _MP3player->stopTrack();
	  log_action("fade-out done, track stopped: ", _lastTrackPlayed);
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

uint32_t BtUtils::getCurrentTrackLocation() {
  int status = getPlayerStatus();
  if (status == IS_PLAYING || status == IS_PAUSED) {
    return _MP3player->currentPosition();
  }
  return 0;
}


#if BTUTILS_ENABLE_START_AFTER_DELAY
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
#endif

void BtUtils::startTrack(int trackNumber, uint32_t location) {
  LOG_ACTION("start track ", trackNumber);
  if (_fadeInTime > 0) {
    _setActualVolume(0);       // fade-in: start with zero
    _thisFadeInTime = _fadeInTime;
  } else {
    _setVolume(_targetVolume, _targetVolume);   // normal: start with full requested volume
  }
  if (_MP3player->isPlaying()) {
    _MP3player->stopTrack();
  }
  _MP3player->playTrack(trackNumber);
  if (location) {
    
    // Note to self: This skipTo() feature just doesn't work. It has to have been playing
    // for at least 1 second or thereabouts before the MP3 player knows where it is; prior
    // to that it just ignores the skipTo() function. Not only that, but once you do skipTo(),
    // subsequent getCurrentTrackLocation() calls return the DIFFERENCE from where you last
    // started, not the actual location! So this one-second delay is necessary, and basically
    // makes the feature useless.
    delay(1000);
    _MP3player->skipTo(location);
    delay(100);
    if (_MP3player->isPlaying() != 1) {	// did it happen to reach the end?
      stopTrack();
    }
  }
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
#if BTUTILS_ENABLE_START_AFTER_DELAY
  if (_playerStatus != IS_WAITING || _startDelay <= 0) {
    return;
  }
  unsigned long elapsedTime = millis() - _lastStartTime;
  if (elapsedTime < _startDelay) {
    return;
  }
  LOG_ACTION("wait time (milliseconds) completed: ", _startDelay);
  startTrack(_lastTrackPlayed);
#endif
}

void BtUtils::doTimerTasks() {

  // Every time through the loop, see if it's time to increase/decrease the volume
  // or to start a time-delayed track

  _startTrackIfStartDelayReached();
  _doVolumeFadeInAndOut();
}
