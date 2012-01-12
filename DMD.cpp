/*--------------------------------------------------------------------------------------

 DMD.cpp - Function and support library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 Copyright (C) 2011 Marc Alexander (info <at> freetronics <dot> com)

 Note that the DMD library uses the SPI port for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD is being written to.

 ---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

--------------------------------------------------------------------------------------*/
#include "DMD.h"

/*--------------------------------------------------------------------------------------
 Setup and instantiation of DMD library
 Note this currently uses the SPI port for the fastest performance to the DMD, be
 careful of possible conflicts with other SPI port devices
--------------------------------------------------------------------------------------*/
DMD::DMD(byte panelsWide, byte panelsHigh)
{
    uint16_t ui;
    DisplaysWide=panelsWide;
    DisplaysHigh=panelsHigh;
    DisplaysTotal=DisplaysWide*DisplaysHigh;
    bDMDScreenRAM = (byte *) malloc(DisplaysTotal*DMD_RAM_SIZE_BYTES);

    // initialize the SPI port
    SPI.begin();		// probably don't need this since it inits the port pins only, which we do just below with the appropriate DMD interface setup
    SPI.setBitOrder(MSBFIRST);	//
    SPI.setDataMode(SPI_MODE0);	// CPOL=0, CPHA=0
    SPI.setClockDivider(SPI_CLOCK_DIV128);	// system clock / 2 = 8MHz SPI CLK to shift registers

    digitalWrite(PIN_DMD_A, LOW);	// 
    digitalWrite(PIN_DMD_B, LOW);	// 
    digitalWrite(PIN_DMD_CLK, LOW);	// 
    digitalWrite(PIN_DMD_SCLK, LOW);	// 
    digitalWrite(PIN_DMD_R_DATA, HIGH);	// 
    digitalWrite(PIN_DMD_nOE, LOW);	//

    pinMode(PIN_DMD_A, OUTPUT);	//
    pinMode(PIN_DMD_B, OUTPUT);	//
    pinMode(PIN_DMD_CLK, OUTPUT);	//
    pinMode(PIN_DMD_SCLK, OUTPUT);	//
    pinMode(PIN_DMD_R_DATA, OUTPUT);	//
    pinMode(PIN_DMD_nOE, OUTPUT);	//

    clearScreen(true);

    // init the scan line/ram pointer to the required start point
    bDMDByte = 48;
}

//DMD::~DMD()
//{
//   // nothing needed here
//}

/*--------------------------------------------------------------------------------------
 Set or clear a pixel at the x and y location (0,0 is the top left corner)
--------------------------------------------------------------------------------------*/
void
 DMD::writePixel(int bX, int bY, byte bGraphicsMode, byte bPixel)
{
    unsigned int uiDMDRAMPointer;
    unsigned int panelX=bX/DMD_PIXELS_ACROSS;
    unsigned int panelY=bY/DMD_PIXELS_DOWN;

    if (!(bGraphicsMode & GRAPHICS_UNBOUNDED)) {
        if (bX<0 || bY<0) return;
	    if (bX > (DMD_PIXELS_ACROSS*DisplaysWide) || bY > (DMD_PIXELS_DOWN*DisplaysHigh)) {
	        return;
	    }
    } else {
	    bGraphicsMode = bGraphicsMode ^ GRAPHICS_UNBOUNDED;
    }
    bX=bX % DMD_PIXELS_ACROSS;
    bY=bY % DMD_PIXELS_DOWN;
    //set pointer to DMD RAM byte to be modified
    uiDMDRAMPointer = (panelX * DMD_RAM_SIZE_BYTES) +
                      (panelY * DisplaysWide * DMD_RAM_SIZE_BYTES) +
                      (bX >> 3) +	// /8 pixels per byte
	                  (bY << 2);	// x4 bytes for X

    switch (bGraphicsMode) {
    case GRAPHICS_NORMAL:
	{
	    if (bPixel == true)
		bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[bX & 0x07]);	// zero bit is pixel on
	    else
		bDMDScreenRAM[uiDMDRAMPointer] |= (bPixelLookupTable[bX & 0x07]);	// one bit is pixel off
	    break;
	}
    case GRAPHICS_INVERSE:
	{
	    if (bPixel == false)
		bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[bX & 0x07]);	// zero bit is pixel on
	    else
		bDMDScreenRAM[uiDMDRAMPointer] |= (bPixelLookupTable[bX & 0x07]);	// one bit is pixel off
	    break;
	}
    case GRAPHICS_TOGGLE:
	{
	    if (bPixel == true) {
		if ((bDMDScreenRAM[uiDMDRAMPointer] &
		     (bPixelLookupTable[bX & 0x07])) == 0)
		    bDMDScreenRAM[uiDMDRAMPointer] |= (bPixelLookupTable[bX & 0x07]);	// one bit is pixel off
		else
		    bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[bX & 0x07]);	// one bit is pixel off
	    }
	    break;
	}
    case GRAPHICS_OR:
	{
	    //only set pixels on
	    if (bPixel == true)
		bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[bX & 0x07]);	// zero bit is pixel on
	    break;
	}
    case GRAPHICS_NOR:
	{
	    //only clear on pixels
	    if ((bPixel == true)
		&&
		((bDMDScreenRAM[uiDMDRAMPointer] &
		  (bPixelLookupTable[bX & 0x07]))
		 == 0))
		bDMDScreenRAM[uiDMDRAMPointer] |= (bPixelLookupTable[bX & 0x07]);	// one bit is pixel off
	    break;
	}
    }

}

