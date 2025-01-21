/*
 * MechWarriorsWebNeopixels
 *
 * brief: define and control the playing of a sequence on a string of neopixels
 * using buttons on a web-enabled application.
 * 
 * this application is based on the webServer example below and snippets from
 * the teststrand neopixel example, provided by adafruit.
 *
 * sequence definition and playout is custom added code, as is the
 * html/js/c to put up the button webpage and handle button input.
 * 
 * libraries used:
 * - ArduinoJson by Benoit Blanchon v7.3.0 see documentation:
 *   https://arduinojson.org/v7/tutorial/deserialization/
 * - Adafruit NeoPixel by Adafruit v1.12.3
 *
 * Some notes regarding how this works 
 * (see the README.md for some things regarding the webserver example):
 * 
 * The webserver application provides the web server itself, and some built in 
 * functionality:
 * - webserver
 * - LittleFS for storing files on a flash-disk (persists through firmware flashing)
 *    (https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
 *   NOTE: the file system is located in the flash memory.
 *         the board type specification says how much of the flash is reserved for the fs
 *         seems to be reserved whether it's being used or not
 *         littleFS is said to do wear balancing (just as the name implies)
 * - built in functions for uploading files (maybe for deleting)
 *   (note: files are overwritten by files of the same name: useful for development)
 * If not specific URL is sent to the sebserver the implementation looks for a file
 * named index.htm.  That's the mechanism that I used for the button page (i.e. uploaded
 * html/js for the page in a file called index.htm)
 *
 * Some major discoveries:
 *
 * How to get a value from an htlm button to a c language callback ?
 * major learning: existence of server.arg("plain")

 * Regarding data flow from the button (html) -> js -> c callback:
 * html puts up buttons with an onclick: directive; that calls the 
 * javascript to make a POST to the server.  The button has an value= that
 * identifies which specific button was pressed, while calling the same js callback
 * for all buttons.  i.e. the identification of the button is sent via the 'this' argument
 * to the callback in the button html line.
 * 
 * javascript can see the value of the button as o.button, where o is the passed in argument.
 * it converts it to a json object, attaching the tag "sequence".  This json string becomes the 
 * body of the fetch/POST request to the server.
 *
 * the server makes the body of the POST available to ...
 *
 * index.html .........................................MechWarriorsWebNeopixels.ino
 * html/button .............callCfunction() ...........handleButton() 
 * html button "value" -> js o.value -> POST body -> server.arg("plain")
 *
 *
 * How to parse/deserialize a simple json string?
 * major learning: overloading and function of jsonDoc[]
 * see this in handleButton():
 *  const char *seq;
 *  JsonDocument jsonDoc;
 *  DeserializationError err;
 *  ...
 *  err = deserializeJson(jsonDoc, buf);
 *  if(err)  {
 *    TRACE("Deserialization of button failed: %s\n", err.f_str());
 *  }
 *  else  {
 *    TRACE("json parsing successful, extracting value\n");
 *    seq = jsonDoc["sequence"];
 *
 * jsonDoc[] is overloaded with a finite list of return types.
 * that's why const char *seq was necessary.
 *
 * 
 * How to open and read a file using littleFS ?
 * major learning: once the file is open, the read functions are provided
 *                 by the "file descriptor"-like class functions
 * see this in neo_load_sequence():
 *  TRACE("Loading filename %s ...\n", label);
 *  if((fd = LittleFS.open(label, "r")) != 0)  {
 *    while(fd.available())  {
 *      *pbuf++ = fd.read();
 *      ...
 * 
 * How to deserialize a json string with two dimensional array ?
 *
 * don't know yet ... working it out in neo_load_sequence()
 *
 * Last couple of neopixels were flickering during playback?
 *
 * seems eliminating the clear all pixels before writing the new
 * value seemed to eliminate it.  Although might have just been that
 * the update got faster ?
 *
 *
 * TODO:
 * o understand how the upload/delete is handled since it's a POST.
 *   determine if it has any conflicts with the button implementation.
 * o single shot sequences using -2 at termination
 * o more built in sequences
 * o scaling for cell phone versus computer (e.g. button size/layout)
 * o file based user uploaded sequences (probably json formatted files)
 * 
 *
 * (c) Daniel J. Zimmerman  Jan 2025
 *
 */

