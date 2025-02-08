/*
 * functions to play out the neo_pixel patterns
 */
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.

#include <ArduinoJson.h>

#include "neo_data.h"

// TRACE output simplified, can be deactivated here
#define TRACE(...) Serial.printf(__VA_ARGS__)

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
Adafruit_NeoPixel *pixels;


/*
 * housekeeping for the sequence state machine
 */
#define NEO_SEQ_START    0
#define NEO_SEQ_WAIT     1
#define NEO_SEQ_WRITE    2
#define NEO_SEQ_STOPPING 3
#define NEO_SEQ_STOPPED  4
static uint8_t neo_state = NEO_SEQ_START;  // state of the cycling state machine

seq_strategy_t current_strategy = SEQ_STRAT_POINTS;

uint64_t current_millis = 0; // mS of last update
int32_t current_index = 0;   // index into the pattern array

/*
 * return the index in neo_sequences[] that matches
 * the label given as an argument.  Do *not* set the global
 * index value that is used to play the sequence.
 */
int8_t neo_find_sequence(const char *label)  {
  int8_t ret = -1;
  for(int i = 0; i < MAX_SEQUENCES; i++)  {
    if(strcmp(label, neo_sequences[i].label) == 0)
      ret = i;
  }
  return(ret);
}


/*
 * which/set sequence are we playing out
 * returns: -1 if the label doesn't match a sequence
 * reset the playout index and state if the found index
 * is different than the currently running index.
 */
int8_t seq_index = -1;  // global used to hold the index of the currently running sequence
int8_t neo_set_sequence(const char *label, const char *strategy)  {
  int8_t ret = NEO_SEQ_ERR;
  int8_t new_index = 0;
  seq_strategy_t new_strat;


  /*
   * attempt to set the sequence
   */
  new_index = neo_find_sequence(label);
  if((new_index >= 0) && (new_index != seq_index))  {
    seq_index = new_index;  // set the sequence index that is to be played
    ret = NEO_SUCCESS; // success
  }

  /*
   * if sequence setting was successful, attempt to set the strategy
   *
   * allow for the strategy argument to be a null string so,
   * for example, in the case of a built-in it might remain
   * the initialized value
   */
  if(strategy[0] == '\0')  {
    TRACE("neo_set_sequence: using built in strategy %s for seq_index %d\n", neo_sequences[seq_index].strategy,seq_index);
    if(ret == NEO_SUCCESS)  {
      if((new_strat = neo_set_strategy(neo_sequences[seq_index].strategy)) == SEQ_STRAT_UNDEFINED)
        ret = NEO_STRAT_ERR;
    }
  }
  else {
    /*
    * if sequence setting was successful, attempt to set the strategy
    */
    if(ret == NEO_SUCCESS)  {
      if((new_strat = neo_set_strategy(strategy)) == SEQ_STRAT_UNDEFINED)
        ret = NEO_STRAT_ERR;
    }
  }

  /*
  * if all above was successful, set up the globals and start the sequence
  */
  if(ret == NEO_SUCCESS)  {
    current_index = 0;  // reset the pixel count
    neo_state = NEO_SEQ_START;  // cause the state machine to start at the start
    current_strategy = new_strat;
    TRACE("neo_set_sequence: set sequence to %d and strategy to %d\n", seq_index, current_strategy);
  }

  return(ret);
}

/*
 * check if the label matches a predefined USER button
 * NOTE: this was simplified when the filename attribute was
 * added to the html file
 */
int8_t neo_is_user(const char *label)  {
  int8_t ret = NEO_FILE_LOAD_NOTUSER;

  if(strncmp(label, "USER", 4) == 0)
    ret = NEO_SUCCESS;

  return(ret);
}

/*
 * look for a label matching the argument, label,
 * and load a sequence from file of the same name.
 * NOTE: currently the requested sequence placeholder of the name
 * requested must exist in neo_sequences[] for this to succeed.
 *
 * return:   0: successfully loaded
 *          -1: file not found or error opening
 *          -2: error deserializing file
 */
