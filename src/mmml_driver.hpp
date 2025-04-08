/*H************************************************************
*  FILENAME:      mmml_driver.hpp
*  DESCRIPTION:   Micro Music Macro Language (μMML) Player for
*                 AVR microcontrollers.
*
*  NOTES:         To be compiled with any Mircochip C compiler
*                 via Arduino IDE for compatibility.
*
*                 A four-channel MML inspired 1-bit music
*                 player. Three channels of harmonic pulse
*                 wave and a percussive sampler or noise
*                 generator.
*
*                 Needs to be updated every 100 Microseconds or 0.1ms
*                 or 10kHz.
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

#ifndef MMML_DRIVER_HPP
#define MMML_DRIVER_HPP

#include <stdint.h>

// stuff you shouldn't really mess with
#define CHANNELS       4    // the number of channels
#define SAMPLE_SPEED   5    // the sampler playback rate
#define SAMPLE_LENGTH  127  // the length of the sample array
#define MAXLOOPS       5    // the maximum number of nested loops

typedef struct MMMLDriver {
  unsigned char out[CHANNELS];
  unsigned char octave[CHANNELS];
  unsigned char length[CHANNELS];
  unsigned char volume[CHANNELS];
  unsigned char loops_active[CHANNELS];
  unsigned char current_length[CHANNELS];

  unsigned int data_pointer[CHANNELS];
  unsigned int waveform[CHANNELS];
  unsigned int pitch_counter[CHANNELS];
  unsigned int frequency[CHANNELS];
  unsigned int loop_duration[MAXLOOPS][CHANNELS];
  unsigned int loop_point[MAXLOOPS][CHANNELS];
  unsigned int pointer_location[CHANNELS];

  // sampler variables
	unsigned char current_byte;
  unsigned char current_bit;
  unsigned char sample_counter;
  unsigned char current_sample;

	// temporary data storage variables
	unsigned char buffer1;
  unsigned char buffer2;
  unsigned char buffer3;
	unsigned int buffer4;

	// main timer variables
	unsigned int tick_counter;
  unsigned int tick_speed; //tempo

  unsigned int output_pin;

  const unsigned char* music_data;
} BuzzerDriver;

void mmml_driver_init(MMMLDriver* driver, unsigned char pin);

void mmml_driver_play(MMMLDriver* driver, const unsigned char* music_data);

/*
 * Returns buzzer output (use for debug/show purpuses.)
 */
uint8_t mmml_driver_update(MMMLDriver* driver);

void mmml_driver_stop(MMMLDriver* driver);

#endif
