// compiler error handling
#include "Compiler_Errors.h"

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

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = 0;

// mp3 behaviour defines
#define REPLAY_MODE TRUE  // By default, touching an electrode repeatedly will 
                          // play the track again from the start each time.
                          //
                          // If you set this to FALSE, repeatedly touching an 
                          // electrode will stop the track if it is already 
                          // playing, or play it from the start if it is not.

// touch behaviour definitions
#define firstPin 0
#define lastPin 11

// sd card instantiation
SdFat sd;

void setup() {  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) ; {} //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Touch MP3 player");

  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  result = MP3player.begin();
  MP3player.setVolume(10,10);
 
  if (result != 0)  {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }
   
}

void loop() {
  readTouchInputs();
}

void readTouchInputs() {
  if (MPR121.touchStatusChanged()) {
    
    MPR121.updateTouchData();
    for (int i=0; i < 12; i++) {  // Check which electrodes were pressed
      if (MPR121.isNewTouch(i)) {

	//pin i was just touched
	Serial.print("pin ");
	Serial.print(i);
	Serial.println(" was just touched");
	digitalWrite(LED_BUILTIN, HIGH);

	MP3player.enableTestSineWave(100+i);
      }
      else {
	if (MPR121.isNewRelease(i)) {
	  Serial.print("pin ");
	  Serial.print(i);
	  Serial.println(" is no longer being touched");
	  digitalWrite(LED_BUILTIN, LOW);

	  MP3player.disableTestSineWave();
	}
      }
    }
  }
}