int8_t neo_load_sequence(const char *file)  {

  FSInfo fs_info;
  LittleFS.info(fs_info);

  JsonDocument jsonDoc;
  DeserializationError err;

  int8_t ret = 0;
  File fd;  // file pointer to read from
  char buf[1024];  // buffer in which to read the file contents  TODO: paramaterize
  char *pbuf;  // helper
 
  pbuf = buf;

  /*
   * can I see the FS from here ? ... yep.
   */
  TRACE("Total bytes in FS = %d\n", fs_info.totalBytes);
  TRACE("Total bytes used in FS = %d\n", fs_info.usedBytes);

  /*
   * read the contents of the user sequence file and put it
   * in the character buffer buf
   */
  if (LittleFS.exists(file) == false)  {
      TRACE("Filename %s does not exist in file system\n", file);
      ret = NEO_FILE_LOAD_NOFILE;
  }
  else  {

    TRACE("Loading filename %s ...\n", file);
    if((fd = LittleFS.open(file, "r")) == false)  
      ret = NEO_FILE_LOAD_NOFILE;

    else  {
      while(fd.available())  {
        *pbuf++ = fd.read();
      }
      *pbuf = '\0';  // terminate the char string
      fd.close();
      TRACE("Raw file contents:\n%s\n", buf);

      /*
      * deserialize the json contents of the file which
      * is now in buf  -> JsonDocument jsonDoc
      */
      err = deserializeJson(jsonDoc, buf);
      if(err)  {
        TRACE("Deserialization of file %s failed ... no change in sequence\n", file);
        ret = NEO_FILE_LOAD_DESERR;
      }

      /*
      * jsonDoc contains an array of points as JsonObjects
      * convert to a JsonArray points[]
      */
      else  {
        JsonArray points = jsonDoc["points"].as<JsonArray>();
        const char *label, *bonus;
        label = jsonDoc["label"];
        bonus = jsonDoc["bonus"];
        TRACE("For sequence \"%s\" : \n", label);
        TRACE("   \"bonus\": %s\n", bonus);
        int8_t seq_idx = neo_find_sequence(label);

        /*
        * iterate over the points in the array
        * this syntax was introduced in C++11 and is equivalent to:
        * for (size_t i = 0; i < points.size(); i++) {
        *   JsonObject obj = points[i];
        */
        if(seq_idx < 0)  {
          ret = NEO_FILE_LOAD_NOPLACE;
          TRACE("neo_load_sequence: no placeholder for %s in sequence array\n", label);
        }

        /*
        * if the label was found, load the points from the json file
        * into the neo_sequences[] array to be played out
        *
        * TODO: super-verbose for now for debugging
        */
        else  {
          strncpy(neo_sequences[seq_idx].bonus, bonus, strlen(bonus));

          uint16_t i = 0;
          for(JsonObject obj : points)  {
            uint8_t r, g, b, w;
            int32_t t;
            r = obj["r"];
            g = obj["g"];
            b = obj["b"];
            w = obj["w"];
            t = obj["t"];
            TRACE("colors = %d %d %d %d  interval = %d\n", r, g, b, w, t);
            neo_sequences[seq_idx].point[i].red = r;
            neo_sequences[seq_idx].point[i].green = g;
            neo_sequences[seq_idx].point[i].blue = b;
            neo_sequences[seq_idx].point[i].white = w;
            neo_sequences[seq_idx].point[i].ms_after_last = t;
            i++;
          }
          ret = neo_set_sequence(label, jsonDoc["strategy"]);
        }
      }
    }
  }
  return(ret);
}

/*
 * helper for writing a single color to all pixels
 */
void neo_write_pixel(bool clear)  {
  if(clear != 0)  pixels->clear(); // Set all pixel colors to 'off'

  /*
    * send the next point in the sequence to the strand
    */
  for(int i=0; i < pixels->numPixels(); i++) { // For each pixel...
    pixels->setPixelColor(i, pixels->Color( neo_sequences[seq_index].point[current_index].red, 
                                          neo_sequences[seq_index].point[current_index].green,
                                          neo_sequences[seq_index].point[current_index].blue));

  pixels->show();   // Send the updated pixel colors to the hardware.
  }
}