// @file WebServer.ino
// @brief Example implementation using the ESP8266 WebServer.
//
// See also README.md for instructions and hints.
//
// Changelog:
// 21.07.2021 creation, first version

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "secrets.h"  // add WLAN Credentials in here.

#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.

#include "bt_eepromlib.h"
#include "neo_data.h"  // for neopixels

// mark parameters not used in example
#define UNUSED __attribute__((unused))

// TRACE output simplified, can be deactivated here
#define TRACE(...) Serial.printf(__VA_ARGS__)

// name of the server. You reach it using http://webserver
#define HOSTNAME "warhammer"

// local time zone definition (US Central Daylight Time)
#define TIMEZONE "CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"

// get access to the eeprom based configuration structure
net_config *pmon_config = get_mon_config_ptr();

// need a WebServer for http access on port 80.
ESP8266WebServer server(80);

// The text of builtin files are in this header file
#include "builtinfiles.h"

// ===== Simple functions used to answer simple GET requests =====

// This function is called when the WebServer was requested without giving a filename.
// This will redirect to the file index.htm when it is existing otherwise to the built-in $upload.htm page
//
// this is used to display the main page after having uploaded a index.htm page with buttons
void handleRedirect() {
  TRACE("Redirect...");
  String url = "/index.htm";

  if (!LittleFS.exists(url)) { url = "/$update.htm"; }

  server.sendHeader("Location", url, true);
  server.send(302);  // send "found redirection" return code
}  // handleRedirect()


