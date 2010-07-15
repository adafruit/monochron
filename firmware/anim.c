/* ***************************************************************************
// anim.c - the main animation and drawing code for MONOCHRON
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

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
#include "ks0108conf.h"
#include "glcd.h"
#include "font5x7.h"

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;

extern volatile uint8_t blinkingdots;
uint8_t digitsmutex = 0;

extern volatile uint8_t second_changed, minute_changed, hour_changed;

uint8_t redraw_time = 0;
uint8_t last_score_mode = 0;

extern const uint8_t zero_seg[];
extern const uint8_t one_seg[];

// special pointer for reading from ROM memory
PGM_P zero_p PROGMEM = zero_seg;
PGM_P one_p PROGMEM = one_seg;


uint8_t steps = 0;


uint8_t old_digits[4] = {0};
uint8_t new_digits[4] = {0};

void initanim(void) {
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));

}

void initdisplay(uint8_t inverted) {
  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);

  steps = 0;

  uint8_t newleft, newright;
  newleft = time_h;
  newright = time_m;
  if (time_format == TIME_12H) {
    newleft = (newleft + 23)%12 + 1;
  }
  
  old_digits[0] = old_digits[1] = old_digits[2] = old_digits[3] = 255;
  new_digits[3] = newright % 10;	
  new_digits[2] = newright / 10;
  new_digits[1] = newleft%10;
  new_digits[0] = newleft/10;
  
  while (steps++ <= MAX_STEPS) {
    transitiondigit(DISPLAY_M1_X, DISPLAY_TIME_Y,
		    old_digits[3], new_digits[3], inverted);
    transitiondigit(DISPLAY_M10_X, DISPLAY_TIME_Y,
		    old_digits[2], new_digits[2], inverted);
    transitiondigit(DISPLAY_H1_X, DISPLAY_TIME_Y,
		    old_digits[1], new_digits[1], inverted);
    transitiondigit(DISPLAY_H10_X, DISPLAY_TIME_Y,
		    old_digits[0], new_digits[0], inverted);
  }
  putstring("done init");
  for (uint8_t i=0; i<4; i++) {
    old_digits[i] = new_digits[i];
  }

  drawdots(inverted);

  steps = 0;
}

void drawdot(uint8_t x, uint8_t y, uint8_t inverted) {
  glcdFillRectangle(x, y-3, 1, 6, !inverted);
  glcdFillRectangle(x-1, y-3, 1, 6, !inverted);

  glcdFillRectangle(x-2, y-2, 1, 4, !inverted);
  glcdFillRectangle(x+1, y-2, 1, 4, !inverted);

  glcdFillRectangle(x-3, y-1, 1, 2, !inverted);
  glcdFillRectangle(x+2, y-1, 1, 2, !inverted);

  //glcdFillCircle(x, y, DOTRADIUS-1, !inverted);
}

void drawdots(uint8_t inverted) {
  drawdot(64, 20, inverted);
  drawdot(64, 64-DOTRADIUS-2, inverted);
}




void drawdisplay(uint8_t inverted) {
  static uint8_t last_mode = 0;
  static uint8_t lastinverted = 0;

  /*
  putstring("**** ");
  uart_putw_dec(time_h);
  uart_putchar(':');
  uart_putw_dec(time_m);
  uart_putchar(':');
  uart_putw_dec(time_s);
  putstring_nl("****");
  */

  if ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)) {
    // putstring_nl("time");
    if (last_mode != score_mode) {
      if (! digitsmutex) {
	for (uint8_t i=0; i<4; i++) {
	  old_digits[i] = new_digits[i];
	}
	uint8_t newleft, newright;
	if (score_mode == SCORE_MODE_TIME) {
	  newleft = time_h;
	  newright = time_m;
	} else {
	  newleft = alarm_h;
	  newright = alarm_m;
	}
	if (time_format == TIME_12H) {
	  newleft = (newleft + 23)%12 + 1;
	}
	new_digits[3] = newright % 10;	
	new_digits[2] = newright / 10;
	new_digits[0] = newleft/10;
	new_digits[1] = newleft%10;

	drawdots(inverted);

	last_mode = score_mode;
	digitsmutex++;
      }
    }

    if (minute_changed || hour_changed) {
      //putstring_nl("changed");
      if (! digitsmutex) {
	digitsmutex++;
	
	old_digits[3] = new_digits[3];
	new_digits[3] = time_m % 10;
	old_digits[2] = new_digits[2];
	new_digits[2] = time_m / 10;
	  
	if (hour_changed) {
	  uint8_t newleft = time_h;
	  if (time_format == TIME_12H) {
	    newleft = (time_h + 23)%12 + 1;
	  }
	  old_digits[0] = new_digits[0];
	  old_digits[1] = new_digits[1];
	  new_digits[0] = newleft/10;
	  new_digits[1] = newleft%10;
	  }
	  minute_changed = hour_changed = 0;
	}
    }

    if ((lastinverted != inverted) && !digitsmutex) {
      glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);

      drawdots(inverted);

      drawdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, new_digits[3], inverted);
      drawdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, new_digits[2], inverted);
      drawdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, new_digits[1], inverted);
      drawdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, new_digits[0], inverted);
    }
   
  } else if (score_mode == SCORE_MODE_DATE) {
    if (last_mode != score_mode) {
      putstring_nl("date!");
      if (! digitsmutex) {
	digitsmutex++;
	last_mode = score_mode;

	for (uint8_t i=0; i<4; i++) {
	  old_digits[i] = new_digits[i];
	}
	uint8_t left, right;
	if (region == REGION_US) {
	  left = date_m;
	  right = date_d;
	} else {
	  left = date_d;
	  right = date_m;
	}
	new_digits[0] = left / 10;
	new_digits[1] = left % 10;
	new_digits[2] = right / 10;
	new_digits[3] = right % 10;

	drawdots(!inverted);
      }
    }
  } else if (score_mode == SCORE_MODE_YEAR) {
    if (last_mode != score_mode) {
      putstring_nl("year!");
      if (! digitsmutex) {
	digitsmutex++;
	putstring_nl("draw");
	last_mode = score_mode;

	for (uint8_t i=0; i<4; i++) {
	  old_digits[i] = new_digits[i];
	}

	new_digits[0] = 2;
	new_digits[1] = 0;
	new_digits[2] = date_y / 10;
	new_digits[3] = date_y % 10;

	glcdFillRectangle(60, 35, 7, 5, inverted);
      }
    }
  }

  /*
  for (uint8_t i=0; i<4; i++) {
    if (old_digits[i] != new_digits[i]) {
      uart_putw_dec(old_digits[0]);
      uart_putw_dec(old_digits[1]);
      uart_putc(':');
      uart_putw_dec(old_digits[2]);
      uart_putw_dec(old_digits[3]);
      putstring(" -> ");
      uart_putw_dec(new_digits[0]);
      uart_putw_dec(new_digits[1]);
      uart_putc(':');
      uart_putw_dec(new_digits[2]);
      uart_putw_dec(new_digits[3]);
      putstring_nl("");
      break;
    }
  }
  */

  if (old_digits[3] != new_digits[3]) {
    transitiondigit(DISPLAY_M1_X, DISPLAY_TIME_Y,
		    old_digits[3], new_digits[3], inverted);
  }
  if (old_digits[2] != new_digits[2]) {
    transitiondigit(DISPLAY_M10_X, DISPLAY_TIME_Y,
		    old_digits[2], new_digits[2], inverted);
  }
  if (old_digits[1] != new_digits[1]) {
    transitiondigit(DISPLAY_H1_X, DISPLAY_TIME_Y,
		    old_digits[1], new_digits[1], inverted);
  }
  if (old_digits[0] != new_digits[0]) {
    transitiondigit(DISPLAY_H10_X, DISPLAY_TIME_Y,
		    old_digits[0], new_digits[0], inverted);
  }

  if (digitsmutex) {
    steps++;
    if (steps > MAX_STEPS) {
      steps = 0;
      old_digits[3] = new_digits[3];
      old_digits[2] = new_digits[2];
      old_digits[1] = new_digits[1];
      old_digits[0] = new_digits[0];
      digitsmutex--;

      if (score_mode == SCORE_MODE_DATE) {
	glcdFillRectangle(60, 35, 7, 5, !inverted);
      } else if (score_mode == SCORE_MODE_YEAR) {

      } else if (score_mode == SCORE_MODE_TIME) {
	//drawdots(inverted);
      }
    }
  }
  
  lastinverted = inverted;

}



