/*
 * functions to play out the neo_pixel patterns
 */
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "neo_data.h"

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel pixels(NEO_NUMPIXELS, NEO_PIN, NEO_TYPE);

/*
 * housekeeping for the sequence state machine
 */
#define NEO_SEQ_START    0
#define NEO_SEQ_WAIT     1
#define NEO_SEQ_WRITE    2
#define NEO_SEQ_STOPPING 3
#define NEO_SEQ_STOPPED  4

static uint8_t neo_state = NEO_SEQ_START;

/*
 * initialize the neopixel strand and set it to off/idle
 */
void neo_init(void)  {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  neo_state = NEO_SEQ_STOPPED;
}

/*
 * which/set sequence are we playing out
 * returns: -1 if the label doesn't match a sequence
 */
int8_t seq_index = 0;
int8_t neo_set_sequence(const char *label)  {
  int8_t ret = -1;
  for(int i = 0; i < MAX_SEQUENCES; i++)  {
    if(strcmp(label, neo_sequences[i].label) == 0)  {
      seq_index = i;
      ret = 0;
      neo_state = NEO_SEQ_START;
    }
  }
  return(ret);
}

/*
 * check if the specified time since last change has occured
 * and update the strand if so.
 */

uint64_t current_millis = 0;
int32_t current_index = 0;   // index into the pattern array

/*
 * helper for writing a single pixel
 */
void neo_write_pixel(void)  {
  pixels.clear(); // Set all pixel colors to 'off'
  /*
    * send the next point in the sequence to the strand
    */
  for(int i=0; i < NEO_NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color( neo_sequences[seq_index].point[current_index].red, 
                                          neo_sequences[seq_index].point[current_index].green,
                                          neo_sequences[seq_index].point[current_index].blue));

  pixels.show();   // Send the updated pixel colors to the hardware.
  }
}


void neo_cycle_next(void)  {
  uint64_t new_millis = 0;

  switch(neo_state)  {

    case NEO_SEQ_STOPPED:
      break;

    case NEO_SEQ_STOPPING:
      pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
      pixels.clear(); // Set all pixel colors to 'off'
      pixels.show();   // Send the updated pixel colors to the hardware.
      neo_state = NEO_SEQ_STOPPED;
      break;

    case NEO_SEQ_START:
      neo_write_pixel();
      neo_state = NEO_SEQ_WAIT;
      break;
    
    case NEO_SEQ_WAIT:
      /*
       * if the timer has expired (or assumed that if current_millis == 0, then it will be)
       */
      if(((new_millis = millis()) - current_millis) >= neo_sequences[seq_index].point[current_index].ms_after_last)  {
        current_millis = new_millis;
        current_index++;
      }
      neo_state = NEO_SEQ_WRITE;
      break;
    
    case NEO_SEQ_WRITE:
      if(neo_sequences[seq_index].point[current_index].ms_after_last < 0)  // list terminator: nothing to write
          current_index = 0;
      neo_write_pixel();
      neo_state = NEO_SEQ_WAIT;
      break;

    default:
      break;
  }
}

/*
 * stop the sequence i.e. turn off neopixel strand
 */
void neo_cycle_stop(void)  {
  neo_state = NEO_SEQ_STOPPING;
}