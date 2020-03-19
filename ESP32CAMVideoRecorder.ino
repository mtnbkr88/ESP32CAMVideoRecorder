/********************************************************************************************

ESP32-CAM Video Recorder

03/18/2020 Ed Williams 

This version of the ESP32-CAM Video Recorder is built on the work of many other listed below.
It has been hugely modified to be a fairly complete web camera server with the following
capabilities:
  - Record AVI video and peak, stream, download or delete the saved videos.
  - Take JPG pictures and view, download or delete the saved pictures.
  - Up to 250 videos and 250 pictures can be saved on the SD card. Beyond that as more
    are saved the oldest are deleted.
  - Removed the ability to change the size, frame rate and quality. They are permanently 
    set to size 640x480, 125ms and quality 10.
  - Recording times can be set from 10 to 300 seconds
  - Motion detection is available if a PIR motion sensor is connected to GPIO3 (U0RXD)
    (after the firmware is uploaded). 
    Motion detection can trigger the following actions:
    - Take a picture.
    - Take a picture and email it.
    - Make a video.
    - Make a video and send an email with name of video.
    Motion detection is checked every second and starts checking again one minute
    after a trigger, so if you don't want any gaps in motion detected videos, set 
    recording time to 60 seconds.
  - Normal web traffic is on port 80, streaming comes from port 90. If accessing from 
    outside local network, forward port X to 80 and port X+10 to port 90.
  - The original included FTP server was removed since files can now be managed through
    the web site.
  - Note: This sketch uses a simple sendemail library I found on github by Peter Gorasz
    (sendemail.cpp and sendemail.h) but modified by me to allow sending one file from 
    the SD card as an attachment. The sendemail pieces are included in this sketch and 
    can be moved to external files if you want to use them for something else.
  - Use a top quality SD card with at least 16G. If the camera crashes after recording a
    lot of videos, the SD card might be full.

When using the camera and SD card, I was only able to get GPIO3 (U0RXD) pin to work
with the PIR but I had to wire it up as follows:

Use a 2N4401 transistor with the collector hooked to a 1K resistor then hooked to GPIO3
(connect after done uploading sketch). Connect the Dout wire from the PIR to a 1K resistor
then connect to base of transistor. Connect the emitter of the transistor to GND.

Used pinMode(GPIO_NUM_3, INPUT_PULLUP) in the setup to pull the pin high. Now when 
motion is detected, GPIO3 will be pulled low, otherwise it will be high.


The descriptons below are from authors of previous versions of this sketch and some of 
the capabilities they describe, like using an FTP Server and changing size, frame rate 
and quality of recorded videos have been removed.

****************************************************************************************/


/***********************************************************************************************************************************************************
 *  TITLE: This sketch is derived from the following Github repository and is mainly for testing the current work done by the team. It allows you 
 *  to record video onto a microSD card, using the ESP32-CAM board. Videos can be remotely obtained by accessing the FTP server contained within the sketch.
 *
 *  By Frenoy Osburn
 *  YouTube Video: https://youtu.be/lc_gXfkoRZo
 *  BnBe Post: https://www.bitsnblobs.com/video-recording-with-the-esp32-cam-board
 ***********************************************************************************************************************************************************/
/********************************************************************************************************************
 *  Board Settings:
 *  Board: "ESP32 Wrover Module"
 *  Upload Speed: "921600"
 *  Flash Frequency: "80MHz"
 *  Flash Mode: "QIO"
 *  Partition Scheme: "Hue APP (3MB No OTA/1MB SPIFFS)"
 *  Core Debug Level: "None"
 *  COM Port: Depends *On Your System*
 *********************************************************************************************************************/
 
 /*
  TimeLapseAvi

  ESP32-CAM Video Recorder

  This program records an AVI video on the SD Card of an ESP32-CAM.

  by James Zahary July 20, 2019  TimeLapseAvi23x.ino
     jamzah.plc@gmail.com

  https://github.com/jameszah/ESP32-CAM-Video-Recorder
    jameszah/ESP32-CAM-Video-Recorder is licensed under the
    GNU General Public License v3.0

  ~~~
  Update Sep 15, 2019 TimeLapseAvi39x.ino
  - work-in-progress
  - I'm publishing this as a few people have been asking or working on this
  
  - program now uses both cpu's with cpu 0 taking pictures and queue'ing them
    and a separate task on cpu 1 moving the pictures from the queue and adding
    them to the avi file on the sd card
  - the loop() task on cpu 1 now just handles the ftp system and http server
  - dropped fixed ip and switch to mDNS with name "desklens", which can be typed into
    browser, and also used as wifi name on router
  - small change to ftp to cooperate with WinSCP program
  - fixed bug so Windows would calulcate the correct length (time length) of avi 
  - when queue of frames gets full, it slips every other frame to try to catch up
  - camera is re-configued when changing from UXGA <> VGA to allow for more buffers 
    with the smaller frames
  ~~~

  The is Arduino code, with standard setup for ESP32-CAM
    - Board ESP32 Wrover Module
    - Partition Scheme Huge APP (3MB No OTA)

  This program records an AVI video on the SD Card of an ESP32-CAM.

  It will record realtime video at limited framerates, or timelapses with the full resolution of the ESP32-CAM.
  It is controlled by a web page it serves to stop and start recordings with many parameters, and look through the viewfinder.

  You can control framesize (UXGA, VGA, ...), quality, length, and fps to record, and fps to playback later, etc.

  There is also an ftp server to download the recordings to a PC.

  Instructions:

  The program uses a fixed IP of 192.168.1.188, so you can browse to it from your phone or computer.

  http://192.168.1.188/ -- this gives you the status of the recording in progress and lets you look through the viewfinder

  http://192.168.1.188/stop -- this stops the recording in progress and displays some sample commands to start new recordings

  ftp://192.168.1.188/ -- gives you the ftp server

  The ftp for esp32 seems to not be a full ftp.  The Chrome Browser and the Windows command line ftp's did not work with this, but
  the Raspbarian command line ftp works fine, and an old Windows ftp I have called CoffeeCup Free FTP also works, which is what I have been using.
  You can download at about 450 KB/s -- which is better than having to retreive the SD chip if you camera is up in a tree!

  http://192.168.1.188/start?framesize=VGA&length=1800&interval=250&quality=10&repeat=100&speed=1&gray=0  -- this is a sample to start a new recording

  framesize can be UXGA, SVGA, VGA, CIF (default VGA)
  length is length in seconds of the recording 0..3600 (default 1800)
  interval is the milli-seconds between frames (default 200)
  quality is a number 5..50 for the jpeg  - smaller number is higher quality with bigger and more detailed jpeg (default 10)
  repeat is a number of how many of the same recordings should be made (default 100)
  speed is a factor to speed up realtime for a timelapse recording - 1 is realtime (default 1)
  gray is 1 for a grayscale video (default 0 - color)

  These factors have to be within the limit of the SD chip to receive the data.
  For example, using a LEXAR 300x 32GB microSDHC UHS-I, the following works for me:

  UXGA quality 10,  2 fps (or interval of 500ms)
  SVGA quality 10,  5 fps (200ms)
  VGA  quality 10, 10 fps (100ms)
  CIG  quality 10, 20 fps (50ms)

  If you increase fps, you might have to reduce quality or framesize to keep it from dropping frames as it writes all the data to the SD chip.

  Also, other SD chips will be faster or slower.  I was using a SanDisk 16GB microSDHC "Up to 653X" - which was slower and more unpredictable than the LEXAR ???

  Search for "zzz" to find places to modify the code for:
    1.  Your wifi name and password
    2.  Your preferred ip address (with default gateway, etc)
    3.  Your Timezone for use in filenames
    4.  Defaults for framesize, quality, ... and if the recording should start on reboot of the ESP32 without receiving a command

  Acknowlegements:

  1.  https://robotzero.one/time-lapse-esp32-cameras/
      Timelapse programs for ESP32-CAM version that sends snapshots of screen.
  2.  https://github.com/nailbuster/esp8266FTPServer
      ftp server (slightly modifed to get the directory function working)
  3.  https://github.com/ArduCAM/Arduino/tree/master/ArduCAM/examples/mini
      ArduCAM Mini demo (C)2017 LeeWeb: http://www.ArduCAM.com
      I copied the structure of the avi file, some calculations.

*/


//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_camera.h"

//#include <WiFi.h>   // redundant

#include <HTTPClient.h>
//#include "SD_MMC.h"
//#include <FS.h>


/**********************************************************************************
 * 
 *  From here to end comment below is the contents of sendemail.h
 * 
 */

#ifndef __SENDEMAIL_H
#define __SENDEMAIL_H

// uncomment for debug output on Serial port
//#define DEBUG_EMAIL_PORT

// in order to send attachments, this SendEmail class assumes an SD card is mounted as /sdcard

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <FS.h>
#include "SD_MMC.h"

class SendEmail
{
  private:
    const String host;
    const int port;
    const String user;
    const String passwd;
    const int timeout;
    const bool ssl;
    WiFiClient* client;
    String readClient();

  public:
    SendEmail(const String& host, const int port, const String& user, const String& passwd, const int timeout, const bool ssl);

    // attachment is a full path filename to a file on the sd card
    // set attachment to NULL to not include an attachment
    bool send(const String& from, const String& to, const String& subject, const String& msg, const String& attachment);

    void close() {client->stop(); delete client;}
};

#endif

/* end of sendemail.h */

// Time
#include "time.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

// MicroSD
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

//
// EDIT ssid and password and other default values
//
// Look for zzz here and a couple times further down to edit default values
//
const char* ssid = "YourSSID";
const char* password = "TheSSIDPwd";
// edit email server info for the send part, recipient address is set through Settings in the app
const char* emailhost = "smtp.gmail.com";
const int emailport = 465;
const char* emailsendaddr = "YourEmail\@gmail.com";
const char* emailsendpwd = "YourEmailPwd";
char email[40] = "DefaultMotionDetectEmail\@hotmail.com";  // this can be changed through Settings in the app

const int SERVER_PORT = 80;  // port the the web server is listening on
            // the stream web server will be listening on this port + 10 

// don't change the below unless you are certain you know what you are doing
int capture_interval = 125; // microseconds between captures (set to 8 frames per second)
int total_frames = 480;     // 8 frames per second times 60 seconds
int recording = 0;          // turned off until start of setup
int framesize = 6;          // 10 uxga, 7 SVGA, 6 VGA, 5 CIF
int repeat = 0;             // no repeating videos at this time
int quality = 10;           // 10 on the 0..64 scale, lower number is better quality
int xspeed = 1;             // playback speed multiplier
int gray = 0;               // 1 grayscale, 0 color
int xlength = total_frames * capture_interval / 1000;
int new_config = 3;         // starts in VGA mode


const int MAX_AVI_Files_On_SD = 250;  // max number of avi files to keep
const int MAX_FILENAME_LENGTH = 40;
char avi_filenames[MAX_AVI_Files_On_SD][MAX_FILENAME_LENGTH] = {'\0'};  // array of avi file names on SD
int avi_file_count = 0;  // count of avi files on SD
int avi_newest_index = 0;  // points to newest filename in avi_filenames
int avi_oldest_index = 0;  // points to oldest filename in avi_filenames

const int MAX_JPG_Files_On_SD = 250;  // max number of jpg files to keep
const int MAX_JPG_FILENAME_LENGTH = 25;
char jpg_filenames[MAX_JPG_Files_On_SD][MAX_JPG_FILENAME_LENGTH] = {'\0'};  // array of jpg file names on SD
int jpg_file_count = 0;  // count of jpg files on SD
int jpg_newest_index = 0;  // points to newest filename in jpg_filenames
int jpg_oldest_index = 0;  // points to oldest filename in jpg_filenames

int motion = 0;  // 1 if currently working on a detected motion
int motionDetect = 0;  // > 10 enables motion detection
int triggerDetect = 0;  // > 10 enables trigger detection
int triggerH = 0;  // trigger hour
int triggerM = 0;  // trigger minute

// EEPROM setup for saving values over reboots
#include <EEPROM.h>
#define EEPROM_READY_ADDR 0x00
byte eeprom_ready[] = {0x52, 0x65, 0x61, 0x64, 0x79};
#define EEPROM_LENGTH_ADDR (EEPROM_READY_ADDR+sizeof(eeprom_ready))
#define EEPROM_EMAIL_ADDR (EEPROM_LENGTH_ADDR+sizeof(xlength))
#define EEPROM_MOTION_ADDR (EEPROM_EMAIL_ADDR+sizeof(email))
#define EEPROM_TRIGGER_ADDR (EEPROM_MOTION_ADDR+sizeof(motionDetect))
#define EEPROM_TRIGGERH_ADDR (EEPROM_TRIGGER_ADDR+sizeof(triggerDetect))
#define EEPROM_TRIGGERM_ADDR (EEPROM_TRIGGERH_ADDR+sizeof(triggerH))

gpio_num_t PIR_Pin = GPIO_NUM_3;  // use GPIO3 (U0RXD) for listening for PIR

unsigned long current_millis;
unsigned long last_capture_millis = 0;
static esp_err_t cam_err;
static esp_err_t card_err;
char strftime_buf[64];
int file_number = 0;
bool internet_connected = false;
struct tm timeinfo;
time_t now;
char boottime[30];

char *filename ;
char *stream ;
int newfile = 0;
int frames_so_far = 0;
unsigned long bp;
unsigned long ap;
unsigned long bw;
unsigned long aw;
unsigned long totalp;
unsigned long totalw;
float avgp;
float avgw;
int overtime_count = 0;

#define uS_TO_S_FACTOR 1000000LL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP5S  5         /* Time ESP32 will go to sleep (in seconds) */

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// GLOBALS
#define BUFFSIZE 512

// global variable used by these pieces

char str[20];
uint16_t n;
uint8_t buf[BUFFSIZE];

static int i = 0;
uint8_t temp = 0, temp_last = 0;
unsigned long fileposition = 0;
uint16_t frame_cnt = 0;
uint16_t remnant = 0;
uint32_t length = 0;
uint32_t startms;
uint32_t elapsedms;
uint32_t uVideoLen = 0;
bool is_header = false;
unsigned long bigdelta = 0;
int other_cpu_active = 0;
int skipping = 0;
int skipped = 0;

int fb_max = 12;

camera_fb_t * fb_q[30];
int fb_in = 0;
int fb_out = 0;