// This function is called when the WebServer was requested to list all existing files in the filesystem.
// a JSON array with file information is returned.
void handleListFiles() {
  Dir dir = LittleFS.openDir("/");
  String result;

  result += "[\n";
  while (dir.next()) {
    if (result.length() > 4) { result += ","; }
    result += "  {";
    result += " \"name\": \"" + dir.fileName() + "\", ";
    result += " \"size\": " + String(dir.fileSize()) + ", ";
    result += " \"time\": " + String(dir.fileTime());
    result += " }\n";
    // jc.addProperty("size", dir.fileSize());
  }  // while
  result += "]";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleListFiles()


// This function is called when the sysInfo service was requested.
void handleSysInfo() {
  String result;

  FSInfo fs_info;
  LittleFS.info(fs_info);

  result += "{\n";
  result += "  \"flashSize\": " + String(ESP.getFlashChipSize()) + ",\n";
  result += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
  result += "  \"fsTotalBytes\": " + String(fs_info.totalBytes) + ",\n";
  result += "  \"fsUsedBytes\": " + String(fs_info.usedBytes) + ",\n";
  result += "}";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleSysInfo()

// This function is called when the netInfo service was requested.
// ... added this to see if I could extend the built in functions ... worked (and useful)
void handleNetInfo() {
  String result;

  result += "{ NETWORK STATUS, \n";
  result += "  {";
    if(WiFi.status() == WL_CONNECTED) result += " \"status \": connected,\n";
    else result += " \"status \": disconnected,\n";  // not sure how this will ever be sent
    result += "    \"ip_address\":" + WiFi.localIP().toString() + ",\n";
    result += "  }\n";
  result += "}\n";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleNetInfo()

//
// handle button presses from the index.htm file
// - all buttons on the default page call this same function based
// - on being registered with a server.on(api/button) below
// - the javascript in index.htm constructs and sends a json string ("sequence" : "value"),
//   as the body of the POST,
//   to identify the identity of the specific button being pressed.
// - non-sequence, special purpose "value"'s are intercepted and processed
//   by this function, otherwise the value is sent to neo_set_sequence()
//   to do what the function name says
// 
void handleButton()  {
  int8_t neoerr = NEO_SUCCESS;
  char buf[128];
  char *file = NULL;
  const char *seq;
  JsonDocument jsonDoc;
  DeserializationError err;

  if(server.method() == HTTP_POST)  {
    /*
     * get the value of the button pressed
     */
    String body = server.arg("plain");
    TRACE("Button pressed: ");
    body.toCharArray(buf, sizeof(buf));
    TRACE("return buffer <%s>\n", buf);

    /*
     * parse the json to get to just the sequence label
     * NOTE: this code is very sensitive to types of variables
     * used in extracting values after parsing.  
     * e.g. const char *seq; was specifically required to get the
     * jsonDoc["sequence"] to build, apparently due to the overloading
     * that's built into this function (i.e. adding the const specifier).
     */

    err = deserializeJson(jsonDoc, buf);
    if(err)  {
      TRACE("Deserialization of button failed: %s\n", err.f_str());
      neoerr = NEO_DESERR;
    }
    else  {
      TRACE("json parsing successful, extracting value\n");
      seq = jsonDoc["sequence"];
      /*
       * process the button that was pressed based on the seq string
       */
      if(seq != NULL)  {
        TRACE("Setting sequence to %s\n", seq);
        /*
         * was it the stop button
         */
        if(strcmp(seq, "STOP") == 0)
          neo_cycle_stop();

        /*
         * if not STOP, see if it was a USER defined sequence
         * if so, load the file and set the sequence
         */
        else if((neoerr = neo_is_user(seq, &(file))) == NEO_SUCCESS)  {
          neoerr = neo_load_sequence(file);
        }

        /*
         * if not STOP or USER-x, then attempt to set the sequence,
         * assuming that it's a pre-defined button
         */
        else if((neoerr = neo_set_sequence(seq)) != NEO_SUCCESS)
          TRACE("Error setting sequence after proper detection\n");
      }
      else  {
        neoerr = NEO_NOPLACE;
        TRACE("\"sequence\" not found in json data\n");
      }
    }
    if(neoerr != NEO_SUCCESS)
      server.send(404, "text/plain", "handleButton(): Couldn't process button press");
    else
      server.send(201, "text/plain", "handleButton(): success");
  }
  else
    server.send(405, "text/plain", "handleButton(): Method Not Allowed");
}

// ===== Request Handler class used to answer more complex requests =====

// The FileServerHandler is registered to the web server to support DELETE and UPLOAD of files into the filesystem.
class FileServerHandler : public RequestHandler {
public:
  // @brief Construct a new File Server Handler object
  // @param fs The file system to be used.
  // @param path Path to the root folder in the file system that is used for serving static data down and upload.
  // @param cache_header Cache Header to be used in replies.
  FileServerHandler() {
    TRACE("FileServerHandler is registered\n");
  }


  // @brief check incoming request. Can handle POST for uploads and DELETE.
  // @param requestMethod method of the http request line.
  // @param requestUri request ressource from the http request line.
  // @return true when method can be handled.
  bool canHandle(HTTPMethod requestMethod, const String UNUSED &_uri) override {
    return ((requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE));
  }  // canHandle()


  bool canUpload(const String &uri) override {
    // only allow upload on root fs level.
    return (uri == "/");
  }  // canUpload()


  bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String &requestUri) override {
    // ensure that filename starts with '/'
    String fName = requestUri;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (requestMethod == HTTP_POST) {
      // all done in upload. no other forms.

    } else if (requestMethod == HTTP_DELETE) {
      if (LittleFS.exists(fName)) { LittleFS.remove(fName); }
    }  // if

    server.send(200);  // all done.
    return (true);
  }  // handle()


  // uploading process
  void upload(ESP8266WebServer UNUSED &server, const String UNUSED &_requestUri, HTTPUpload &upload) override {
    // ensure that filename starts with '/'
    String fName = upload.filename;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (upload.status == UPLOAD_FILE_START) {
      // Open the file
      if (LittleFS.exists(fName)) { LittleFS.remove(fName); }  // if
      _fsUploadFile = LittleFS.open(fName, "w");

    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // Write received bytes
      if (_fsUploadFile) { _fsUploadFile.write(upload.buf, upload.currentSize); }

    } else if (upload.status == UPLOAD_FILE_END) {
      // Close the file
      if (_fsUploadFile) { _fsUploadFile.close(); }
    }  // if
  }    // upload()

protected:
  File _fsUploadFile;
};


// SETUP everything to make the webserver work.
void setup(void) {
  delay(3000);  // wait for serial monitor to start completely.

  // Use Serial port for some trace information from the example
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  /*
   * initialize the EEPROM
   */
  eeprom_begin();
  
  Serial.println("Press any key to change settings");

  int8_t tries = 5;
  bool out = false;
  char inChar = '\0';
  while((out == false) && (tries > 0))  {
    Serial.print(tries);Serial.print(" . ");
  // check for incoming serial data:
    if (Serial.available() > 0) {
      // read incoming serial data:
      inChar = Serial.read();
      out = true;
    }
    delay(1000);
    tries--;
  }
  Serial.println();

  /*
   * prompt the user for input if the out bool was set above
   * (if not, this function does nothing)
   */
  eeprom_user_input(out);

  TRACE("Starting WebServer example...\n");

  TRACE("Mounting the filesystem...\n");
  if (!LittleFS.begin()) {
    TRACE("could not mount the filesystem...\n");
    delay(2000);
    ESP.restart();
  }

  /*
   * setup wifi to use a fixed IP address
   * attempt to convert the ip address from the eeprom, and
   * if that fails load a default address
   * TODO: in progress - DJZ
   */
  uint8_t ip[4]; // ip address as octets
  if(eeprom_convert_ip(pmon_config->ipaddr, ip) != 0)  {
    TRACE("Failed to convert eeprom IP address value ... loading default\n");
    ip[0] = 192; ip[1] = 168; ip[2] = 1; ip[3] = 37;
  }
  const byte gateway[] = {192, 168, 1, 1}; // Gateway address
  const byte subnet[] = {255, 255, 255, 0}; // Subnet mask
  WiFi.config(ip, gateway, subnet); // Set static IP ... DZ added this

  // start WiFI
  WiFi.mode(WIFI_STA);
  if (strlen(ssid) == 0) {
    WiFi.begin();
  } else {
    WiFi.begin(ssid, passPhrase);
  }

  // allow to address the device by the given name e.g. http://webserver
  WiFi.setHostname(HOSTNAME);

  TRACE("Connect to WiFi...\n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    TRACE(".");
  }
  TRACE("connected at %s\n", WiFi.localIP().toString().c_str());

  // Ask for the current time using NTP request builtin into ESP firmware.
  TRACE("Setup ntp...\n");
  if(strlen(pmon_config->tz_offset_gmt) > 0)
    configTime(pmon_config->tz_offset_gmt, "pool.ntp.org");
  else
    configTime(TIMEZONE, "pool.ntp.org");

  TRACE("Register service handlers...\n");

  // serve a built-in htm page
  server.on("/$upload.htm", []() {
    server.send(200, "text/html", FPSTR(uploadContent));
  });

  // register a redirect handler when only domain name is given.
  server.on("/", HTTP_GET, handleRedirect);

  //
  // register some REST services
  // NOTE: the server attempts to find a match with the URL's registered here,
  // before falling back to the server.addHandler(new FileServerHandler()) below
  // (e.g. "/api/button" intercepts POST requests for buttons ... POSTS for
  // file operations are passed along)
  //
  server.on("/$list", HTTP_GET, handleListFiles);
  server.on("/$sysinfo", HTTP_GET, handleSysInfo);
  server.on("/$netinfo", HTTP_GET, handleNetInfo);
  server.on("/api/button", HTTP_POST, handleButton);

  // UPLOAD and DELETE of files in the file system using a request handler.
  server.addHandler(new FileServerHandler());

  // enable CORS header in webserver results
  server.enableCORS(true);

  // enable ETAG header in webserver results from serveStatic handler
  server.enableETag(true);

  // serve all static files
  server.serveStatic("/", LittleFS, "/");

  // handle cases when file is not found
  server.onNotFound([]() {
    // standard not found in browser.
    server.send(404, "text/html", FPSTR(notFoundContent));
  });

  server.begin();
  TRACE("hostname=%s\n", WiFi.getHostname());

  // initialize neopixel strip
  TRACE("Initialize neopixel strip with %d pixels...\n", atoi(pmon_config->neocount));
  if(atoi(pmon_config->neocount) > 0)
    neo_init(atoi(pmon_config->neocount), NEO_PIN, NEO_TYPE);
  else
    neo_init(NEO_NUMPIXELS, NEO_PIN, NEO_TYPE);


}  // setup


// run the server...
void loop(void) {
  server.handleClient();
  neo_cycle_next();
}  // loop()

// end.
