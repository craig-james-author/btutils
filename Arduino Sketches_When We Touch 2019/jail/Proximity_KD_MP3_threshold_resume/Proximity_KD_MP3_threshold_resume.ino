
/*******************************************************************************

 Bare Conductive Proximity MP3 player
 ------------------------------
 
 proximity_mp3.ino - proximity triggered MP3 playback
 
 Based on code by Jim Lindblom and plenty of inspiration from the Freescale 
 Semiconductor datasheets and application notes.
 
 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.
 
 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

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
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = -1;

// touch behaviour definitions
#define firstPin 0
#define lastPin 11

// sd card instantiation
SdFat sd;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

void setup(){  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) ; {} //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Proximity MP3 player");

  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);
  
  // Changes from Touch MP3
  
  // this is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch; the lower the number the further away the start
  // the bigger the number, the close you need to be the start it
  MPR121.setTouchThreshold(1);
  
  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  // default value is 20 for touch; the higher the number the further away the release/stop
  MPR121.setReleaseThreshold(0);  

  result = MP3player.begin();
  MP3player.setVolume(10,10);// the higher the number, the lower the sound
 
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }
   
}

void loop(){
  readTouchInputs();
}

void readTouchInputs(){
  if(MPR121.touchStatusChanged()){
    
    MPR121.updateTouchData();
    
    // only make an action if we have one or fewer pins touched
    // ignore multiple touches
    
    if(MPR121.getNumTouches()<=1){
      for (int i=0; i < 12; i++){  // Check which electrodes were pressed
        if(MPR121.isNewTouch(i)){
          
          //pin i was just touched
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" was just touched");
          digitalWrite(LED_BUILTIN, HIGH);
            
          if(MP3player.isPlaying()){
            if (lastPlayed==i) {
              if (MP3player.getState() == paused_playback) {
                MP3player.resumeMusic();
                Serial.print("resuming track ");
                Serial.println(lastPlayed);
              } else {
                // if we're already playing the requested track, pause it
                MP3player.pauseMusic();
                Serial.print("pausing track ");
                Serial.println(i-firstPin);
              }
            } else {
              // if we're already playing a different track, stop that 
              // one and play the newly requested one
              MP3player.stopTrack();
              MP3player.playTrack(i-firstPin);
              Serial.print("playing track ");
              Serial.println(i-firstPin);
                  
              // don't forget to update lastPlayed - without it we don't
              // have a history
              lastPlayed = i;
            }
          } else {
            MP3player.stopTrack( );
            MP3player.playTrack(i-firstPin);
            Serial.print("playing track ");
            Serial.println(i-firstPin);
            lastPlayed = i;
          }
        }else if(MPR121.isNewRelease(i)){
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          Serial.print("pausing track ");
          Serial.println(lastPlayed);
          digitalWrite(LED_BUILTIN, LOW);
          MP3player.pauseMusic();
        } 
      }
    }
  }
}
