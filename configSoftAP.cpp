#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Arduino_DebugUtils.h>

#include "bt_eepromlib.h"
#include "configSoftAP.h"


ESP8266WebServer ap_server(80);  // Web server on port 80
DNSServer dnsServer;           // DNS server for redirection
#define GET_CONFIG_BUF_SIZE 4096
static char getConfigContent[GET_CONFIG_BUF_SIZE];  // to consolidate captive page contents

static char getConfigContent_js[] PROGMEM = R"==(
  <br>
  <title>Enter new configuration values and press <Submit> to save; <Cancel> to leave unchanged</title>
  <br>
  <script>
    function deviceConfig(event) {
      event.preventDefault();
      var ssid = document.getElementById('WIFI_SSID').value;
      var password = document.getElementById('WIFI_Password').value;
      var dhcp = document.getElementById('WIFI_DHCP').value;

      let config_data = {
        action: String("save"),
        ssid: String(document.getElementById('WIFI_SSID').value),
        password: String(document.getElementById('WIFI_Password').value),
        dhcp: String(document.getElementById('WIFI_DHCP').value)
      }
      let jsonData = JSON.stringify(config_data);
      console.log(jsonData);

      fetch('api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: jsonData
      })
      .then(response => {
        if(response.status != 200) {
          window.alert("Error processing configuration");
        }
        return response.text();
      })
      .then(result => console.log(result))
      .catch(error => console.error('Error:', error));
    }

    function handleCancel()  {
      console.log("action cancelled");
    }
  </script>
)==";



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
  char buf[128];
  JsonDocument jsonDoc;
  DeserializationError err;

  if(ap_server.method() == HTTP_POST)  {
    /*
     * get the value of the button pressed
     */
    String body = ap_server.arg("plain");
    DEBUG_DEBUG("Config Form Received: ");
    body.toCharArray(buf, sizeof(buf));
    DEBUG_DEBUG("return buffer <%s>\n", buf);
    ap_server.send(200, "text/html", "From C callback handleSubmit(): Submit Pressed");
  }
}


void configSoftAP(void) {

  const char *ssid_AP = AP_SSID;  // SoftAP SSID
  const char *password_AP = AP_PASSWD;     // SoftAP Password ... must be long-ish for ssid to be advertised

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
   * add the dynamically created, using current eeprom settings, html portion of the response,
   * to the buffer containing the javascript portion from above.
   * take care not to overrun the buffer
   * NOTE: tried to do this in the root callback and it kept core dumping ...
   */
  strncpy(getConfigContent, getConfigContent_js, GET_CONFIG_BUF_SIZE);
  createHTMLfromEEPROM((char *)(getConfigContent+strlen(getConfigContent)), GET_CONFIG_BUF_SIZE-strlen(getConfigContent));
  DEBUG_DEBUG("getConfigContent after html:\n%s\n", getConfigContent);

  Serial.println("Press any key to close server");

  while(Serial.available() == 0)  {
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


