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
Adafruit_NeoPixel pixels(NEO_NUMPIXELS, NEO_PIN, NEO_TYPE);

/*
 * housekeeping for the sequence state machine
 */
#define NEO_SEQ_START    0
#define NEO_SEQ_WAIT     1
#define NEO_SEQ_WRITE    2
#define NEO_SEQ_STOPPING 3
#define NEO_SEQ_STOPPED  4

static uint8_t neo_state = NEO_SEQ_START;  // state of the cycling state machine
uint64_t current_millis = 0; // mS of last update
int32_t current_index = 0;   // index into the pattern array

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
int8_t neo_set_sequence(const char *label)  {
  int8_t ret = NEO_SEQ_ERR;
  int8_t new_index = 0;

  new_index = neo_find_sequence(label);
  if((new_index >= 0) && (new_index != seq_index))  {
    seq_index = new_index;  // set the sequence index that is to be played
    ret = NEO_SUCCESS; // success
    current_index = 0;  // reset the pixel count
    neo_state = NEO_SEQ_START;  // cause the state machine to start at the start
  }
  return(ret);
}

/*
 * check if the label matches a predefined USER button
 * and return the filename to be loaded.  Just return the pointer
 * since we're searching an initialized const array of strings.
 */
int8_t neo_is_user(const char *label, char **file)  {
  int8_t ret = NEO_FILE_LOAD_NOTUSER;
  *file = NULL;

  /*
   * determine if label points to a user sequence file
   */
  int8_t i = 0;
  while((i < MAX_USER_SEQ) && (ret != NEO_SUCCESS)) {
    if(strcmp(label, neo_user_files[i].label) == 0)
      ret = NEO_SUCCESS;
    else
      i++;
  }

  /*
   * if the label points to a user sequence file, 
   * copy the filename
   */
  if(ret == NEO_SUCCESS)
    *file = (char *)(neo_user_files[i].file);

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
        const char *label;
        label = jsonDoc["label"];
        TRACE("For sequence \"%s\" : \n", label);
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
          neo_set_sequence(label);
        }
      }
    }
  }
  return(ret);
}

/*
 * helper for writing a single pixel
 */
void neo_write_pixel(bool clear)  {
  if(clear != 0)  pixels.clear(); // Set all pixel colors to 'off'

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

/*
 * check if the specified time since last change has occured
 * and update the strand if so.
 */
void neo_cycle_next(void)  {
  uint64_t new_millis = 0;

  switch(neo_state)  {

    case NEO_SEQ_STOPPED:
      break;

    case NEO_SEQ_STOPPING:
      //pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
      pixels.clear(); // Set all pixel colors to 'off'
      pixels.show();   // Send the updated pixel colors to the hardware.
      current_index = 0;
      neo_state = NEO_SEQ_STOPPED;
      break;

    case NEO_SEQ_START:
      neo_write_pixel(true);
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
      neo_write_pixel(false);
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
  seq_index = -1;  // so it doesn't match
}