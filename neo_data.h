/*
 * data to play out on neopixels
 * neopixel strand and connection data is here too
 */
#ifndef __NEO_DATA_H__

#include <c_types.h>

/*
 * struct for individual points in the pattern
 */
typedef struct  {
  const char *label;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;  // not always used
  int32_t ms_after_last;  // wait this many uS after last change to play
} neo_data_t;

#define NEO_NUMPIXELS 10
#define NEO_PIN 15
#define NEO_TYPE NEO_GRB+NEO_KHZ800

void neo_cycle_next(void);
void neo_init(void);

extern neo_data_t red_med[];

#define __NEO_DATA_H__
#endif