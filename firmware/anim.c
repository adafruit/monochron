#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 
#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"
#include "font5x7.h"

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;

extern volatile uint8_t second_changed, minute_changed, hour_changed;

uint8_t redraw_time = 0;
uint8_t last_score_mode = 0;



void initanim(void) {
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));

}

void initdisplay(uint8_t inverted) {
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
}


void drawdisplay(void) {
  uint8_t inverted = 0;

  if (score_mode == SCORE_MODE_YEAR) {
    drawdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, 2 , inverted);
    drawdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, 0, inverted);
    drawdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, (date_y % 100)/10, inverted);
    drawdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, date_y % 10, inverted);
    drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, !inverted);
    drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, !inverted);
  } else if (score_mode == SCORE_MODE_DATE) {
    uint8_t left, right;
    if (region == REGION_US) {
      left = date_m;
      right = date_d;
    } else {
      left = date_d;
      right = date_m;
    }
    drawdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, left/10 , inverted);
    drawdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, left%10, inverted);
    drawdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, right/10, inverted);
    drawdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, right % 10, inverted);
    drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, !inverted);
    drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, !inverted);
  } else if ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)) {
    // draw time or alarm
    uint8_t left, right;
    if (score_mode == SCORE_MODE_ALARM) {
      left = alarm_h;
      right = alarm_m;
    } else {
      left = time_h;
      right = time_m;
    }
    if (TIME_12H) {
      left = (left + 23)%12 + 1;
    }

    // draw hours
    if (left >= 10) {
      drawdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, left/10, inverted);
    } else {
      drawdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, 8, !inverted);
    }
    drawdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, left%10, inverted);
    
    drawdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, right/10, inverted);
    drawdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, right%10, inverted);
    
    if (second_changed && time_s%2) {
      drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, 0);
      drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, 0);
    } else {
      drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*1/3, 1);
      drawdot(GLCD_XPIXELS/2, GLCD_YPIXELS*2/3, 1);
    }
  }
}


void step(void) {
}

void drawdot(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillCircle(x, y, DOTRADIUS, !inverted);
}

void drawdigit(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted) {
  switch (d) {
  case 0:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, inverted);
    drawsegment('f', x, y, inverted);

    drawsegment('g', x, y, !inverted);
    break;
  case 1:
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);

    drawsegment('a', x, y, !inverted);
    drawsegment('d', x, y, !inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, !inverted);
    drawsegment('g', x, y, !inverted);
    break;
  case 2:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, inverted);
    drawsegment('g', x, y, inverted);

    drawsegment('c', x, y, !inverted);
    drawsegment('f', x, y, !inverted);
    break;
  case 3:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, !inverted);
    drawsegment('g', x, y, inverted);
    break;
  case 4:
    drawsegment('a', x, y, !inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, !inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, inverted);
    drawsegment('g', x, y, inverted);
    break;
  case 5:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, !inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, inverted);
    drawsegment('g', x, y, inverted);
    break;
  case 6:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, !inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, inverted);
    drawsegment('f', x, y, inverted);
    drawsegment('g', x, y, inverted);
    break;
  case 7:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, !inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, !inverted);
    drawsegment('g', x, y, !inverted);
    break;
  case 8:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, inverted);
    drawsegment('f', x, y, inverted);
    drawsegment('g', x, y, inverted);
    break;
  case 9:
    drawsegment('a', x, y, inverted);
    drawsegment('b', x, y, inverted);
    drawsegment('c', x, y, inverted);
    drawsegment('d', x, y, inverted);
    drawsegment('e', x, y, !inverted);
    drawsegment('f', x, y, inverted);
    drawsegment('g', x, y, inverted);
    break;
  }
}
void drawsegment(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted) {
  switch (s) {
  case 'a':
    drawhseg(x+VSEGMENT_W/2+1, y, inverted);
    break;
  case 'b':
    drawvseg(x+HSEGMENT_W+2, y+HSEGMENT_H/2+2, inverted);
    break;
  case 'c':
    drawvseg(x+HSEGMENT_W+2, y+GLCD_YPIXELS/2+2, inverted);
    break;
  case 'd':
    drawhseg(x+VSEGMENT_W/2+1, GLCD_YPIXELS-HSEGMENT_H, inverted);
    break;
  case 'e':
    drawvseg(x, y+GLCD_YPIXELS/2+2, inverted);
    break;
  case 'f':
    drawvseg(x,y+HSEGMENT_H/2+2, inverted);
    break;
  case 'g':
    drawhseg(x+VSEGMENT_W/2+1, (GLCD_YPIXELS - HSEGMENT_H)/2, inverted);
    break;    
  }
}
void drawvseg(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillRectangle(x, y+2, VSEGMENT_W, VSEGMENT_H-4, ! inverted);

  glcdFillRectangle(x+1, y+1, VSEGMENT_W-2, 1, ! inverted);
  glcdFillRectangle(x+2, y, VSEGMENT_W-4, 1, ! inverted);

  glcdFillRectangle(x+1, y+VSEGMENT_H-2, VSEGMENT_W-2, 1, ! inverted);
  glcdFillRectangle(x+2, y+VSEGMENT_H-1, VSEGMENT_W-4, 1, ! inverted);
}

void drawhseg(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillRectangle(x+2, y, HSEGMENT_W-4, HSEGMENT_H, ! inverted);

  glcdFillRectangle(x+1, y+1, 1, HSEGMENT_H - 2, ! inverted);
  glcdFillRectangle(x, y+2, 1, HSEGMENT_H - 4, ! inverted);

  glcdFillRectangle(x+HSEGMENT_W-2, y+1, 1, HSEGMENT_H - 2, ! inverted);
  glcdFillRectangle(x+HSEGMENT_W-1, y+2, 1, HSEGMENT_H - 4, ! inverted);
}

// 8 pixels high
static unsigned char __attribute__ ((progmem)) BigFont[] = {
	0xFF, 0x81, 0x81, 0xFF,// 0
	0x00, 0x00, 0x00, 0xFF,// 1
	0x9F, 0x91, 0x91, 0xF1,// 2
	0x91, 0x91, 0x91, 0xFF,// 3
	0xF0, 0x10, 0x10, 0xFF,// 4
	0xF1, 0x91, 0x91, 0x9F,// 5
	0xFF, 0x91, 0x91, 0x9F,// 6
	0x80, 0x80, 0x80, 0xFF,// 7
	0xFF, 0x91, 0x91, 0xFF,// 8 
	0xF1, 0x91, 0x91, 0xFF,// 9
	0x00, 0x00, 0x00, 0x00,// SPACE
};

static unsigned char __attribute__ ((progmem)) MonthText[] = {
	0,0,0,
	'J','A','N',
	'F','E','B',
	'M','A','R',
	'A','P','R',
	'M','A','Y',
	'J','U','N',
	'J','U','L',
	'A','U','G',
	'S','E','P',
	'O','C','T',
	'N','O','V',
	'D','E','C',
};

static unsigned char __attribute__ ((progmem)) DOWText[] = {
	'S','U','N',
	'M','O','N',
	'T','U','E',
	'W','E','D',
	'T','H','U',
	'F','R','I',
	'S','A','T',
};

uint8_t dotw(uint8_t mon, uint8_t day, uint8_t yr)
{
  uint16_t month, year;

    // Calculate day of the week
    
    month = mon;
    year = 2000 + yr;
    if (mon < 3)  {
      month += 12;
      year -= 1;
    }
    return (day + (2 * month) + (6 * (month+1)/10) + year + (year/4) - (year/100) + (year/400) + 1) % 7;
}
