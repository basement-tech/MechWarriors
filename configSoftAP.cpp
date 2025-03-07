#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include "configSoftAP.h"


ESP8266WebServer ap_server(80);  // Web server on port 80
DNSServer dnsServer;           // DNS server for redirection

static const char getssidContent[] PROGMEM = R"==(
  <script>
    function wifiCredentials(event) {
      event.preventDefault();
      var ssid = document.getElementById('ssid').value;
      var password = document.getElementById('password').value;
      var dhcp = document.getElementById('dhcp').value;

      console.log("SSID: " + ssid);
      console.log("Password: " + password);
      console.log("DHCP: " + dhcp);
    }
  </script>
  <form onsubmit="wifiCredentials(event)">
    <label for="credentials">Enter WiFi Credentials:</label>
    <br>
    <input type="text" id="ssid" name="ssid" placeholder="<ssid>"><br><br>
    <input type="text" id="password" name="password" placeholder="<password>)"><br><br>
    <input type="text" id="dhcp" name="dhcp" placeholder="DHCP(yes or no)"><br><br>
    <button type="submit">Submit</button>
  </form>
)==";


void handleRoot() {
  ap_server.send(200, "text/html", getssidContent);
}

void handleNotFound() {
  ap_server.sendHeader("Location", String("http://192.168.4.1/"), true);
  ap_server.send(302, "text/plain", "");
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

  // Start web server
  ap_server.begin();
  Serial.println("Web server started!");

  Serial.print("Free Heap Before SoftAP Cleanup: ");
  Serial.println(ESP.getFreeHeap());  

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


