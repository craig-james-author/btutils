/*******************************************************************************

 Bare Conductive "Flappy Bird" game
 ---------------------------------------

 flappy_bird.ino - proximity-based game using the Adafruit NeoPixel shield
 
 In order to use the NeoPixel shield and play sounds simultaneously, the 
 connection between the digital pin D6 and Din on the NeoPixel shield needs 
 to be interrupted by cutting a trace. The NeoPixel Din pin then needs to be 
 connected to pin 11 on the Touch Board. 
 
 To ensure that the game runs smoothly, the sound effect audio files need be 
 mono at a sample rate of 44100 Hz and MP3 encoded with a constant bitrate of 
 128 kbps. 
 
 You can download example audio samples for this code here:
 
 
 Bare Conductive code written by Pascal Loose and Stefan Dzisiewski-Smith.

 This work is licensed under a MIT license https://opensource.org/licenses/MIT
 
 Copyright (c) 2017, Bare Conductive
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*******************************************************************************/
// uncomment the line below to enable audio output
// #define enable_audio

// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// led includes
#include <Adafruit_NeoPixel.h>

#define PIN 6 // If you haven't enabled audio this must be PIN 6
               // If you have enabled audio, change it to PIN 11
Adafruit_NeoPixel panel = Adafruit_NeoPixel(5 * 8, PIN, NEO_GRB + NEO_KHZ800);

#define LOW_DIFF 0
#define HIGH_DIFF 50
#define filterWeight 0.7f // 0.0f to 1.0f - higher value = more smoothing
float lastProx = 0;

#ifdef enable_audio
  // mp3 includes
  #include <SPI.h>
  #include <SFEMP3Shield.h>

  // mp3 variables
  SFEMP3Shield MP3player;
  byte result;

  // sd card instantiation
  SdFat sd;
#endif //enable_audio

// game state initialisation
uint8_t userPosition = 0;
uint8_t holePosition1 = 0;
uint8_t holePosition2 = 4;
uint8_t prevholePosition1;
uint8_t prevholePosition2;

// game reset
bool gameOver = true; // when uploading the code wait until start is pressed
uint8_t lineIndex1 = -1;
uint8_t lineIndex2 = -1;
uint8_t levelIndex = 0;
unsigned long previousMillis = 0;
bool startSecondWall = false;
int intervalFrameMS = 800; // the speed at which the wall is falling at the start of the game 
                           // reducing the interval increases the difficulty of the game
                           // this is set in milliseconds
                          
// game variables
uint8_t roundOver = 8 - 1; // defines at which line the round is over
uint8_t startButton = 0; // electrode that is used as the "Start" button
uint8_t userButton = 11; // electrode that is moving the user
uint8_t volumePlus = 2; // electrode to increase the volume
uint8_t volumeMinus = 3; // electrode to decrease the volume
uint8_t brightnessPlus = 5; // electrode to increase the brightness
uint8_t brightnessMinus = 6; // electrode to decrease the brightness
uint8_t volumeRight = 10; // initial value for the right channel volume
uint8_t volumeLeft = 10; // initial value for the left channel volume
uint8_t brightnessArray[6] = {16, 32, 64, 128, 192, 255}; // set values to set the brightness
                                                          // 255 is the maximum brightness 
uint8_t brightnessArrayIndex = sizeof(brightnessArray) - 1; // initial value for the array index
int minIntervalFrameMS = 400;
bool played = true;

// led number display arrays
uint8_t arrayx0[12] = {0, 1, 2, 8, 10, 16, 18, 24, 26, 32, 33, 34};
uint8_t arrayx1[7] = {0, 8, 16, 24, 32};
uint8_t arrayx2[11] = {0, 1, 2, 10, 16, 17, 18, 24, 32, 33, 34};
uint8_t arrayx3[11] = {0, 1, 2, 8, 16, 17, 18, 24, 32, 33, 34};
uint8_t arrayx4[9] = {0, 8, 16, 17, 18, 24, 26, 32, 34};
uint8_t arrayx5[11] = {0 , 1, 2, 8, 16, 17, 18, 26, 32, 33, 34};
uint8_t arrayx6[12] = {0, 1, 2, 8, 10, 16, 17, 18, 26, 32, 33, 34};
uint8_t arrayx7[7] = {0, 8, 16, 24, 32, 33, 34};
uint8_t arrayx8[13] = {0, 1, 2, 8, 10, 16, 17, 18, 24, 26, 32, 33, 34};
uint8_t arrayx9[12] = {0, 1, 2, 8, 16, 17, 18, 24, 26, 32, 33, 34}; 