/*
 * blink a status color to the strip reps times
 * the color is from the Adafruit colorwheel representation of colors
 * t is the numbe of mS between changes on/off (i.e. blink rate/2)
 *
 * NOTE: this is a blocking function i.e. not suitable for use in the loop()
 * NOTE: the neopixel strand must have been initialized prior to calling this function
 *
 */
void neo_n_blinks(uint8_t r, uint8_t g, uint8_t b, int8_t reps, int32_t t)  {
  uint32_t color = pixels->Color(r, g, b);

  for(int8_t j = reps; j > 0; j--)  {
    /*
    * send the next point in the sequence to the strand
    */
    for(int i=0; i < pixels->numPixels(); i++) { // For each pixel...
      pixels->setPixelColor(i, color);
      pixels->show();   // Send the updated pixel colors to the hardware.
    }
    delay(t);
    pixels->clear();
    pixels->show();
    delay(t);
  }
}

/*
 * initialize the neopixel strand and set it to off/idle
 */
void neo_init(uint16_t numPixels, int16_t pin, neoPixelType pixelFormat)  {
  pixels = new Adafruit_NeoPixel(numPixels, pin, pixelFormat);

  pixels->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels->clear(); // Set all pixel colors to 'off'
  pixels->show();   // Send the updated pixel colors to the hardware.
  neo_state = NEO_SEQ_STOPPED;
}

/*
 * TODO: figure out a better way to do this
 */
void noop(void) {}
void start_noop(bool clear) {}


/*
 * SEQ_STRAT_POINTS
 * each line in the json is a single point in the sequence.
 * the "t" times are mS between points
 * the sequence restarts at the end and runs continuously
 *
 * NOTE: keep neo_points_start() in sync with neo_single_start()
 * when making changes.
 */
void neo_points_start(bool clear) {
  neo_write_pixel(true);  // clear the strand and write the first value
  current_millis = millis();
  neo_state = NEO_SEQ_WAIT;
}

void neo_points_write(void) {
  if(neo_sequences[seq_index].point[current_index].ms_after_last < 0)  // list terminator: nothing to write
    current_index = 0;
  neo_write_pixel(false);
  neo_state = NEO_SEQ_WAIT;
}

void neo_points_wait(void)  {
  uint64_t new_millis = 0;

  /*
    * if the timer has expired (or assumed that if current_millis == 0, then it will be)
    * i.e. done waiting move to the next state
    */
  if(((new_millis = millis()) - current_millis) >= neo_sequences[seq_index].point[current_index].ms_after_last)  {
    current_millis = new_millis;
    current_index++;
    neo_state = NEO_SEQ_WRITE;
  }

}

void neo_points_stopping(void)  {
  pixels->clear(); // Set all pixel colors to 'off'
  pixels->show();   // Send the updated pixel colors to the hardware.
  current_index = 0;
  seq_index = -1; // so it doesn't match

  neo_state = NEO_SEQ_STOPPED;
}

// end of SEQ_STRAT_POINTS callbacks

/*
 * SEQ_STRAT_SINGLE
 * each line in the json is a single point in the sequence.
 * the "t" times are mS between points
 * the sequence runs once per button press and stops
 *
 * NOTE: keep neo_points_start() in sync with neo_single_start()
 * when making changes.
 */
static int8_t single_repeats = 1;

void neo_single_start(bool clear) {
  neo_write_pixel(true);  // clear the strand and write the first value

  /*
   * if the number of repeats given in the bonus value
   * seems to be valid use it to set the number of 
   * times the sequence is repeated, otherwise just indicate
   * that the sequence should be played once.
   */
  if(strlen(neo_sequences[seq_index].bonus) > 0)
    single_repeats = atoi(neo_sequences[seq_index].bonus);
  else
    single_repeats = 1;

  /*
   * get the timing started
   */
  current_millis = millis();
  neo_state = NEO_SEQ_WAIT;
}

