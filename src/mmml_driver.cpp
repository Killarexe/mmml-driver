/*H************************************************************
*  FILENAME:      mmml_driver.cpp
*  DESCRIPTION:   Micro Music Macro Language (μMML) Player for
*                 AVR microcontrollers.
*
*  NOTES:         To be compiled with any Microchip C compiler
*                 via Arduino IDE for compatibility.
*
*                 A four-channel MML inspired 1-bit music
*                 player. Three channels of harmonic pulse
*                 wave and a percussive sampler or noise
*                 generator.
*
*                 Needs to be updated every 120 Microseconds or 0.12ms
*                 or 8.3kHz.
*
*                 Eight different duty cycles possible:
*                 50%, 25%, 12.5%, 6.25%, 3.125%, 1.5625%,
*                 0.78125% and 0.390625%. The 'thinner' pulse
*                 widths are perceived as a change in waveform
*                 power (volume) as opposed to a change in
*                 timbre.
*
*                 Tested on:
*                   * Attiny13/45/85 @ 8MHz
*                   * Atmega168/328  @ 8MHz
*                   * Raspberry Pi Pico 2
*                   * ESP32
*
*  AUTHOR:        Blake 'PROTODOME' Troise
*                 modified by Killar
************************************************************H*/

#include "mmml_driver.hpp"
#include <Arduino.h>

// note table (plus an initial 'wasted' entry for rests)
const unsigned int NOTES[13] =
{
	// the rest command is technically note 0 and thus requires a frequency
	255,
	// one octave of notes, equal temperament in Gb
	1024,967,912,861,813,767,724,683,645,609,575,542
};

// location of individual samples in sample array
const unsigned char SAMPLE_INDICIES[6] =
{
	0,19,34,74,118,126
};

// raw PWM sample data
const unsigned char SAMPLES[SAMPLE_LENGTH] =
{
	// bwoop (0)
	0b10101010,0b10110110,0b10000111,0b11111000,
	0b10000100,0b00110111,0b11101000,0b11000001,
	0b00000111,0b00111101,0b11111000,0b11100000,
	0b10010001,0b10000111,0b00000111,0b00001111,
	0b00001111,0b00011011,0b00011110,
	// beep (19)
	0b10101010,0b00101010,0b00110011,0b00110011,
	0b00110011,0b00110011,0b00110011,0b11001101,
	0b11001100,0b11001100,0b11001100,0b10101100,
	0b10011001,0b00110001,0b00110011,
	// kick (34)
	0b10010101,0b10110010,0b00000000,0b11100011,
	0b11110000,0b00000000,0b11111111,0b00000000,
	0b11111110,0b00000000,0b00000000,0b00000000,
	0b11111111,0b11111111,0b11111111,0b00100101,
	0b00000000,0b00000000,0b00000000,0b00000000,
	0b11111111,0b11110111,0b11111111,0b11111111,
	0b11111111,0b10111111,0b00010010,0b00000000,
	0b10000000,0b00000000,0b00000000,0b00000000,
	0b00000000,0b11101110,0b11111111,0b11111111,
	0b11111111,0b11110111,0b11111111,0b11111110,
	// snare (74)
	0b10011010,0b10011010,0b10101010,0b10010110,
	0b01110100,0b10010101,0b10001010,0b11011110,
	0b01110100,0b10100000,0b11110111,0b00100101,
	0b01110100,0b01101000,0b11111111,0b01011011,
	0b01000001,0b10000000,0b11010100,0b11111101,
	0b11011110,0b00010010,0b00000100,0b00100100,
	0b11101101,0b11111011,0b01011011,0b00100101,
	0b00000100,0b10010001,0b01101010,0b11011111,
	0b01110111,0b00010101,0b00000010,0b00100010,
	0b11010101,0b01111010,0b11101111,0b10110110,
	0b00100100,0b10000100,0b10100100,0b11011010,
	// hi-hat (118)
	0b10011010,0b01110100,0b11010100,0b00110011,
	0b00110011,0b11101000,0b11101000,0b01010101,
	0b01010101,
	// end (126)
};