void step(void) {
 
}


void drawdigit(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted) {
  blitsegs_rom(x, y, zero_p+d*DIGIT_HEIGHT*4, 64, inverted);
}
void transitiondigit(uint8_t x, uint8_t y, uint8_t o, uint8_t t, uint8_t inverted) {
  uint8_t oline[4], tline[4];
  uint8_t bitmap[DIGIT_WIDTH * DIGIT_HEIGHT / 8] = {0};

  for (uint8_t line=0; line<64; line++) {
    /*
      putstring("Line #");
    uart_putw_dec(line);
    putstring_nl("");
    */
    if (o == 255) {
      oline[0] = oline[1] = oline[2] = oline[3] = 0;
    } else {
      oline[0] = pgm_read_byte(zero_p+o*DIGIT_HEIGHT*4+4*line);
      oline[1] = pgm_read_byte(zero_p+o*DIGIT_HEIGHT*4+4*line+1);
      oline[2] = pgm_read_byte(zero_p+o*DIGIT_HEIGHT*4+4*line+2);
      oline[3] = pgm_read_byte(zero_p+o*DIGIT_HEIGHT*4+4*line+3);
    }
    uint8_t osegs = 0;
    if (oline[0] != 255)
      osegs++;
    if (oline[2] != 255)
      osegs++;

    tline[0] = pgm_read_byte(zero_p+t*DIGIT_HEIGHT*4+line*4);
    tline[1] = pgm_read_byte(zero_p+t*DIGIT_HEIGHT*4+line*4+1);
    tline[2] = pgm_read_byte(zero_p+t*DIGIT_HEIGHT*4+line*4+2);
    tline[3] = pgm_read_byte(zero_p+t*DIGIT_HEIGHT*4+line*4+3);
    uint8_t tsegs = 0;
    if (tline[0] != 255)
      tsegs++;
    if (tline[2] != 255)
      tsegs++;

    uint8_t segs = (osegs > tsegs ? osegs : tsegs);
    uint8_t oseg[2], tseg[2];
    uint16_t cseg[2];

    for (uint8_t j=0; j<segs; j++) {
      if (oline[j*2] != 255) {
	oseg[0] = oline[j*2];
	oseg[1] = oline[j*2+1];
      } else {
	oseg[0] = oline[0];
	oseg[1] = oline[1];
      }

      if (tline[j*2] != 255) {
	tseg[0] = tline[j*2];
	tseg[1] = tline[j*2+1];
      } else {
	tseg[0] = tline[0];
	tseg[1] = tline[1];
      }

      cseg[0] = tseg[0] - oseg[0];
      cseg[0] *= steps;
      cseg[0] += MAX_STEPS/2;
      cseg[0] /= MAX_STEPS;
      cseg[0] += oseg[0];

      cseg[1] = tseg[1] - oseg[1];
      cseg[1] *= steps;
      cseg[1] += MAX_STEPS/2;
      cseg[1] /= MAX_STEPS;
      cseg[1] += oseg[1];

      /*
      putstring("orig seg = (");
      uart_putw_dec(oseg[0]);
      putstring(", "); 
      uart_putw_dec(oseg[1]);
      putstring_nl(")");

      putstring("target seg = (");
      uart_putw_dec(tseg[0]);
      putstring(", "); 
      uart_putw_dec(tseg[1]);
      putstring_nl(")");

      putstring("current seg = (");
      uart_putw_dec(cseg[0]);
      putstring(", "); 
      uart_putw_dec(cseg[1]);
      putstring_nl(")");
      */

      //     uart_getchar();


      cseg[0] &= 0xff;
      cseg[1] &= 0xff;

      while (cseg[0] < cseg[1]) {
	//bitmap[cseg[0] + (i*DIGIT_WIDTH)/8 ] |= _BV(i%8);
	/*
	  putstring("byte #");
	  uart_putw_dec(cseg[0] + ( (line/8) *DIGIT_WIDTH));
	  putstring_nl("");
	*/

	bitmap[cseg[0] + (line/8)*DIGIT_WIDTH ] |= _BV(line%8);

	//glcdSetDot(x+cseg[0], y+i);
	cseg[0]++;
      }
    }
  }
  bitblit_ram(x,y, bitmap, DIGIT_HEIGHT*DIGIT_WIDTH/8, inverted);
}



