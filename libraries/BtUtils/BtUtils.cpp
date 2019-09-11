/* -*-C++-*- */

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
  log_action("Touch threshold: ", touchThreshold);
  log_action("Release threshold: ", releaseThreshold);
}  

int BtUtils::getPinTouchStatus(int *whichTrack) {

  if (!MPR121.touchStatusChanged()) {
    *whichTrack = -1;
    return TOUCH_NO_CHANGE;
  }

  MPR121.updateTouchData();
  if (MPR121.getNumTouches() > 1) {    // Ignore when two or more pins touched
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

uint8_t BtUtils::_volumnPctToByte(int percent) {
  if (percent > 100)	// max volume
    percent = 100;
  else if (percent < 0) // min volume
    percent = 0;
  float p = (float)percent/100.0;
  p = (1 - (1/(pow(10.0*(p+0.1), 1.5))))/0.974;
  uint8_t b = ((1.0 - p) * 254.0);
  log_action("_volumnPctToByte percent: ", percent);
  log_action("_volumnPctToByte byte:    ", b);
  return b;
}

void BtUtils::_setActualVolume(int percent) {
  uint8_t volume = _volumnPctToByte(percent);
  _MP3player->setVolume(volume);
  _actualVolume = percent;
}

void BtUtils::setVolume(int percent) {
  log_action("set volume set: ", percent);
  _targetVolume = percent;
  _setActualVolume(percent);
}

void BtUtils::setFadeInTime(int milliseconds) {
  _fadeInTime = milliseconds;
}

void BtUtils::setFadeOutTime(int milliseconds) {
  _fadeOutTime = milliseconds;
}

void BtUtils::_doVolumeFadeIn() {

  // Ignore if not playing, no fade-in specified, or if we've finished the fade-in

  if (!_fadeInTime || _playerStatus != IS_PLAYING || _actualVolume >= _targetVolume) {
    return;
  }
  
  // Calculate the target volume based on how much elapsed time since the track started playing.

  float timeToMax = _fadeInTime == 0 ? 1.0 : (float)_fadeInTime;        // avoid divide-by-zero error
  unsigned long elapsedTime = millis() - _lastStartTime;

  int newVolumePercent = int((float)_targetVolume*(float)elapsedTime/timeToMax);

  // Time to increase volume?

  if (newVolumePercent != _actualVolume) {
    if (newVolumePercent >= _targetVolume) {
      newVolumePercent = _targetVolume;
    }
    _actualVolume = newVolumePercent;
    _MP3player->setVolume(_volumnPctToByte(_actualVolume));
  }
}

void BtUtils::_doVolumeFadeOut() {

  // Ignore if not stopped, no fade-out specified, or we've finished the fade-out
  if (_playerStatus != IS_STOPPED || _fadeOutTime == 0 ||_actualVolume == 0) {
    return;
  }

  // Calculate the target volume based on how much elapsed time since the track stopped playing.

  float timeToMin = _fadeOutTime == 0 ? 1.0 : (float)_fadeOutTime;      // avoid divide-by-zero error
  unsigned long elapsedTime = millis() - _lastStopTime;

  int newVolumePercent = _targetVolume - int((float)(_targetVolume)*(float)elapsedTime/timeToMin);

  // Time to decrease volume?

  if (newVolumePercent != _actualVolume) {
    if (newVolumePercent < 0) {
      newVolumePercent = 0;
      _MP3player->stopTrack();
    }
    _actualVolume = newVolumePercent;
    _MP3player->setVolume(_volumnPctToByte(_actualVolume));
  }
}

/*----------------------------------------------------------------------
 * Queuing, start, stop, resume of tracks
 ----------------------------------------------------------------------*/

int BtUtils::getPlayerStatus() {
  if (_playerStatus == IS_PLAYING && _MP3player->isPlaying() != 1) {
    _playerStatus = IS_STOPPED;
    log_action("player finished track: ", _lastTrackPlayed);
  }
  return _playerStatus;
}

int BtUtils::getLastTrackPlayed() {
  return _lastTrackPlayed;
}

void BtUtils::queueTrackToStartAfterDelay(int trackNumber) {
  log_action("queue track, waiting for timeout, track ", trackNumber);
  if (_MP3player->isPlaying()) {
    _MP3player->stopTrack();
  }
  _lastTrackPlayed = trackNumber;
  _lastStartTime = millis();
  _lastActionTime = _lastStartTime;
  _playerStatus = IS_WAITING;
}

void BtUtils::startTrack(int trackNumber) {
  log_action("start track ", trackNumber);
  if (_fadeInTime > 0) {
    _setActualVolume(0);       // fade-in: start with zero
  } else {
    setVolume(_targetVolume);   // normal: start with full requested volume
  }
  if (_MP3player->isPlaying()) {
    _MP3player->stopTrack();
  }
  _MP3player->playTrack(trackNumber);
  _lastTrackPlayed = trackNumber;
  _lastStartTime = millis();
  _lastActionTime = _lastStartTime;
  _playerStatus = IS_PLAYING;
}

void BtUtils::resumeTrack() {
  log_action("resume track ", _lastTrackPlayed);
  if (_lastTrackPlayed < 0) {
    startTrack(0);
  } else {
    if (_lastActionTime > 0 && _startOverIfIdleTime > 0) {
      unsigned long lastActionElapsed = millis() - _lastActionTime;
      if (lastActionElapsed >= _startOverIfIdleTime) {
	log_action("startOverIfIdleTime exceeded, restarting track: ", _lastTrackPlayed);
	startTrack(_lastTrackPlayed);
	return;
      }
    }
    _MP3player->resumeMusic();
    _playerStatus = IS_PLAYING;
  }
  _lastActionTime = millis();
}

void BtUtils::pauseTrack() {
  log_action("pause track ", _lastTrackPlayed);
  _MP3player->pauseMusic();
  _playerStatus = IS_PAUSED;
  _lastActionTime = millis();
}

void BtUtils::stopTrack() {
  log_action("stop track ", _lastTrackPlayed);
  _playerStatus = IS_STOPPED;
  _lastTrackPlayed = -1;
  _lastStopTime = millis();
  if (_fadeOutTime > 0) {
    _lastStopTime = millis();
  } else {
    _MP3player->stopTrack();
  }
  _lastActionTime = _lastTrackPlayed;
}

void BtUtils::startOverAfterNoTouchTime(int seconds) {
  log_action("startOverAfterNoTouchTime = ", seconds);
  _startOverIfIdleTime = (unsigned long)seconds * 1000;
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
  log_action("wait time (milliseconds) completed: ", _startDelay);
  startTrack(_lastTrackPlayed);
}

void BtUtils::doTimerTasks() {

  // Every time through the loop, see if it's time to increase/decrease the volume
  // or to start a time-delayed track

  _startTrackIfStartDelayReached();
  _doVolumeFadeIn();
  _doVolumeFadeOut();
}
