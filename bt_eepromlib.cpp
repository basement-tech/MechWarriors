/*
 * EEPROM
 * ------
 * this section deals with writing the persistent parameters
 * to the EEPROM and reading them on subsequent reboots
 * E.g. WIFI credentials
 *
 * mon_config[] holds the working copies of the eeprom contents.
 * values are maintained in both mon_config[] and the eeprom as 
 * character strings.  They are expected to be converted on use
 * by the code using them based upon local context.
 *
 * struct eeprom_in eeprom_inputs[] provides the list of values that
 * the user is prompted for, using get_all_eeprom_inputs().  It also provides
 * the strings necessary to create individual prompt messages.
 *
 * a version validation string is kept in EEPROM_VALID in sketch main file.
 * management of that string is currently done by main code.  Perhaps it should be moved.
 *
 * eeprom_begin() is expected to be called in setup().
 * eeprom_get() copies the contents of the eeprom to mon_config.
 * eeprom_put() writes the contents of the eeprom with mon_config.
 *
 * Copied this from a previous project and modified it to work with this one - DJZ
 *
 */
 
#include <EEPROM.h>
#include "bt_eepromlib.h"

/*
 * place to hold the settings for network, mqtt, calibration, etc.
 */
struct net_config mon_config;

/*
 * this section deals with getting the user input to
 * potentially change the eeprom parameter values
 * (e.g. change the WIFI credentials)
 * NOTE: cannot be merged with the net_config structure because
 * the structure determines the byte-for-byte contents of the eeprom data.
 */
struct eeprom_in  {
  char prompt[64];  /* user visible prompt */
  char label[32];   /* label when echoing contents of eeprom */
  char *value;      /* pointer to the data in net_config (mon_config) */
  int  buflen;      /* length of size in EEPROM */
};

#define EEPROM_ITEMS 6
struct eeprom_in eeprom_input[EEPROM_ITEMS] {
  {"",                                       "Validation",    mon_config.valid,            sizeof(mon_config.valid)},
  {"Enter WIFI SSID",                        "WIFI SSID",     mon_config.wlan_ssid,        sizeof(mon_config.wlan_ssid)},
  {"Enter WIFI Password",                    "WIFI Password", mon_config.wlan_pass,        sizeof(mon_config.wlan_pass)},
  {"Enter Fixed IP Addr",                    "Fixed IP Addr", mon_config.ipaddr,           sizeof(mon_config.ipaddr)},
  {"Enter GMT offset (+/- secs)",            "GMT offset",    mon_config.tz_offset_gmt,    sizeof(mon_config.tz_offset_gmt)},
  {"Enter debug level (0 -> 9)",             "debug level",   mon_config.debug_level,      sizeof(mon_config.debug_level)},
};

void init_eeprom_input()  {
    eeprom_input[0].value = mon_config.valid;
    eeprom_input[1].value = mon_config.wlan_ssid;
    eeprom_input[2].value = mon_config.wlan_pass;
    eeprom_input[3].value = mon_config.ipaddr;
    eeprom_input[3].value = mon_config.tz_offset_gmt;
    eeprom_input[4].value = mon_config.debug_level;
}

/*
 * break the data compartmentalization a little and allow the calling
 * data space to access mon_config directly.  I'm hoping this will be read-only.
 */
net_config *get_mon_config_ptr(void) {
	return(&mon_config);
}

/*
 * prompt for and set one input in eeprom_input[].value.
 * return: that which comes back from l_read_string()
 */
int getone_eeprom_input(int i)  {
  char inbuf[64];
  int  insize;

  /*
   * if there is no prompt associated with the subject
   * parameter, skip it
   */
  if(eeprom_input[i].prompt[0] != '\0')  {
    Serial.print(eeprom_input[i].prompt);
    Serial.print("[");Serial.print(eeprom_input[i].value);Serial.print("]");
    Serial.print("(max ");Serial.print(eeprom_input[i].buflen - 1);Serial.print(" chars):");
    if((insize = l_read_string(inbuf, sizeof(inbuf), true)) > 0)  {
      if(insize < (eeprom_input[i].buflen))
        strcpy(eeprom_input[i].value, inbuf);
      else  {
        Serial.println(); 
        Serial.println("Error: too many characters; value will be unchanged");
      }
    }
    Serial.println();
  }
  return(insize);
}

void getall_eeprom_inputs()  {
  int i;
  int ret;
  
  Serial.println();    
  Serial.println("Press <enter> alone to accept previous EEPROM value shown");
  Serial.println("Press <esc> as the first character to skip to the end");
  Serial.println();

  /*
   * loop through getting all of the EEPROM parameter user inputs.
   * if <esc> (indicated by -2) is pressed, skip the balance.
   */
  i = 0;
  ret = 0;
  while((i < EEPROM_ITEMS) && (ret != -2))  {
    ret = getone_eeprom_input(i);
    i++;
  }
}

void dispall_eeprom_parms()  {
  
  Serial.println();    
  Serial.print("Local copy of EEPROM contents(");
  Serial.print(sizeof(mon_config));Serial.print(" of ");
  Serial.print(EEPROM_RESERVE); Serial.println(" bytes used):");

  /*
   * loop through getting all of the EEPROM parameter user inputs.
   * if <esc> (indicated by -2) is pressed, skip the balance.
   */
  for(int i = 0; i < EEPROM_ITEMS; i++)  {
    Serial.print(eeprom_input[i].label);
    Serial.print(" ->"); Serial.print(eeprom_input[i].value); Serial.println("<-");
  }
}