camera_fb_t * fb = NULL;
camera_fb_t * fbr = NULL;

FILE *avifile = NULL;
FILE *idxfile = NULL;
FILE *fbfile = NULL;
FILE *jpgfile = NULL;

#define AVIOFFSET 240 // AVI main header length

unsigned long movi_size = 0;
unsigned long jpeg_size = 0;
unsigned long idx_offset = 0;

uint8_t zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t   dc_buf[4] = {0x30, 0x30, 0x64, 0x63};    // "00dc"
uint8_t avi1_buf[4] = {0x41, 0x56, 0x49, 0x31};    // "AVI1"
uint8_t idx1_buf[4] = {0x69, 0x64, 0x78, 0x31};    // "idx1"

uint8_t  vga_w[2] = {0x80, 0x02}; // 640
uint8_t  vga_h[2] = {0xE0, 0x01}; // 480
uint8_t  cif_w[2] = {0x90, 0x01}; // 400
uint8_t  cif_h[2] = {0x28, 0x01}; // 296
uint8_t svga_w[2] = {0x20, 0x03}; // 800
uint8_t svga_h[2] = {0x58, 0x02}; // 600
uint8_t uxga_w[2] = {0x40, 0x06}; // 1600
uint8_t uxga_h[2] = {0xB0, 0x04}; // 1200


const int avi_header[AVIOFFSET] PROGMEM = {
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4E, 0x46, 0x4F,
  0x10, 0x00, 0x00, 0x00, 0x6A, 0x61, 0x6D, 0x65, 0x73, 0x7A, 0x61, 0x68, 0x61, 0x72, 0x79, 0x20,
  0x76, 0x33, 0x39, 0x20, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// AviWriterTask runs on cpu 1 to write the avi file
//

TaskHandle_t CameraTask, AviWriterTask;
SemaphoreHandle_t baton;
//int counter = 0;

void codeForAviWriterTask( void * parameter )
{

  print_stats("AviWriterTask runs on Core: ");

  for (;;) {
    make_avi();
    delay(1);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// CameraTask runs on cpu 0 to take pictures and drop them in a queue
//

void codeForCameraTask( void * parameter )
{

  print_stats("CameraTask runs on Core: ");

  for (;;) {

    if (other_cpu_active == 1 ) {
      current_millis = millis();
      if (current_millis - last_capture_millis > capture_interval) {

        last_capture_millis = millis();

        xSemaphoreTake( baton, portMAX_DELAY );

        if  ( ( (fb_in + fb_max - fb_out) % fb_max) + 1 == fb_max ) {
          xSemaphoreGive( baton );

          Serial.print(" S ");  // the queue is full
          skipped++;
          skipping = 1;

          //Serial.print(" Q: "); Serial.println( (fb_in + fb_max - fb_out) % fb_max );
          //Serial.print(fb_in); Serial.print(" / "); Serial.print(fb_out); Serial.print(" / "); Serial.println(fb_max);
          //delay(1);

        } 
        if (skipping > 0 ) {

          if (skipping % 2 == 0) {  // skip every other frame until queue is cleared

            frames_so_far = frames_so_far + 1;
            frame_cnt++;

            fb_in = (fb_in + 1) % fb_max;
            bp = millis();
            fb_q[fb_in] = esp_camera_fb_get();
            totalp = totalp - bp + millis();

          } else {
            //Serial.print(((fb_in + fb_max - fb_out) % fb_max));  Serial.print("-s ");  // skip an extra frame to empty the queue
            skipped++;
          }
          skipping = skipping + 1;
          if (((fb_in + fb_max - fb_out) % fb_max) == 0 ) {
            skipping = 0;
            Serial.print(" == ");
          }

          xSemaphoreGive( baton );

        } else {

          skipping = 0;
          frames_so_far = frames_so_far + 1;
          frame_cnt++;

          fb_in = (fb_in + 1) % fb_max;
          bp = millis();
          fb_q[fb_in] = esp_camera_fb_get();
          totalp = totalp - bp + millis();
          xSemaphoreGive( baton );

        }


      } else {
        //delay(5);     // waiting to take next picture
      }
    } else {
      //delay(50);  // big delay if not recording
    }
    delay(1);
  }
}


//
// Writes an uint32_t in Big Endian at current file position
//
static void inline print_quartet(unsigned long i, FILE * fd)
{
  uint8_t x[1];

  x[0] = i % 0x100;
  size_t i1_err = fwrite(x , 1, 1, fd);
  i = i >> 8;  x[0] = i % 0x100;
  size_t i2_err = fwrite(x , 1, 1, fd);
  i = i >> 8;  x[0] = i % 0x100;
  size_t i3_err = fwrite(x , 1, 1, fd);
  i = i >> 8;  x[0] = i % 0x100;
  size_t i4_err = fwrite(x , 1, 1, fd);
}


void startCameraServer();
httpd_handle_t camera_httpd = NULL;
void startStreamServer();
httpd_handle_t camera2_httpd = NULL;

//char the_page[3000];
//char the_page[50000];
char the_page[15000];

char localip[20];
WiFiEventId_t eventID;

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// setup() runs on cpu 1
//

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector  // creates other problems

  Serial.begin(115200);

  Serial.setDebugOutput(true);

  Serial.println("                                    ");
  Serial.println("-------------------------------------");
  Serial.println("ESP-CAM Video Recorder\n");
  Serial.println("-------------------------------------");

  print_stats("Begin setup Core: ");
 
  pinMode(33, OUTPUT);    // little red led on back of chip
  digitalWrite(33, LOW);  // turn on the red LED on the back of chip

  WiFi.persistent(false);
  eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.disconnected.reason);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("*** connected/disconnected issue!   WiFi disconnected ???...");
      WiFi.disconnect();
    } else {
      Serial.println("*** WiFi disconnected ???...");
    }
  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
    sprintf(localip, "%s", WiFi.localIP().toString().c_str());

    init_time();
    time(&now);

    // zzz  set timezone here
    //setenv("TZ", "GMT0BST,M3.5.0/01,M10.5.0/02", 1);
    //setenv("TZ", "MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00", 1);  // mountain time zone
    setenv("TZ", "PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00", 1);  // pacific time zone
    tzset();
    delay(1000);
    time(&now);
    localtime_r(&now, &timeinfo);

    Serial.print("After timezone : "); Serial.println(ctime(&now));
    strftime(boottime, sizeof(boottime), "%B %d %Y %H:%M:%S", &timeinfo);
  }

  baton = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    codeForCameraTask,
    "CameraTask",
    10000,
    NULL,
    1,
    &CameraTask,
    0);

  delay(1000);

  xTaskCreatePinnedToCore(
    codeForAviWriterTask,
    "AviWriterTask",
    10000,
    NULL,
    2,
    &AviWriterTask,
    1);

  delay(500);

  print_stats("After Task 1 Core: ");

  if (psramFound()) {
  } else {
    Serial.println("paraFound wrong - major fail");
    major_fail();
  }

  // SD card init
  card_err = init_sdcard();
  if (card_err != ESP_OK) {
    Serial.printf("SD Card init failed with error 0x%x", card_err);
    major_fail();
    return;
  }

  print_stats("After SD init Core: ");

  // delete all files with zero length
  deleteZeroLengthFiles();
  
  // count number of avi files on sd card
  avi_file_count = countAviFiles();

  // only keep MAX_AVI_Files_On_SD 
  if ( avi_file_count > MAX_AVI_Files_On_SD ) 
    Serial.println( "Number of AVI files on SD card is " + String(avi_file_count) );
  while ( avi_file_count > MAX_AVI_Files_On_SD ) {
    deleteOldestAVIFile();
    avi_file_count--;
  }
  Serial.println( "Number of AVI files on SD card is " + String(avi_file_count) + "\n" );

  // load sorted index of avi files since cant get sorted list from filesystem in real time 
  avi_newest_index = load_avi_filenames();
  avi_oldest_index = 0;

  // list first and last 5 for info on serial port
  if ( avi_file_count > 0 ) {
    if ( avi_newest_index < 10 ) {
      Serial.println( "AVI files on SD card:" );
      for (int i = 0; i < avi_newest_index+1; i++) {
        Serial.println( avi_filenames[i] );
      }    
    }
    else {
      Serial.println( "Oldest 5 AVI files on SD card:" );
      for (int i = 0; i < 5; i++) {
        Serial.println( avi_filenames[i] );
      }
      Serial.println( "Newest 5 AVI files on SD card:" );
      for (int i = avi_newest_index-4; i < avi_newest_index+1; i++) {
        Serial.println( avi_filenames[i] );
      }
    }
  }
  Serial.println( "" );
  
  // count number of jpg files on sd card
  jpg_file_count = countJpgFiles();

  // only keep MAX_JPG_Files_On_SD 
  if ( jpg_file_count > MAX_JPG_Files_On_SD ) 
    Serial.println( "Number of JPG files on SD card is " + String(jpg_file_count) );
  while ( jpg_file_count > MAX_JPG_Files_On_SD ) {
    deleteOldestJPGFile();
    jpg_file_count--;
  }
  Serial.println( "Number of JPG files on SD card is " + String(jpg_file_count) + "\n" );

  // load sorted index of jpg files since cant get sorted list from filesystem in real time 
  jpg_newest_index = load_jpg_filenames();
  jpg_oldest_index = 0;

  // list first and last 5 for info on serial port
  if ( jpg_file_count > 0 ) {
    if ( jpg_newest_index < 10 ) {
      Serial.println( "JPG files on SD card:" );
      for (int i = 0; i < jpg_newest_index+1; i++) {
        Serial.println( jpg_filenames[i] );
      }    
    }
    else {
      Serial.println( "Oldest 5 JPG files on SD card:" );
      for (int i = 0; i < 5; i++) {
        Serial.println( jpg_filenames[i] );
      }
      Serial.println( "Newest 5 JPG files on SD card:" );
      for (int i = jpg_newest_index-4; i < jpg_newest_index+1; i++) {
        Serial.println( jpg_filenames[i] );
      }
    }
  }
  Serial.println( "" );
  

  // start web servers
  startCameraServer();
  startStreamServer();

  print_stats("After Server init Core: ");

  newfile = 0;    // no file is open  // don't fiddle with this!
  recording = 0;  // we are NOT recording

  // temporary grab memory to keep camera from taking too much
  static char * memtmp = (char *) malloc(32767);
  
  config_camera();

  // give the memory back
  free(memtmp);
  memtmp = NULL;
  
  // load saved values from EEPROM
  EEPROM.begin(EEPROM_TRIGGERM_ADDR+sizeof(triggerM));
  int eeprom_init = 0; // 0 is ready, 1 is not ready
  for(int b = EEPROM_READY_ADDR; b < EEPROM_READY_ADDR + sizeof(eeprom_ready); b++) {
    // EEPROM = Init Code?
    if(EEPROM.read(b) != eeprom_ready[b]) {
      // EEPROM not ready, set Ready
      EEPROM.write(b, eeprom_ready[b]);
      // set eeprom_init flag to 1
      eeprom_init = 1;
    }
  }
  if( eeprom_init == 0 ) { // EEPROM is ready, load values from it
    Serial.println("Loading initial values from EEPROM");
    EEPROM.get(EEPROM_LENGTH_ADDR, xlength);
    Serial.print( "Recording length set to " ); Serial.println( xlength );
    total_frames = xlength * 8; // because the frame interval is 125ms
    EEPROM.get(EEPROM_EMAIL_ADDR, email);
    Serial.print( "Email address set to " ); Serial.println( email );
    EEPROM.get(EEPROM_MOTION_ADDR, motionDetect);
    Serial.print( "Motion detect set to " ); Serial.println( motionDetect );
    EEPROM.get(EEPROM_TRIGGER_ADDR, triggerDetect);
    Serial.print( "Trigger detect set to " ); Serial.println( triggerDetect );
    EEPROM.get(EEPROM_TRIGGERH_ADDR, triggerH);
    Serial.print( "Trigger hour set to " ); Serial.println( triggerH );
    EEPROM.get(EEPROM_TRIGGERM_ADDR, triggerM);
    Serial.print( "Trigger minute set to " ); Serial.println( triggerM );
  } else { // EEPROM not ready, save default values
    Serial.println("First run after flash, saving default values in EEPROM");
    EEPROM.put(EEPROM_LENGTH_ADDR, xlength);
    EEPROM.put(EEPROM_EMAIL_ADDR, email);
    EEPROM.put(EEPROM_MOTION_ADDR, motionDetect);
    EEPROM.put(EEPROM_TRIGGER_ADDR, triggerDetect);
    EEPROM.put(EEPROM_TRIGGERH_ADDR, triggerH);
    EEPROM.put(EEPROM_TRIGGERM_ADDR, triggerM);
    EEPROM.commit();
  }
  
  //recording = 1;  // we are recording

  //Serial.println("GPIO 3 setup for listening to PIR");
  Serial.println( "GPIO " + String(PIR_Pin) + " setup for listening to PIR");
  pinMode(PIR_Pin, INPUT_PULLUP);   // enable pin as an INPUT pin, could also set as OUTPUT, INPUT_PULLUP
  // note: in this build high on this pin indicates no motion, low indicates motion

  print_stats("End of setup on Core: ");

  digitalWrite(33, HIGH);  // turn off the red LED on the back of chip

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// print_stats to keep track of memory during debugging
//

void print_stats(char *the_message) {

  Serial.print(the_message);  Serial.println(xPortGetCoreID());
  Serial.print(" Free Heap: "); Serial.print(ESP.getFreeHeap());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  printf(" Himem is %dKiB, Himem free %dKiB, ", (int)ESP.getPsramSize() / 1024, (int)ESP.getFreePsram() / 1024);
  printf("Flash is %dKiB, Sketch is %dKiB \n", (int)ESP.getFlashChipSize() / 1024, (int)ESP.getSketchSize() / 1024);

  Serial.print(" Write Q: "); Serial.print((fb_in + fb_max - fb_out) % fb_max); Serial.print(" in/out  "); Serial.print(fb_in); Serial.print(" / "); Serial.println(fb_out);
  Serial.println(" ");
}