void setup(){  
  panel.begin();
  panel.setPixelColor(7, panel.Color(32, 32, 0)); // display user at each reboot
  panel.show();
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) ; {} //uncomment when using the serial monitor 
  if(!MPR121.begin(MPR121_ADDR)){ 
    Serial.println("error setting up MPR121");  
    switch(MPR121.getError()){
      case NO_ERROR:
        Serial.println("no error");
        break;  
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;      
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;      
    }
    while(1);
  }

  MPR121.setTouchThreshold(userButton, 4);
  MPR121.setReleaseThreshold(userButton, 2);  

  #ifdef enable_audio
    if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
    MP3player.begin();
    // to decrease the volume choose higher values for setVolume()
    MP3player.setVolume(volumeLeft,volumeRight); 
  #endif // enable_audio
}

void loop(){
  unsigned long currentMillis = millis();

  moveUser();
  
  if(MPR121.touchStatusChanged()){
    electrodeTouched();
  }
  
  if(!gameOver) {
    if(currentMillis - previousMillis >= intervalFrameMS) {
      Serial.print(levelIndex);
      previousMillis = currentMillis;
      checkStatus();
      
      if(gameOver) {
        delay(1000);
        drawScore();
        panel.show();
      } else {
        updateWall();
      }
    }
  }
}

void electrodeTouched() {
  MPR121.updateTouchData();

  // the user can reset the game whenever
  // this resets the game reset variables
  if(MPR121.isNewTouch(startButton)){
    gameOver = false;
    startSecondWall = false;
    levelIndex = 0;
    lineIndex1 = -1;
    lineIndex2 = -1;
    previousMillis = 0;
    intervalFrameMS = 800;
  }

  if(MPR121.isNewTouch(volumePlus)) {
    if(volumeLeft > 10 && volumeRight > 10) {
      volumeLeft = volumeLeft - 10;
      volumeRight = volumeRight - 10;
    }
    #ifdef enable_audio
      MP3player.setVolume(volumeLeft,volumeRight);
    #endif // enable_audio
  }

  if(MPR121.isNewTouch(volumeMinus)) {
    if(volumeLeft < 170 && volumeRight < 170) {
      volumeLeft = volumeLeft + 10;
      volumeRight = volumeRight + 10;
    }
    #ifdef enable_audio
      MP3player.setVolume(volumeLeft,volumeRight);
    #endif // enable_audio 
  }

  if(MPR121.isNewTouch(brightnessPlus)) {
    if(brightnessArrayIndex < sizeof(brightnessArray)-1) {
      brightnessArrayIndex++;
    }
    panel.setBrightness(brightnessArray[brightnessArrayIndex]);
  }

  if(MPR121.isNewTouch(brightnessMinus)) {
    if(brightnessArrayIndex > 0) {
      brightnessArrayIndex--;
    }
    panel.setBrightness(brightnessArray[brightnessArrayIndex]);
  }
}

void moveUser() {

  // only update the user's position when the round is not over
  if(lineIndex1 != roundOver && lineIndex2 != roundOver) {
    MPR121.updateAll();
    int reading = MPR121.getBaselineData(userButton)-MPR121.getFilteredData(userButton);
    unsigned int prox = constrain(reading, LOW_DIFF, HIGH_DIFF);
    lastProx = (filterWeight*lastProx) + ((1-filterWeight)*(float)prox);
    uint8_t thisOutput = (uint8_t)map(lastProx,LOW_DIFF,HIGH_DIFF,0,5);
    userPosition = thisOutput;
    drawUser();
  }
}

void drawUser() {

  // only play TRACK000.mp3 when the user jumps from 0
  if(userPosition != 0) {
    if(played) {
      playTrack(0);
      played = false;
    }
  } else { 
    played = true;
  }
  eraseTrace();
  panel.setPixelColor(((userPosition+1)*8)-1, panel.Color(32, 32, 0));
  panel.show(); 
}

void updateWall() {
  eraseField();

  // create a new hole when the round is over
  if(lineIndex1 == roundOver) {
    lineIndex1 = 0;
    holePosition1 = random(0, 5);
    decreaseIntervalFrameMS();
  } else {
    lineIndex1++;
  }
  drawWall(lineIndex1, holePosition1);

  if(startSecondWall) {
    if(lineIndex2 == roundOver) {
      lineIndex2 = 0;
      holePosition2 = random(0, 5);
      decreaseIntervalFrameMS();
    } else {
      lineIndex2++;
    }
    drawWall(lineIndex2, holePosition2);
  }

  panel.show();
}

