#include <Arduino.h>
#include <WiFi.h>

/**
 * Print nearby WiFi networks with SSID und RSSI 
 */
void printNearbyNetworks()
{
  int n = WiFi.scanNetworks();
  printf(R"(
Nearby WiFi networks:
--------------------
)");
  for (int i = 0; i < n; i++)
  {
    printf("%s\t%d\r\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
}


/**
 * Use a raw string literal to print a formatted
 * string of WiFi connection details
 */
void printConnectionDetails()
{
  printf(R"(
Connection Details:
------------------
  SSID       : %s
  Hostname   : %s
  IP-Address : %s
  MAC-Address: %s
  RSSI       : %d (received signal strength indicator)

  )", WiFi.SSID().c_str(),
	    WiFi.getHostname(),   // ESP32 
      WiFi.localIP().toString().c_str(),
      WiFi.macAddress().c_str(),
      WiFi.RSSI());  
}


/**
 * Establish the WiFi connection with router 
 * and set a hostname for the ESP32
 */
bool initWiFi(const char ssid[], const char password[], const char hostname[])
{
  WiFi.setHostname(hostname);  // ESP32
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  return (WiFi.waitForConnectResult() != WL_CONNECTED) ? false : true;
}
