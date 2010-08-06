/*! \file glcd.c \brief Graphic LCD API functions. */
//*****************************************************************************
//
// File Name	: 'glcd.c'
// Title		: Graphic LCD API functions
// Author		: Pascal Stang - Copyright (C) 2002
// Date			: 5/30/2002
// Revised		: 5/30/2002
// Version		: 0.5
// Target MCU	: Atmel AVR
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef WIN32
// AVR specific includes
	#include <avr/io.h>
	#include <avr/pgmspace.h>
#endif

#include "glcd.h"

// include hardware support
#include "ks0108.h"
// include fonts
#include "font5x7.h"
#include "fontgr.h"

#include "util.h"




// graphic routines

// set dot
void glcdSetDot(u08 x, u08 y)
{
	unsigned char temp;

	//putstring("->addr "); uart_putw_dec(x);
	//putstring(", "); uart_putw_dec(y/8);
	//putstring_nl(")");

	glcdSetAddress(x, y/8);
	temp = glcdDataRead();	// dummy read
	temp = glcdDataRead();	// read back current value
	glcdSetAddress(x, y/8);
	glcdDataWrite(temp | (1 << (y % 8)));
	glcdStartLine(0);
}

// clear dot
void glcdClearDot(u08 x, u08 y)
{
	unsigned char temp;

	glcdSetAddress(x, y/8);
	temp = glcdDataRead();	// dummy read
	temp = glcdDataRead();	// read back current value
	glcdSetAddress(x, y/8);
	glcdDataWrite(temp & ~(1 << (y % 8)));

	glcdStartLine(0);
}

// draw line
void glcdLine(u08 x1, u08 y1, u08 x2, u08 y2)
{
};

// draw rectangle
void glcdRectangle(u08 x, u08 y, u08 w, u08 h)
{
  /*
  unsigned char j;

  for (j = 0; j < a; j++) {
   glcdSetDot(x, y + j);
   glcdSetDot(x + b - 1, y + j);
   }

  for (j = 0; j < b; j++)	{
    glcdSetDot(x + j, y);
    glcdSetDot(x + j, y + a - 1);
  }
*/
  // optimized!
  
  glcdFillRectangle(x, y, 1, h, ON);
  glcdFillRectangle(x+w-1, y, 1, h, ON);
  glcdFillRectangle(x, y, w, 1, ON);
  glcdFillRectangle(x, y+h-1, w, 1, ON);
}


// draw filled rectangle
void glcdFillRectangle(u08 x, u08 y, u08 a, u08 b, u08 color)
{
  unsigned char i, j, temp, bitsleft;
  signed char k;

  /*
// slow :(
  for (i = 0; i < a; i++) {
    if ( (x+i) > GLCD_XPIXELS )
      break;
    for (j = 0; j < b; j++) {
      if ( (y+j) > GLCD_YPIXELS )
	break;
      if (color == ON) {
	glcdSetDot(x + i, y + j);
      } else {
	glcdClearDot(x + i, y + j);
      }
    }
  }
  */

  // fast! :)
  /*
  for (i=0; i < a; i++) {
    glcdSetAddress(x+i, y/8);
    bitsleft = b;
    j = 0;
    // first byte is strange
    if ((bitsleft % 8) || (y%8)) {
      temp = glcdDataRead();	// dummy read
      temp = glcdDataRead();	// read back current value
      // not on a perfect boundary
      for (k=(y%8); k < 8; k++) {
	if (bitsleft) {
	  if (color == ON)
	    temp |= _BV(k);
	  else
	    temp &= ~_BV(k);
	  bitsleft--;
	}
      }
      glcdSetAddress(x+i, (y+j)/8);
      glcdDataWrite(temp);
      j = 8;
    }
    
    for(; bitsleft >= 8; bitsleft-=8) {
      glcdSetAddress(x+i, (y+j)/8);
      if (color == ON) {
	glcdDataWrite(0xFF);  
      } else {
	glcdDataWrite(0x00);
      }
      j+=8;
    }
    // do the remainder
    if (bitsleft) {
      glcdSetAddress(x+i, (y+j)/8);
      temp = glcdDataRead();	// dummy read
      temp = glcdDataRead();	// read back current value
      if (color == ON)
	temp |= (1 << ((y+b)%8)) - 1;
      else
	temp &= ~ ((1 << (y+b)%8) - 1);
      
      glcdSetAddress(x+i, (y+j)/8);
      glcdDataWrite(temp);
    }
  }
  */

  // fastest!
  if (y%8) {
    for (i=0; i<a; i++) {
      glcdSetAddress(x+i, y/8);
      temp = glcdDataRead();	// dummy read
      temp = glcdDataRead();	// read back current value
      // not on a perfect boundary
      for (k=(y%8); k < (y%8)+b && (k<8); k++) {
	if (color == ON)
	  temp |= _BV(k);
	else
	  temp &= ~_BV(k);
      }
      glcdSetAddress(x+i, y/8);
      glcdDataWrite(temp);
    } 
    // we did top section so remove it

    if (b > 8-(y%8))
      b -= 8-(y%8);
    else 
      b = 0;
    y -= (y%8);
    y+=8;
  }
  // skip to next section
  for (j=(y/8); j < (y+b)/8; j++) {
    glcdSetAddress(x, j);
    for (i=0; i<a; i++) {
      if (color == ON)
	glcdDataWrite(0xFF);
      else
	glcdDataWrite(0x00);
    }
  }
  b = b%8;
  // do remainder
  if (b) {
    for (i=0; i<a; i++) {
      glcdSetAddress(x+i, j);
      temp = glcdDataRead();	// dummy read
      temp = glcdDataRead();	// read back current value
      // not on a perfect boundary
      for (k=0; k < b; k++) {
	if (color == ON)
	  temp |= _BV(k);
	else
	  temp &= ~_BV(k);
      }
      glcdSetAddress(x+i, j);
      glcdDataWrite(temp);
    }
  }
  glcdStartLine(0);
}