void DMD::drawString(int bX, int bY, const char *bChars, byte length,
		     byte bGraphicsMode)
{
    if (bX > DMD_PIXELS_ACROSS || bY > DMD_PIXELS_DOWN)
	return;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);

    int maxLetters = 0;
    int cX = bX;
    int cY = bY;
    int strWidth = 0;

    for (int i = 0; i < length; i++) {
	int charWide = charWidth(bChars[i]);
	if (charWide > 0) {
	    strWidth += charWide + 1;
	    if (cX >= -charWide)
		this->drawChar(cX, cY, bChars[i], bGraphicsMode);
	    cX = bX + strWidth;
	    cY = bY;
	    this->drawLine(cX - 1, cY, cX - 1, cY + height,
			   GRAPHICS_INVERSE);
	}
	if (cX >= DMD_PIXELS_ACROSS || cY >= DMD_PIXELS_DOWN)
	    return;
    }
}

void DMD::drawMarquee(const char *bChars, byte length, byte top)
{
    marqueeWidth = 0;
    for (int i = 0; i < length; i++) {
	marqueeText[i] = bChars[i];
	marqueeWidth += charWidth(bChars[i]) + 1;
    }
    marqueeText[length] = '\0';
    marqueeOffset = DMD_PIXELS_ACROSS;
    marqueeLength = length;
    marqueeTop = top;
}

boolean DMD::stepMarquee(int amount)
{
    boolean ret=false;
    marqueeOffset -= amount;
    if (marqueeOffset < -(marqueeWidth)) {
	    marqueeOffset = DMD_PIXELS_ACROSS;
	    clearScreen(true);
        ret=true;
    }
    drawString(marqueeOffset, marqueeTop, marqueeText, marqueeLength,
	       GRAPHICS_NORMAL);
    return ret;
}