//
// if we have no camera, or sd card, then flash rear led on and off to warn the human SOS - SOS
//
void major_fail() {

  for  (int i = 0;  i < 5; i++) {
    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);
    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);
    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);

    delay(150);

    digitalWrite(33, LOW);  delay(500);
    digitalWrite(33, HIGH); delay(150);
    digitalWrite(33, LOW);  delay(500);
    digitalWrite(33, HIGH); delay(150);
    digitalWrite(33, LOW);  delay(500);
    digitalWrite(33, HIGH); delay(150);

    delay(150);

    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);
    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);
    digitalWrite(33, LOW);   delay(150);
    digitalWrite(33, HIGH);  delay(150);

    delay(450);
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP5S * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

  //ESP.restart();        

}


// load avi_filenames array and sort it with oldest at index zero, return avi_newest_index
int load_avi_filenames(){

  int index = 0;
  
  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".avi" ) ) {
        //avi_filenames[index++] = String(file.name());
        strcpy(avi_filenames[index++], file.name());
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }

  // sort avi_filenames
  if ( index > 0 ) {
    // sort avi_filenames
    char tmp[50] = "";
    for (size_t i = 1; i < index; i++) {
      for (size_t j = i; j > 0 && ((String) avi_filenames[j-1] > (String) avi_filenames[j]); j--) {
        strcpy(tmp,avi_filenames[j-1]);
        strcpy(avi_filenames[j-1],avi_filenames[j]);
        strcpy(avi_filenames[j],tmp);
      }
    }

    index--;
  }
  return index;
}

// delete zero length files
void deleteZeroLengthFiles(){

  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".avi" ) && (file.size() == 0) ) {
        SD_MMC.remove( file.name() );
        Serial.print("Deleted zero length file ");
        Serial.println( file.name() );
      }
      if ( EndsWithTail( file.name(), ".jpg" ) && (file.size() == 0) ) {
        SD_MMC.remove( file.name() );
        Serial.print("Deleted zero length file ");
        Serial.println( file.name() );
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }
  Serial.println("");
}

// Count Avi Files in root directory
int countAviFiles(){

  int AviFileCount = 0;

  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".avi" ) ) {
        AviFileCount++;
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }
  return AviFileCount;
}

// return true if end of url matches tail
int EndsWithTail(const char *url, char* tail)
{
    if (strlen(tail) > strlen(url))
        return 0;

    int len = strlen(url);

    if (strcmp(&url[len-strlen(tail)],tail) == 0)
        return 1;
    return 0;
}

void deleteOldestAVIFile(){
  
  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();
  String oldestFilename = "";

  String oldestModified = "99999999999999";
  String lastModified;

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".avi" ) ) {
        char ft[21];
        time_t ftime = file.getLastWrite();
        strftime(ft, sizeof(ft), "%Y%m%d%H%M%S", gmtime(&ftime));
        lastModified = String(ft);
        if ( lastModified < oldestModified ) {
          oldestModified = lastModified;
          oldestFilename = file.name();
        }
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }
  if ( oldestFilename.length() > 0 ) {
    SD_MMC.remove( oldestFilename );
    Serial.println( "Deleting old file " + oldestFilename );
  }
}


// load jpg_filenames array and sort it with oldest at index zero, return avi_newest_index
int load_jpg_filenames(){

  int index = 0;
  
  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".jpg" ) ) {
        //jpg_filenames[index++] = String(file.name());
        strcpy(jpg_filenames[index++], file.name());
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }

  // sort jpg_filenames
  if ( index > 0 ) {
    // sort jpg_filenames
    char tmp[50] = "";
    for (size_t i = 1; i < index; i++) {
      for (size_t j = i; j > 0 && ((String) jpg_filenames[j-1] > (String) jpg_filenames[j]); j--) {
        strcpy(tmp,jpg_filenames[j-1]);
        strcpy(jpg_filenames[j-1],jpg_filenames[j]);
        strcpy(jpg_filenames[j],tmp);
      }
    }

    index--;
  }
  return index;
}

// Count JPG Files in root directory
int countJpgFiles(){

  int jpgFileCount = 0;

  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".jpg" ) ) {
        jpgFileCount++;
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }
  return jpgFileCount;
}

void deleteOldestJPGFile(){
  
  File dir = SD_MMC.open( "/" );
  File file = dir.openNextFile();
  String oldestFilename = "";

  String oldestModified = "99999999999999";
  String lastModified;

  while( file == 1) {
    // Skip directories
    if (!file.isDirectory()) {
      if ( EndsWithTail( file.name(), ".jpg" ) ) {
        char ft[21];
        time_t ftime = file.getLastWrite();
        strftime(ft, sizeof(ft), "%Y%m%d%H%M%S", gmtime(&ftime));
        lastModified = String(ft);
        if ( lastModified < oldestModified ) {
          oldestModified = lastModified;
          oldestFilename = file.name();
        }
      }
    }
    file = dir.openNextFile();
    if (file < 1 ) {
      break;
    }
  }
  if ( oldestFilename.length() > 0 ) {
    SD_MMC.remove( oldestFilename );
    Serial.println( "Deleting old file " + oldestFilename );
  }
}


bool init_wifi()
{
  int connAttempts = 0;
  
    // this is the fixed ip stuff 
    // zzz  set static IP address here
    // 
    IPAddress local_IP(192, 168, 2, 33);

    // Set your Gateway IP address
    IPAddress gateway(192, 168, 2, 1);

    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8); // optional
    IPAddress secondaryDNS(8, 8, 4, 4); // optional

    WiFi.mode(WIFI_STA);

    WiFi.setHostname("ESP32CAM33");  // does not seem to do anything with my wifi router ???

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
    major_fail();
    }

    WiFi.printDiag(Serial);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10) {
      Serial.println("Cannot connect");
      WiFi.printDiag(Serial);
      major_fail();
      return false;
    }
    connAttempts++;
    }
    return true;

}

void init_time()
{

  do_time();

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_setservername(1, "time.windows.com");
  sntp_setservername(2, "time.nist.gov");

  sntp_init();

  // wait for time to be set
  time_t now = 0;
  timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 10;
  while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
    Serial.printf("Waiting for system time to be set... (%d/%d) -- %d\n", retry, retry_count, timeinfo.tm_year);
    delay(2000);
    time(&now);
    localtime_r(&now, &timeinfo);
    Serial.println(ctime(&now));
  }

  if (timeinfo.tm_year < (2016 - 1900)) {
    major_fail();
  }
}

