/*
 * data to play out on neopixels
 * neopixel strand and connection data is here too
 */
#ifndef __NEO_DATA_H__

#include <c_types.h>
#include <Adafruit_NeoPixel.h>

#define MAX_USER_SEQ  3  // maximum number of user buttons/files
#define MAX_SEQUENCES 6  // number of sequences to allocate
#define MAX_NUM_SEQ_POINTS 256   // maximum number of points per sequence

typedef struct  {
  const char *label;
  const char *file;
  int8_t      loaded;
} neo_user_t;

/*
 * return error codes for reading a user sequence file
 * and maybe other functions
 */
#define   NEO_LOADED              1
#define   NEO_EMPTY               0
#define   NEO_STALE              -1

#define   NEO_SUCCESS             0
#define   NEO_DESERR             -1
#define   NEO_NOPLACE            -2
#define   NEO_SEQ_ERR            -3
#define   NEO_FILE_LOAD_NOTUSER  -4
#define   NEO_FILE_LOAD_NOFILE   -5
#define   NEO_FILE_LOAD_DESERR   -6
#define   NEO_FILE_LOAD_NOPLACE  -7
#define   NEO_FILE_LOAD_OTHER    -9

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
  neo_seq_point_t point[MAX_NUM_SEQ_POINTS];
} neo_data_t;


/*
 * describes the hardware configuration of the neopixel strip
 */
#define NEO_NUMPIXELS 10
#define NEO_PIN 15
#define NEO_TYPE NEO_GRB+NEO_KHZ800

/*
 * public functions relating to neopixels
 */
void neo_cycle_next(void);
void neo_init(uint16_t numPixels, int16_t pin, neoPixelType pixelFormat);
int8_t neo_is_user(const char *label, char **file);
int8_t neo_load_sequence(const char *label);
int8_t neo_set_sequence(const char *label);
void neo_cycle_stop(void);

/*
 * array of neopixel sequences and the index to the currently playing one
 * neo_user_files[] : map of button labels to user file names
 */
extern const neo_user_t neo_user_files[MAX_USER_SEQ];
extern neo_data_t neo_sequences[MAX_SEQUENCES];  // sequence specifications
extern int8_t seq_index;  // which sequence is being played out

#define __NEO_DATA_H__
#endif