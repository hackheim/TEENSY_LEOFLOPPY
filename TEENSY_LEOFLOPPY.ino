/*
  LEOFLOPPY!
  midi to floppy player for Tenssy 3.1
  Presents itself as a MIDI Keyboard via USB on your computer and plays everything that you throw at it.
  
  
  Todo?:
  * selectable midi channels in CHANNEL_LOCK mode
  * selectable max/min channel when not in CHANNEL_LOCK mode
    - listen only to these selected channels (for now wee listen to ALL channels)
  * MIDI Passtrough / MIDI IN on serial istead of just USB
  * Complete arduino compatiblity.   For now only Teensy USB works
 
  Original inspirator and reference:
    RubleRail : http://www.schoar.de/tinkering/rumblerail/  GitHub : https://github.com/kiu/rumblerail
    
  Optionals Arduino ARCORE library:
    To use usbMIDI on an arduino Leonardo
    https://github.com/rkistner/arcore
    
*/



/* CONFIGURATION */

// CHANNEL LOCK
// Lock each floppydrive to its own midi channel
// if not defined the instrument listens to all inncomming channels and plays every note from the first available floppy in ascending order.
// If this is set, each floppychannel is mapped to its own floppy channel, from ch 1 to 8
#define CHANNEL_LOCK 

// ARDuINO Compatible
// Enable this to make it compile on an 16MHz arduino controller (leonardo or uno)
// We automaticly look if the teensy library is loaded, if not we enable arduino compatiblity
#ifndef CORE_TEENSY
  #define ARDUINO_COMPAT
#endif


#include "elapsedMillis.h"           // Includeing this library for compatiblity with arduino chips


/* GLOBAL VARIABLES */

const uint8_t FLOPPY_MAX_POS = 158;  // Number of steps on a floppy drive, be sure the floppys dont "crash" into the end of the track
volatile uint8_t floppy_pos[8] =     {0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    };   // Last floppy position, defaults to 0 at startup
volatile uint8_t floppy_pin_dir[8] = {3    , 5    , 7    , 9    , 11   , 13   , A1   , A3   };   // Pin of Stepper direction
volatile uint8_t floppy_pin_pos[8] = {2    , 4    , 6    , 8    , 10   , 12   , A0   , A2   };   // Pin of Stepper position
volatile bool floppy_dir[8] =        {LOW  , LOW  , LOW  , LOW  , LOW  , LOW  , LOW  , LOW  };   // Direction setting of stepper
volatile bool floppy_step[8] =       {LOW  , LOW  , LOW  , LOW  , LOW  , LOW  , LOW  , LOW  };   // Ocilating possition of stepper
int8_t floppy_note[8] =              {0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    };   // Note to play in note_tick[], 0 = off
uint16_t floppy_next[8]  =           {65434, 65434, 65434, 65434, 65434, 65434, 65434, 65434};   // Next position in counter to move stepper, invrements of note_tick[]
elapsedMillis floppy_endtime[8];     // Millisec playtime of note, used to stop a "wacked" note
elapsedMicros floppy_elapsed[8];     // counter for calculating next floppy movement position in microsec


/* Configuration finished */