/*--------------------------------------------------------------------------------------
 Clear the screen in DMD RAM
--------------------------------------------------------------------------------------*/
void DMD::clearScreen(byte bNormal)
{
    byte b;

    if (bNormal)
	    b = 0xFF;		// clear all pixels
    else
	    b = 0x00;		// set all pixels

    //initialise the DMD RAM 
    memset(bDMDScreenRAM,b,DMD_RAM_SIZE_BYTES*DisplaysTotal);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
--------------------------------------------------------------------------------------*/
void DMD::drawLine(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;

    if (dy < 0) {
	    dy = -dy;
	    stepy = -1;
    } else {
	    stepy = 1;
    }
    if (dx < 0) {
	    dx = -dx;
	    stepx = -1;
    } else {
	    stepx = 1;
    }
    dy <<= 1;			// dy is now 2*dy
    dx <<= 1;			// dx is now 2*dx

    writePixel(x1, y1, bGraphicsMode, true);
    if (dx > dy) {
	    int fraction = dy - (dx >> 1);	// same as 2*dy - dx
	    while (x1 != x2) {
	        if (fraction >= 0) {
		        y1 += stepy;
		        fraction -= dx;	// same as fraction -= 2*dx
	        }
	        x1 += stepx;
	        fraction += dy;	// same as fraction -= 2*dy
	        writePixel(x1, y1, bGraphicsMode, true);
	    }
    } else {
	    int fraction = dx - (dy >> 1);
	    while (y1 != y2) {
	        if (fraction >= 0) {
		        x1 += stepx;
		        fraction -= dy;
	        }
	        y1 += stepy;
	        fraction += dx;
	        writePixel(x1, y1, bGraphicsMode, true);
	    }
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
--------------------------------------------------------------------------------------*/
void DMD::drawCircle(int xCenter, int yCenter, int radius,
		     byte bGraphicsMode)
{
    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;

    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    while (x < y) {
	    x++;
	    if (p < 0) {
	        p += 2 * x + 1;
	    } else {
	        y--;
	        p += 2 * (x - y) + 1;
	    }
	    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    }
}

void DMD::drawCircleSub(int cx, int cy, int x, int y, byte bGraphicsMode)
{

    if (x == 0) {
	    writePixel(cx, cy + y, bGraphicsMode, true);
	    writePixel(cx, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy, bGraphicsMode, true);
	    writePixel(cx - y, cy, bGraphicsMode, true);
    } else if (x == y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
    } else if (x < y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy + x, bGraphicsMode, true);
	    writePixel(cx - y, cy + x, bGraphicsMode, true);
	    writePixel(cx + y, cy - x, bGraphicsMode, true);
	    writePixel(cx - y, cy - x, bGraphicsMode, true);
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD::drawBox(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    drawLine(x1, y1, x2, y1, bGraphicsMode);
    drawLine(x2, y1, x2, y2, bGraphicsMode);
    drawLine(x2, y2, x1, y2, bGraphicsMode);
    drawLine(x1, y2, x1, y1, bGraphicsMode);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD::drawFilledBox(int x1, int y1, int x2, int y2,
			byte bGraphicsMode)
{
    for (byte b = x1; b <= x2; b++) {
	    drawLine(b, y1, b, y2, bGraphicsMode);
    }
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
--------------------------------------------------------------------------------------*/
void DMD::drawTestPattern(byte bPattern)
{
    unsigned int ui;

    for (ui = 0; ui < 512; ui++) {
	//512 pixels
	switch (bPattern) {
	case PATTERN_ALT_0:	// every alternate pixel, first pixel on
	    {
		if ((ui & 1) == 0) {
		    //even pixel
		    if ((ui & 32) == 0)
			//even row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, true);
		    else
			//odd row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, false);
		} else {
		    //odd pixel
		    if ((ui & 32) == 0)
			//even row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, false);
		    else
			//odd row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, true);
		}
		break;
	    }
	case PATTERN_ALT_1:	// every alternate pixel, first pixel off
	    {
		if ((ui & 1) == 0) {
		    //even pixel
		    if ((ui & 32) == 0)
			//even row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, false);
		    else
			//odd row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, true);
		} else {
		    //odd pixel
		    if ((ui & 32) == 0)
			//even row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, true);
		    else
			//odd row
			writePixel((ui & 31), ((ui & ~31) / 32),
				   GRAPHICS_NORMAL, false);
		}
		break;
	    }
	case PATTERN_STRIPE_0:	// vertical stripes, first stripe on
	    {
		if ((ui & 1) == 0)
		    //even pixel
		    writePixel((ui & 31), ((ui & ~31) / 32),
			       GRAPHICS_NORMAL, true);
		else
		    //odd pixel
		    writePixel((ui & 31), ((ui & ~31) / 32),
			       GRAPHICS_NORMAL, false);
		break;
	    }
	case PATTERN_STRIPE_1:	// vertical stripes, first stripe off
	    {
		if ((ui & 1) == 0)
		    //even pixel
		    writePixel((ui & 31), ((ui & ~31) / 32),
			       GRAPHICS_NORMAL, false);
		else
		    //odd pixel
		    writePixel((ui & 31), ((ui & ~31) / 32),
			       GRAPHICS_NORMAL, true);
		break;
	    }
	}
    }
}

/*--------------------------------------------------------------------------------------
 Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
 Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
 Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
--------------------------------------------------------------------------------------*/
void DMD::scanDisplayBySPI()
{
    //if PIN_OTHER_SPI_nCS is in use during a DMD scan request then scanDisplayBySPI() will exit without conflict! (and skip that scan)
    if( digitalRead( PIN_OTHER_SPI_nCS ) == HIGH )
    {
        //SPI transfer 128 pixels to the display hardware shift registers
        for (byte panel=0;panel<DisplaysTotal;panel++){
            for (byte i=0;i<4;i++) {
                SPI.transfer(bDMDScreenRAM[bDMDByte + i]);
                SPI.transfer(bDMDScreenRAM[bDMDByte + i - 16]);
                SPI.transfer(bDMDScreenRAM[bDMDByte + i - 32]);
                SPI.transfer(bDMDScreenRAM[bDMDByte + i - 48]);
            }
        }

        OE_DMD_ROWS_OFF();
        LATCH_DMD_SHIFT_REG_TO_OUTPUT();
        switch (bDMDByte) {
        case 48:			// row 1, 5, 9, 13 were clocked out
            LIGHT_DMD_ROW_01_05_09_13();
            bDMDByte=52;
            break;
        case 52:			// row 2, 6, 10, 14 were clocked out
            LIGHT_DMD_ROW_02_06_10_14();
            bDMDByte=56;
            break;
        case 56:			// row 3, 7, 11, 15 were clocked out
            LIGHT_DMD_ROW_03_07_11_15();
            bDMDByte=60;
            break;
        case 60:			// row 4, 8, 12, 16 were clocked out
            LIGHT_DMD_ROW_04_08_12_16();
            bDMDByte=48;
            break;
        }
        OE_DMD_ROWS_ON();
    }
}

void DMD::selectFont(const uint8_t * font)
{
    this->Font = font;
}


int DMD::drawChar(int bX, int bY, const char letter, byte bGraphicsMode)
{
    char c = letter;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (c == ' ') {
	int charWide = charWidth('n');
	this->drawFilledBox(bX, bY, bX + charWide, bY + height,
			    GRAPHICS_INVERSE);
	return 0;
    }
    uint8_t width = 0;
    uint8_t bytes = (height + 7) / 8;

    uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
    uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

    uint16_t index = 0;

    if (c < firstChar || c >= (firstChar + charCount)) {
	return 1;
    }
    c -= firstChar;

    if (pgm_read_byte(this->Font + FONT_LENGTH) == 0
	    && pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0) {
	    // zero length is flag indicating fixed width font (array does not contain width data entries)
	    width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
	    index = c * bytes * width + FONT_WIDTH_TABLE;
    } else {
	    // variable width font, read width data, to get the index
	    for (uint8_t i = 0; i < c; i++) {
	        index += pgm_read_byte(this->Font + FONT_WIDTH_TABLE + i);
	    }
	    index = index * bytes + charCount + FONT_WIDTH_TABLE;
	    width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
    }
    if (bX > DMD_PIXELS_ACROSS || bY > DMD_PIXELS_DOWN || bX < -width
	|| bY < -height)
	return -1;
    // last but not least, draw the character
    for (uint8_t j = 0; j < width; j++) {
	for (uint8_t i = bytes - 1; i < 254; i--) {
	    uint8_t data =
		pgm_read_byte(this->Font + index + j + (i * width));
	    for (uint8_t k = 0; k < 8; k++) {
		int offset = (i * 8) + k;
		if ((i == bytes - 1) && bytes > 1) {
		    offset = offset - (8 - (height - (i * 8)));
		    if (offset < i * 8) {
			offset = 255;
		    }
		}
		if (offset <= height) {
		    if (data & (1 << k)) {
			writePixel(bX + j, bY + offset, bGraphicsMode,
				   true);
		    } else {
			writePixel(bX + j, bY + offset, bGraphicsMode,
				   false);
		    }
		}
	    }
	}
    }
    return 0;
}

int DMD::charWidth(const char letter)
{
    char c = letter;
    if (c == ' ')
	c = 'n';
    uint8_t width = 0;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    uint8_t bytes = (height + 7) / 8;

    uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
    uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

    uint16_t index = 0;

    if (c < firstChar || c >= (firstChar + charCount)) {
	return 0;
    }
    c -= firstChar;

    if (pgm_read_byte(this->Font + FONT_LENGTH) == 0
	&& pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0) {
	// zero length is flag indicating fixed width font (array does not contain width data entries)
	width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
	index = c * bytes * width + FONT_WIDTH_TABLE;
    } else {
	// variable width font, read width data, to get the index
	for (uint8_t i = 0; i < c; i++) {
	    index += pgm_read_byte(this->Font + FONT_WIDTH_TABLE + i);
	}
	index = index * bytes + charCount + FONT_WIDTH_TABLE;
	width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
    }
    return width;
}