void drawWall(uint8_t lineIndex, uint8_t holePosition) {
  uint32_t colour = panel.Color(0, 32, 0);

  // if the user's position is identical to the position of the hole and the round is over,
  // change the wall's colour to white and play TRACK001.mp3
  if(userPosition == holePosition && lineIndex == roundOver) {
    colour = panel.Color(32, 32, 32);
    playTrack(1);
  }

  // if the user's position is not identical to the position of the hole and the round is over,
  // change the wall's colour to red and play TRACK002.mp3
  if(userPosition != holePosition && lineIndex == roundOver) {
    colour = panel.Color(32, 0, 0);
    playTrack(2);
  }

  // draw a wall using the specified colour
  for(int i = 0; i < 5; i++) {
    if(i != holePosition) {
      panel.setPixelColor(lineIndex + (i*8), colour);
    }
  }
}

void checkStatus() {

  // check the status of the game only when the round is over
  if(lineIndex1 == roundOver) {
    if(userPosition == holePosition1) {
      levelIndex++;
    } else {
      gameOver = true;
    }
  } else if (lineIndex2 == roundOver) {
    if(userPosition == holePosition2) {
      levelIndex++;
    } else {
      gameOver = true;
    }
  } else if (lineIndex1 == 3) {
    // If the first wall passed the middle, start the second wall
    // This happens only once
    startSecondWall = true;
  }
}

void decreaseIntervalFrameMS() {

  // increase the speed of the wall
  if(intervalFrameMS > minIntervalFrameMS) {
      intervalFrameMS = intervalFrameMS - 100;
  } else {
      intervalFrameMS = minIntervalFrameMS;
  }
}

void playTrack(uint8_t trackNumber) {

  // if audio is enabled, stop any track that is playing and play new specific track
  #ifdef enable_audio
      MP3player.stopTrack();
      MP3player.playTrack(trackNumber);
  #endif
}

void eraseTrace() {
  
  // erase the user's previous position
  if(lineIndex1 != roundOver || lineIndex2 != roundOver) {
    for(uint16_t i = 0; i <= 5; i++) {
        panel.setPixelColor((i*8)-1, panel.Color(0, 0, 0));
    }
  }
}

void eraseField() {

  // erase the game field
  for(uint16_t i = 0; i <= panel.numPixels(); i++) {
    if(i != ((userPosition+1)*8)-1) {
      panel.setPixelColor(i, panel.Color(0, 0, 0));
    }
  }
}

void erase() {

  // erase the whole display
  for(uint16_t i=0; i<panel.numPixels(); i++) {
      panel.setPixelColor(i, panel.Color(0, 0, 0));
  }  
}

void drawScore(){
  erase();
  if(levelIndex > 99) return; // only 0 - 99 can be displayed
  switch(levelIndex % 10) {
    // calculate the modulus of the levelIndex
    // this is the number x0-x9
    case 0:
      printArray(arrayx0, 12, false);
      break;
    case 1:
      printArray(arrayx1, 7, false);
      break;
    case 2:
      printArray(arrayx2, 11, false);
      break;
    case 3:
      printArray(arrayx3, 11, false);
      break;
    case 4:
      printArray(arrayx4, 9, false);
      break;
    case 5:
      printArray(arrayx5, 11, false);
      break;
    case 6:
      printArray(arrayx6, 12, false);
      break;
    case 7:
      printArray(arrayx7, 7, false);
      break;
    case 8:
      printArray(arrayx8, 13, false);
      break;
    case 9:
      printArray(arrayx9, 12, false);
      break; 
  }
  switch(levelIndex / 10) {
    // divide the levelIndex by 10
    // this is the number 1x-9x
    case 1:
      printArray(arrayx1, 7, true);
      break;
    case 2:
      printArray(arrayx2, 11, true);
      break;
    case 3:
      printArray(arrayx3, 11, true);
      break;
    case 4:
      printArray(arrayx4, 9, true);
      break;
    case 5:
      printArray(arrayx5, 11, true);
      break;
    case 6:
      printArray(arrayx6, 12, true);
      break;
    case 7:
      printArray(arrayx7, 7, true);
      break;
    case 8:
      printArray(arrayx8, 13, true);
      break;
    case 9:
      printArray(arrayx9, 12, true);
      break; 
  }
}

void printArray(uint8_t *arrayDraw, uint8_t arrayLength, bool shift) {
  for(uint8_t i = 0; i < arrayLength; i++) {
    if(shift) {
      panel.setPixelColor(arrayDraw[i]+4, panel.Color(32, 32, 32));
    } else {
      panel.setPixelColor(arrayDraw[i], panel.Color(32, 32, 32));
    }
  }
}

