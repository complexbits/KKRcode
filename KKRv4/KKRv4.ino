/*******************************************************************************
Kiss Kiss Revolution v4
Interaction Logic & Sound Control for the Bare Conductive Board

Jamie Schwettmann
@complexbits
@jstarnow

*******************************************************************************/

// compiler error handling (environment control)
#include "Compiler_Errors.h"

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h> 
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;

// sd card instantiation
SdFat sd;
SdFile file;

// incoming Makey Makey input
int makeyPin = 10; //DDT
int makeyTouch = 0; //DDT

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

// Kiss Timing Variables
int maxKissLength=4; // max kiss length in Seconds
int maxGapLength=15; // max gap length in Seconds
int maxKPM = 120; // fastest kiss speed in Kisses Per Minute


void setup(){  
  Serial.begin(192000);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("...");
  Serial.println("Bare Conductive Random Touch MP3 player");
  
  // initialise the Arduino pseudo-random number generator with 
  // a bit of noise for extra randomness - this is good general practice
  randomSeed(analogRead(0));
  
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
  
  result = MP3player.begin();
  MP3player.setVolume(10,10);
  
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }
}

// Initialize kiss counters
long int prevKiss=0;
long int newKiss=0; 
long int prevNoKiss=0; 

// Initialize touch timers
float kissTime=0; 
float kissTime_init=0;

float gapTime=0;
float gapTime_init=0;

float longKiss = float(maxKissLength)*1000;
float longGap = float(maxGapLength)*1000;
float thisKPM=0;
float thisKPM_time=0;
float lastKPM_time=0;

// Main Loop
void loop(){
  readMakey();
}

// Touch Detection & Logic
void readMakey(){ 
  
  makeyTouch = digitalRead(makeyPin);   // read the input pin
  
  if (makeyTouch){ // If the input pin is HIGH //////////////////////////////////
    
    if ( prevKiss==0 ){  // new kiss
      
      kissTime_init=float(millis()); // timestamp for touch initiation
      digitalWrite(LED_BUILTIN, HIGH); // light LED
      Serial.println("pin was just touched");
      
      // Reset gap counters   
      prevNoKiss=0;          
      gapTime=0;
      gapTime_init=0;
      
      newKiss++; // count new kisses
      
      if ( newKiss % 5 == 0 ){ // every 5 kisses, calculate KPM
        thisKPM_time=float(millis()); // timestamp now
        thisKPM = 5.0/((thisKPM_time - lastKPM_time)/60000); // 5 kisses in this many minutes
        Serial.print("KPM ");
        Serial.println(thisKPM);
        lastKPM_time=thisKPM_time; // update last timestamp
      }
      
      if (!MP3player.isPlaying()){ // if a track isn't currently playing...
        if (thisKPM >= maxKPM){
          playRandomSound(3); // Fast Kiss soundmode=3
        }else{
          playRandomSound(1);  // Normal Kiss soundmode=1
        }
      }
    }
    
    prevKiss++;  // count continuous kiss loop iterations
    kissTime = float(millis()) - kissTime_init; // current kiss length
    
    if ( kissTime >= longKiss && !MP3player.isPlaying()){ // If the kiss has been going a while...
      
      playRandomSound(2); // Long kiss soundmode=2         
      kissTime_init=float(millis()); // reset kiss length timer
    }
    
  }else{ // Input pin LOW ////////////////////////////////////////////
    
    if ( prevNoKiss==0 ){ // new gap
      
      gapTime_init=float(millis()); // timestamp for gap initiation
      digitalWrite(LED_BUILTIN, LOW); // stop LED 
      
      // Reset touch counters
      prevKiss = 0; 
      kissTime=0;
      kissTime_init=0;
    }
    
    prevNoKiss++;  // count continous gap loop iterations
    gapTime = float(millis()) - gapTime_init; // current gap length
    
    if ( gapTime >= longGap ){ // If the gap has been going a while...
      
      playRandomSound(0); // Callout soundmode=0        
      prevNoKiss=0; // reset gap counter
    }    
  }
}

// playRandomSound setup /////////////////////////////////////////////

char* soundDirs[]={"Attract","Normal","Long","Fast"}; // Specify directory names on SD card
int filesPerDir[]={7,122,8,4}; // TODO: Add file number detection function to setup routine

String thisSound;
char playSound[8];

void playRandomSound (int soundmode){
  
  Serial.print("Soundmode activated: ");
  Serial.println(soundDirs[soundmode]);
  
  sd.chdir(); // Reset directory
  sd.chdir(soundDirs[soundmode]); // Change to appropriate soundmode directory
  
  thisSound = String(random(0, filesPerDir[soundmode]), DEC); 
  thisSound = String(thisSound + ".MP3");
  thisSound.toCharArray(playSound, 8);
  MP3player.playMP3(playSound);
  
  Serial.print("Playing sound: ");
  Serial.print(soundDirs[soundmode]);
  Serial.print("/");
  Serial.print(playSound);
  Serial.println();
  //Serial.println(playSound);
  
}  