static esp_err_t init_sdcard()
{
  esp_err_t ret = ESP_FAIL;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    //.max_files = 10,
    .max_files = 6,
  };
  sdmmc_card_t *card;

  Serial.println("Mounting SD card...");
  ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret == ESP_OK) {
    Serial.println("SD card mount successfully!");
  }  else  {
    Serial.printf("Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
    major_fail();
  }

  Serial.print("SD_MMC Begin: "); Serial.println(SD_MMC.begin());
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Make the avi movie in 4 pieces
//
// make_avi() called in every loop, which calls below, depending on conditions
//   start_avi() - open the file and write headers
//   another_pic_avi() - write one more frame of movie
//   end_avi() - write the final parameters and close the file



void make_avi() {

  // we are recording, but no file is open

  if (newfile == 0 && recording == 1) {                                     // open the file

    digitalWrite(33, HIGH);
    newfile = 1;
    start_avi();

  } else {

    // we have a file open, but not recording

    if (newfile == 1 && recording == 0) {                                  // got command to close file

      digitalWrite(33, LOW);
      end_avi();

      Serial.println("Done capture due to command");

      frames_so_far = total_frames;

      newfile = 0;    // file is closed
      recording = 0;  // DO NOT start another recording

    } else {

      if (newfile == 1 && recording == 1) {                            // regular recording

        if (frames_so_far >= total_frames)  {                                // we are done the recording

          Serial.println("Done capture for total frames!");

          digitalWrite(33, LOW);                                                       // close the file
          end_avi();

          frames_so_far = 0;
          newfile = 0;          // file is closed

          if (repeat > 0) {
            recording = 1;        // start another recording
            repeat = repeat - 1;
          } else {
            recording = 0;
          }

        } else if ((millis() - startms) > (total_frames * capture_interval)) {

          Serial.println (" "); Serial.println("Done capture for time");
          Serial.print("Time Elapsed: "); Serial.print(millis() - startms); Serial.print(" Frames: "); Serial.println(frame_cnt);
          Serial.print("Config:       "); Serial.print(total_frames * capture_interval ) ; Serial.print(" (");
          Serial.print(total_frames); Serial.print(" x "); Serial.print(capture_interval);  Serial.println(")");

          digitalWrite(33, LOW);                                                       // close the file

          end_avi();

          frames_so_far = 0;
          newfile = 0;          // file is closed
          if (repeat > 0) {
            recording = 1;        // start another recording
            repeat = repeat - 1;
          } else {
            recording = 0;
          }

        } else  {                                                            // regular

          another_save_avi();

        }
      }
    }
  }
}

static esp_err_t config_camera() {

  camera_config_t config;

  Serial.println("config camera");

  if (new_config > 2) {

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (new_config == 3) {

      config.frame_size = FRAMESIZE_VGA;
      fb_max = 20;  // from 12
      new_config = 1;
    } else {
      config.frame_size = FRAMESIZE_UXGA;
      fb_max = 4;
      new_config = 2;
    }

    config.jpeg_quality = 10;
    //config.jpeg_quality = quality;
    config.fb_count = fb_max;

    print_stats("Before deinit() runs on Core: ");

    esp_camera_deinit();

    print_stats("After deinit() runs on Core: ");

    // camera init
    cam_err = esp_camera_init(&config);
    if (cam_err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", cam_err);
      major_fail();
    }

    print_stats("After the new init runs on Core: ");

    delay(500);
  }

  sensor_t * ss = esp_camera_sensor_get();
  ss->set_quality(ss, quality);
  ss->set_framesize(ss, (framesize_t)framesize);
  if (gray == 1) {
    ss->set_special_effect(ss, 2);  // 0 regular, 2 grayscale
  } else {
    ss->set_special_effect(ss, 0);  // 0 regular, 2 grayscale
  }

  //Serial.println("after the sensor stuff");

  for (int j = 0; j < 5; j++) {
    do_fb();  // start the camera ... warm it up
    delay(20);
  }

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// start_avi - open the files and write in headers
//

static esp_err_t start_avi() {

  Serial.println("Starting an avi ");

  //config_camera();
  for (int j = 0; j < 5; j++) {
    do_fb();  // start the camera ... warm it up
    delay(20);
  }

  time(&now);
  localtime_r(&now, &timeinfo);

  strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

  char fname[100];

  if (framesize == 6) {
    sprintf(fname, "/sdcard/%s_VGA_L0_M0.avi", strftime_buf);
    //sprintf(fname, "/sdcard/%s_vga_Q%d_I%d_L%d_S%d.avi", strftime_buf, quality, capture_interval, xlength, xspeed);
  } else if (framesize == 7) {
    sprintf(fname, "/sdcard/%s_svga_Q%d_I%d_L%d_S%d.avi", strftime_buf, quality, capture_interval, xlength, xspeed);
  } else if (framesize == 10) {
    sprintf(fname, "/sdcard/%s_uxga_Q%d_I%d_L%d_S%d.avi", strftime_buf, quality, capture_interval, xlength, xspeed);
  } else  if (framesize == 5) {
    sprintf(fname, "/sdcard/%s_cif_Q%d_I%d_L%d_S%d.avi", strftime_buf, quality, capture_interval, xlength, xspeed);
  } else {
    Serial.println("Wrong framesize");
    sprintf(fname, "/sdcard/%s_xxx_Q%d_I%d_L%d_S%d.avi", strftime_buf, quality, capture_interval, xlength, xspeed);
  }

  Serial.print("\nFile name will be (size added at end)>"); Serial.print(fname); Serial.println("<");

  avifile = fopen(fname, "w");
  idxfile = fopen("/sdcard/idx.tmp", "w"); // tmp file for creating index of frame positions in file

  if (avifile != NULL)  {

    //Serial.printf("File open: %s\n", fname);

  }  else  {
    Serial.println("Could not open file");
    major_fail();
  }

  if (idxfile != NULL)  {

    //Serial.printf("File open: %s\n", "/sdcard/idx.tmp");

  }  else  {
    Serial.println("Could not open file");
    major_fail();
  }

  // only keep MAX_AVI_Files_On_SD 
  avi_file_count++;
  if ( avi_file_count > MAX_AVI_Files_On_SD ) {
    // delete oldest file
    if( SD_MMC.exists( avi_filenames[avi_oldest_index] )) {
      SD_MMC.remove( avi_filenames[avi_oldest_index] );
    }
    avi_file_count--;
    strcpy(avi_filenames[avi_oldest_index], "");
    avi_oldest_index++;
    if (avi_oldest_index == MAX_AVI_Files_On_SD)
      avi_oldest_index = 0;
  }
  Serial.println( "Number of AVI files on SD card is " + String(avi_file_count) + "\n" );
  if ( avi_file_count > 1 )
    avi_newest_index++;
  if (avi_newest_index == MAX_AVI_Files_On_SD)
    avi_newest_index = 0;
  strcpy(avi_filenames[avi_newest_index],fname+7);
  //for (int i = 0; i < MAX_AVI_Files_On_SD; i++) {
  //  Serial.print( "index " + String(i) + " filename is " );
  //  Serial.println( avi_filenames[i] );
  //}


  for ( i = 0; i < AVIOFFSET; i++)
  {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }

  size_t err = fwrite(buf, 1, AVIOFFSET, avifile);

  if (framesize == 6) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(vga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(vga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(vga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(vga_h, 1, 2, avifile);

  } else if (framesize == 10) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(uxga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(uxga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(uxga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(uxga_h, 1, 2, avifile);

  } else if (framesize == 7) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(svga_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(svga_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(svga_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(svga_h, 1, 2, avifile);

  }  else if (framesize == 5) {

    fseek(avifile, 0x40, SEEK_SET);
    err = fwrite(cif_w, 1, 2, avifile);
    fseek(avifile, 0xA8, SEEK_SET);
    err = fwrite(cif_w, 1, 2, avifile);
    fseek(avifile, 0x44, SEEK_SET);
    err = fwrite(cif_h, 1, 2, avifile);
    fseek(avifile, 0xAC, SEEK_SET);
    err = fwrite(cif_h, 1, 2, avifile);
  }

  fseek(avifile, AVIOFFSET, SEEK_SET);

  Serial.print(F("\nRecording "));
  Serial.print(total_frames);
  Serial.println(F(" video frames ...\n"));

  startms = millis();
  bigdelta = millis();
  totalp = 0;
  totalw = 0;
  overtime_count = 0;
  jpeg_size = 0;
  movi_size = 0;
  uVideoLen = 0;
  idx_offset = 4;


  frame_cnt = 0;
  frames_so_far = 0;

  skipping = 0;
  skipped = 0;

  newfile = 1;

  other_cpu_active = 1;

} // end of start avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  another_save_avi runs on cpu 1, saves another frame to the avi file
//
//  the "baton" semaphore makes sure that only one cpu is using the camera subsystem at a time
//

static esp_err_t another_save_avi() {

  xSemaphoreTake( baton, portMAX_DELAY );

  if (fb_in == fb_out) {        // nothing to do

    xSemaphoreGive( baton );

  } else {

    //if ( (fb_in + fb_max - fb_out) % fb_max > 3) {  // more than 1 in queue ??
    //Serial.print(millis()); Serial.print(" Write Q: "); Serial.print((fb_in + fb_max - fb_out) % fb_max); Serial.print(" in/out  "); Serial.print(fb_in); Serial.print(" / "); Serial.println(fb_out);
    //}

    fb_out = (fb_out + 1) % fb_max;

    int fblen;
    fblen = fb_q[fb_out]->len;

    //xSemaphoreGive( baton );

    digitalWrite(33, LOW);

    jpeg_size = fblen;
    movi_size += jpeg_size;
    uVideoLen += jpeg_size;

    bw = millis();
    size_t dc_err = fwrite(dc_buf, 1, 4, avifile);
    size_t ze_err = fwrite(zero_buf, 1, 4, avifile);

    //bw = millis();
    size_t err = fwrite(fb_q[fb_out]->buf, 1, fb_q[fb_out]->len, avifile);
    if (err == 0 ) {
      Serial.println("Error on avi write");
      major_fail();
    }

    //xSemaphoreTake( baton, portMAX_DELAY );
    esp_camera_fb_return(fb_q[fb_out]);     // release that buffer back to the camera system
    xSemaphoreGive( baton );

    remnant = (4 - (jpeg_size & 0x00000003)) & 0x00000003;

    print_quartet(idx_offset, idxfile);
    print_quartet(jpeg_size, idxfile);

    idx_offset = idx_offset + jpeg_size + remnant + 8;

    jpeg_size = jpeg_size + remnant;
    movi_size = movi_size + remnant;
    if (remnant > 0) {
      size_t rem_err = fwrite(zero_buf, 1, remnant, avifile);
    }

    fileposition = ftell (avifile);       // Here, we are at end of chunk (after padding)
    fseek(avifile, fileposition - jpeg_size - 4, SEEK_SET);    // Here we are at the 4-bytes blank placeholder

    print_quartet(jpeg_size, avifile);    // Overwrite placeholder with actual frame size (without padding)

    fileposition = ftell (avifile);

    fseek(avifile, fileposition + 6, SEEK_SET);    // Here is the FOURCC "JFIF" (JPEG header)
    // Overwrite "JFIF" (still images) with more appropriate "AVI1"

    size_t av_err = fwrite(avi1_buf, 1, 4, avifile);

    fileposition = ftell (avifile);
    fseek(avifile, fileposition + jpeg_size - 10 , SEEK_SET);  // position at end of file for next frame
    //Serial.println("Write done");
    totalw = totalw + millis() - bw;

    //if (((fb_in + fb_max - fb_out) % fb_max) > 0 ) {
    //  Serial.print(((fb_in + fb_max - fb_out) % fb_max)); Serial.print(" ");
    //}

    digitalWrite(33, HIGH);
  }

} // end of another_pic_avi

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  end_avi runs on cpu 1, empties the queue of frames, writes the index, and closes the files
//

static esp_err_t end_avi() {

  unsigned long current_end = 0;

  other_cpu_active = 0 ;

  Serial.print(" Write Q: "); Serial.print((fb_in + fb_max - fb_out) % fb_max); Serial.print(" in/out  "); Serial.print(fb_in); Serial.print(" / "); Serial.println(fb_out);

  for (int i = 0; i < fb_max; i++) {           // clear the queue
    another_save_avi();
  }

  Serial.print(" Write Q: "); Serial.print((fb_in + fb_max - fb_out) % fb_max); Serial.print(" in/out  "); Serial.print(fb_in); Serial.print(" / "); Serial.println(fb_out);

  current_end = ftell (avifile);

  Serial.println("End of avi - closing the files");

  elapsedms = millis() - startms;
  float fRealFPS = (1000.0f * (float)frame_cnt) / ((float)elapsedms) * xspeed;
  float fmicroseconds_per_frame = 1000000.0f / fRealFPS;
  uint8_t iAttainedFPS = round(fRealFPS);
  uint32_t us_per_frame = round(fmicroseconds_per_frame);


  //Modify the MJPEG header from the beginning of the file, overwriting various placeholders

  fseek(avifile, 4 , SEEK_SET);
  print_quartet(movi_size + 240 + 16 * frame_cnt + 8 * frame_cnt, avifile);

  fseek(avifile, 0x20 , SEEK_SET);
  print_quartet(us_per_frame, avifile);

  unsigned long max_bytes_per_sec = movi_size * iAttainedFPS / frame_cnt;

  fseek(avifile, 0x24 , SEEK_SET);
  print_quartet(max_bytes_per_sec, avifile);

  fseek(avifile, 0x30 , SEEK_SET);
  print_quartet(frame_cnt, avifile);

  fseek(avifile, 0x8c , SEEK_SET);
  print_quartet(frame_cnt, avifile);

  fseek(avifile, 0x84 , SEEK_SET);
  print_quartet((int)iAttainedFPS, avifile);

  fseek(avifile, 0xe8 , SEEK_SET);
  print_quartet(movi_size + frame_cnt * 8 + 4, avifile);

  Serial.println(F("\n*** Video recorded and saved ***\n"));
  Serial.print(F("Recorded "));
  Serial.print(elapsedms / 1000);
  Serial.print(F("s in "));
  Serial.print(frame_cnt);
  Serial.print(F(" frames\nFile size is "));
  Serial.print(movi_size + 12 * frame_cnt + 4);
  Serial.print(F(" bytes\nActual FPS is "));
  Serial.print(fRealFPS, 2);
  Serial.print(F("\nMax data rate is "));
  Serial.print(max_bytes_per_sec);
  Serial.print(F(" byte/s\nFrame duration is "));  Serial.print(us_per_frame);  Serial.println(F(" us"));
  Serial.print(F("Average frame length is "));  Serial.print(uVideoLen / frame_cnt);  Serial.println(F(" bytes"));
  Serial.print("Average picture time (ms) "); Serial.println( totalp / frame_cnt );
  Serial.print("Average write time (ms) "); Serial.println( totalw / frame_cnt );
  Serial.print("Frames Skipped % ");  Serial.println( 100.0 * skipped / frame_cnt, 1 );

  Serial.println("Writing the index");

  fseek(avifile, current_end, SEEK_SET);

  fclose(idxfile);

  size_t i1_err = fwrite(idx1_buf, 1, 4, avifile);

  print_quartet(frame_cnt * 16, avifile);

  idxfile = fopen("/sdcard/idx.tmp", "r");

  if (idxfile != NULL)  {

    //Serial.printf("File open: %s\n", "/sdcard/idx.tmp");

  }  else  {
    Serial.println("Could not open file");
    //major_fail();
  }

  char * AteBytes;
  AteBytes = (char*) malloc (8);

  // save index of frame positions in file
  for (int i = 0; i < frame_cnt; i++) {

    size_t res = fread ( AteBytes, 1, 8, idxfile);
    size_t i1_err = fwrite(dc_buf, 1, 4, avifile);
    size_t i2_err = fwrite(zero_buf, 1, 4, avifile);
    size_t i3_err = fwrite(AteBytes, 1, 8, avifile);

  }

  free(AteBytes);

  fclose(idxfile); // tmp index file can be discarded after its closed
  SD_MMC.remove( "/idx.tmp" );

  fclose(avifile);

  // add size to end of just closed file name
  char oldname[100];
  strcpy( oldname, avi_filenames[avi_newest_index]);
  
  File f = SD_MMC.open( oldname, "r" );
  int avisz = (f.size() / 1000000) + 1;
  if ( f.size() == 0 ) { avisz = 0; }
  f.close();

  oldname[strlen(oldname)-8] = '\0';
  
  char newname[100];
  sprintf(newname, "%s%i_M%i.avi", oldname, elapsedms / 1000, avisz);   

  SD_MMC.rename( avi_filenames[avi_newest_index], newname ); 
  strcpy( avi_filenames[avi_newest_index], newname );
  
  Serial.print("\nFinal file name is> "); Serial.print(newname); Serial.println(" <");
  // the M in the filename now indicates size in Megabytes

  Serial.println("---");
  //WiFi.printDiag(Serial);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  do_fb - just takes a picture and discards it
//

static esp_err_t do_fb() {
  xSemaphoreTake( baton, portMAX_DELAY );
  camera_fb_t * fb = esp_camera_fb_get();

  Serial.print("Pic, len="); Serial.println(fb->len);

  esp_camera_fb_return(fb);
  xSemaphoreGive( baton );
}

void do_time() {

  int numberOfNetworks = WiFi.scanNetworks();

  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void process_Detection(int detection_type) {
  // detection_type 0 is motion detection, detection_type 1 is time detection
  int dAction;
  String dMsg;
  if ( detection_type == 0 ) {
    dAction = motionDetect;
    dMsg = "Motion detected";
  } else {
    dAction = triggerDetect;
    dMsg = "Time trigger";
  }
  Serial.print( dMsg ); Serial.print(" and action will be "); Serial.println( dAction );

  // get a camera frame buffer and make sure it has something to see
  camera_fb_t * fb = NULL;
  xSemaphoreTake( baton, portMAX_DELAY );
  fb = esp_camera_fb_get();

  if (!fb) {
    xSemaphoreGive( baton );
    Serial.println("Failed to get frame from camera");
    major_fail();
  }
  if ( fb->len < 12000 ) { // dont save files smaller than 10K, they are usually blank
    esp_camera_fb_return(fb);
    xSemaphoreGive( baton );
    Serial.println("Skipping camera event action, camera buffer has nothing interesting to show");
  } else {
    if (  dAction == 10 || dAction == 11 ) { // take a picture
      // build name for new jpg file
      time(&now);
      localtime_r(&now, &timeinfo);
 
      strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

      char fname[100];

      sprintf(fname, "/sdcard/%s.jpg", strftime_buf);
      Serial.print("\nFile name is > "); Serial.print(fname); Serial.println(" <");

      // open new jpg file for write
      jpgfile = fopen(fname, "w");
      if (jpgfile != NULL)  {
        //Serial.printf("File open: %s\n", fname);
      }  else  {
        Serial.println("Could not open file");
        major_fail();
      }

      // only keep MAX_JPG_Files_On_SD 
      jpg_file_count++;
      if ( jpg_file_count > MAX_JPG_Files_On_SD ) {
        // delete oldest file
        if( SD_MMC.exists( jpg_filenames[jpg_oldest_index] )) {
          SD_MMC.remove( jpg_filenames[jpg_oldest_index] );
        }
        jpg_file_count--;
        strcpy(jpg_filenames[jpg_oldest_index], "");
        jpg_oldest_index++;
        if (jpg_oldest_index == MAX_JPG_Files_On_SD)
          jpg_oldest_index = 0;
      }
      Serial.println( "Number of JPG files on SD card is " + String(jpg_file_count) + "\n" );
      if ( jpg_file_count > 1 )
        jpg_newest_index++;
      if ( jpg_newest_index == MAX_JPG_Files_On_SD )
        jpg_newest_index = 0;
      strcpy(jpg_filenames[jpg_newest_index],fname+7);

      // save the frame to the jpg file
      size_t err = fwrite(fb->buf, 1, fb->len, jpgfile);
      esp_camera_fb_return(fb);
      xSemaphoreGive( baton );
      fclose(jpgfile);
      if (err == 0 ) {
        Serial.println("Error on jpg write");
        major_fail();
      }
      Serial.print( dMsg ); Serial.println(" picture taken");
      delay(3);

      if (  dAction == 11 ) {  // email the picture
        SendEmail e(emailhost, emailport, emailsendaddr, emailsendpwd, 5000, true); 
        // Send Email
        char send[40];
        sprintf(send,"\<%s\>", emailsendaddr);
        char recv[40];
        sprintf(recv,"\<%s\>", email);
        if ( e.send(send, recv, "Camera Event Detected", "Please see attachment", jpg_filenames[jpg_newest_index]) ) {
          Serial.print( dMsg ); Serial.println(" email sent with a picture");
        } else {
          Serial.print("Failed to send "); Serial.print( dMsg );
          Serial.println(" email with a picture");
        }
        e.close(); // close email client
      }
    }
    if (  dAction == 12 || dAction == 13 ) { // take a video
      // turn on recording (might already be recording)
      recording = 1;
      if ( dAction == 13 ) { // send an email with preliminary name of video
        delay(1000);
        // build name for new jpg file
        time(&now);
        localtime_r(&now, &timeinfo);
 
        strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

        char fname[100];

        sprintf(fname, "/sdcard/%s.jpg", strftime_buf);
        Serial.print("\nFile name is > "); Serial.print(fname); Serial.println(" <");

        // open new jpg file for write
        jpgfile = fopen(fname, "w");
        if (jpgfile != NULL)  {
          //Serial.printf("File open: %s\n", fname);
        }  else  {
          Serial.println("Could not open file");
          major_fail();
        }

        // only keep MAX_JPG_Files_On_SD 
        jpg_file_count++;
        if ( jpg_file_count > MAX_JPG_Files_On_SD ) {
          // delete oldest file
          if( SD_MMC.exists( jpg_filenames[jpg_oldest_index] )) {
            SD_MMC.remove( jpg_filenames[jpg_oldest_index] );
          }
          jpg_file_count--;
          strcpy(jpg_filenames[jpg_oldest_index], "");
          jpg_oldest_index++;
          if (jpg_oldest_index == MAX_JPG_Files_On_SD)
            jpg_oldest_index = 0;
        }
        Serial.println( "Number of JPG files on SD card is " + String(jpg_file_count) + "\n" );
        if ( jpg_file_count > 1 )
          jpg_newest_index++;
        if ( jpg_newest_index == MAX_JPG_Files_On_SD )
          jpg_newest_index = 0;
        strcpy(jpg_filenames[jpg_newest_index],fname+7);

        // save the frame to the jpg file
        size_t err = fwrite(fb->buf, 1, fb->len, jpgfile);
        esp_camera_fb_return(fb);
        xSemaphoreGive( baton );
        fclose(jpgfile);
        if (err == 0 ) {
          Serial.println("Error on jpg write");
          major_fail();
        }
        delay(5000);

        SendEmail e(emailhost, emailport, emailsendaddr, emailsendpwd, 5000, true); 
        // Send Email
        char send[40];
        sprintf(send,"\<%s\>", emailsendaddr);
        char recv[40];
        sprintf(recv,"\<%s\>", email);
        String msg = dMsg;
        msg += ". Check ESP32-CAM web site for a video ~named ";
        msg += avi_filenames[avi_newest_index]+1;
        if ( e.send(send, recv, "Camera Event Detected", msg, jpg_filenames[jpg_newest_index] ) ) {
          Serial.print( dMsg ); Serial.println(" email sent with video name");
        } else {
          Serial.print("Failed to send ");
          Serial.print( dMsg ); Serial.println(" email with video name");
        }
        e.close(); // close email client
      } else {
        esp_camera_fb_return(fb);
        xSemaphoreGive( baton );
      }
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_index() {

  Serial.print("do_index ");

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM Video Recorder</title>
<style> 
#image-container {
  width: 640px;
  height: 480px;
  background-color: powderblue;
}
a {color: blue;}
a:visited {color: blue;}
a:hover {color: blue;}
a:active {color: blue;}
</style>
<script>
  
  var recording = %i;
  document.addEventListener('DOMContentLoaded', function() {
    var h = document.location.hostname;
    var p = document.location.port;
    if ( p == null || p == 0 )
      p = 80;
    p = parseInt(p,10) + 10;
    document.getElementById('image-container').innerHTML = '<img src="http://'+`${h}:${p}/stream?_cb=${Date.now()}`+'">';
    setTimeout(function(){ window.scrollTo(0,0); }, 2000);
  });

  var XHR = function() {
    this.get = function(aUrl, aCallback) {
      var anXHR = new XMLHttpRequest();
      anXHR.onreadystatechange = function() { 
        if (anXHR.readyState == 4 && anXHR.status == 200)
          aCallback(anXHR.responseText);
      }
      anXHR.open( "GET", aUrl, true );            
      anXHR.send( null );
    }
  }

  function do_recording() {
    var c = document.location.origin;
    
    // toggle recording
    if ( recording == 0 ) {
      recording = 1;
      document.getElementById('rec').innerHTML = "Stop Recording";
    } else {
      recording = 0;
      document.getElementById('rec').innerHTML = "Start Recording";
    }

    var client = new XHR();
    client.get(`${c}/record?action=${recording}&_cb=${Date.now()}`, function(response) {
      // do something with response
      alert( response );
    });
  }

  function do_picture() {
    var c = document.location.origin;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', `${c}/picture?_cb=${Date.now()}`, true);
    xhr.overrideMimeType('text/plain; charset=x-user-defined');
    xhr.onloadend = function(){
      var binary = "";
      for(i=0;i<xhr.response.length;i++){
        binary += String.fromCharCode(xhr.response.charCodeAt(i) & 0xff);
      }
      document.getElementById('pic-container').innerHTML = '<img id="pic" onclick="close_pic()" src="">';
      document.getElementById('pic').src = 'data:image/jpeg;base64,' + btoa(binary);
      
      var cd = xhr.getResponseHeader('Content-Disposition');
      var startIndex = cd.indexOf("filename=") + 9; 
      var endIndex = cd.length; 
      var fname = cd.substring(startIndex, endIndex);
      document.getElementById('pic-title').innerHTML = fname;
    }
    xhr.send(null);  
  }

  function close_pic() {
    document.getElementById('pic-container').innerHTML = '';
    document.getElementById('pic-title').innerHTML = '';
    window.scrollTo(0,0);
  }

  // force reload when going back to page
  if(!!window.performance && window.performance.navigation.type == 2) {
    window.location.reload();
  }
  
</script>
</head>
<body><center>
<h1>ESP32-CAM Video Recorder</h1>
The stream below shows one frame every two seconds when recording<br><br>
<div id="image-container"></div>
<br><table><tr>
<td><a id="rec" href="javascript:do_recording();">%s</a></td>
<td>&nbsp&nbsp</td> 
<td><a href="javascript:do_picture()">Take Picture</a></td> 
<td>&nbsp&nbsp</td> 
<td><a href="/savedavi">Access Saved Recordings</a></td> 
<td>&nbsp&nbsp</td>
<td><a href="/savedjpg">Access Saved Pictures</a></td> 
<td>&nbsp&nbsp</td>
<td><a href="/settings">Settings</a></td>
</tr></table><br>
<div id="pic-container"></div>
<br>
<div id="pic-title"></div>

</center></body>
</html>)rawliteral";

  char msg2[20];
  strcpy(msg2, "Start Recording");
  if ( recording == 1 ) 
    strcpy(msg2, "Stop Recording");

  sprintf(the_page, msg, recording, msg2);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t index_handler(httpd_req_t *req) {

  print_stats("Index Handler  Core: ");

  do_index();
  httpd_resp_send(req, the_page, strlen(the_page));
  return ESP_OK;
}


// definitions for web page header stream content
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t stream_handler(httpd_req_t *req) {

  print_stats("Stream Handler  Core: ");

  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  char * part_buf[64];

  // wait one second
  delay(1000);

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while ( true ) {
    xSemaphoreTake( baton, portMAX_DELAY );
    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera capture failed");
      httpd_resp_send_500(req);
      xSemaphoreGive( baton );
      return ESP_FAIL;
    }

    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *) fb->buf, fb->len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    else {
      res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    }

    esp_camera_fb_return(fb);
    fb = NULL;
    xSemaphoreGive( baton );

    if(res != ESP_OK){
      Serial.println( "Breaking out of stream loop due to error response from browser" );
      delay(2000); 
      break;
    }

    // wait two seconds between frames when recording
    if ( recording == 1 ) {
      delay(2000); 
    }
  }

  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t record_handler(httpd_req_t *req) {

  print_stats("Record Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[4];
  int action;

  // query parameters - get filename
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "action", param, sizeof(param)) == ESP_OK) {
        action = atoi( param );
      }
    }
  }
  // action = 1 is start recording, action = 0 is stop recording

  if ( action == 1 && recording == 1 ) {
    // turn on recording, but already recording
    sprintf(buf, "Recording in progress");
  }
  if ( action == 1 && recording == 0 ) {
    // turn on recording
    recording = 1;
    sprintf(buf, "Recording started");
  }
  if ( action == 0 && recording == 0 ) {
    // turn off recording, but already stopped recording
    sprintf(buf, "Recording already stopped");
  }
  if ( action == 0 && recording == 1 ) {
    // turn off recording
    recording = 0;
    sprintf(buf, "Recording stopped");
  }

  Serial.println ( buf );

  //sprintf(buf, "%i", recording);
  httpd_resp_send(req, (const char *)buf, strlen(buf));
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// 
static esp_err_t picture_handler(httpd_req_t *req) {

  print_stats("Picture Handler  Core: ");

  // build name for new jpg file
  time(&now);
  localtime_r(&now, &timeinfo);

  strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

  char fname[100];

  sprintf(fname, "/sdcard/%s.jpg", strftime_buf);
  Serial.print("\nFile name is > "); Serial.print(fname); Serial.println(" <");

  // open new jpg file for write
  jpgfile = fopen(fname, "w");
  if (jpgfile != NULL)  {
    //Serial.printf("File open: %s\n", fname);
  }  else  {
    Serial.println("Could not open file");
    major_fail();
  }

  // only keep MAX_JPG_Files_On_SD 
  jpg_file_count++;
  if ( jpg_file_count > MAX_JPG_Files_On_SD ) {
    // delete oldest file
    if( SD_MMC.exists( jpg_filenames[jpg_oldest_index] )) {
      SD_MMC.remove( jpg_filenames[jpg_oldest_index] );
    }
    jpg_file_count--;
    strcpy(jpg_filenames[jpg_oldest_index], "");
    jpg_oldest_index++;
    if (jpg_oldest_index == MAX_JPG_Files_On_SD)
      jpg_oldest_index = 0;
  }
  Serial.println( "Number of JPG files on SD card is " + String(jpg_file_count) + "\n" );
  if ( jpg_file_count > 1 )
    jpg_newest_index++;
  if ( jpg_newest_index == MAX_JPG_Files_On_SD )
    jpg_newest_index = 0;
  strcpy(jpg_filenames[jpg_newest_index],fname+7);

  // get a camera frame to save as jpg
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  xSemaphoreTake( baton, portMAX_DELAY );
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    httpd_resp_send_500(req);
    xSemaphoreGive( baton );
    fclose(jpgfile);
    return ESP_FAIL;
  }

  // save the frame to the jpg file
  size_t err = fwrite(fb->buf, 1, fb->len, jpgfile);
  if (err == 0 ) {
    Serial.println("Error on jpg write");
    esp_camera_fb_return(fb);
    xSemaphoreGive( baton );
    fclose(jpgfile);
    major_fail();
  }

  fclose(jpgfile);

  // now send the new jpg back to the browser
  char buf[200];
  sprintf(buf, "inline; filename=%s", fname+8);

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", buf);

  httpd_resp_send(req, (const char *)fb->buf, fb->len);

  esp_camera_fb_return(fb);
  xSemaphoreGive( baton );

  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_savedavi(httpd_req_t *req) {

  char buf[500];
  Serial.println("do_savedavi "); 


  // send HTTP response in chunked encoding 
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Transfer-Encoding", "chunked");

  const char msg1[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>ESP32-CAM Video Recorder</title>
<style>
a {color: blue;}
a:visited {color: blue;}
a:hover {color: blue;}
a:active {color: blue;}
</style>
<script>
// the function below will highlight one clicked row at a time
function highlight_row() {
    var table = document.getElementById('dataTable');
    var rows = table.getElementsByTagName('tr');

    for (var i = 1; i < rows.length; i++) {
        // Take each row
        var row = rows[i];
        // do something on onclick event for row
        row.onclick = function () {
            // Get the row id where the cell exists
            var rowId = this.rowIndex;

            var rowsNotSelected = table.getElementsByTagName('tr');
            for (var row2 = 0; row2 < rowsNotSelected.length; row2++) {
                rowsNotSelected[row2].style.backgroundColor = "";
                rowsNotSelected[row2].classList.remove('selected');
            }
            var rowSelected = table.getElementsByTagName('tr')[rowId];
            rowSelected.style.backgroundColor = '#BCD4EC';
            rowSelected.className += " selected";
        }
    }
} //end of function

function do_delete(fname, index) {
  if ( confirm( 'Are you sure you want to delete\n'+fname+'?' ) ) {
    var XHR = new XMLHttpRequest();
    XHR.open( "GET", `/deleteavi?filename=${fname}&index=${index}`, true );            
    XHR.send( null );
    setTimeout(function(){ location.reload(true) }, 3000);
  }  
}

  // force reload when going back to page
  if(!!window.performance && window.performance.navigation.type == 2) {
    location.reload(true);
  }

window.onload = highlight_row;
</script>
</head>
<body>
<center>
<h1>ESP32-CAM Video Recorder <br></h1>
 <h2>Saved AVI Files (Total %i)</h2>
 <table id="dataTable">
 <tr><td><br></td><td colspan="4">PEEK - Displays first frame of AVI file<br>
 STREAM - Streams AVI file</td></tr> )rawliteral";
 
  sprintf(the_page, msg1, avi_file_count);

  // send first chunk
  httpd_resp_send_chunk(req, the_page, strlen(the_page));

  // display filenames from avi_newest_index to avi_oldest_index
  strcpy(the_page, ""); // clear page for next chunk
  int i = avi_newest_index;
  int j = 0; // counter for lines in one chunk
  while (i != avi_oldest_index) {
    if ( avi_filenames[i] ) {
      sprintf(buf, "<tr><td><a href=\"/peekavi?filename=%s\">PEEK</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/streamavi?filename=%s\">STREAM</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/download?filename=%s\">DOWNLOAD</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td>%s&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"javascript:do_delete('%s','%i');\">DELETE</a></td></tr>\n", avi_filenames[i]+1, i);
      strcat(the_page, buf);
      j++;
      if ( j == 20 ) {
        // send another chunk
        httpd_resp_send_chunk(req, the_page, strlen(the_page));
        strcpy(the_page, ""); // clear page for next chunk
        j = 0;
      }
    }
    i--;
    if ( i < 0 ) { i = MAX_AVI_Files_On_SD - 1; }
  } 
  // loop above cut out before displaying avi_oldest_index, so display it now
    if ( avi_filenames[i] ) {
      sprintf(buf, "<tr><td><a href=\"/peekavi?filename=%s\">PEEK</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/streamavi?filename=%s\">STREAM</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/download?filename=%s\">DOWNLOAD</a>&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td>%s&nbsp&nbsp&nbsp</td>", avi_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"javascript:do_delete('%s','%i');\">DELETE</a></td></tr>\n", avi_filenames[i]+1, i);
      strcat(the_page, buf);
    }
  
  const char msg2[] PROGMEM = R"rawliteral(</td></tr></table></body></html>)rawliteral";

  strcat(the_page, msg2);

  // send last chunks
  httpd_resp_send_chunk(req, the_page, strlen(the_page));
  httpd_resp_send_chunk(req, NULL, 0);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t savedavi_handler(httpd_req_t *req) {

  print_stats("SavedAvi Handler  Core: ");

  do_savedavi(req);
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
uint8_t * frameb = NULL;  // will be populated by avi_fb_get() and jpg_fb_get()
size_t avi_fb_get() { 
  // this function assumes file pointer is at start of frame to be read  

  uint8_t fbs[4];
  size_t fbsize;

  // frame type is in positions 0 - 3
  fread( fbs, 1, 4, fbfile );  // read frame type
  if (!strncmp( (const char *) fbs, "idx1", 4 )) {
    // idx1 indicates the end of the avi data
    return 0;
  }

  // fb size is in positions 4 - 7
  fread( fbs, 1, 4, fbfile );  // read fb size
  fbsize = fbs[0] * 0x01;
  fbsize = fbsize + (fbs[1] * 0x100);
  fbsize = fbsize + (fbs[2] * 0x10000);
  fbsize = fbsize + (fbs[3] * 0x1000000);
  //Serial.print( "fbsize " ); Serial.println( fbsize );

  // after size read above filepos is at start of actual fb
  fread( frameb, 1, fbsize, fbfile );
  // specify frame is jpg
  frameb[6] = 'J';
  frameb[7] = 'F';
  frameb[8] = 'I';
  frameb[9] = 'F';
  //Serial.print( "filepos " ); Serial.println( ftell( fbfile ) ); 
  return fbsize;
  // this function exits with file pointer at start of next frame to be read
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t peekavi_handler(httpd_req_t *req) {

  print_stats("PeekAvi Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char peekfilename[MAX_FILENAME_LENGTH+10] = {'\0'};
  size_t frameb_len = 0;

  // query parameters - get filename
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(peekfilename,"/sdcard/");
        strcat(peekfilename,param);
      }
    }
  }

  if ( recording == 1 && !strcmp((const char *) param, (const char *) avi_filenames[avi_newest_index]+1 )) {
    sprintf(buf, "<h2><center><br><br>File %s is being used for recording</h2>", param);
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  fbfile = fopen(peekfilename, "r");
  if (fbfile == NULL) {
    sprintf(buf, "<h2><center><br><br>File not found. Go back, refresh the Saved AVI Files page and try again</h2>");
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  // remove .avi characters from name so _peek.jpg can be added
  param[strlen(param)-4] = '\0';  
  sprintf(buf, "inline; filename=%s_peek.jpg", param);

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", buf);

  // allocate memory for frame buffer from PSRAM
  frameb = (uint8_t*) heap_caps_calloc(62000, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  fseek( fbfile, 240, SEEK_SET );  // seek to 240 in file
  frameb_len = avi_fb_get();  // read first frame from file into frameb, returns size

  httpd_resp_send(req, (const char *)frameb, frameb_len);

  fclose(fbfile);

  // free frame buffer memory
  free(frameb);
  frameb = NULL;
    
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t streamavi_handler(httpd_req_t *req) {

  print_stats("StreamAVI Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char streamfilename[MAX_FILENAME_LENGTH+10] = {'\0'};
  size_t frameb_len = 0;
  char * part_buf[64];

  // query parameters - get filename
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(streamfilename,"/sdcard/");
        strcat(streamfilename,param);
      }
    }
  }

  // no streaming while recording in progress
  if ( recording == 1 ) {
    strcpy(buf, "<h2><center><br><br>Streaming is unavailable while a recording in progress<br><br>");
    strcat(buf, "Downloading is okay</h2>");
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  fbfile = fopen(streamfilename, "r");
  if (fbfile == NULL) {
    sprintf(buf, "<h2><center><br><br>File not found. Go back, refresh the Saved AVI Files page and try again</h2>");
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  // allocate memory for frame buffer from PSRAM
  frameb = (uint8_t*) heap_caps_calloc(62000, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  fseek( fbfile, 240, SEEK_SET );  // seek to 240 in file
  frameb_len = avi_fb_get();  // read first frame from file into frameb, returns size

  esp_err_t res = ESP_OK;
  unsigned long st = millis();  // used to force sending one frame every 125ms
  while ( frameb_len > 0 ) {
    // tell browser size of stream part
    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, frameb_len);
    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    // send the stream part
    if ( res == ESP_OK ) {
      res = httpd_resp_send_chunk(req, (const char *)frameb, frameb_len);
    }
    // send stream boundary
    if ( res == ESP_OK ) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    } else {
      res = httpd_resp_send_chunk(req, (const char *)frameb, frameb_len);
    }
    if(res != ESP_OK){
      Serial.println( "Breaking out of stream loop due to error response from browser" );
      break;
    }

    // It appears the network is not keeping up so sending every fourth frame
    frameb_len = avi_fb_get();  // read next frame from file into frameb, skip sending this one

    // try to send one frame every 125ms
    st = millis() - st;
    if ( st < 125 ) { delay( 125 - st ); }
    st = millis();
  }

  // free frame buffer memory
  free(frameb);
  frameb = NULL;
  fclose(fbfile);
    
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_deleteavi(char * delfilename, int index) {

  Serial.print("do_deleteavi "); 

  // delete the file
  Serial.println( " delfilename " + String(delfilename) + ", index " + String(index) );
  if( SD_MMC.exists( delfilename )) {
    SD_MMC.remove( delfilename );

    // fix avi_filenames, avi_newest_index, avi_oldest_index
    avi_file_count--;
    strcpy(avi_filenames[index], "");
  
    if ( avi_file_count > 0 ) {
      if ( index == avi_newest_index ) {
        avi_newest_index--;
        if ( avi_newest_index < 0 ) 
          avi_newest_index = MAX_AVI_Files_On_SD - 1;
      } else if ( index == avi_oldest_index ) {
        avi_oldest_index++;
        if ( avi_oldest_index == MAX_AVI_Files_On_SD )
          avi_oldest_index = 0;
      } else {
        int idx = index;
        int idx2 = idx + 1;
        if ( idx2 == MAX_AVI_Files_On_SD )
          idx2 = 0;
        while ( idx != avi_newest_index ) {
          strcpy(avi_filenames[idx], avi_filenames[idx2]);
      
          idx++;
          if (idx == MAX_AVI_Files_On_SD )
            idx = 0;
          idx2++;
          if (idx2 == MAX_AVI_Files_On_SD )
            idx2 = 0;
        }
        strcpy(avi_filenames[idx], "");
        avi_newest_index--;
        if ( avi_newest_index < 0 )
          avi_newest_index = MAX_AVI_Files_On_SD - 1;
      }
    }
  }
  
  // build delete response  (note: this response is currently not sent 
  //                since savedavi page reloads itself after 3 seconds)
  const char msg1[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>Delete</title>
<script>
function start() {
  window.location.replace("/savedavi");
}
</script>
</head>
<body onload=start();>
 </body>
</html>)rawliteral";

  sprintf(the_page, msg1);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t deleteavi_handler(httpd_req_t *req) {

  print_stats("DeleteAvi Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char delfilename[MAX_FILENAME_LENGTH] = {'\0'};
  int index;

  // query parameters
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(delfilename,"/");
        strcat(delfilename,param);
      }
      if (httpd_query_key_value(buf, "index", param, sizeof(param)) == ESP_OK) {
        index = atoi(param);
      }
    }
  }

  if ( recording == 1 && !strcmp((const char *) delfilename+1, (const char *) avi_filenames[avi_newest_index]+1 )) {
    sprintf(buf, "<h2><center><br><br>File %s is being used for recording</h2>", delfilename+1);
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  do_deleteavi(delfilename, index);
  // no need to respond since page will refresh on its own
  //httpd_resp_send(req, the_page, strlen(the_page));
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_savedjpg(httpd_req_t *req) {

  char buf[500];
  Serial.println("do_savedjpg "); 

  // and send HTTP response in chunked encoding 
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Transfer-Encoding", "chunked");

  const char msg1[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>ESP32-CAM Video Recorder</title>
<style>
a {color: blue;}
a:visited {color: blue;}
a:hover {color: blue;}
a:active {color: blue;}
</style>
<script>
// the function below will highlight one clicked row at a time
function highlight_row() {
    var table = document.getElementById('dataTable');
    var rows = table.getElementsByTagName('tr');

    for (var i = 1; i < rows.length; i++) {
        // Take each row
        var row = rows[i];
        // do something on onclick event for row
        row.onclick = function () {
            // Get the row id where the cell exists
            var rowId = this.rowIndex;

            var rowsNotSelected = table.getElementsByTagName('tr');
            for (var row2 = 0; row2 < rowsNotSelected.length; row2++) {
                rowsNotSelected[row2].style.backgroundColor = "";
                rowsNotSelected[row2].classList.remove('selected');
            }
            var rowSelected = table.getElementsByTagName('tr')[rowId];
            rowSelected.style.backgroundColor = '#BCD4EC';
            rowSelected.className += " selected";
        }
    }
} //end of function

function do_delete(fname, index) {
  if ( confirm( 'Are you sure you want to delete\n'+fname+'?' ) ) {
    var XHR = new XMLHttpRequest();
    XHR.open( "GET", `/deletejpg?filename=${fname}&index=${index}`, true );            
    XHR.send( null );
    setTimeout(function(){ location.reload(true) }, 3000);
  }  
}

window.onload = highlight_row;
</script>
</head>
<body>
<center>
<h1>ESP32-CAM Video Recorder <br></h1>
 <h2>Saved JPG Files (Total %i)</h2>
 <table id="dataTable">
 <tr><td><br></td><td colspan="4">VIEW - Displays JPG file</td></tr> )rawliteral";

  sprintf(the_page, msg1, jpg_file_count);

  // send first chunk
  httpd_resp_send_chunk(req, the_page, strlen(the_page));

  // display filenames from jpg_newest_index to jpg_oldest_index
  strcpy(the_page, ""); // clear page for next chunk
  int i = jpg_newest_index;
  int j = 0; // counter for lines in one chunk
  while (i != jpg_oldest_index && jpg_file_count > 0) {
    if ( jpg_filenames[i] ) {
      sprintf(buf, "<tr><td><a href=\"/viewjpg?filename=%s\">VIEW</a>&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/download?filename=%s\">DOWNLOAD</a>&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td>%s&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"javascript:do_delete('%s','%i');\">DELETE</a></td></tr>\n", jpg_filenames[i]+1, i);
      strcat(the_page, buf);
      j++;
      if ( j == 20 ) {
        // send another chunk
        httpd_resp_send_chunk(req, the_page, strlen(the_page));
        strcpy(the_page, ""); // clear page for next chunk
        j = 0;
      }
    }
    i--;
    if ( i < 0 ) { i = MAX_JPG_Files_On_SD - 1; }
  } 
  // loop above cut out before displaying jpg_oldest_index, so display it now
    if ( jpg_filenames[i] && jpg_file_count > 0 ) {
      sprintf(buf, "<tr><td><a href=\"/viewjpg?filename=%s\">VIEW</a>&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"/download?filename=%s\">DOWNLOAD</a>&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td>%s&nbsp&nbsp&nbsp</td>", jpg_filenames[i]+1);
      strcat(the_page, buf);
      sprintf(buf, "<td><a href=\"javascript:do_delete('%s','%i');\">DELETE</a></td></tr>\n", jpg_filenames[i]+1, i);
      strcat(the_page, buf);
    }
  
  const char msg2[] PROGMEM = R"rawliteral(</td></tr></table></body></html>)rawliteral";

  strcat(the_page, msg2);

  // send last chunks
  httpd_resp_send_chunk(req, the_page, strlen(the_page));
  httpd_resp_send_chunk(req, NULL, 0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t savedjpg_handler(httpd_req_t *req) {

  print_stats("SavedJpg Handler  Core: ");

  do_savedjpg(req);
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t viewjpg_handler(httpd_req_t *req) {

  print_stats("ViewJPG Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char viewfilename[MAX_FILENAME_LENGTH+10] = {'\0'};
  size_t view_len = 0;

  // query parameters - get filename
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(viewfilename,"/sdcard/");
        strcat(viewfilename,param);
      }
    }
  }

  fbfile = fopen(viewfilename, "r");
  if (fbfile == NULL) {
    sprintf(buf, "<h2><center><br><br>File not found. Go back, refresh the Saved JPG Files page and try again</h2>");
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  sprintf(buf, "inline; filename=%s", param);

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", buf);

  // allocate memory for frame buffer from PSRAM
  frameb = (uint8_t*) heap_caps_calloc(62000, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  view_len = fread( frameb, 1, 32767, fbfile );  

  httpd_resp_send(req, (const char *)frameb, view_len);

  fclose(fbfile);

  // free frame buffer memory
  free(frameb);
  frameb = NULL;
    
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_deletejpg(char * delfilename, int index) {

  Serial.print("do_deletejpg "); 

  // delete the file
  Serial.println( " delfilename " + String(delfilename) + ", index " + String(index) );
  if( SD_MMC.exists( delfilename )) {
    SD_MMC.remove( delfilename );

    // fix jpg_filenames, jpg_newest_index, jpg_oldest_index
    jpg_file_count--;
    strcpy(jpg_filenames[index], "");
  
    if ( jpg_file_count > 0 ) {
      if ( index == jpg_newest_index ) {
        jpg_newest_index--;
        if ( jpg_newest_index < 0 ) 
          jpg_newest_index = MAX_JPG_Files_On_SD - 1;
      } else if ( index == jpg_oldest_index ) {
        jpg_oldest_index++;
        if ( jpg_oldest_index == MAX_JPG_Files_On_SD )
          jpg_oldest_index = 0;
      } else {
        int idx = index;
        int idx2 = idx + 1;
        if ( idx2 == MAX_JPG_Files_On_SD )
          idx2 = 0;
        while ( idx != jpg_newest_index ) {
          strcpy(jpg_filenames[idx], jpg_filenames[idx2]);
      
          idx++;
          if (idx == MAX_JPG_Files_On_SD )
            idx = 0;
          idx2++;
          if (idx2 == MAX_JPG_Files_On_SD )
            idx2 = 0;
        }
        strcpy(jpg_filenames[idx], "");
        jpg_newest_index--;
        if ( jpg_newest_index < 0 )
          jpg_newest_index = MAX_JPG_Files_On_SD - 1;
      }
    }
  }
  
  // build delete response  (note: this response is currently not sent 
  //                since savedavi page reloads itself after 3 seconds)
  const char msg1[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>Delete</title>
<script>
function start() {
  window.location.replace("/savedjpg");
}
</script>
</head>
<body onload=start();>
 </body>
</html>)rawliteral";

  sprintf(the_page, msg1);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t deletejpg_handler(httpd_req_t *req) {

  print_stats("DeleteJpg Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char delfilename[MAX_FILENAME_LENGTH] = {'\0'};
  int index;

  // query parameters
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(delfilename,"/");
        strcat(delfilename,param);
      }
      if (httpd_query_key_value(buf, "index", param, sizeof(param)) == ESP_OK) {
        index = atoi(param);
      }
    }
  }

  do_deletejpg(delfilename, index);
  // no need to respond since page will refresh on its own
  //httpd_resp_send(req, the_page, strlen(the_page));
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t download_handler(httpd_req_t *req) {

  print_stats("Download Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[MAX_FILENAME_LENGTH];
  char downloadfilename[MAX_FILENAME_LENGTH+10] = {'\0'};

  // query parameters - get filename to download
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "filename", param, sizeof(param)) == ESP_OK) {
        strcpy(downloadfilename,"/sdcard/");
        strcat(downloadfilename,param);
      }
    }
  }

  if ( recording == 1 && !strcmp((const char *) param, (const char *) avi_filenames[avi_newest_index]+1 )) {
    sprintf(buf, "<h2><center><br><br>File %s is being used for recording</h2>", param);
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  // open file for download
  FILE *dlfile = fopen(downloadfilename, "r");
  if (dlfile == NULL) {
    sprintf(buf, "<h2><center><br><br>File %s was not found</h2>", param);
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, (const char *)buf, strlen(buf));
    return ESP_OK;
  }

  // tell browser to get ready to accept a file, file will be sent one piece at a time
  Serial.print( "  Downloading "); Serial.println ( downloadfilename+8 );
  char dname[100] = "attachment; filename=";
  strcat(dname,downloadfilename+8);
  httpd_resp_set_type(req, "application/octet-stream");
  httpd_resp_set_hdr(req, "Content-Disposition", dname);

  // The is handled one piece at a time via calls to do_download
  // Read file in chunks (relaxes any constraint due to large file sizes)
  // and send HTTP response in chunked encoding 
  char   dlchunk[8192];
  size_t dlchunksize;
  do {
    dlchunksize = fread(dlchunk, 1, sizeof(dlchunk), dlfile);
    if (httpd_resp_send_chunk(req, dlchunk, dlchunksize) != ESP_OK) {
      fclose(dlfile);
      return ESP_FAIL;
    }
  } while (dlchunksize != 0);

  httpd_resp_send_chunk(req, NULL, 0);
  fclose(dlfile);
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_settings() {

  Serial.println("do_settings "); 

  const char msg1[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>ESP32-CAM Video Recorder</title>
<style>
a {color: blue;}
a:visited {color: blue;}
a:hover {color: blue;}
a:active {color: blue;}
</style>
<script>

  document.addEventListener('DOMContentLoaded', function() {
    do_motion(0);
    do_motionaction(0);
    do_trigger(0);
    do_triggeraction(0);
  });

function do_reset() {
  if ( confirm( 'Are you sure you want to reboot the ESP32-CAM?' ) ) {
    window.location = "/settings?action=reset";
  }  
}

var timeout = null;

var curreclen = %i;

function do_length() {
  // send changes when changes are done
  var reclen = parseInt(document.getElementById("reclen").value);
  if ( reclen > 9 && reclen < 301 ) {
    clearTimeout(timeout);
    curreclen = reclen;
    document.getElementById("reclen").value = reclen;
    timeout = setTimeout(function () {
      var XHR = new XMLHttpRequest();
      XHR.open( "GET", `/settings?action=length&reclength=${reclen}`, true );            
      XHR.onloadend = function(){
        alert( XHR.responseText );
      }
      XHR.send( null );
    }, 2000);
  } else {
    document.getElementById("reclen").value = curreclen;  
  }
}
        
function do_email() {
  var email = document.getElementById("email").value;
  var XHR = new XMLHttpRequest();
  XHR.open( "GET", `/settings?action=email&emailaddr=${email}`, true );  
  XHR.onloadend = function(){
    alert( XHR.responseText );
  }
  XHR.send( null );
}

var curmotion = %i;

function do_motion(action) {
  if ( action == 0 ) { // display current value
    if ( curmotion < 10 ) { // motion detection is off
      document.getElementById('motion').innerHTML = '<a href="javascript:do_motion(1);">Turn On Motion Detection</a>';
    } else { // motion detection is on
      document.getElementById('motion').innerHTML = '<a href="javascript:do_motion(1);">Turn Off Motion Detection</a>';    
    }
  } else { // toggle value
    if ( curmotion < 10 ) { // turn on motion detection
      curmotion = curmotion + 10;
      document.getElementById('motion').innerHTML = '<a href="javascript:do_motion(1);">Turn Off Motion Detection</a>';
    } else { // turn off motion detection
      curmotion = curmotion - 10;
      document.getElementById('motion').innerHTML = '<a href="javascript:do_motion(1);">Turn On Motion Detection</a>';    
    }
    var XHR = new XMLHttpRequest();
    XHR.open( "GET", `/settings?action=motion&maction=${curmotion}`, true );   
    XHR.onloadend = function(){
      alert( XHR.responseText );
    }
    XHR.send( null );
  }
}
        
function do_motionaction(action) {
  var a = curmotion; // a is action
  if ( a > 9 ) 
    a = a - 10;
  var of = 0; // of is on off value
  if ( curmotion > 9 ) 
    of = 10;
  if ( action == 0 ) { // display current value
    document.getElementById("maction").selectedIndex = a;
  } else { // process a possible change
    var e = document.getElementById("maction");
    var na = e.options[e.selectedIndex].value - 1;
    if ( na != a ) { // drop down selection has changed
      curmotion = of + na;
      var XHR = new XMLHttpRequest();
      XHR.open( "GET", `/settings?action=motion&maction=${curmotion}`, true );   
      XHR.onloadend = function(){
        alert( XHR.responseText );
      }
      XHR.send( null );
    }
  }
}
        
var curtrigger = %i;

function do_trigger(action) {
  if ( action == 0 ) { // display current value
    if ( curtrigger < 10 ) { // trigger is off
      document.getElementById('trigger').innerHTML = '<a href="javascript:do_trigger(1);">Turn On Time Trigger</a>';
    } else { // motion detection is on
      document.getElementById('trigger').innerHTML = '<a href="javascript:do_trigger(1);">Turn Off Time Trigger</a>';    
    }
  } else { // toggle value
    if ( curtrigger < 10 ) { // turn on time trigger
      curtrigger = curtrigger + 10;
      document.getElementById('trigger').innerHTML = '<a href="javascript:do_trigger(1);">Turn Off Time Trigger</a>';
    } else { // turn off time trigger
      curtrigger = curtrigger - 10;
      document.getElementById('trigger').innerHTML = '<a href="javascript:do_trigger(1);">Turn On Time Trigger</a>';    
    }
    var XHR = new XMLHttpRequest();
    XHR.open( "GET", `/settings?action=trigger&taction=${curtrigger}`, true );   
    XHR.onloadend = function(){
      alert( XHR.responseText );
    }
    XHR.send( null );
  }
}
        
function do_triggeraction(action) {
  var a = curtrigger; // a is action
  if ( a > 9 ) 
    a = a - 10;
  var of = 0; // of is on off value
  if ( curtrigger > 9 ) 
    of = 10;
  if ( action == 0 ) { // display current value
    document.getElementById("taction").selectedIndex = a;
  } else { // process a possible change
    var e = document.getElementById("taction");
    var na = e.options[e.selectedIndex].value - 1;
    if ( na != a ) { // drop down selection has changed
      curtrigger = of + na;
      var XHR = new XMLHttpRequest();
      XHR.open( "GET", `/settings?action=trigger&taction=${curtrigger}`, true );   
      XHR.onloadend = function(){
        alert( XHR.responseText );
      }
      XHR.send( null );
    }
  }
}

var curth = %i;
var curtm = %i;

function do_triggertime() {
  // send changes when changes are done
  var th = parseInt(document.getElementById("th").value);
  var tm = parseInt(document.getElementById("tm").value);
  clearTimeout(timeout);
  if ( th > -1 && th < 24 && tm > -1 && tm < 60 ) {
    curth = th;
    curtm = tm;
    document.getElementById("th").value = th;
    document.getElementById("tm").value = tm;
    timeout = setTimeout(function () {
      var XHR = new XMLHttpRequest();
      XHR.open( "GET", `/settings?action=triggertime&th=${th}&tm=${tm}`, true );            
      XHR.onloadend = function(){
        alert( XHR.responseText );
      }
      XHR.send( null );
    }, 4000);
  } else {
    document.getElementById("th").value = curth;  
    document.getElementById("tm").value = curtm;  
  }
}
        
</script>
</head>
<body>
<center>
<h1>ESP32-CAM Video Recorder</h1>
 <h2>Settings</h2>
 <table id="actions">
   <tr><td>All video recordings default to VGA (640x480), one frame every 125ms for 60 seconds.<br>
           In the video name, L is length in seconds and M is size in megabytes. The default length can be changed.<br>
           Up to 250 videos and 250 pictures can be saved. After that the oldest are deleted as new are created.<br>
           If the stream fails to show on the main page, reload the page until it shows.<br>
           Click the taken picture with the left button to dismiss it.<br>
   </td></tr>
 </table>   
 <table id="actions">
   <tr><td>&nbsp</td></tr>
   <tr><td>Set recording length in seconds (10 to 300):
     <input type="number" id="reclen" value="%i" min="10" max="300" style="width: 4em" onchange="do_length();"/>
   <tr><td>&nbsp</td></tr>
   <tr><td>Set email address: 
     <input type="text" id="email" value="%s" maxlength="40" size="40">&nbsp;
     <a href="javascript:do_email();">Send Test Email</a><br>
     Wait 15 seconds for the test to complete. 
     The new email address will be saved if the test succeeds.</td></tr>
   <tr><td>&nbsp</td></tr>
   <tr><td><span id="motion"><a href="javascript:do_motion(1);">Turn On Motion Detection</a></span>
     &nbsp;Motion detect action:
     <select id="maction" onchange="do_motionaction(1);">
       <option value="1">Take a picture</option>
       <option value="2">Take a picture and email it</option>
       <option value="3">Take a video</option>
       <option value="4">Take a video and email name of video</option>
     </select><br>
     Motion detection starts again 60 seconds after a trigger.
   </td></tr>
   <tr><td>&nbsp</td></tr>
   <tr><td><span id="trigger"><a href="javascript:do_trigger(1);">Turn On Time Trigger</a></span>
     &nbsp;Time trigger action:
     <select id="taction" onchange="do_triggeraction(1);">
       <option value="1">Take a picture</option>
       <option value="2">Take a picture and email it</option>
       <option value="3">Take a video</option>
       <option value="4">Take a video and email name of video</option>
     </select>
     <br>Trigger time (0-23H : 0-59M):
     <input type="number" id="th" value="%i" min="0" max="23" style="width: 3em" onchange="do_triggertime();"/>
     H :
     <input type="number" id="tm" value="%i" min="0" max="59" style="width: 3em" onchange="do_triggertime();"/>
     M
   </td></tr>
   <tr><td>&nbsp</td></tr>
   <tr><td><a href="javascript:do_reset();">ESP32-CAM Reboot</a></td></tr>
 </table>
  <table id="boottime">
   <tr><td><br>SD card has %iMB available, %iMB used and %iMB free.</td></tr>
   <tr><td><br>ESP32-CAM has been running since %s.</td></tr>
 </table>   
</body></html>)rawliteral";

  int used = SD_MMC.usedBytes() / (1024 * 1024);
  int total = SD_MMC.totalBytes() / (1024 * 1024);
  int free = total - used;
  sprintf(the_page, msg1, xlength, motionDetect, triggerDetect, triggerH, triggerM, xlength, email, triggerH, triggerM, total, used, free, boottime);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
void do_reset() {

  Serial.print("do_reset ");

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM Video Recorder</title>
<script>
  document.addEventListener('DOMContentLoaded', function() {
    var c = document.location.origin;
    var i = 60;
    var timing = 1000;
    function loop() {
      document.getElementById('image-container').innerText = 'ESP32-CAM will be ready in '+`${i}`+' seconds';
      i = i - 1;
      if ( i > 0 ) {
        window.setTimeout(loop, timing);
      } else {
        window.location.replace("/");
      }
    }
    loop();
  });
</script>
</head>
<body><center>
<h1>ESP32-CAM Resetting</h1>
<h2><div id="image-container"></div></h2>
If the ESP32-CAM does not respond after 60 seconds, keep waiting.<br>
It will keep resetting about once per minute if it does not start clean.
</center></body>
</html>)rawliteral";

  sprintf(the_page,  msg);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t reset_handler(httpd_req_t *req) {

  print_stats("Reset Handler  Core: ");

  do_reset();
  httpd_resp_send(req, the_page, strlen(the_page));

  delay(4000);  // wait 4 seconds
  
  // sleep 5 seconds and boot up
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP5S * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
static esp_err_t settings_handler(httpd_req_t *req) {

  print_stats("Settings Handler  Core: ");

  char buf[500];
  size_t buf_len;
  char param[60];
  char action[20];
  int reclength = 60;
  char new_email[60];
  int newmotionDetect;
  int newtriggerDetect;
  int newtriggerH;
  int newtriggerM;
  strcpy(action, "show");

  // query parameters - get action
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "action", param, sizeof(param)) == ESP_OK) {
        strcpy(action, param);
      }
      if (httpd_query_key_value(buf, "reclength", param, sizeof(param)) == ESP_OK) {
        reclength = atoi(param);
      }
      if (httpd_query_key_value(buf, "emailaddr", param, sizeof(param)) == ESP_OK) {
        strcpy(new_email,param);
      }
      if (httpd_query_key_value(buf, "maction", param, sizeof(param)) == ESP_OK) {
        newmotionDetect = atoi( param );
      }
      if (httpd_query_key_value(buf, "taction", param, sizeof(param)) == ESP_OK) {
        newtriggerDetect = atoi( param );
      }
      if (httpd_query_key_value(buf, "th", param, sizeof(param)) == ESP_OK) {
        newtriggerH = atoi( param );
      }
      if (httpd_query_key_value(buf, "tm", param, sizeof(param)) == ESP_OK) {
        newtriggerM = atoi( param );
      }
    }
  }

  if ( !strcmp( action, "length" ) ) {
    if ( reclength > 9 && reclength < 301 ) {
      // set new recording length      
      sprintf(the_page, "Recording length set to %i seconds", reclength);
      Serial.println( the_page );
      total_frames = reclength * 8; // based on fixed frame interval of 125ms
      xlength = reclength;
      EEPROM.put(EEPROM_LENGTH_ADDR, xlength);
      EEPROM.commit();
      httpd_resp_send(req, the_page, strlen(the_page));
    }
  } else if ( !strcmp( action, "email" ) ) {
    static char msg[30];
    char * p = msg;
    p += sprintf(p,"This is a test email\n");
    // create SendEmail object 
    delay(3);

    SendEmail e(emailhost, emailport, emailsendaddr, emailsendpwd, 5000, true); 
    // Send Email
    char send[40];
    sprintf(send,"\<%s\>", emailsendaddr);
    char recv[40];
    sprintf(recv,"\<%s\>", new_email);
    //Serial.println("SendEmail object created, now trying to send...");
    if ( e.send(send, recv, "ESP32-CAM Test Email", msg, jpg_filenames[jpg_newest_index]) ) {
    //if ( e.send(send, recv, "ESP32-CAM Test Email", msg, NULL) ) {
      strcpy(email,new_email);
      sprintf(the_page, "New email address is %s", email);    
      EEPROM.put(EEPROM_EMAIL_ADDR, email);
      EEPROM.commit();
    } else {
      sprintf(the_page, "New email address test failed");    
    }
    e.close(); // close email client
    Serial.println( the_page );
    httpd_resp_send(req, the_page, strlen(the_page));
  } else if ( !strcmp( action, "motion" ) ) {
    if ( abs( newmotionDetect - motionDetect ) > 9 ) { // motion detect turned on or off
      motionDetect = newmotionDetect;
      if ( newmotionDetect < 10 ) {
        sprintf(the_page, "Motion detection turned off");
      } else {
        sprintf(the_page, "Motion detection turned on");
      }
    } else { // motion detection action changed
      motionDetect = newmotionDetect;
      if ( newmotionDetect > 9 )
        newmotionDetect = newmotionDetect - 10;
      if ( newmotionDetect == 0 )
        sprintf(the_page, "Motion detect action set to Take a picture");
      if ( newmotionDetect == 1 )
        sprintf(the_page, "Motion detect action set to Take a picture and email it");
      if ( newmotionDetect == 2 )
        sprintf(the_page, "Motion detect action set to Take a video");
      if ( newmotionDetect == 3 )
        sprintf(the_page, "Motion detect action set to Take a video and email name of video");
    }
    EEPROM.put(EEPROM_MOTION_ADDR, motionDetect);
    EEPROM.commit();
    httpd_resp_send(req, the_page, strlen(the_page));
  } else if ( !strcmp( action, "trigger" ) ) {
    if ( abs( newtriggerDetect - triggerDetect ) > 9 ) { // trigger detect turned on or off
      triggerDetect = newtriggerDetect;
      if ( newtriggerDetect < 10 ) {
        sprintf(the_page, "Time trigger turned off");
      } else {
        sprintf(the_page, "Time trigger turned on");
      }
    } else { // trigger action changed
      triggerDetect = newtriggerDetect;
      if ( newtriggerDetect > 9 )
        newtriggerDetect = newtriggerDetect - 10;
      if ( newtriggerDetect == 0 )
        sprintf(the_page, "Time trigger action set to Take a picture");
      if ( newtriggerDetect == 1 )
        sprintf(the_page, "Time trigger action set to Take a picture and email it");
      if ( newtriggerDetect == 2 )
        sprintf(the_page, "Time trigger action set to Take a video");
      if ( newtriggerDetect == 3 )
        sprintf(the_page, "Time trigger action set to Take a video and email name of video");
    }
    EEPROM.put(EEPROM_TRIGGER_ADDR, triggerDetect);
    EEPROM.commit();
    httpd_resp_send(req, the_page, strlen(the_page));
  } else if ( !strcmp( action, "triggertime" ) ) {
    if ( newtriggerH > -1 && newtriggerH < 24 & newtriggerM > -1 && newtriggerM < 60 ) {
      // set new trigger time      
      sprintf(the_page, "Trigger time set to %i H : %i M", newtriggerH, newtriggerM);
      Serial.println( the_page );
      triggerH = newtriggerH;
      triggerM = newtriggerM;
      EEPROM.put(EEPROM_TRIGGERH_ADDR, triggerH);
      EEPROM.put(EEPROM_TRIGGERM_ADDR, triggerM);
      EEPROM.commit();
      httpd_resp_send(req, the_page, strlen(the_page));
    }
  } else if ( !strcmp( action, "reset" ) ) {
    // call reset handler
    reset_handler(req);
  } else {
    // display settings page
    do_settings();
    httpd_resp_send(req, the_page, strlen(the_page));
  }
  
  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

void startCameraServer() {

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 15;
  config.max_resp_headers = 10;
  config.stack_size = 12288;
  config.recv_wait_timeout=10;
  config.send_wait_timeout=10;
  config.backlog_conn = 5;
  config.server_port = SERVER_PORT;
  config.lru_purge_enable = true;
  //config.recv_wait_timeout=30;
  //config.send_wait_timeout=30;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t record_uri = {
    .uri       = "/record",
    .method    = HTTP_GET,
    .handler   = record_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t picture_uri = {
    .uri       = "/picture",
    .method    = HTTP_GET,
    .handler   = picture_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t savedavi_uri = {
    .uri       = "/savedavi",
    .method    = HTTP_GET,
    .handler   = savedavi_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t peekavi_uri = {
    .uri       = "/peekavi",
    .method    = HTTP_GET,
    .handler   = peekavi_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t streamavi_uri = {
    .uri       = "/streamavi",
    .method    = HTTP_GET,
    .handler   = streamavi_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t deleteavi_uri = {
    .uri       = "/deleteavi",
    .method    = HTTP_GET,
    .handler   = deleteavi_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t savedjpg_uri = {
    .uri       = "/savedjpg",
    .method    = HTTP_GET,
    .handler   = savedjpg_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t viewjpg_uri = {
    .uri       = "/viewjpg",
    .method    = HTTP_GET,
    .handler   = viewjpg_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t deletejpg_uri = {
    .uri       = "/deletejpg",
    .method    = HTTP_GET,
    .handler   = deletejpg_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t download_uri = {
    .uri       = "/download",
    .method    = HTTP_GET,
    .handler   = download_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t settings_uri = {
    .uri       = "/settings",
    .method    = HTTP_GET,
    .handler   = settings_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &record_uri);
    httpd_register_uri_handler(camera_httpd, &picture_uri);
    httpd_register_uri_handler(camera_httpd, &savedavi_uri);
    httpd_register_uri_handler(camera_httpd, &peekavi_uri);
    httpd_register_uri_handler(camera_httpd, &streamavi_uri);
    httpd_register_uri_handler(camera_httpd, &deleteavi_uri);
    httpd_register_uri_handler(camera_httpd, &savedjpg_uri);
    httpd_register_uri_handler(camera_httpd, &viewjpg_uri);
    httpd_register_uri_handler(camera_httpd, &deletejpg_uri);
    httpd_register_uri_handler(camera_httpd, &download_uri);
    httpd_register_uri_handler(camera_httpd, &settings_uri);
  }
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

void startStreamServer() {

  // start a second server for streaming
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = SERVER_PORT+10;
  config2.ctrl_port = 42768;
  config2.max_uri_handlers = 4;
  config2.max_resp_headers = 8;
  config2.lru_purge_enable = true;


  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  // I don't know why but the main web server sometimes hangs
  // use this uri on the stream web server to reset the camera
  httpd_uri_t reset_uri = {
    .uri       = "/reset",
    .method    = HTTP_GET,
    .handler   = reset_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&camera2_httpd, &config2) == ESP_OK) {
    httpd_register_uri_handler(camera2_httpd, &stream_uri);
    httpd_register_uri_handler(camera2_httpd, &reset_uri);
  }

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

void stopCameraServer() {
  httpd_stop( camera_httpd );
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

void stopStreamServer() {
  httpd_stop( camera2_httpd );
}


////////////////////////////////////////////////////////////////////////////////////
//
// some globals for the loop()
//

unsigned long last_wakeup_1sec = 0;
unsigned long last_wakeup_1min = 0;
unsigned long last_wakeup_10min = 0;
unsigned long motion1min = 0;

void loop()
{

  if (WiFi.status() != WL_CONNECTED) {
    init_wifi();
    Serial.println("***** WiFi reconnect *****");
  }

  if (millis() - last_wakeup_1sec > (1000) ) {       // 1 second
    last_wakeup_1sec = millis();
    if ( motionDetect > 9 && motion == 0 && !digitalRead( PIR_Pin ) ) { // Low if motion detected
      int mcount = 0;
      while ( !digitalRead ( PIR_Pin ) && mcount < 4 ) {
        delay(5);
        mcount++;
        if ( mcount == 4 ) {
          // function to handle motion detected
          motion = 1;
          motion1min = millis();
          process_Detection(0);  // 0 is motion detection 
        }
      }
      
    }
  }

  if ( millis() - last_wakeup_1min > (1 * 60 * 1000) ) {       // 1 minute
    last_wakeup_1min = millis();

    // check current time and restart web servers after midnight (in case it got stuck during the day)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
    } else {
      char hc[3];  // current hour
      strftime(hc, sizeof(hc), "%H", &timeinfo);
      int h = atoi(hc);
      char mc[3];  // current minute
      strftime(mc, sizeof(mc), "%M", &timeinfo);
      int m = atoi(mc);

      // restart stream web server at midnight just in case it is stuck
      if ( h == 0 && m == 0 ) {
        Serial.println("Daily restart of StreamServer");
        stopStreamServer();
        delay(500);
        startStreamServer();
      }

      // check if time trigger happened
      if ( triggerDetect > 9 && triggerH == h && triggerM == m ) {
        // do time trigger
        process_Detection(1);  // 1 is time trigger detection 
      }
    }
  }

  if ( millis() - motion1min > (1 * 60 * 1000) && motion == 1 ) {  // 1 motion minute
    // allow motion detection after 1 minute
    motion = 0;
  }
 
  if (millis() - last_wakeup_10min > (10 * 60 * 1000) ) {       // 10 minutes
    last_wakeup_10min = millis();
    print_stats("Wakeup in loop() Core: ");

    // restart the main camera web server - it easily gets stuck
    Serial.println("Restarting CameraServer in case it got stuck");
    stopCameraServer();
    delay(500);
    startCameraServer();
    
  }

  delay(10);

}


/*****************************************************************************
 * 
 *  From here to the end is the contents of sendemail.cpp
 *  
 *  *** Uncomment the #include if the below is moved back into sendemail.cpp ***
 *  
 */

//#include "sendemail.h"

SendEmail::SendEmail(const String& host, const int port, const String& user, const String& passwd, const int timeout, const bool ssl) :
    host(host), port(port), user(user), passwd(passwd), timeout(timeout), ssl(ssl), client((ssl) ? new WiFiClientSecure() : new WiFiClient())
{

}

String SendEmail::readClient()
{
  String r = client->readStringUntil('\n');
  r.trim();
  while (client->available()) r += client->readString();
  return r;
}

// attachment is a full path filename to a file on the SD card
// set attachment to NULL to not include an attachment
bool SendEmail::send(const String& from, const String& to, const String& subject, const String& msg, const String& attachment)
{
  if (!host.length())
  {
    return false;
  }
  client->stop();
  client->setTimeout(timeout);
  // smtp connect
#ifdef DEBUG_EMAIL_PORT
  Serial.print("Connecting: ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
#endif
  if (!client->connect(host.c_str(), port))
  {
    return false;
  }
  String buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  // note: F(..) as used below puts the string in flash instead of RAM
  if (!buffer.startsWith(F("220")))
  {
    return false;
  }
  buffer = F("HELO ");
  buffer += client->localIP();
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  if (user.length()>0  && passwd.length()>0 )
  {
    buffer = F("AUTH LOGIN");
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("334")))
    {
      return false;
    }
    base64 b;
    buffer = user;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("334")))
    {
      return false;
    }
    buffer = this->passwd;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("235")))
    {
      return false;
    }
  }
  // smtp send mail
  buffer = F("MAIL FROM: ");
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  buffer = F("RCPT TO: ");
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  buffer = F("DATA");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("354")))
  {
    return false;
  }
  buffer = F("From: ");
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("To: ");
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("Subject: ");
  buffer += subject;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  // setup to send message body
  buffer = F("MIME-Version: 1.0\r\n");
  buffer += F("Content-Type: multipart/mixed; boundary=\"{BOUNDARY}\"\r\n\r\n");
  buffer += F("--{BOUNDARY}\r\n");
  buffer += F("Content-Type: text/plain\r\n");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = msg;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  if ( attachment ) {
    FILE *atfile = NULL;
    char * aname = (char * ) malloc( attachment.length() + 8 );
    char * pos = NULL;
    size_t flen;
    base64 b;
    // full path to file on SD card
    strcpy( aname, "/sdcard" );
    strcat( aname, attachment.c_str() );
    if ( atfile = fopen(aname, "r") ) {
      // get filename from attachment name
      pos = strrchr( aname, '/' );
      strcpy( aname, pos+1 );
      // attachment will be pulled from the file named
      buffer = F("\r\n--{BOUNDARY}\r\n");
      buffer += F("Content-Type: application/octet-stream\r\n");
      buffer += F("Content-Transfer-Encoding: base64\r\n");
      buffer += F("Content-Disposition: attachment;filename=\"");
      buffer += aname ;
      buffer += F("\"\r\n");
      client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
      // read data from file, base64 encode and send it
      // 3 binary bytes (57) becomes 4 base64 bytes (76)
      // plus CRLF is ideal for one line of MIME data
      // 570 byes will be read at a time and sent as ten lines of base64 data
      uint8_t * fdata = NULL;  
      fdata = (uint8_t*) malloc( 570 );
      // read data from file
      flen = fread( fdata, 1, 570, atfile );
      String buffer2 = "";
      int lc = 0;
      size_t bytecount = 0;
      while ( flen > 0 ) {
        while ( flen > 56 ) {
          // convert to base64 in 57 byte chunks
          buffer = b.encode( fdata+bytecount, 57 );
          buffer2 += buffer;
          // tack on CRLF
          buffer2 += "\r\n";
          bytecount += 57;
          flen -= 57;
        }
        if ( flen > 0 ) {
          // convert last set of byes to base64
          buffer = b.encode( fdata+bytecount, flen );
          buffer2 += buffer;
          // tack on CRLF
          buffer2 += "\r\n";
        }
        // send the lines in buffer
        client->println( buffer2 );
        buffer2 = "";
        bytecount = 0;
        delay(10);
        flen = fread( fdata, 1, 570, atfile );
      }
      fclose( atfile );
      free( fdata );
    } 
    free( aname );
  }
  buffer = F("\r\n--{BOUNDARY}--\r\n.");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("QUIT");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  return true;
}