static const uint16_t note_tick[] = {
  /*
  microsec counters for each note, calculated to 1MHz intervals / Microsecounds
  
  Generator: (Python)
    import math
    tick = float(8000000) / 8
    for note in range(0,128):
    freq = pow(2,(float(note)-69)/12) * 440 * 2
    ocr = int(tick / freq)
    print "\t%d, // Note %d" % (ocr, note)
  */
  //	8MHz/8 = 1MHz ticks
        61156, // Note 0
  	57723, // Note 1
  	54483, // Note 2
  	51425, // Note 3
  	48539, // Note 4
  	45815, // Note 5
  	43243, // Note 6
  	40816, // Note 7
  	38525, // Note 8
  	36363, // Note 9
  	34322, // Note 10
  	32396, // Note 11
  	30578, // Note 12
  	28861, // Note 13
  	27241, // Note 14
  	25712, // Note 15
  	24269, // Note 16
  	22907, // Note 17
  	21621, // Note 18
  	20408, // Note 19
  	19262, // Note 20
  	18181, // Note 21
  	17161, // Note 22
  	16198, // Note 23
  	15289, // Note 24
  	14430, // Note 25
  	13620, // Note 26
  	12856, // Note 27
  	12134, // Note 28
  	11453, // Note 29
  	10810, // Note 30
  	10204, // Note 31
  	9631, // Note 32
  	9090, // Note 33
  	8580, // Note 34
  	8099, // Note 35
  	7644, // Note 36
  	7215, // Note 37
  	6810, // Note 38
  	6428, // Note 39
  	6067, // Note 40
  	5726, // Note 41
  	5405, // Note 42
  	5102, // Note 43
  	4815, // Note 44
  	4545, // Note 45
  	4290, // Note 46
  	4049, // Note 47
  	3822, // Note 48
  	3607, // Note 49
  	3405, // Note 50
  	3214, // Note 51
  	3033, // Note 52
  	2863, // Note 53
  	2702, // Note 54
  	2551, // Note 55
  	2407, // Note 56
  	2272, // Note 57
  	2145, // Note 58
  	2024, // Note 59
  	1911, // Note 60
  	1803, // Note 61
  	1702, // Note 62
  	1607, // Note 63
  	1516, // Note 64
  	1431, // Note 65
  	1351, // Note 66
  	1275, // Note 67
  	1203, // Note 68
  	1136, // Note 69
  	1072, // Note 70
  	1012, // Note 71
  	955, // Note 72
  	901, // Note 73
  	851, // Note 74
  	803, // Note 75
  	758, // Note 76
  	715, // Note 77
  	675, // Note 78
  	637, // Note 79
  	601, // Note 80
  	568, // Note 81
  	536, // Note 82
  	506, // Note 83
  	477, // Note 84
  	450, // Note 85
  	425, // Note 86
  	401, // Note 87
  	379, // Note 88
  	357, // Note 89
  	337, // Note 90
  	318, // Note 91
  	300, // Note 92
  	284, // Note 93
  	268, // Note 94
  	253, // Note 95
  	238, // Note 96
  	225, // Note 97
  	212, // Note 98
  	200, // Note 99
  	189, // Note 100
  	178, // Note 101
  	168, // Note 102
  	159, // Note 103
  	150, // Note 104
  	142, // Note 105
  	134, // Note 106
  	126, // Note 107
  	119, // Note 108
  	112, // Note 109
  	106, // Note 110
  	100, // Note 111
  	94, // Note 112
  	89, // Note 113
  	84, // Note 114
  	79, // Note 115
  	75, // Note 116
  	71, // Note 117
  	67, // Note 118
  	63, // Note 119
  	59, // Note 120
  	56, // Note 121
  	53, // Note 122
  	50, // Note 123
  	47, // Note 124
  	44, // Note 125
  	42, // Note 126
  	39, // Note 127

};



void setup_pins() {
  /*
     Setup all floppy pins as output
  */
  for (uint8_t floppy = 0; floppy <= 7; floppy++) {
    pinMode(floppy_pin_dir[floppy], OUTPUT);
    pinMode(floppy_pin_pos[floppy], OUTPUT);
    //if (Serial) {Serial.print("Setting OUTPUT: "); Serial.print(floppy_pin_dir[floppy]); Serial.print(" "); Serial.print(floppy_pin_pos[floppy]); Serial.println(" ");}
  }
}

void go_home() {
  /*
     Returns all floppys to it's home/start position
     Sets direction pin HIGH and cycles 120 steps (240 up/downs)
  */
  for (uint8_t floppy = 0; floppy <= 7; floppy++) {
    digitalWrite(floppy_pin_dir[floppy], HIGH);
    boolean po = LOW;
    for (int i = 0; i < 240; i++) {
      po = !po;
      digitalWrite(floppy_pin_pos[floppy], po);
      delay(2);
    }
    digitalWrite(floppy_pin_dir[floppy], LOW);
  }
}




void OnNoteOff(byte channel, byte note, byte velocity) {
  /*
     Callback procedure from Teensy Midi library
  */
#ifdef CHANNEL_LOCK  
  if (channel > 9 && channel < 1) return;
  int floppy_ch = channel-1;

  if (floppy_note[channel-1] == note) {
    floppy_note[channel-1] = 0x0;
    floppy_endtime[channel-1] = 0;
  } else {
    
  }
#else
  for (uint8_t i = 0; i <= 7; i++)
    if (floppy_note[i] == note) {
      floppy_note[i] = 0x0;
      floppy_endtime[i] = 0;
      break;
    }
#endif
}


