#include <avr/pgmspace.h>
#include "pitches.h"

#define NUM_TONES 78
//Mario tone from http://www.linuxcircle.com/2013/03/31/playing-mario-bros-tune-with-arduino-and-piezo-buzzer/

PROGMEM prog_uint16_t melody[] = {
  
  //0
  NOTE_E7, NOTE_E7, 0, NOTE_E7, 
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
  NOTE_G6, 0, 0, 0, 

  //16
  NOTE_C7, 0, 0, NOTE_G6, 
  0, 0, NOTE_E6, 0, 
  0, NOTE_A6, 0, NOTE_B6, 
  0, NOTE_AS6, NOTE_A6, 0, 

  //32
  NOTE_G6, NOTE_E7, NOTE_G7, //Tones on this line have shorter duration
  NOTE_A7, 0, NOTE_F7, NOTE_G7, 
  0, NOTE_E7, 0,NOTE_C7, 
  NOTE_D7, NOTE_B6, 0, 0,

  //47
  NOTE_C7, 0, 0, NOTE_G6, 
  0, 0, NOTE_E6, 0, 
  0, NOTE_A6, 0, NOTE_B6, 
  0, NOTE_AS6, NOTE_A6, 0, 

  //63
  NOTE_G6, NOTE_E7, NOTE_G7,  //Tones on this line have shorter duration
  NOTE_A7, 0, NOTE_F7, NOTE_G7, 
  0, NOTE_E7, 0,NOTE_C7, 
  NOTE_D7, NOTE_B6, 0, 0

  //78
};



int getToneAtThisPosition(int position){
  return pgm_read_word_near(melody + position);
}

int getDurationAtThisPosition(int position){
  if((32 <= position && position <= 34)
      || (63 <= position && position <= 65)){
        return 9;
  } else {
    return 12;
  }

}




