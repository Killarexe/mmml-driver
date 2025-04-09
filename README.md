# µMML Driver

## About

µMML Driver or MMML Driver is a simple driver to play 1-bit music into a buzzer via a microcontoller.

## What is µMML

µMML or Micro Music Macro Language is a derivative of [MML](https://en.wikipedia.org/wiki/Music_Macro_Language)*(Music Macro Language)* for 1-bit music implmentation.

The original µMML compiler is made by [protodomemusic](https://github.com/protodomemusic) in C. If you want to see, get [here.](https://github.com/protodomemusic/mmml/tree/master/compiler)

## How to use

MMMLDriver have only 4 functions:

```c

void mmml_driver_init(MMMLDriver* driver, unsigned char output_pin);

void mmml_driver_play(MMMLDriver* driver, unsigned char* music_code);

unsigned char mmml_driver_update(MMMLDriver* driver);

void mmml_driver_stop(MMMLDriver* driver);

```
