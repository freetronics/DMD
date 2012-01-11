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
// DMD Library fonts
#include "DMD_Fonts.c"

/*--------------------------------------------------------------------------------------
 Setup and instantiation of DMD library
 Note this currently uses the SPI port for the fastest performance to the DMD, be
 careful of possible conflicts with other SPI port devices
--------------------------------------------------------------------------------------*/
DMD::DMD()
{
   uint16_t ui;

   // initialize the SPI port
   SPI.begin();                            // probably don't need this since it inits the port pins only, which we do just below with the appropriate DMD interface setup
   SPI.setBitOrder(MSBFIRST);              //
   SPI.setDataMode(SPI_MODE0);             // CPOL=0, CPHA=0
   SPI.setClockDivider(SPI_CLOCK_DIV128);  // system clock / 2 = 8MHz SPI CLK to shift registers
  
   digitalWrite( PIN_DMD_A,       LOW );   // 
   digitalWrite( PIN_DMD_B,       LOW );   // 
   digitalWrite( PIN_DMD_CLK,     LOW );   // 
   digitalWrite( PIN_DMD_SCLK,    LOW );   // 
   digitalWrite( PIN_DMD_R_DATA, HIGH );   // 
   digitalWrite( PIN_DMD_nOE,     LOW );   //
   
   pinMode( PIN_DMD_A,           OUTPUT ); //
   pinMode( PIN_DMD_B,           OUTPUT ); //
   pinMode( PIN_DMD_CLK,         OUTPUT ); //
   pinMode( PIN_DMD_SCLK,        OUTPUT ); //
   pinMode( PIN_DMD_R_DATA,      OUTPUT ); //
   pinMode( PIN_DMD_nOE,         OUTPUT ); //
   
   clearScreen( true );

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
void DMD::writePixel( byte bX, byte bY, byte bGraphicsMode, byte bPixel )
{
   unsigned int uiDMDRAMPointer;

   //set pointer to DMD RAM byte to be modified
   uiDMDRAMPointer = (bX >> 3) +   // /8 pixels per byte
                     (bY << 2);    // x4 bytes for X
   
   switch( bGraphicsMode )
   {
      case GRAPHICS_NORMAL:
      {
         if( bPixel == true )
            bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[ bX & 0x07 ]);   // zero bit is pixel on
         else
            bDMDScreenRAM[uiDMDRAMPointer] |=  (bPixelLookupTable[ bX & 0x07 ]);   // one bit is pixel off
         break;
      }
      case GRAPHICS_INVERSE:
      {
         if( bPixel == false )
            bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[ bX & 0x07 ]);   // zero bit is pixel on
         else
            bDMDScreenRAM[uiDMDRAMPointer] |=  (bPixelLookupTable[ bX & 0x07 ]);   // one bit is pixel off
         break;
      }
      case GRAPHICS_TOGGLE:
      {
         if( bPixel == true )
         {
            if( (bDMDScreenRAM[uiDMDRAMPointer] & (bPixelLookupTable[ bX & 0x07 ])) == 0 )
               bDMDScreenRAM[uiDMDRAMPointer] |=  (bPixelLookupTable[ bX & 0x07 ]);// one bit is pixel off
            else
               bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[ bX & 0x07 ]);// one bit is pixel off
         }
         break;
      }
      case GRAPHICS_OR:
      {
         //only set pixels on
         if( bPixel == true )
            bDMDScreenRAM[uiDMDRAMPointer] &= ~(bPixelLookupTable[ bX & 0x07 ]);   // zero bit is pixel on
         break;
      }
      case GRAPHICS_NOR:
      {
         //only clear on pixels
         if( ( bPixel == true ) && ( (bDMDScreenRAM[uiDMDRAMPointer] & (bPixelLookupTable[ bX & 0x07 ])) == 0 ) )
            bDMDScreenRAM[uiDMDRAMPointer] |=  (bPixelLookupTable[ bX & 0x07 ]);   // one bit is pixel off
         break;
      }
   }

}

/*--------------------------------------------------------------------------------------
 Draw a character with the 5 x 7 font table at the x and y location. bSet true is on, false is inverted
--------------------------------------------------------------------------------------*/
void DMD::drawCharacter_5x7( byte bX, byte bY, byte bChar, byte bGraphicsMode )
{
   byte x, y, w;
   
   w = FONT5X7WIDTH;
   for( y = 0 ; y < FONT5X7HEIGHT ; y++ )
   {
      for( x = 0 ; x < FONT5X7WIDTH ; x++ )
      {
         w--;
         if( ((( pgm_read_byte( &bDMDFont_5x7[((bChar-0x20)*FONT5X7HEIGHT)+y] ) ) >> w) & 0x01) != 0 )
         {
            // pixel is on in font table
            writePixel( bX+x, bY+y, bGraphicsMode, true );
         }
         else
         {
            // pixel is off in font table
            writePixel( bX+x, bY+y, bGraphicsMode, false );
         }
         if( w == 0 )
            w = FONT5X7WIDTH;
      }
   }
}