void neo_single_write(void) {
  if(neo_sequences[seq_index].point[current_index].ms_after_last < 0)  {  // list terminator
    current_index = 0;  // rewind in case we're going to play it again
    if(--single_repeats > 0)  {  // are we going to play it again?
      neo_state = NEO_SEQ_WAIT;  // yep
      neo_write_pixel(false);
    }
    else
      neo_state = NEO_SEQ_STOPPING;  // nope
  }
  else  {  // just write the point and continue
    neo_write_pixel(false);
    neo_state = NEO_SEQ_WAIT;
  }
}

// end of SEQ_STRAT_SINGLE callbacks

/*
 * SEQ_STRAT_SLOWP
 * this is a slowly moving pulse sequence
 * only a two points are expected in the json, from which
 * the endpoint/maximum (color and intensity)  and the starting intensity
 * of the pulse is taken
 *
 * "t" from the first line is interpreted a the total number of seconds for the wave
 * this is a calculated sequence (based on NEO_SLOWP_POINTS):
 * - the interval between changes is based on the "t" seconds parameter
 * - the delta change is calculated
 *
 * "bonus"  from the json sequence file is interpretted as the number
 * of random flashes that should occur during the sequence.  The sign of
 * the "bonus" value indicates whether the flash should be 255 (bright),
 * or 0 (dark).
 */
static int32_t slowp_idx = 0;  // counting through the NEO_SLOWP_POINTS
static int8_t slowp_dir = 1;  // +1 -1 to indicate the direction we're traveling
static uint32_t delta_time;  // calculated time between changes
static float delta_r, delta_g, delta_b;  // calculated increment for each color ... must be floats or gets rounded to 0 between calls
static float slowp_r, slowp_g, slowp_b;  // remember where we are in the sequence
#define SLOWP_FLICKERS 10
static int16_t slowp_flickers[SLOWP_FLICKERS];  // random points to flicker
static uint8_t slowp_flicker_idx = 0;
static int8_t flicker_dir = 0;  // flicker bright or dark
static int8_t flicker_count = 0;  // how many flickers

void neo_slowp_start(bool clear)  {

  slowp_idx = 0;
  slowp_dir = 1;  // start by going up
  slowp_flicker_idx = 0;  // start at the start
  flicker_count = 0;  // assume none to Start



  /*
   * calculate delta time in mS based on the first (and only)
   * line in the json sequence file
   */
  delta_time = (neo_sequences[seq_index].point[0].ms_after_last * 1000) / NEO_SLOWP_POINTS;

  /*
   * calculate the delta chance for each color
   * the first line in the json sequence has the max/endpoint
   * of the sequence
   *
   */
  delta_r = (neo_sequences[seq_index].point[1].red - neo_sequences[seq_index].point[0].red) / (float)NEO_SLOWP_POINTS;  // cast needed to force floating point math
  delta_g = (neo_sequences[seq_index].point[1].green - neo_sequences[seq_index].point[0].green) / (float)NEO_SLOWP_POINTS;
  delta_b = (neo_sequences[seq_index].point[1].blue - neo_sequences[seq_index].point[0].blue) / (float)NEO_SLOWP_POINTS;

  /*
   * test for bad input and cause no intensity if bad
   */
  if(delta_r < 0)  delta_r = 0;
  if(delta_g < 0)  delta_g = 0;
  if(delta_b < 0)  delta_b = 0;

  /*
   * start from the json specified starting point
   */
  slowp_r = neo_sequences[seq_index].point[0].red;
  slowp_g = neo_sequences[seq_index].point[0].green;
  slowp_b = neo_sequences[seq_index].point[0].blue;

  /*
   * obtain the random places where the lights will flicker
   * based on the "bonus" parameter from the json sequence file
   */
  if(strlen(neo_sequences[seq_index].bonus) > 0)  {
    flicker_count = atoi(neo_sequences[seq_index].bonus);
    if(abs(flicker_count) > SLOWP_FLICKERS) flicker_count = SLOWP_FLICKERS;  //boundary check

    /*
     * set the direction that the flicker brightness will take
     */
    if     (flicker_count > 0)  flicker_dir = 1;
    else if(flicker_count < 0)  flicker_dir = -1;
    else                        flicker_dir = 0;

    /*
     * we'll use flicker_count and flicker_dir separately from here
     */
    flicker_count = abs(flicker_count);
  }
  randomSeed(analogRead(0));  // different each time through
  for(uint8_t j = 0; j < flicker_count; j++)
    slowp_flickers[j] = random(0, NEO_SLOWP_POINTS);

  TRACE("Starting slowp: dr = %f, dg = %f, db = %f dt = %d\n", delta_r, delta_g, delta_b, delta_time);
  TRACE("Randoms are:");
  for(uint8_t j = 0; j < flicker_count; j++)
    TRACE("%d  ", slowp_flickers[j]);
  TRACE("\n");

  pixels->clear();
  pixels->show();

  current_millis = millis();

  neo_state = NEO_SEQ_WAIT;

}