void mmml_driver_init(MMMLDriver* driver, unsigned char pin) {
  driver->current_byte = 0;
  driver->current_bit = 0;
  driver->sample_counter = 0;
  driver->current_sample = 0;
  driver->output_pin = pin;

  driver->buffer1 = 0;
  driver->buffer2 = 0;
  driver->buffer2 = 0;
  driver->buffer4 = 0;

  driver->tick_counter = 0;
  driver->tick_speed = 1024;

  driver->music_data = nullptr;

  for (unsigned char i = 0; i < CHANNELS; i++) {
    driver->data_pointer[i] = 0;
    driver->frequency[i] = 255;
    driver->volume[i] = 1;
    driver->octave[i] = 3;
  }

  // initialise output pin
  pinMode(driver->output_pin, OUTPUT);
}

void mmml_driver_play(MMMLDriver* driver, const unsigned char* music_data) {
  driver->current_byte = 0;
  driver->current_bit = 0;
  driver->sample_counter = 0;
  driver->current_sample = 0;

  driver->buffer1 = 0;
  driver->buffer2 = 0;
  driver->buffer2 = 0;
  driver->buffer4 = 0;

  driver->tick_counter = 0;
  driver->tick_speed = 1024;

  driver->music_data = music_data;

  /* Set each channel's data pointer to that channel's data location in the core data array.
   * Initialise each channel's frequencies. By default they're set to zero which causes out of
   * tune notes (due to timing errors) until every channel is assigned frequency data.
   * Additionally, default values are set should no volume or octave be specified
   */
  for (unsigned char i = 0; i < CHANNELS; i++) {
    driver->data_pointer[i] = music_data[i * 2] << 8;
    driver->data_pointer[i] = driver->data_pointer[i] | music_data[i * 2 + 1];
    driver->frequency[i] = 255; // random frequency (won't ever be sounded)
    driver->volume[i] = 1;      // default volume : 50% pulse wave
    driver->octave[i] = 3;      // default octave : o3
  }
}