/*--------------------------------------------------------------------------------------
 Draw a character with the 6 x 16 font table at the x and y location. bSet true is on, false is inverted
 The 6 x 16 font table is designed to be good for large clock and 4 digit number displays with a colon in the centre
--------------------------------------------------------------------------------------*/
void DMD::drawCharacter_6x16( byte bX, byte bY, byte bChar, byte bGraphicsMode )
{
   byte x, y, w;
   
   w = FONT6X16WIDTH;
   for( y = 0 ; y < FONT6X16HEIGHT ; y++ )
   {
      for( x = 0 ; x < FONT6X16WIDTH ; x++ )
      {
         w--;
         if( ((( pgm_read_byte( &bDMDFont_6x16[((bChar-0x2B)*FONT6X16HEIGHT)+y] ) ) >> w) & 0x01) != 0 )
         {
            // pixel is on in font table
            writePixel( bX+x, bY+y, bGraphicsMode, true );
         }
         else
         {
            // pixel is off in font table
            writePixel( bX+x, bY+y, bGraphicsMode, false );
         }
         if( w == 0 )
            w = FONT6X16WIDTH;
      }
   }
}

/*--------------------------------------------------------------------------------------
 Clear the screen in DMD RAM
--------------------------------------------------------------------------------------*/
void DMD::clearScreen( byte bNormal )
{
   unsigned int ui;
   byte b;

   if( bNormal )
      b = 0xFF;   // clear all pixels
   else
      b = 0x00;   // set all pixels

   //initialise the DMD RAM with all pixels off (0xFF)
   for( ui = 0 ; ui < DMD_RAM_SIZE_BYTES ; ui++ )
   {
      bDMDScreenRAM[ui] = b;      // Note: 0x00 is all pixels ON, 0xFF is all pixels OFF.
   }
   
}

