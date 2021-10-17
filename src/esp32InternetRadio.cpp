/**
 * Program      esp32InternetRadioMax98357.cpp
 * Author       2021-07-27 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      This program shows how to use the ESP32-audioI2S library 
 *              from "Wolle" aka "schreibfaul" to build a internet radio 
 *              It is an adaptation of his example program "Simple_WiFi_Radio".
 *              I used a MAX98357A DAC/Amplifier in mono configuration.
 *              In contrast to the internet radio with the ESP8266, the serial 
 *              interface is available for inputs and outputs with the ESP32. 
 *              Therefore I designed the user interface as a simple CLI menu. 
 *              It allows the
 *                - selection of 24 radio stations (easy to expand), 
 *                - text-to-speech output with 3 examples in the languages 
 *                  English, German, Italian, 
 *                - volume control up and down, 
 *                - switching on and off of the loudspeaker 
 *                - display of the currently played radio station
 *                - redisplaying the menu. 
 *              
 * Parts        - ESP32 Doit DevKit V1
 *              - MAX98357A breakout board or UDA1334A 
 *              - Speaker 8 ohm / 0.5 W
 * 
 * Wiring                            .-----------------. 
 *               GPIO_NUM_25 -->     o LRC             |  
 *               GPIO_NUM_26 -->     o BCLK       MAX  |
 *               GPIO_NUM_27 -->     o DIN       98357 |
 *                                   o Gain            |   Spkr
 *                                   o SD              |    _/|
 *                       GND -->     o GND             o---|  |
 *                       5V  -->     o Vin (5V)        o---|_ |
 *                                   `-----------------´     \|
 * 
 *              👉 With exactly the same wiring you can also connect a 
 *                 UDA1334A I2S DAC with stereo output for headphones 
 *  
 * Remarks      To run update the WiFi credentials and compile
 *
 * References   https://github.com/schreibfaul1
 *              https://how2electronics.com/esp32-music-audio-mp3-player/
 *              https://diyi0t.com/i2s-sound-tutorial-for-esp32/
 *              https://www.hackster.io/earlephilhower/esp8266-digital-radio-ee747f
 */

#include <Arduino.h>
#include <WiFi.h>
#include "Audio.h"
 
// I2S pins
#define I2S_LRC        GPIO_NUM_25  // LRC  of MAX98357
#define I2S_BCLK       GPIO_NUM_26  // BCLK     "
#define I2S_DOUT       GPIO_NUM_27  // DIN      "

#define CLEAR_LINE     Serial.printf("\r%*s\r", 128, "")
#define MIN_VOLUME     0
#define MAX_VOLUME     21
#define DEFAULT_VOLUME 10


 
Audio audio;

// WiFi credentials 
const char ssid[]     = "YOUR SSID";
const char password[] = "YOUR PSK";
const char hostname[] = "esp32-radio";

// Example texts for text-to-speech demo
const char *text[]     = {
"Internet radio (also web radio, net radio, streaming radio, e-radio, IP radio, online radio) is a digital audio service transmitted via the Internet.",
"Als Internetradio (auch Webradio) bezeichnet man ein Internet-basiertes Angebot an Hörfunksendungen.",
"Internet radio (anche web radio) è il termine usato per descrivere una gamma di programmi radiofonici su Internet.",
};

typedef struct { const char key; const char *name; const char *url; } Radiostation;
Radiostation station[] =
{
  { '0', "Klassik Radio",     "http://stream.klassikradio.de/live/mp3-128/stream.klassikradio.de" },
  { '1', "SRF1 AG-SO",        "http://stream.srg-ssr.ch/m/regi_ag_so/mp3_128" },
  { '2', "SRF2",              "http://stream.srg-ssr.ch/m/drs2/mp3_128" },
  { '3', "SRF3",              "http://stream.srg-ssr.ch/m/drs3/mp3_128" },
  { '4', "SRF4 News",         "http://stream.srg-ssr.ch/m/drs4news/mp3_128" },
  { '5', "Swiss Classic",     "http://stream.srg-ssr.ch/m/rsc_de/mp3_128" },
  { '6', "Swiss Jazz",        "http://stream.srg-ssr.ch/m/rsj/mp3_128" },
  { '7', "SRF Musikwelle",    "http://stream.srg-ssr.ch/m/drsmw/mp3_128" },
  { '8', "Alles Blasmusik",   "http://stream.bayerwaldradio.com/allesblasmusik" },
  { '9', "WKVI-AM",           "http://kvbstreams.dyndns.org:8000/wkvi-am" },
  { 'a', "DLF",               "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3" },
  { 'b', "WDR 1 Live",        "http://www.wdr.de/wdrlive/media/einslive.m3u" },
  { 'c', "SWR1 BW",           "http://mp3-live.swr.de/swr1bw_m.m3u" },
  { 'd', "SWR2",              "http://mp3-live.swr.de/swr2_m.m3u" },
  { 'e', "SWR3",              "http://mp3-live.swr3.de/swr3_m.m3u" },
  { 'f', "SWR4 BW",           "http://mp3-live.swr.de/swr4bw_m.m3u" },
  { 'g', "SWR Aktuell",       "http://mp3-live.swr.de/swraktuell_m.m3u" },
  { 'h', "DasDing",           "http://mp3-live.dasding.de/dasding_m.m3u" },
  { 'i', "Jazz MMX",          "http://jazz.streamr.ru/jazz-64.mp3" },
  { 'j', "Irish Pub Radio",   "http://macslons-irish-pub-radio.com/media.asx" },
  { 'k', "HIT Radio FFH MP3", "http://mp3.ffh.de/radioffh/hqlivestream.mp3" },
  { 'l', "Capital London",    "http://vis.media-ice.musicradio.com/CapitalMP3" },
  { 'm', "ORF",               "https://orf-live.ors-shoutcast.at/vbg-q1a" },
  { 'n', "Beatles Radio",     "http://www.beatlesradio.com:8000/stream/1/" },
  { '!', "Speech 1",              "" },
  { '.', "Speech 2",              "" },
  { ',', "Speech 3",              "" },
  { '+', "Increment volume",      "" },
  { '-', "Decrement volume",      "" },
  { 'T', "Toggle speaker on/off", "" },
  { 'C', "Show current Station",  "" },
  { 'S', "Show Menu",             "" },
};
constexpr uint8_t nbrRadiostations = sizeof(station) / sizeof(station[0]);

