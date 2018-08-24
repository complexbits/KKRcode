
/*******************************************************************************

 Bare Conductive Random Touch MP3 player
 ---------------------------------------
 
 Random_Touch_MP3.ino - touch triggered MP3 playback taken randomly from microSD

 You need twelve folders named E0 to E11 in the root of the microSD card. Each 
 of these folders can have as many MP3 files in as you like, named however you 
 like (as long as they have a .mp3 file extension).

 When you touch electrode E0, a random file from the E0 folder will play. When 
 you touch electrode E1, a random file from the E1 folder will play, and so on. 
 You should note that this is not the same file structure as for Touch_MP3.

  SD card    
  │
  ├──E0
  │    some_mp3.mp3  
  │    another_mp3.mp3
  │
  ├──E1
  │    dog-barking-1.mp3
  │    dog-barking-2.mp3
  │    dog-growling.mp3
  │    dog-howling.mp3
  │
  ├──E2
  │    1.mp3
  │    2.mp3
  │    3.mp3
  │    4.mp3
  │    5.mp3
  │    6.mp3
  │
  └──...and so on for other electrodes   
 
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
#include <SdFatUtil.h> 
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = 0;
int makeyPin = 10; //DDT
int makeyTouch = 0; //DDT



// sd card instantiation
SdFat sd;
SdFile file;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

// mp3 behaviour defines
#define REPLAY_MODE TRUE  // By default, touching an electrode repeatedly will 
                          // play the track again from the start each time.
                          //
                          // If you set this to FALSE, repeatedly touching an 
                          // electrode will stop the track if it is already 
                          // playing, or play it from the start if it

void setup(){  
  Serial.begin(192000);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  Serial.println("...");
  Serial.println("Bare Conductive Random Touch MP3 player");

  // initialise the Arduino pseudo-random number generator with 
  // a bit of noise for extra randomness - this is good general practice
  randomSeed(analogRead(0));

  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);


  MPR121.setTouchThreshold(5); // Are these milliseconds? Volts?
  MPR121.setReleaseThreshold(50);

  result = MP3player.begin();
  MP3player.setVolume(10,10);
 
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
   }
}

// Initialize touch counters
int prevTouch=0;
int newTouch=0; 
int prevNoTouch=0; 

// Initialize touch timers
float touchSecs=0; 
float touchSecs_init=0;

float notouchSecs=0;
float notouchSecs_init=0;

int longTouch = 20000;
int longNoTouch = 100000;
int minTouchCycle = 5000;


void loop(){

  readMakey();

}


void readMakey(){ 

 makeyTouch = digitalRead(makeyPin);   // read the input pin
 
   if (makeyTouch){ // If the input pin is HIGH //////////////////////////////////
    
        if (prevTouch==0){  // new touch
          
            touchSecs_init=float(now()); // timestamp for touch initiation
            digitalWrite(LED_BUILTIN, HIGH); // light LED
            Serial.println("pin was just touched");
            
            playRandomTrack(1);  // play track on new touch (normal soundmode=1)

            // Reset gap counters   
            prevNoTouch=0;          
            notouchSecs=0;
            notouchSecs_init=0;

            // count new touches
            newTouch++;
        }
        
        prevTouch++;  // count continuous touch loop iterations
        touchSecs = float(now()) - touchSecs_init; // current touch length

        if (touchSecs>=longTouch){
            // Do stuff if the touch has been going a while. (Long kiss soundmode=2)
            playRandomTrack(2);
            
            // reset touch counter
            prevTouch=0;
         }
           
    }else{ // Input pin LOW ////////////////////////////////////////////
      
        if (prevNoTouch==0){ // new gap
            
            notouchSecs_init=float(now()); // timestamp for gap initiation
            digitalWrite(LED_BUILTIN, LOW); // stop LED 

            // Reset touch counters
            prevTouch = 0; 
            touchSecs=0;
            touchSecs_init=0;

        }
      
      prevNoTouch++;  // count continous gap loop iterations
      notouchSecs = float(now()) - notouchSecs_init; // current gap length

      if (notouchSecs>=longNoTouch){
          // Do stuff if the gap has been going a while. (Callout soundmode=0)
          playRandomTrack(0);
          
          // reset gap counter
          prevNoTouch=0;
      }    
   }
   
}



/** Replacing void playRandomTrack(int electrode){ with void playRandomTrack(int soundmode){
    Soundmodes will be stored in folders accessed similarly to electrode folders.
    Soundmodes are activated by kiss frequency detection
**/

void playRandomTrack(int soundmode){
  

	// build our directory name from the electrode
	char thisFilename[14]; // allow for 8 + 1 + 3 + 1 (8.3 with terminating null char)
	// start with "E00" as a placeholder
	char thisDirname[] = "E00";


		// if <10, replace first digit...
		thisDirname[1] = soundmode + '0';
		// ...and add a null terminating character
		thisDirname[2] = 0;


	sd.chdir(); // set working directory back to root (in case we were anywhere else)
	if(!sd.chdir(thisDirname)){ // select our directory
		Serial.println("error selecting directory"); // error message if reqd.
	}

	size_t filenameLen;
	char* matchPtr;
	unsigned int numMP3files = 0;

	// we're going to look for and count
	// the MP3 files in our target directory
  while (file.openNext(sd.vwd(), O_READ)) {
    file.getFilename(thisFilename);
    file.close();

    // sorry to do this all without the String object, but it was
    // causing strange lockup issues
    filenameLen = strlen(thisFilename);
    matchPtr = strstr(thisFilename, ".MP3");
    // basically, if the filename ends in .MP3, we increment our MP3 count
    if(matchPtr-thisFilename==filenameLen-4) numMP3files++;
  }

  // generate a random number, representing the file we will play
  unsigned int chosenFile = random(numMP3files);

  // loop through files again - it's repetitive, but saves
  // the RAM we would need to save all the filenames for subsequent access
	unsigned int fileCtr = 0;

 	sd.chdir(); // set working directory back to root (to reset the file crawler below)
	if(!sd.chdir(thisDirname)){ // select our directory (again)
		Serial.println("error selecting directory"); // error message if reqd.
	} 

  while (file.openNext(sd.vwd(), O_READ)) {
    file.getFilename(thisFilename);
    file.close();

    filenameLen = strlen(thisFilename);
    matchPtr = strstr(thisFilename, ".MP3");
    // this time, if we find an MP3 file...
    if(matchPtr-thisFilename==filenameLen-4){
    	// ...we check if it's the one we want, and if so play it...
    	if(fileCtr==chosenFile){
    		// this only works because we're in the correct directory
    		// (via sd.chdir() and only because sd is shared with the MP3 player)
				Serial.print("playing track ");
				Serial.println(thisFilename); // should update this for long file names
				MP3player.playMP3(thisFilename);
				return;
    	} else {
    			// ...otherwise we increment our counter
    		fileCtr++;
    	}
    }
  }  
}