void neo_slowp_write(void) {
  uint8_t r, g, b;

  /*
   * currently going up
   */
  if(slowp_dir > 0)  {
    if(slowp_idx < NEO_SLOWP_POINTS)  {  // have not reached the top of the sequence
      if((slowp_r += delta_r) > 255) slowp_r = 255;  // could be by rounding error
      if((slowp_g += delta_g) > 255) slowp_g = 255;
      if((slowp_b += delta_b) > 255) slowp_b = 255;
      slowp_idx++;
    }
    else  {
      slowp_dir = -1;  // change to going down
      slowp_idx--;

      /*
       * reset to the ending point in case of rounding error
       */
      slowp_r = neo_sequences[seq_index].point[1].red;
      slowp_g = neo_sequences[seq_index].point[1].green;
      slowp_b = neo_sequences[seq_index].point[1].blue;
    }
  }

  /*
   * currently going down
   */
  else  {
    if(slowp_idx > 0)  {  // list terminator: nothing to write
      if((slowp_r -= delta_r) < 0) slowp_r = 0;
      if((slowp_g -= delta_g) < 0) slowp_g = 0;
      if((slowp_b -= delta_b) < 0) slowp_b = 0;
      slowp_idx--;
    }
    else  {
      slowp_dir = 1;  // change to going down
      slowp_idx++;

      /*
       * reset to the starting point  in case of rounding error
       */
      slowp_r = neo_sequences[seq_index].point[0].red;
      slowp_g = neo_sequences[seq_index].point[0].green;
      slowp_b = neo_sequences[seq_index].point[0].blue;
    }
  }

  /*
   * send the next point in the sequence to the strand
   */
  if(flicker_count == 0)  {
          r = slowp_r; g = slowp_g, b = slowp_b;
  }
  else  {
    if(slowp_idx == slowp_flickers[slowp_flicker_idx])  {
      if(flicker_dir > 0)
        r = g = b = 255;
      else
        r = g = b = 0;
      if(++slowp_flicker_idx > flicker_count)
        slowp_flicker_idx = 0;
    }
    else
      r = slowp_r; g = slowp_g, b = slowp_b;
  }
  for(int i=0; i < pixels->numPixels(); i++)  // For each pixel...
      pixels->setPixelColor(i, pixels->Color(r, g, b));

  pixels->show();   // Send the updated pixel colors to the hardware.

#ifdef DEBUG_HACK
  TRACE("neo_slowp_write: Showed %d  %d  %d\n", slowp_r, slowp_g, slowp_b);
  while(Serial.available() == 0);
  Serial.read();
#endif

  neo_state = NEO_SEQ_WAIT;
}


void neo_slowp_wait(void)  {
  uint64_t new_millis = 0;

  /*
    * if the timer has expired (or assumed that if current_millis == 0, then it will be)
    * i.e. done waiting move to the next state
    */
  if(((new_millis = millis()) - current_millis) >= delta_time)  {
    current_millis = new_millis;
    neo_state = NEO_SEQ_WRITE;
  }

}

