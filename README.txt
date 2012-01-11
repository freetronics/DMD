DMD library
--------------
Marc Alexander, Freetronics
Email: info (at) freetronics.com
URL:   http://www.freetronics.com/dmd-library

A library for driving the Freetronics 512 pixel dot matrix LED display "DMD", a 32 x 16 layout.

Includes:
- High speed display connection straight to SPI port and pins.
- A full 5 x 7 pixel font set and character routines for display.
- A numerical and symbol 6 x 16 font set with a colon especially for clocks and other fun large displays.
- Special graphics modes: Normal, Inverse, Toggle, OR and NOR!
- Clear screen with all pixels off or on.
- Point to point line drawing.
- Circle drawing.
- Box (rectangle) drawing, border and filled versions.
- Test pattern generation.

For the DMD panel see: http://www.freetronics.com/dmd

USAGE NOTES
-----------

- Place the DMD library folder into the "arduino/libraries/" folder of your Arduino installation.
- Get the TimerOne library from here: http://code.google.com/p/arduino-timerone/downloads/list
  or download the local copy from the DMD library page (which may be older but was used for this creation)
  and place the TimerOne library folder into the "arduino/libraries/" folder of your Arduino installation.
- Restart the IDE.
- In the Arduino IDE, you can open File > Examples > DMD > dmd_demo, or dmd_clock_readout, and get it
  running straight away!

* The DMD comes with a pre-made data cable and DMDCON connector board so you can plug-and-play straight
  into any regular size Arduino Board (Uno, Freetronics Eleven, EtherTen, USBDroid, etc)
  
* Please note that the Mega boards have SPI on different pins, so this library does not currently support
  the DMDCON connector board for direct connection to Mega's, please jumper the DMDCON pins to the
  matching SPI pins on the other header on the Mega boards.

PROJECT HOME
------------

http://www.freetronics.com/dmd-library