void OnNoteOn(byte channel, byte note, byte velocity) {
  /*
     Callback procedure from Teensy Midi library
  */
  if (channel > 9 && channel < 1) return;
  if (velocity = 0) { 
    // In the midi world a noteOff can be represented by a "noteOn" with velocity:0
    // If this is detected pass it over to the OnNoteOff procedure
    OnNoteOff(channel, note, velocity); 
    return;
  }
#ifdef CHANNEL_LOCK
  int floppy_ch = channel-1;
  
  floppy_note[floppy_ch] = note;
  floppy_next[floppy_ch] = micros()+10;
    
  if (floppy_pos[floppy_ch] > FLOPPY_MAX_POS/2) floppy_dir[floppy_ch] = HIGH;
  else floppy_dir[floppy_ch] = LOW;
  digitalWrite(floppy_pin_dir[floppy_ch], floppy_dir[floppy_ch]);
   
  floppy_endtime[floppy_ch] = 0;
  return;
  
#else

  for (uint8_t i = 0; i <= 7; i++) {
    if (floppy_note[i] == 0) {
      floppy_note[i] = note;
      floppy_next[i] = micros()+10;
      
      //  Change direction of stepper if over half way across...
      if (floppy_pos[i] > FLOPPY_MAX_POS/2) floppy_dir[i] = HIGH;
      else floppy_dir[i] = LOW;
      digitalWrite(floppy_pin_dir[i], floppy_dir[i]);
      // --- 

      // Save timestamp of note start
      floppy_endtime[i] = 0;
        

      
      break;
    }
  }
#endif

    
}


void setup() {
  //  Serial.begin(9600);
      //while (!Serial) {delay(100);}
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
#ifdef ARDUINO_COMPAT  
  // Arduino midi initialization
  // Not complete
  #ifdef MIDI_ENABLED
    // Initializes the Midi usb controller on a arduino Leonardo with the ARCORE library
  #endif
#else
  // Teensy Misi initialization
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);  
#endif
  Serial.println("beginning");
  setup_pins();
  go_home();
    Serial.println("blink");

  delay(1000);

  digitalWrite(13, LOW); 
  delay(250);
  digitalWrite(13, HIGH);
  delay(250);
  digitalWrite(13, LOW);


}



void loop() {  
  /* Main midi routine                 
     Check USB midi driver for input   
     This routine has its own callback functions:   NoNoteOn / OnNoteOff
  */
#ifdef ARDUINO_COMPAT
  //Read midi on arduino board
  // Not complete
#else
  // Read midi on Teensy board
  usbMIDI.read();
#endif
  /* Main "movement" loop                     
     1Mhz loops using microsec counter and elapsedMicros 
  */
  for (uint8_t i = 0; i <= 7; i++) {
    if (floppy_elapsed[i] >= note_tick[floppy_note[i]])  {    // If we have passed the rigth number of nanosec's
      if (floppy_note[i] != 0x0) {

        if (floppy_endtime[i] > 5000) {
           // Stop playing note if it has been pressed for 5sec+
           // Failsafe for midi apps that dont stop notes on pause signal.
           floppy_note[i] = 0x0;
           break;
        }
        
        floppy(i);                 // Move floppy head
        floppy_elapsed[i] = 0;     // Reset microsec counter for this floppy
        

        
      }
    }
  }
}





void floppy(uint8_t nr) {
  /* 
    Used to move floppy head in both directions
    Moves one step when fiered
  */
  // Direction selector
  if (floppy_pos[nr] == FLOPPY_MAX_POS) {
    floppy_dir[nr] = HIGH;
    digitalWrite(floppy_pin_dir[nr], HIGH);
    
  }
  if (floppy_pos[nr] == 0) {
    floppy_dir[nr] = LOW;
    digitalWrite(floppy_pin_dir[nr], LOW);
  }
  if (floppy_dir[nr]) {
    floppy_pos[nr]--;
  } else floppy_pos[nr]++;
  //if (Serial) Serial.println(floppy_pos[nr]);

  // Step motor, cycles high and low.
  floppy_step[nr] = !floppy_step[nr];
  digitalWrite(floppy_pin_pos[nr], floppy_step[nr]);


}