// end of SEQ_STRAT_SLOWP callbacks

/*
 * SEQ_STRAT_RAINBOW
 * cycle a rainbow color pallette along the whole strip
 * (adapted from the Adafruit strandtest example)
 */
long firstPixelHue = 0;
void neo_rainbow_start(bool clear)  {
  pixels->clear();
  pixels->show();

  firstPixelHue = 0;

  current_millis = millis();

  neo_state = NEO_SEQ_WRITE;

}

/*
 * wait a fixed 10mS
 */
void neo_rainbow_wait(void)  {
  uint64_t new_millis = 0;

  /*
    * if the timer has expired (or assumed that if current_millis == 0, then it will be)
    * i.e. done waiting move to the next state
    */
  if(((new_millis = millis()) - current_millis) >= 10)  {
    current_millis = new_millis;
    neo_state = NEO_SEQ_WRITE;
  }
}

/*
 * advance and write a pixel
 */
void neo_rainbow_write(void) {
  pixels->rainbow(firstPixelHue);
  pixels->show();

  firstPixelHue += 256;

  if(firstPixelHue >= 5*65536)
    firstPixelHue = 0;

  neo_state = NEO_SEQ_WAIT;

}

void neo_rainbow_stopping(void)  {
  pixels->clear(); // Set all pixel colors to 'off'
  pixels->show();   // Send the updated pixel colors to the hardware.

  seq_index = -1; // so it doesn't match

  neo_state = NEO_SEQ_STOPPED;
}

// end of SEQ_STRAT_RAINBOW callbacks

/*
 * function calls by strategy for each state in the playback machine
 * TODO: delete the 'x' before the labels after implementing a strategy
 */
seq_callbacks_t seq_callbacks[NEO_SEQ_STRATEGIES] = {
//  strategy              label                start                wait              write                stopping             stopped
  { SEQ_STRAT_POINTS,    "points",         neo_points_start,  neo_points_wait,   neo_points_write,    neo_points_stopping,      noop},
  { SEQ_STRAT_SINGLE,    "single",         neo_single_start,  neo_points_wait,   neo_single_write,    neo_points_stopping,      noop},
  { SEQ_STRAT_CHASE,     "xchase",          start_noop,           noop,               noop,                 noop,               noop},
  { SEQ_STRAT_PONG,      "xpong",           start_noop,           noop,               noop,                 noop,               noop},
  { SEQ_STRAT_RAINBOW,   "rainbow",       neo_rainbow_start, neo_rainbow_wait,  neo_rainbow_write,    neo_rainbow_stopping,     noop},
  { SEQ_STRAT_SLOWP,     "slowp",          neo_slowp_start,   neo_slowp_wait,    neo_slowp_write,     neo_points_stopping,      noop},
};

/*
 * expose a method to set the strategy from the "main"
 * look through the labels for a match with the argument
 * and set the global 
 */
seq_strategy_t neo_set_strategy(const char *sstrategy)  {
  seq_strategy_t ret = SEQ_STRAT_UNDEFINED;

  for(int8_t i=0; i < NEO_SEQ_STRATEGIES; i++)  {
    if(strcmp(sstrategy, seq_callbacks[i].label) == 0)  {
      ret = seq_callbacks[i].strategy;
    }
  }
  return(ret);
}

/*
 * check if the specified time since last change has occured
 * and update the strand if so.
 */
void neo_cycle_next(void)  {
  uint64_t new_millis = 0;

  switch(neo_state)  {

    case NEO_SEQ_STOPPED:
      seq_callbacks[current_strategy].stopped();
      break;

    case NEO_SEQ_STOPPING:
      seq_callbacks[current_strategy].stopping();
      break;

    case NEO_SEQ_START:
      seq_callbacks[current_strategy].start(true);  // clear the strand and write the first value
      break;
    
    case NEO_SEQ_WAIT:
      seq_callbacks[current_strategy].wait();
      break;
    
    case NEO_SEQ_WRITE:
      seq_callbacks[current_strategy].write();
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
  seq_index = -1;  // so it doesn't match
}