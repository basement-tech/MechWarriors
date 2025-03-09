#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Arduino_DebugUtils.h>

#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.

#include "bt_eepromlib.h"
#include "configSoftAP.h"


ESP8266WebServer ap_server(80);  // Web server on port 80
DNSServer dnsServer;           // DNS server for redirection
#define GET_CONFIG_BUF_SIZE 5120
static char getConfigContent[GET_CONFIG_BUF_SIZE];  // to consolidate captive page contents
static bool config_done = false;  // done config ... reboot


/*
 * this function is only executed when the device is being configured.
 * so, not worrying about PROGMEM for html/js and hoping the buffer get
 * reused.
 */
void handleRoot(void) {
  ap_server.send(200, "text/html", getConfigContent);
}

void handleNotFound(void) {
  ap_server.sendHeader("Location", String("http://192.168.4.1/"), true);
  ap_server.send(302, "text/plain", "");
}

void handleSubmit(void)  {
  JsonDocument jsonDoc;
  DeserializationError err;
  const char *jbuf;  // jsonDoc[] requires this type

  if(ap_server.method() == HTTP_POST)  {
    /*
     * get the value of the button pressed
     */
    String body = ap_server.arg("plain");
    DEBUG_DEBUG("Config Form Received\n");

    err = deserializeJson(jsonDoc, body);
    if(err)  {
      DEBUG_ERROR("ERROR: Deserialization of config response failed\n");
    }
    else {
      if(jsonDoc["action"].isNull())
        DEBUG_ERROR("WARNING: config response has no member \"action\" ... no change\n");
      else  {
        jbuf = jsonDoc["action"];
        if(strcmp(jbuf, "save") == 0)  {
          saveJsonToEEPROM(jsonDoc);
          ap_server.send(200, "text/html", "Successfully saved");
        }
        else if(strcmp(jbuf, "cancel") == 0)  {
          config_done = true;
          ap_server.send(200, "text/html", "Configuration Cancelled");
        }
        else  {
          config_done = true;
          DEBUG_ERROR("WARNING: invalid value for \"action\" ... no change\n");
          ap_server.send(404, "text/html", "Invalid value for \"action\"");
        }
      }
    }


  }
}


void configSoftAP(void) {
  int8_t ret = 0;
  File fd;  // file pointer to read from
  char *pbuf;  // helper

  const char *ssid_AP = AP_SSID;  // SoftAP SSID
  const char *password_AP = AP_PASSWD;     // SoftAP Password ... must be long-ish for ssid to be advertised

  config_done = false;

  IPAddress local_IP(AP_LOCAL_IP);       // Custom IP Address
  IPAddress gateway(AP_GATEWAY);        // Gateway
  IPAddress subnet(AP_SUBNET);       // Subnet Mask

  Serial.println("Starting local AP for configuration");
  Serial.print("Connect to: ");
  Serial.print(AP_SSID);
  Serial.println(" to configure");

  // Configure and start SoftAP
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid_AP, password_AP);

  Serial.print("SoftAP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS Server to redirect all requests to ESP8266
  dnsServer.start(53, "*", local_IP);

  // Define web server routes
  ap_server.on("/", handleRoot);
  ap_server.onNotFound(handleNotFound);
  ap_server.on("/api/config", HTTP_POST, handleSubmit);

  // Start web server
  ap_server.begin();
  Serial.println("Web server started!");

  Serial.print("Free Heap Before SoftAP Cleanup: ");
  Serial.println(ESP.getFreeHeap());  

  /*
   * copy the top of the html and javascript part of the config page
   * out of the file AP_JS_NAME
   * NOTE: the final </html> is added after the dynamically created html is created.
   */
  if (LittleFS.exists((const char *)(AP_JS_NAME)) == false)  {
      DEBUG_ERROR("ERROR: Filename %s does not exist in file system\n", AP_JS_NAME);
      ret = -1;
  }
  else  {
    DEBUG_INFO("Loading filename %s ...\n", AP_JS_NAME);
    if((fd = LittleFS.open((const char *)(AP_JS_NAME), "r")) == false)  
      ret = -1;
    else  {
      pbuf = getConfigContent;
      while(fd.available())  {
        *pbuf++ = fd.read();
      }
      *pbuf = '\0';  // terminate the char string
      fd.close();
      DEBUG_VERBOSE("Raw file contents:\n%s\n", getConfigContent);
    }
  }

  /*
   * add the dynamically created, using current eeprom settings, html portion of the response,
   * to the buffer containing the javascript portion.
   * take care not to overrun the buffer.
   * NOTE: tried to do this in the root callback and it kept core dumping ...
   *  may have been the upper limit of the debug message utility, but I like
   *  this better anyway.
   */
  createHTMLfromEEPROM((char *)(getConfigContent+strlen(getConfigContent)), GET_CONFIG_BUF_SIZE-strlen(getConfigContent));
  strncpy((char*)(getConfigContent+strlen(getConfigContent)), "</html>\n",  (GET_CONFIG_BUF_SIZE-strlen(getConfigContent) < 0 ? 0 : GET_CONFIG_BUF_SIZE-strlen(getConfigContent)));
  getConfigContent[GET_CONFIG_BUF_SIZE-1] = '\0';  // just in case ... at least it's a string even if incomplete

  Serial.println("Press any key to close server");

  while((Serial.available() == 0) && (config_done == false))  {
    dnsServer.processNextRequest();  // Handle DNS requests
    ap_server.handleClient();           // Handle web requests
  }

  // Stop services
  ap_server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);

  Serial.print("Free Heap After SoftAP Cleanup: ");
  Serial.println(ESP.getFreeHeap()); 

  ESP.restart();
}