/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
--------------------------------------------------------------------------------------*/	
void DMD::drawLine(  byte x1, byte y1, byte x2, byte y2, byte bGraphicsMode )
{
   int dy = y2 - y1;
   int dx = x2 - x1;
   int stepx, stepy;

   if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
   if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
   dy <<= 1;                                      // dy is now 2*dy
   dx <<= 1;                                      // dx is now 2*dx

   writePixel( x1, y1, bGraphicsMode, true );
   if (dx > dy)
   {
      int fraction = dy - (dx >> 1);              // same as 2*dy - dx
      while (x1 != x2)
      {
         if (fraction >= 0)
         {
            y1 += stepy;
            fraction -= dx;                        // same as fraction -= 2*dx
         }
         x1 += stepx;
         fraction += dy;                           // same as fraction -= 2*dy
         writePixel( x1, y1, bGraphicsMode, true );
      }
   }
   else
   {
      int fraction = dx - (dy >> 1);
      while (y1 != y2)
      {
         if (fraction >= 0)
         {
            x1 += stepx;
            fraction -= dy;
         }
         y1 += stepy;
         fraction += dx;
         writePixel( x1, y1, bGraphicsMode, true );
      }
   }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
--------------------------------------------------------------------------------------*/	
void DMD::drawCircle( int xCenter, int yCenter, int radius, byte bGraphicsMode )
{
   int x = 0;
   int y = radius;
   int p = (5 - radius*4)/4;

   drawCircleSub( xCenter, yCenter, x, y, bGraphicsMode );
   while (x < y) 
   {
      x++;
      if (p < 0)
      {
         p += 2*x+1;
      }
      else 
      {
         y--;
         p += 2*(x-y)+1;
      }
      drawCircleSub( xCenter, yCenter, x, y, bGraphicsMode );
   }
}
void DMD::drawCircleSub( int cx, int cy, int x, int y, byte bGraphicsMode )
{
   
   if (x == 0)
   {
      writePixel( cx, cy + y, bGraphicsMode, true );
      writePixel( cx, cy - y, bGraphicsMode, true );
      writePixel( cx + y, cy, bGraphicsMode, true );
      writePixel( cx - y, cy, bGraphicsMode, true );
   }
   else if (x == y)
   {
      writePixel( cx + x, cy + y, bGraphicsMode, true );
      writePixel( cx - x, cy + y, bGraphicsMode, true );
      writePixel( cx + x, cy - y, bGraphicsMode, true );
      writePixel( cx - x, cy - y, bGraphicsMode, true );
   }
   else if (x < y)
   {
      writePixel( cx + x, cy + y, bGraphicsMode, true );
      writePixel( cx - x, cy + y, bGraphicsMode, true );
      writePixel( cx + x, cy - y, bGraphicsMode, true );
      writePixel( cx - x, cy - y, bGraphicsMode, true );
      writePixel( cx + y, cy + x, bGraphicsMode, true );
      writePixel( cx - y, cy + x, bGraphicsMode, true );
      writePixel( cx + y, cy - x, bGraphicsMode, true );
      writePixel( cx - y, cy - x, bGraphicsMode, true );
   }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/	
void DMD::drawBox( byte x1, byte y1, byte x2, byte y2, byte bGraphicsMode )
{
   drawLine( x1, y1, x2, y1, bGraphicsMode );
   drawLine( x2, y1, x2, y2, bGraphicsMode );
   drawLine( x2, y2, x1, y2, bGraphicsMode );
   drawLine( x1, y2, x1, y1, bGraphicsMode );
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/	
void DMD::drawFilledBox( byte x1, byte y1, byte x2, byte y2, byte bGraphicsMode )
{
   byte b;
   
   for( b = x1 ; b <= x2 ; b++ )
   {
       drawLine( b, y1, b, y2, bGraphicsMode );
   }
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
--------------------------------------------------------------------------------------*/   
void DMD::drawTestPattern( byte bPattern )
{
   unsigned int ui;

   for( ui = 0 ; ui < 512 ; ui++ )
   {
      //512 pixels
      switch( bPattern )
      {
         case PATTERN_ALT_0:      // every alternate pixel, first pixel on
         {
            if( (ui & 1) == 0 )
            {
               //even pixel
               if( (ui & 32) == 0 )
                  //even row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
               else
                  //odd row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
            }
            else
            {
               //odd pixel
               if( (ui & 32) == 0 )
                  //even row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
               else
                  //odd row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
            }
            break;
         }
         case PATTERN_ALT_1:      // every alternate pixel, first pixel off
         {
            if( (ui & 1) == 0 )
            {
               //even pixel
               if( (ui & 32) == 0 )
                  //even row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
               else
                  //odd row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
            }
            else
            {
               //odd pixel
               if( (ui & 32) == 0 )
                  //even row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
               else
                  //odd row
                  writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
            }
            break;
         }
         case PATTERN_STRIPE_0:   // vertical stripes, first stripe on
         {
            if( (ui & 1) == 0 )
               //even pixel
               writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
            else
               //odd pixel
               writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
            break;
         }
         case PATTERN_STRIPE_1:   // vertical stripes, first stripe off
         {
            if( (ui & 1) == 0 )
               //even pixel
               writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, false );
            else
               //odd pixel
               writePixel( (ui & 31), ((ui & ~31) / 32), GRAPHICS_NORMAL, true );
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
   //if( digitalRead( PIN_OTHER_SPI_nCS ) == HIGH )
   //{
      //SPI transfer 128 pixels to the display hardware shift registers
      SPI.transfer( bDMDScreenRAM[ bDMDByte    ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte-16 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte-32 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte-48 ] );
      
      SPI.transfer( bDMDScreenRAM[ bDMDByte+1    ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+1-16 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+1-32 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+1-48 ] );

      SPI.transfer( bDMDScreenRAM[ bDMDByte+2    ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+2-16 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+2-32 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+2-48 ] );

      SPI.transfer( bDMDScreenRAM[ bDMDByte+3    ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+3-16 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+3-32 ] );
      SPI.transfer( bDMDScreenRAM[ bDMDByte+3-48 ] );
      
      switch( bDMDByte )
      {
         case 48:   // row 1, 5, 9, 13 were clocked out
         {
            OE_DMD_ROWS_OFF();
            LATCH_DMD_SHIFT_REG_TO_OUTPUT();
            LIGHT_DMD_ROW_01_05_09_13();
            OE_DMD_ROWS_ON();
            bDMDByte = 52;   //do row 2 next time
            break;
         }
         case 52:   // row 2, 6, 10, 14 were clocked out
         {
            OE_DMD_ROWS_OFF();
            LATCH_DMD_SHIFT_REG_TO_OUTPUT();
            LIGHT_DMD_ROW_02_06_10_14();
            OE_DMD_ROWS_ON();
            bDMDByte = 56;   //do row 3 next time
            break;
         }
         case 56:   // row 3, 7, 11, 15 were clocked out
         {
            OE_DMD_ROWS_OFF();
            LATCH_DMD_SHIFT_REG_TO_OUTPUT();
            LIGHT_DMD_ROW_03_07_11_15();
            OE_DMD_ROWS_ON();
            bDMDByte = 60;   //do row 4 next time
            break;
         }
         case 60:   // row 4, 8, 12, 16 were clocked out
         {
            OE_DMD_ROWS_OFF();
            LATCH_DMD_SHIFT_REG_TO_OUTPUT();
            LIGHT_DMD_ROW_04_08_12_16();
            OE_DMD_ROWS_ON();
            bDMDByte = 48;   //back to row 1 next time
            break;
         }
      }
   //}
}