int currentStation     = 5;  // preselect Swiss Classic
const char *currentUrl = station[currentStation].url;
int currentVolume      = DEFAULT_VOLUME;

/**
 * Print name and url of current station
 */
void showCurrentStation() 
{
  CLEAR_LINE;
  Serial.printf("Current Station: %s --> %s", station[currentStation].name, currentUrl);
};

/**
 * Display menu on monitor
 */
void showMenu()
{
  // title is packed into a formatted raw string
  Serial.print(
  R"TITLE(
-----------------
 ESP32 Web Radio 
-----------------
)TITLE");

  for (int i = 0; i < nbrRadiostations; i++)
  {
    Serial.printf("[%c] %s\n", station[i].key, station[i].name);
  }
  Serial.print("\nPress a key: ");
}

/**
 * Set volum to the next higher level (0..21)
 */
void incrementVolume()
{
  if (currentVolume < MAX_VOLUME) 
  {
    currentVolume++;
    audio.setVolume(currentVolume);
  }
  CLEAR_LINE;
  Serial.printf("Current Volume: %d", currentVolume);
}

/**
 * Set the volume to the next lower level (0..21)
 */
void decrementVolume()
{
  if (currentVolume > 0) 
  {
    currentVolume--;
    audio.setVolume(currentVolume); // 0...21
  }
  CLEAR_LINE;
  Serial.printf("Current Volume: %d", currentVolume);
}

/**
 * Toggle speaker on and off
 * When speaker os off and volume is at minimal value
 * the default volume is set when speaker is toggled on
 */
void toggleSpeaker()
{
  static bool spkrIsOn = true;
  if (spkrIsOn)
  {
    audio.setVolume(MIN_VOLUME);
    spkrIsOn = false;
    CLEAR_LINE;
    Serial.printf("Speaker is off");
  }
  else
  {
    if (currentVolume == MIN_VOLUME) currentVolume = DEFAULT_VOLUME;
    audio.setVolume(currentVolume);
    spkrIsOn = true;
    CLEAR_LINE;
    Serial.printf("Speaker is on");
  }
}

/**
 * Get the keystroke from the operator and 
 * perform the corresponding action
 */
void doMenu()
{
  char key = Serial.read();
  CLEAR_LINE;

  for (int i = 0; i < nbrRadiostations; i++)
  {
    if (key == station[i].key)
    {
      switch(key)
      {
        case 'S':
          showMenu();
        break;
        case 'C':
          showCurrentStation();
        break;
        case 'T':
          toggleSpeaker();
        break;
        case '!':
          audio.connecttospeech(text[0], "en");
        break;
        case '.':
          audio.connecttospeech(text[1], "de");
        break;
        case ',':
          audio.connecttospeech(text[2], "it");
        break;        
        case '+':
          incrementVolume();
        break;
        case '-':
          decrementVolume();
        break;
        default:
          currentStation = i;
          currentUrl = station[i].url;
          audio.connecttohost(currentUrl);
        break;  
      }

      break; // for loop
    }
  } 
}

/**
 * Use a raw string literal to print a formatted
 * string of WiFi connection details
 */
void printConnectionDetails()
{
  Serial.printf(R"(
Connection Details:
------------------
  SSID       : %s
  Hostname   : %s
  IP-Address : %s
  MAC-Address: %s
  RSSI       : %d (received signal strength indicator)
  )", WiFi.SSID().c_str(),
      //WiFi.hostname().c_str(),  // ESP8266
      WiFi.getHostname(),    // ESP32 
      WiFi.localIP().toString().c_str(),
      WiFi.macAddress().c_str(),
      WiFi.RSSI());
  Serial.printf("\n");  
}

/**
 * Establish the WiFi connection
 */
void initWiFi()
{
  Serial.println("Connecting to WiFi");
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);  // this line only needed for ESP32 to set the hostname
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  // Try forever
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("...Connecting to WiFi");
    delay(1000);
  }
  Serial.println("Connected");
  printConnectionDetails();
}

/**
 * Initialize the audio subsystem
 */
void initAudio()
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(currentVolume); // 0...21
    audio.connecttohost(currentUrl);
}

/**
 * Returns true, as soon as msWait milliseconds have passed.
 * Supply a reference to an unsigned long variable to hold 
 * the previous milliseconds.
 */
bool waitIsOver(uint32_t &msPrevious, uint32_t msWait)
{
  return (millis() - msPrevious >= msWait) ? (msPrevious = millis(), true) : false;
}

void setup() 
{
    Serial.begin(115200);
    initWiFi();
    initAudio();
}
 
void loop()
{
    static uint32_t msPrevious = millis();
    static bool done = false;

    audio.loop();

    // show menu once after all status and info messages have been displayed
    if (!done && waitIsOver(msPrevious, 5000)) { done = true; showMenu(); }

    // handle keystrokes and the menu
    if (Serial.available()) doMenu();    
}
 

// optional event handlers
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreaminfo(const char *info){
    Serial.print("streaminfo  ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}