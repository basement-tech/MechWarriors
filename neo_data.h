/*
 * data to play out on neopixels
 * neopixel strand and connection data is here too
 */
#ifndef __NEO_DATA_H__

#include <c_types.h>

#define MAX_SEQUENCES 4  // number of sequences to allocate
#define MAX_NUM_SEQ_POINTS 128   // maximum number of points per sequence

/*
 * struct for individual points in the pattern
 */
typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;  // not always used
  int32_t ms_after_last;  // wait this many uS after last change to play
} neo_seq_point_t;

typedef struct  {
  const char *label;
  const neo_seq_point_t point[MAX_NUM_SEQ_POINTS];
} neo_data_t;

#define NEO_NUMPIXELS 10
#define NEO_PIN 15
#define NEO_TYPE NEO_GRB+NEO_KHZ800

void neo_cycle_next(void);
void neo_init(void);
int8_t neo_load_sequence(const char *label);
int8_t neo_set_sequence(const char *label);
void neo_cycle_stop(void);

extern neo_data_t neo_sequences[MAX_SEQUENCES];  // sequence specifications
extern int8_t seq_index;  // which sequence is being played out

#define __NEO_DATA_H__
#endif