/*
 * read exactly the number of bytes from eeprom 
 * that match[] is long and compare to see if the eeprom has
 * ever been written with a valid set of data from this
 * exact revision.
 * 
 * returns the value of strcmp()
 */
bool eeprom_validation(char match[])  {
  int mlen;
  char ebuf[32];
  char in;
  int i;

  mlen = strlen(match);

  for(i = 0; i < mlen; i++)
    ebuf[i] = EEPROM.get(i, in);
  ebuf[i] = '\0';
  
#ifdef FL_DEBUG_MSG
  Serial.print("match ->");Serial.print(match); Serial.println("<-");
  Serial.print("ebuf ->");Serial.print(ebuf); Serial.println("<-");
#endif
  
  return(strcmp(match, ebuf));
}

/*
 * OK, I just got tired of trying to figure out the available libraries.
 * This function reads characters from the Serial port until an end-of-line
 * of some sort is encountered.
 * 
 * I used minicom under ubuntu to interact with this function successfully.
 * 
 * buf : is a buffer to which to store the read data
 * blen : is meant to indicate the size of buf
 * echo : whether to echo the characters or not 
 * 
 * Return: (n)  the number of characters read, not counting the end of line,
 *              which is over-written with a string terminator ('\0')
 *         (-1) if the buffer was overflowed
 *         (-2) if the <esc> was entered as the first character
 * 
 */
int l_read_string(char *buf, int blen, bool echo)  {
  int count = 0;
  bool out = false;
  int ret = -1;

  while((out == false) && (count < blen))  {
    if(Serial.available() > 0)  {
      *buf = Serial.read();
#ifdef FL_DEBUG_MSG
      Serial.print("char=");Serial.print(*buf);Serial.println(*buf, HEX);
#endif
      /*
       * echo if commanded to do so by the state of the echo argument.
       * don't echo the <esc>.
       */
      if((echo == true) && (*buf != '\x1B'))
        Serial.print(*buf);
      switch(*buf)  {
        /*
         * terminate the string and get out
         */
        case '\n':
          *buf = '\0';
          out = true;
        break;

        case '\r':
          *buf = '\0';
          out = true;
        break;

        /*
         * <escape> was entered
         * ignored if not the first character
         */
        case '\x1B':
          if(count == 0)  {
            out = true;
            count = -2;
          }
        break;

        /* 
         * backspace: don't increment the buffer pointer which
         * allows this character to be written over by the next
         */
        case '\b':
          if(count > 0)
            buf--;
            count--;
            Serial.print(" \b");  /* blank out the character */
          break;

        /*          
         * normal character
         */
        default:
          buf++;
          count++;
        break;
      }
    }
  }

  /*
   * compiler wouldn't let me have the return() inside the if()'s
   */
  if(out == true) /* legitimate exit */
    ret = count;
  else if(count == blen)  /* buffer size exceeded */
    ret = -1;
  return(ret);
}


/*
 * EEPROM init, read, write
 */
void eeprom_begin(void) {
	EEPROM.begin(EEPROM_RESERVE);
}

void eeprom_get(void) {
	EEPROM.get(0, mon_config);
}

void eeprom_put(void) {
	EEPROM.put(0, mon_config);
	EEPROM.commit();
}

void eeprom_user_input(bool out)  {

  char inbuf[64];

  /*
   * if the user entered a character and caused the above
   * while() to exit before the timeout, prompt the user to 
   * enter new network and mqtt configuration data
   * 
   * present previous, valid data from EEPROM as defaults
   */
  if(out == true)  {
    if(eeprom_validation((char *)EEPROM_VALID) == 0)  {
      eeprom_get();  /* if the EEPROM is valid, get the whole contents */
      Serial.println();
      dispall_eeprom_parms();
    }

    /*
     * run the prompt/input sequenct to get the eeprom changes
     */
    getall_eeprom_inputs();

    Serial.println();
    dispall_eeprom_parms();
    Serial.print("Press any key to accept, or reset to correct");
    while(Serial.available() <= 0);
    Serial.read();
    Serial.println();
    
    /*
     * if agreed, write the new data to the EEPROM and use it
     */
    if(eeprom_validation((char *)EEPROM_VALID) == 0)
      Serial.print("EEPROM: previous data exists ... ");
    else
      Serial.print("EEPROM data never initialized ... ");
      
    Serial.print("overwrite with new values? ('y' or 'n'):");
    out = false;
    do {
      l_read_string(inbuf, sizeof(inbuf), true);
      if(strcmp(inbuf, "y") == 0)
        out = true;
      else if (strcmp(inbuf, "n") == 0)
        out = true;
      else  {
        Serial.println();
        Serial.print("EEPROM data valid ... overwrite with new values? ('y' or 'n'):");
      }
    } while(out == false);
    Serial.println();

    /*
     * write the data to EEPROM if an affirmative answer was given
     */
    if(strcmp(inbuf, "y") == 0)  {
      Serial.println("Writing data to EEPROM ...");
      strcpy(mon_config.valid, EEPROM_VALID);
      eeprom_put();
    }
  } /* entering new data */
  
  if(eeprom_validation((char *)EEPROM_VALID) == 0)  {
    eeprom_get();
    Serial.println("EEPROM data valid ... using it");
    dispall_eeprom_parms();
  }
  else  {
    Serial.println("EEPROM data NOT valid ... reset and try enter valid data");
    Serial.read();
  }
}

