/*******************************************************************************

 Bare Conductive Multi Board Touch MP3 player
 ------------------------------------------
 
 primary_mp3_board.ino - touch triggered MP3 playback from multiple Touch Boards

 Touch electrodes 0 to 11 locally to play TRACK000.MP3 to TRACK011.MP3 from 
 the primary micro SD card. Touch electrodes 0 to 11 on the first secondary board
 to play TRACK012.MP3 to TRACK023.MP3 from the primary micro SD card. Touch 
 electrodes 0 to 11 on the second secondary board to play TRACK024.MP3 to 
 TRACK035.MP3 from the primary micro SD card, and so on. 

 Maximum total number of boards is 7 (one primary board and six 
 secondary boards). All boards must share a common ground connection,
 with TX on all the secondary boards commoned together and connected to RX on
 the primary board. The first secondary board must have a connection between 
 its A0 and A0 on the primary board. The second secondary board must have a 
 connection between its A0 and A1 on the primary board, and so on.

 Each board must also be powered, although up to 4 boards can share power by
 commoning up the 5V connection between them.
 
 Based on code by Jim Lindblom and plenty of inspiration from the Freescale 
 Semiconductor datasheets and application notes.
 
 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.
 
 This work is licensed under a MIT license https://opensource.org/licenses/MIT
 
 Copyright (c) 2016, Bare Conductive
 
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

// number of boards config
// you can reduce this to improve response time, but the code will work fine with it 
// left at 6 - do not try to increase this beyond 6!
const int numSecondaryBoards = 6;
const int totalNumElectrodes = (numSecondaryBoards+1)*12;

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = 0;

// sd card instantiation
SdFat sd;

// serial comms
const int serialPacketSize = 13;
uint8_t incomingPacket[serialPacketSize];

// secondary board touch variables
bool thisExternalTouchStatus[numSecondaryBoards][12];
bool lastExternalTouchStatus[numSecondaryBoards][12];

// compound touch variables
bool touchStatusChanged = false;
bool isNewTouch[totalNumElectrodes];
bool isNewRelease[totalNumElectrodes];
int numTouches = 0;

void setup(){  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) {}; //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Multi Board Touch MP3 player");

  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  result = MP3player.begin();
  MP3player.setVolume(10,10);
 
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }

  for(int i=0; i<numSecondaryBoards; i++){
    for(int j=0; j<12; j++){
      thisExternalTouchStatus[i][j] = false;
      lastExternalTouchStatus[i][j] = false;
    }
  }

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }   

  for(int a=A0; a<=A5; a++){
    pinMode(a, OUTPUT);
    digitalWrite(a, LOW); 
  }

  Serial1.begin(57600);
  Serial1.setTimeout(3); // 3ms is more than enough time for a packet to be transferred (should take 2.26ms)
  delay(100);
}

void loop(){
  
  // reset everything  that we combine from the two boards
  resetCompoundVariables();
  
  readLocalTouchInputs();
  
  readRemoteTouchInputs();
  
  processTouchInputs();
  
}


void readLocalTouchInputs(){

  // update our compound data on the local touch status

  if(MPR121.touchStatusChanged()){
    MPR121.updateTouchData();
    touchStatusChanged = true;
    
    for(int i=0; i<12; i++){
      isNewTouch[i] = MPR121.isNewTouch(i);
      isNewRelease[i] = MPR121.isNewRelease(i);
    }
  }
  numTouches+=MPR121.getNumTouches();

}

void readRemoteTouchInputs(){

  uint8_t numBytesRead;

  for(int a=A0; a<A0+numSecondaryBoards; a++){

    digitalWrite(a, HIGH);

    // try to read a full packet
    numBytesRead = Serial1.readBytesUntil(0x00, incomingPacket, serialPacketSize);

    // only process a complete packet
    if(numBytesRead==serialPacketSize){
      // save last status to detect touch / release edges
      for(int i=0; i<12; i++){
        lastExternalTouchStatus[a-A0][i] = thisExternalTouchStatus[a-A0][i];
      }
      
      if(incomingPacket[0] == 'T'){ // ensure we are synced with the packet 'header'
        for(int i=0; i<12; i++){
          if(incomingPacket[i+1]=='1'){
            thisExternalTouchStatus[a-A0][i] = true;
          } else {
            thisExternalTouchStatus[a-A0][i] = false;
          }
        }
      } 

      // now that we have read the remote touch data, merge it with the local data
      for(int i=0; i<12; i++){
        if(lastExternalTouchStatus[a-A0][i] != thisExternalTouchStatus[a-A0][i]){
          touchStatusChanged = true;
          if(thisExternalTouchStatus[a-A0][i]){
            // shift remote data up the array by 12 so as not to overwrite local data
            isNewTouch[i+(12*((a-A0)+1))] = true;
          } else {
            isNewRelease[i+(12*((a-A0)+1))] = true;
          }
        }

        // add any new touches to the touch count
        if(thisExternalTouchStatus[a-A0][i]){
          numTouches++;
        }
      }

    } else {
      Serial.print("incomplete packet from secondary board ");
      Serial.println(a-A0, DEC);
      Serial.print("number of bytes read was ");
      Serial.println(numBytesRead);
    }

    digitalWrite(a, LOW);
  }
}

void processTouchInputs(){
  // only make an action if we have one or fewer pins touched
  // ignore multiple touches
  
  if(numTouches <= 1){
    for (int i=0; i < totalNumElectrodes; i++){  // Check which electrodes were pressed
      if(isNewTouch[i]){   
        //pin i was just touched
        Serial.print("pin ");
        Serial.print(i);
        Serial.println(" was just touched");
        digitalWrite(LED_BUILTIN, HIGH);
        
        if(MP3player.isPlaying()){
          if(lastPlayed==i){
            // if we're already playing the requested track, stop it
            MP3player.stopTrack();
            Serial.print("stopping track ");
            Serial.println(i);
          } else {
            // if we're already playing a different track, stop that 
            // one and play the newly requested one
            MP3player.stopTrack();
            MP3player.playTrack(i);
            Serial.print("playing track ");
            Serial.println(i);
            
            // don't forget to update lastPlayed - without it we don't
            // have a history
            lastPlayed = i;
          }
        } else {
          // if we're playing nothing, play the requested track 
          // and update lastplayed
          MP3player.playTrack(i);
          Serial.print("playing track ");
          Serial.println(i);
          lastPlayed = i;
        }    
      }else{
        if(isNewRelease[i]){
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          digitalWrite(LED_BUILTIN, LOW);
       } 
      }
    }
  }
}

void resetCompoundVariables(){

  // simple reset for all coumpound variables

  touchStatusChanged = false;
  numTouches = 0;

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }  
}
