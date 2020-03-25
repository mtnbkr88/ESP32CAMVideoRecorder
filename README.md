# ESP32-CAM_Video_Recorder

ESP32-CAM Video Recorder

03/25/2020 Ed Williams 

This version of the ESP32-CAM Video Recorder is built on the work of many other listed in the sketch.
It has been hugely modified to be a fairly complete web camera server with the following capabilities:
  - Record AVI video and peak, stream, download or delete the saved videos.
  - Take JPG pictures and view, download or delete the saved pictures.
  - Up to 250 videos and 250 pictures can be saved on the SD card. Beyond that as more
    are saved the oldest are deleted.
  - Removed the ability to change the size, frame rate and quality. They are permanently 
    set to size 640x480, 125ms and quality 10.
  - Recording times can be set from 10 to 300 seconds
  - Motion detection is available if a PIR motion sensor is connected to GPIO3 (U0RXD)
    (after the firmware is uploaded). 
  - Time action trigger.
  - Motion detection and time can trigger the following actions:
    - Take a picture.
    - Take a picture and email it.
    - Make a video.
    - Make a video and send an email with name of video.   
  - Motion detection is checked every second and starts checking again one minute
    after a trigger, so if you don't want any gaps in motion detected videos, set 
    recording time to 60 seconds.
  - Normal web traffic is on port 80, streaming comes from port 90. If accessing from 
    outside the local network, forward port X to 80 and port X+10 to port 90.
  - OTA firmware updates on port 90.
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
then connect to base of transistor. Connect the emitter of the transistor to GND. See
ECVR_Schematic.jpg for a picture of the wiring.

Used pinMode(GPIO_NUM_3, INPUT_PULLUP) in the setup to pull the pin high. Now when 
motion is detected, GPIO3 will be pulled low, otherwise it will be high.

ECVR_Main_Screen.png shows the main screen for the ESP32-CAM Video Recorder web site.

ECVR_Parts.jpg shows the parts used.

The files ECVR_Box.stl, ECVR_Box_Cover.stl and ECVR_Box_Base.stl are stl files for
3D printing an enclosure to hold everything.

ECVR_Enclosure.jpg show the completed project.

If a motion detect or time trigger video is being recorded and send an email is enabled,
the email sometimes does not get sent. The socket for connecting to the email server fails.
I haven't figured this one out yet.

The Default Partition Scheme must be used for compiling so OTA can work.