void bitblit_ram(uint8_t x_origin, uint8_t y_origin, uint8_t *bitmap_p, uint8_t size, uint8_t inverted) {
  uint8_t x, y, p;

  for (uint8_t i = 0; i<size; i++) {
    p = bitmap_p[i];

    x = i % DIGIT_WIDTH;
    if (x == 0) {
      y = i / DIGIT_WIDTH;
      glcdSetAddress(x+x_origin, (y_origin/8)+y);
    }
    if (inverted) 
      glcdDataWrite(~p);  
    else 
      glcdDataWrite(p);  
  }
}

// number of segments to expect
#define SEGMENTS 2

void blitsegs_rom(uint8_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t height, uint8_t inverted) {
  uint8_t bitmap[DIGIT_WIDTH * DIGIT_HEIGHT / 8] = {0};

  for (uint8_t line = 0; line<height; line++) {
    uint8_t start = pgm_read_byte(bitmap_p+4*line);
    uint8_t stop = pgm_read_byte(bitmap_p+4*line+1);
    
     while (start < stop) {
	bitmap[start + (line/8)*DIGIT_WIDTH ] |= _BV(line%8);
	start++;
      }
    start = pgm_read_byte(bitmap_p+4*line+2);
    stop = pgm_read_byte(bitmap_p+4*line+3);
    while (start < stop) {
      bitmap[start + (line/8)*DIGIT_WIDTH ] |= _BV(line%8);
      start++;
    }
  }
  bitblit_ram(x_origin, y_origin, bitmap, DIGIT_HEIGHT*DIGIT_WIDTH/8, inverted);


 /*
  for (uint8_t i = 0; i<height; i++) {
    uint8_t start = pgm_read_byte(bitmap_p+4*i);
    uint8_t stop = pgm_read_byte(bitmap_p+4*i+1);
    if (start == 255)
      continue;
    while (start < stop) {
      if (inverted)
	glcdClearDot(x_origin+start, y_origin+i);
      else
	glcdSetDot(x_origin+start, y_origin+i);
      start++;
    }
    start = pgm_read_byte(bitmap_p+4*i+2);
    stop = pgm_read_byte(bitmap_p+4*i+3);
    if (start == 255)
      continue;
    while (start < stop) {
      if (inverted)
	glcdClearDot(x_origin+start, y_origin+i);
      else
	glcdSetDot(x_origin+start, y_origin+i);

      start++;
    }
  }
 */
}


void bitblit_rom(uint8_t x_origin, uint8_t y_origin, PGM_P bitmap_p, uint8_t size, uint8_t inverted) {
  uint8_t x, y;

  for (uint8_t i = 0; i<size; i++) {
    uint8_t p = pgm_read_byte(bitmap_p+i);

    y = i / DIGIT_WIDTH;
    x = i % DIGIT_WIDTH;
      
    glcdSetAddress(x+x_origin, (y_origin/8)+y);
    glcdDataWrite(p);  
  }
}