uint8_t mmml_driver_update(MMMLDriver* driver) {
  if (!driver->music_data) {
    return 0;
  }

  /* The code below lowers the volume of the sample channel when the volume is changed,
   * but slows the routine by a tone... Not desired right now, but could be interesting in
   * future.
   *
   * if(volume[3] > 2)
   *  PORTB = 0;
   */
  /**********************
   *  Synthesizer Code  *
   **********************/
  // sampler (channel D) code
  if (driver->sample_counter-- == 0) {
    if (driver->current_byte < driver->current_sample - 1) {
      // read individual bits from the sample array
      driver->out[3] = ((SAMPLES[driver->current_byte] >> driver->current_bit++) & 1);
    } else {
      /* Waste the same number of clock cycles as it takes to process the above to
       * prevent the pitch from changing when the sampler isn't playing. */
      for (unsigned char i = 0; i < 8; i++) {
        asm("nop;nop;");
      }
      // silence the channel when the sample is over
      driver->out[3] = 0;
    }
    // move to the next byte on bit pointer overflow
    if(driver->current_bit > 7) {
      driver->current_byte++;
      driver->current_bit = 0;
    }
    driver->sample_counter = SAMPLE_SPEED;
  }

  /* Port changes (the demarcated 'output' commands) are carefully interleaved with
   * generation code to balance volume of outputs. */
  // channel A (pulse 0 code)
  driver->pitch_counter[0] += driver->octave[0];
  if (driver->pitch_counter[0] >= driver->frequency[0]) {
    driver->pitch_counter[0] = driver->pitch_counter[0] - driver->frequency[0];
  }
  if (driver->pitch_counter[0] <= driver->waveform[0]) {
    driver->out[0] = 1;
  }

  if (driver->pitch_counter[0] >= driver->waveform[0]) {
    driver->out[0] = 0;
  }
  // channel B (pulse 1 code)
  driver->pitch_counter[1] += driver->octave[1];
  if (driver->pitch_counter[1] >= driver->frequency[1]) {
    driver->pitch_counter[1] = driver->pitch_counter[1] - driver->frequency[1];
  }
  if (driver->pitch_counter[1] <= driver->waveform[1]) {
    driver->out[1] = 1;
  }
  if (driver->pitch_counter[1] >= driver->waveform[1]) {
    driver->out[1] = 0;
  }

  // channel C (pulse 2 code)
  driver->pitch_counter[2] += driver->octave[2];
  if (driver->pitch_counter[2] >= driver->frequency[2]) {
    driver->pitch_counter[2] = driver->pitch_counter[2] - driver->frequency[2];
  }
  if (driver->pitch_counter[2] <= driver->waveform[2]) {
    driver->out[2] = 1;
  }
  if (driver->pitch_counter[2] >= driver->waveform[2]) {
    driver->out[2] = 0;
  }
  
  /**************************
   *  Data Processing Code  *
   **************************/
  if (driver->tick_counter-- == 0) {
    // Variable tempo, sets the fastest / smallest possible clock event.
    driver->tick_counter = driver->tick_speed;
    for (unsigned char voice = 0; voice < CHANNELS; voice++) {
      // If the note ended, start processing the next byte of data.
      if (driver->length[voice] == 0) {
        LOOP: // Yup, a goto, I know.
        // Temporary storage of data for quick processing.
        // first nibble of data
        driver->buffer1 = (driver->music_data[driver->data_pointer[voice]] >> 4) & 15;
        // second nibble of data
        driver->buffer2 = driver->music_data[driver->data_pointer[voice]] & 15;
        
        if (driver->buffer1 == 15) {
          // Another buffer for commands that require an additional byte.
          driver->buffer3 =driver->music_data[driver->data_pointer[voice] + 1];
          
          switch (driver->buffer2) {
            case 0: // loop start
              driver->loops_active[voice]++;
              driver->loop_point[driver->loops_active[voice] - 1][voice] = driver->data_pointer[voice] + 2;
              driver->loop_duration[driver->loops_active[voice] - 1][voice] = driver->buffer3 - 1;
              driver->data_pointer[voice] += 2;
              break;
              
            case 1: // loop end
              if (driver->loop_duration[driver->loops_active[voice] - 1][voice] > 0) {
                driver->data_pointer[voice] = driver->loop_point[driver->loops_active[voice] - 1][voice];
                driver->loop_duration[driver->loops_active[voice] - 1][voice]--;
              } else {
                driver->loops_active[voice]--;
                driver->data_pointer[voice]++;
              }
              break;
              
            case 2: // macro
              driver->pointer_location[voice] = driver->data_pointer[voice] + 2;
              driver->data_pointer[voice] = driver->music_data[(driver->buffer3 + CHANNELS) * 2] << 8;
              driver->data_pointer[voice] = driver->data_pointer[voice] | driver->music_data[(driver->buffer3 + CHANNELS) * 2 + 1];
              break;
              
            case 3: // tempo
              driver->tick_speed = driver->buffer3 << 4;
              driver->data_pointer[voice] += 2;
              break;
            
            case 6: //Tie command
              driver->data_pointer[voice]++;
              break;
              
            case 4:
            case 5:
              driver->data_pointer[voice] += 2;
              break;

            case 15:
              if (driver->pointer_location[voice] != 0) {
                driver->data_pointer[voice] = driver->pointer_location[voice];
                driver->pointer_location[voice] = 0;
              } else {
                driver->data_pointer[voice] = driver->music_data[voice * 2] << 8;
                driver->data_pointer[voice] = driver->data_pointer[voice] | driver->music_data[voice * 2 + 1];
              }
              break;
          }
          goto LOOP;
        }

        switch (driver->buffer1) {
          case 13: // octave
            driver->octave[voice] = 1 << driver->buffer2;
            driver->data_pointer[voice]++;
            goto LOOP;
            
          case 14: // volume
            driver->volume[voice] = driver->buffer2;
            driver->data_pointer[voice]++;
            goto LOOP;
            
          default:
            if (driver->buffer1 != 0 && driver->buffer1 < 14) {
              if (voice < 3) {
                driver->buffer4 = NOTES[driver->buffer1];
                driver->frequency[voice] = driver->buffer4;
                driver->waveform[voice] = driver->buffer4 >> driver->volume[voice];
              } else {
                driver->current_bit = 0;
                driver->current_byte = SAMPLE_INDICIES[driver->buffer1 - 1];
                driver->current_sample = SAMPLE_INDICIES[driver->buffer1];
              }
            } else {
              driver->waveform[voice] = 0;
            }
        }

        // note duration value
        if (driver->buffer2 < 8) {
          driver->length[voice] = 0x7F >> driver->buffer2;
        } else {
          driver->length[voice] = 95 >> (driver->buffer2 & 7);
        }
        // next element in data
        driver->data_pointer[voice]++;
      } else {
        // keep waiting until the note is over
        driver->length[voice]--;
      }
    }
  }

  uint8_t output = driver->out[0] | driver->out[1] | driver->out[2] | driver->out[3];
  digitalWrite(driver->output_pin, output);
  return output;
}

void mmml_driver_stop(MMMLDriver* driver) {
  mmml_driver_init(driver, driver->output_pin);
}