// draw circle
void glcdCircle(u08 xcenter, u08 ycenter, u08 radius, u08 color)
{
  int tswitch, y, x = 0;
  unsigned char d;

  d = ycenter - xcenter;
  y = radius;
  tswitch = 3 - 2 * radius;
  while (x <= y) {
    if (color == ON) {
      glcdSetDot(xcenter + x, ycenter + y);     glcdSetDot(xcenter + x, ycenter - y);
      glcdSetDot(xcenter - x, ycenter + y);     glcdSetDot(xcenter - x, ycenter - y);
      glcdSetDot(ycenter + y - d, ycenter + x); glcdSetDot(ycenter + y - d, ycenter - x);
      glcdSetDot(ycenter - y - d, ycenter + x); glcdSetDot(ycenter - y - d, ycenter - x);
    } else {
      glcdClearDot(xcenter + x, ycenter + y);     glcdClearDot(xcenter + x, ycenter - y);
      glcdClearDot(xcenter - x, ycenter + y);     glcdClearDot(xcenter - x, ycenter - y);
      glcdClearDot(ycenter + y - d, ycenter + x); glcdClearDot(ycenter + y - d, ycenter - x);
      glcdClearDot(ycenter - y - d, ycenter + x); glcdClearDot(ycenter - y - d, ycenter - x);
    }
    if (tswitch < 0) tswitch += (4 * x + 6);
    else {
      tswitch += (4 * (x - y) + 10);
      y--;
    }
    x++;
  }
}


// draw circle
void glcdFillCircle(u08 xcenter, u08 ycenter, u08 radius, u08 color)
{

  int tswitch, y, x = 0;
  unsigned char d;

  d = ycenter - xcenter;
  y = radius;
  tswitch = 3 - 2 * radius;
  while (x <= y) {
    glcdFillRectangle(xcenter + x, ycenter - y, 1, y*2, color);
    glcdFillRectangle(xcenter - x, ycenter - y, 1, y*2, color);
    glcdFillRectangle(ycenter + y - d, ycenter - x, 1, x*2, color);
    glcdFillRectangle(ycenter - y - d, ycenter - x, 1, x*2, color);   
    if (tswitch < 0) tswitch += (4 * x + 6);
    else {
      tswitch += (4 * (x - y) + 10);
      y--;
    }
    x++;
  }
}

// text routines

// write a character at the current position
void glcdWriteChar(unsigned char c, uint8_t inverted)
{
	u08 i = 0;

	for(i=0; i<5; i++)
	{
	  if (inverted) {
	    glcdDataWrite(~ pgm_read_byte(&Font5x7[((c - 0x20) * 5) + i]));
	  } else {
	    glcdDataWrite(pgm_read_byte(&Font5x7[((c - 0x20) * 5) + i]));
	  }
	}

	// write a spacer line
	if (inverted) 
	  glcdDataWrite(0xFF);
	else 
	  glcdDataWrite(0x00);
	// unless we're at the end of the display
	//if(xx == 128)
	//	xx = 0;
	//else 
	//	glcdWriteData(0x00);

	//cbi(GLCD_Control, GLCD_CS1);
	//cbi(GLCD_Control, GLCD_CS2);
	glcdStartLine(0);
}

void glcdWriteCharGr(u08 grCharIdx)
{
	u08 idx;
	u08 grLength;
	u08 grStartIdx = 0;

	// get starting index of graphic bitmap
	for(idx=0; idx<grCharIdx; idx++)
	{
		// add this graphic's length to the startIdx
		// to get the startIdx of the next one
		// 2010-03-03 BUG Dataman/CRJONES There's a bug here:  Have to add 1 for the byte-cout.
		// grStartIdx += pgm_read_byte(FontGr+grStartIdx);
		grStartIdx += pgm_read_byte(FontGr+grStartIdx)+1;
		
	}
	grLength = pgm_read_byte(FontGr+grStartIdx);

	// write the lines of the desired graphic to the display
	for(idx=0; idx<grLength; idx++)
	{
		// write the line
		glcdDataWrite(pgm_read_byte(FontGr+(grStartIdx+1)+idx));
	}
}

void glcdPutStr(char *data, uint8_t inverted)
{
  while (*data) {
    glcdWriteChar(*data, inverted);
    data++;
  }
}
