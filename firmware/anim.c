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

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;

uint8_t left_score, right_score;

float ball_x, ball_y;
float oldball_x, oldball_y;
float ball_dx, ball_dy;

int8_t rightpaddle_y, leftpaddle_y;
uint8_t oldleftpaddle_y, oldrightpaddle_y;
int8_t rightpaddle_dy, leftpaddle_dy;

extern volatile uint8_t minute_changed, hour_changed;

uint8_t redraw_time = 0;
uint8_t last_score_mode = 0;

uint32_t rval[2]={0,0};
uint32_t key[4]={
	0x2DE9716E,0x993FDDD1,0x2A77FB57,0xB172E6B0
};

void encipher(void) {
    unsigned int i;
    uint32_t v0=rval[0], v1=rval[1], sum=0, delta=0x9E3779B9;
    for (i=0; i < 32; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    rval[0]=v0; rval[1]=v1;
}

uint16_t crand(void) {
  if((rval[0]==0)&&(rval[1]==0)){
  	//Just powered on, clock was never previously turned on,
  	//or we reset the time manually.
  	wdt_reset();
  	encipher();
  	wdt_reset();
  	rval[0] = alarm_h;
  	rval[0]<<=8;
  	rval[0]|=time_h;
  	rval[0]<<=8;
  	rval[0]|=time_m;
  	rval[0]<<=8;
  	rval[0]|=time_s;
  	key[0]^=rval[0];
  	encipher();
  	wdt_reset();
  	key[1]^=rval[0]<<1;
  	encipher();
  	wdt_reset();
  	key[2]^=rval[0]>>1;
  	encipher();
  	wdt_reset();
  	key[3]^=rval[1];
  	encipher();
  	wdt_reset();
  	rval[0] = alarm_m;
  	rval[0]<<=8;
  	rval[0]|=time_h;
  	rval[0]<<=8;
  	rval[0]|=time_m;
  	rval[0]<<=8;
  	rval[0]|=time_s;
  	key[3]^=rval[0];
  	encipher();
  	wdt_reset();
  	key[1]^=rval[0]>>1;
  	encipher();
  	wdt_reset();
  	key[2]^=rval[0]<<1;
  	encipher();
  	wdt_reset();
  	key[3]^=rval[1];
  	rval[0]=0;
  	rval[1]=0;
  	encipher();
  }
  else
  {
  	wdt_reset();
  	encipher();
  }
  return rval[0]&RAND_MAX;
}

void setscore(void)
{
  if(score_mode != last_score_mode) {
    redraw_time = 1;
    last_score_mode = score_mode;
  }
  switch(score_mode) {
    case SCORE_MODE_TIME:
      if(alarming && (minute_changed || hour_changed)) {
      	if(hour_changed) {
	      left_score = old_h;
	      right_score = old_m;
	    } else if (minute_changed) {
	      right_score = old_m;
	    }
      } else {
        left_score = time_h;
        right_score = time_m;
      }
      break;
    case SCORE_MODE_DATE:
      if(region == REGION_US) {
        left_score = date_m;
        right_score = date_d;
      } else {
        left_score = date_d;
        right_score = date_m;
      }
      break;
    case SCORE_MODE_YEAR:
      left_score = 20;
      right_score = date_y;
      break;
    case SCORE_MODE_ALARM:
      left_score = alarm_h;
      right_score = alarm_m;
      break;
  }
}

void initanim(void) {
  DEBUG(putstring("screen width: "));
  DEBUG(uart_putw_dec(GLCD_XPIXELS));
  DEBUG(putstring("\n\rscreen height: "));
  DEBUG(uart_putw_dec(GLCD_YPIXELS));
  DEBUG(putstring_nl(""));

  leftpaddle_y = 25;
  rightpaddle_y = 25;

  ball_x = (SCREEN_W / 2) - 1;
  ball_y = (SCREEN_H / 2) - 1;
  float angle = random_angle_rads();
  ball_dx = MAX_BALL_SPEED * cos(angle);
  ball_dy = MAX_BALL_SPEED * sin(angle);
}

void initdisplay(uint8_t inverted) {

  glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
  
  // draw top 'line'
  glcdFillRectangle(0, 0, GLCD_XPIXELS, 2, ! inverted);
  
  // bottom line
  glcdFillRectangle(0, GLCD_YPIXELS - 2, GLCD_XPIXELS, 2, ! inverted);

  // left paddle
  glcdFillRectangle(LEFTPADDLE_X, leftpaddle_y, PADDLE_W, PADDLE_H, ! inverted);
  // right paddle
  glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y, PADDLE_W, PADDLE_H, ! inverted);
      
	//left_score = time_h;
	//right_score = time_m;
	setscore();

  // time
	if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
		drawbigdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, ((left_score + 23)%12 + 1)/10, inverted);
  else 
    drawbigdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, left_score/10, inverted);
  
	if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
		drawbigdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, ((left_score + 23)%12 + 1)%10, inverted);
  else
    drawbigdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, left_score%10, inverted);
  
  drawbigdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, right_score/10, inverted);
  drawbigdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, right_score%10, inverted);

  drawmidline(inverted);
}


void step(void) {
  // The keepout is used to know where to -not- put the paddle
  // the 'bouncepos' is where we expect the ball's y-coord to be when
  // it intersects with the paddle area
  static uint8_t right_keepout_top, right_keepout_bot, right_bouncepos, right_endpos;
  static uint8_t left_keepout_top, left_keepout_bot, left_bouncepos, left_endpos;
  static int8_t right_dest, left_dest, ticksremaining;

  // Save old ball location so we can do some vector stuff 
  oldball_x = ball_x;
  oldball_y = ball_y;

  // move ball according to the vector
  ball_x += ball_dx;
  ball_y += ball_dy;
    
  
  /************************************* TOP & BOTTOM WALLS */
  // bouncing off bottom wall, reverse direction
  if (ball_y  > (SCREEN_H - ball_radius*2 - BOTBAR_H)) {
    //DEBUG(putstring_nl("bottom wall bounce"));
    ball_y = SCREEN_H - ball_radius*2 - BOTBAR_H;
    ball_dy *= -1;
  }
  
  // bouncing off top wall, reverse direction
  if (ball_y < TOPBAR_H) {
    //DEBUG(putstring_nl("top wall bounce"));
    ball_y = TOPBAR_H;
    ball_dy *= -1;
  }
  
  // For debugging, print the ball location
  DEBUG(putstring("ball @ (")); 
  DEBUG(uart_putw_dec(ball_x)); 
  DEBUG(putstring(", ")); 
  DEBUG(uart_putw_dec(ball_y)); 
  DEBUG(putstring_nl(")"));
  
  /************************************* LEFT & RIGHT WALLS */
  // the ball hits either wall, the ball resets location & angle
  if ((ball_x  > (SCREEN_W - ball_radius*2)) || ((int8_t)ball_x <= 0)) {
  if(DEBUGGING) {
    if ((int8_t)ball_x <= 0) {
        putstring("Left wall collide");
        if (! minute_changed) {
	  putstring_nl("...on accident");
        } else {
	  putstring_nl("...on purpose");
        }
      } else {
        putstring("Right wall collide");
        if (! hour_changed) {
	  putstring_nl("...on accident");
        } else {
	  putstring_nl("...on purpose");
        }
      }
    }

    // place ball in the middle of the screen
    ball_x = (SCREEN_W / 2) - 1;
    ball_y = (SCREEN_H / 2) - 1;

    float angle = random_angle_rads();
    ball_dx = MAX_BALL_SPEED * cos(angle);
    ball_dy = MAX_BALL_SPEED * sin(angle);

    glcdFillRectangle(LEFTPADDLE_X, left_keepout_top, PADDLE_W, left_keepout_bot - left_keepout_top, 0);
    glcdFillRectangle(RIGHTPADDLE_X, right_keepout_top, PADDLE_W, right_keepout_bot - right_keepout_top, 0);

    right_keepout_top = right_keepout_bot = 0;
    left_keepout_top = left_keepout_bot = 0;
    redraw_time = 1;
    minute_changed = hour_changed = 0;

		//left_score = time_h;
		//right_score = time_m;
		setscore();
	}

 

  // save old paddle position
  oldleftpaddle_y = leftpaddle_y;
  oldrightpaddle_y = rightpaddle_y;

  /************************************* RIGHT PADDLE */
  // check if we are bouncing off right paddle
  if (ball_dx > 0) {
    if ((((int8_t)ball_x + ball_radius*2) >= RIGHTPADDLE_X) && 
	((int8_t)oldball_x + ball_radius*2 <= RIGHTPADDLE_X)) {
    // check if we collided
      DEBUG(putstring_nl("coll?"));
    // determine the exact position at which it would collide
    float dx = RIGHTPADDLE_X - (oldball_x + ball_radius*2);
    // now figure out what fraction that is of the motion and multiply that by the dy
    float dy = (dx / ball_dx) * ball_dy;
    
    if (intersectrect((oldball_x + dx), (oldball_y + dy), ball_radius*2, ball_radius*2, 
		      RIGHTPADDLE_X, rightpaddle_y, PADDLE_W, PADDLE_H)) {
	  if(DEBUGGING) {
      putstring_nl("nosect");
      if (hour_changed) {
	// uh oh
	putstring_nl("FAILED to miss");
      }
      
      
      putstring("RCOLLISION  ball @ ("); uart_putw_dec(oldball_x + dx); putstring(", "); uart_putw_dec(oldball_y + dy);
      putstring(") & paddle @ ["); 
      uart_putw_dec(RIGHTPADDLE_X); putstring(", "); uart_putw_dec(rightpaddle_y); putstring("]");
     }
   
      // set the ball right up against the paddle
      ball_x = oldball_x + dx;
      ball_y = oldball_y + dy;
      // bounce it
      ball_dx *= -1;
      
      right_bouncepos = right_dest = right_keepout_top = right_keepout_bot = 0;
      left_bouncepos = left_dest = left_keepout_top = left_keepout_bot = 0;
    } 
    // otherwise, it didn't bounce...will probably hit the right wall
    DEBUG(putstring(" tix = ")); DEBUG(uart_putw_dec(ticksremaining)); DEBUG(putstring_nl(""));
  }
  

  if ((ball_dx > 0) && ((ball_x+ball_radius*2) < RIGHTPADDLE_X)) {
    // ball is coming towards the right paddle
    
    if (right_keepout_top == 0 ) {
      ticksremaining = calculate_keepout(ball_x, ball_y, ball_dx, ball_dy, &right_bouncepos, &right_endpos);
      if(DEBUGGING) {
        putstring("Expect bounce @ "); uart_putw_dec(right_bouncepos); 
        putstring("-> thru to "); uart_putw_dec(right_endpos); putstring("\n\r");
      }
      if (right_bouncepos > right_endpos) {
	right_keepout_top = right_endpos;
	right_keepout_bot = right_bouncepos + ball_radius*2;
      } else {
	right_keepout_top = right_bouncepos;
	right_keepout_bot = right_endpos + ball_radius*2;
      }
      if(DEBUGGING) {
      //putstring("Keepout from "); uart_putw_dec(right_keepout_top); 
      //putstring(" to "); uart_putw_dec(right_keepout_bot); putstring_nl("");
      }
      
      // Now we can calculate where the paddle should go
      if (!hour_changed) {
	// we want to hit the ball, so make it centered.
	right_dest = right_bouncepos + ball_radius - PADDLE_H/2;
	if(DEBUGGING)
	  putstring("R -> "); uart_putw_dec(right_dest); putstring_nl("");
      } else {
	// we lost the round so make sure we -dont- hit the ball
	if (right_keepout_top <= (TOPBAR_H + PADDLE_H)) {
	  // the ball is near the top so make sure it ends up right below it
	  //  DEBUG(putstring_nl("at the top"));
	  right_dest = right_keepout_bot + 1;
	} else if (right_keepout_bot >= (SCREEN_H - BOTBAR_H - PADDLE_H - 1)) {
	  // the ball is near the bottom so make sure it ends up right above it
	  //DEBUG(putstring_nl("at the bottom"));
	  right_dest = right_keepout_top - PADDLE_H - 1;
	} else {
	  //DEBUG(putstring_nl("in the middle"));
	  if ( ((uint8_t)crand()) & 0x1)
	    right_dest = right_keepout_top - PADDLE_H - 1;
	  else
	    right_dest = right_keepout_bot + 1;
	}
      }
    } else {
      ticksremaining--;
    }

    // draw the keepout area (for debugging
    //glcdRectangle(RIGHTPADDLE_X, right_keepout_top, PADDLE_W, right_keepout_bot - right_keepout_top);
    
    int8_t distance = rightpaddle_y - right_dest;
    
    /*if(DEBUGGING) {
    putstring("dest dist: "); uart_putw_dec(abs(distance)); 
    putstring("\n\rtix: "); uart_putw_dec(ticksremaining);
    putstring("\n\rmax travel: "); uart_putw_dec(ticksremaining * MAX_PADDLE_SPEED);
    putstring_nl("");
    }*/

    // if we have just enough time, move the paddle!
    if (abs(distance) > (ticksremaining-1) * MAX_PADDLE_SPEED) {
      distance = abs(distance);
      if (right_dest > rightpaddle_y ) {
	if (distance > MAX_PADDLE_SPEED)
	  rightpaddle_y += MAX_PADDLE_SPEED;
	else
	  rightpaddle_y += distance;
      }
      if ( right_dest < rightpaddle_y ) {
	if (distance > MAX_PADDLE_SPEED)
	  rightpaddle_y -= MAX_PADDLE_SPEED;
	else
	  rightpaddle_y -= distance;
      }
    } 
  }
  } else {
   /************************************* LEFT PADDLE */
  // check if we are bouncing off left paddle
    if ((((int8_t)ball_x) <= (LEFTPADDLE_X + PADDLE_W)) && 
	(((int8_t)oldball_x) >= (LEFTPADDLE_X + PADDLE_W))) {
    // check if we collided
    // determine the exact position at which it would collide
    float dx = (LEFTPADDLE_X + PADDLE_W) - oldball_x;
    // now figure out what fraction that is of the motion and multiply that by the dy
    float dy = (dx / ball_dx) * ball_dy;

    if (intersectrect((oldball_x + dx), (oldball_y + dy), ball_radius*2, ball_radius*2, 
		      LEFTPADDLE_X, leftpaddle_y, PADDLE_W, PADDLE_H)) {
      if(DEBUGGING) {
        if (minute_changed) {
	  // uh oh
	  putstring_nl("FAILED to miss");
        }
        putstring("LCOLLISION ball @ ("); uart_putw_dec(oldball_x + dx); putstring(", "); uart_putw_dec(oldball_y + dy);
        putstring(") & paddle @ ["); 
        uart_putw_dec(LEFTPADDLE_X); putstring(", "); uart_putw_dec(leftpaddle_y); putstring("]");
      }

      // bounce it
      ball_dx *= -1;

      if ((uint8_t)ball_x != LEFTPADDLE_X + PADDLE_W) {
	// set the ball right up against the paddle
	ball_x = oldball_x + dx;
	ball_y = oldball_y + dy;
      }
      left_bouncepos = left_dest = left_keepout_top = left_keepout_bot = 0;
    } 
    if(DEBUGGING) { putstring(" tix = "); uart_putw_dec(ticksremaining); putstring_nl("");}
    // otherwise, it didn't bounce...will probably hit the left wall
  }

  if ((ball_dx < 0) && (ball_x > (LEFTPADDLE_X + ball_radius*2))) {
    // ball is coming towards the left paddle
    
    if (left_keepout_top == 0 ) {
      ticksremaining = calculate_keepout(ball_x, ball_y, ball_dx, ball_dy, &left_bouncepos, &left_endpos);
      if(DEBUGGING) {
        putstring("Expect bounce @ "); uart_putw_dec(left_bouncepos); 
        putstring("-> thru to "); uart_putw_dec(left_endpos); putstring("\n\r");
      }
      
      if (left_bouncepos > left_endpos) {
	left_keepout_top = left_endpos;
	left_keepout_bot = left_bouncepos + ball_radius*2;
      } else {
	left_keepout_top = left_bouncepos;
	left_keepout_bot = left_endpos + ball_radius*2;
      }
      // if(DEBUGGING) {
      // putstring("Keepout from "); uart_putw_dec(left_keepout_top); 
      //putstring(" to "); uart_putw_dec(left_keepout_bot); putstring_nl("");
      // }
      
      // Now we can calculate where the paddle should go
      if (!minute_changed) {
	// we want to hit the ball, so make it centered.
	left_dest = left_bouncepos + ball_radius - PADDLE_H/2;
	if(DEBUGGING) { putstring("hitL -> "); uart_putw_dec(left_dest); putstring_nl(""); }
      } else {
	// we lost the round so make sure we -dont- hit the ball
	if (left_keepout_top <= (TOPBAR_H + PADDLE_H)) {
	  // the ball is near the top so make sure it ends up right below it
	  DEBUG(putstring_nl("at the top"));
	  left_dest = left_keepout_bot + 1;
	} else if (left_keepout_bot >= (SCREEN_H - BOTBAR_H - PADDLE_H - 1)) {
	  // the ball is near the bottom so make sure it ends up right above it
	  DEBUG(putstring_nl("at the bottom"));
	  left_dest = left_keepout_top - PADDLE_H - 1;
	} else {
	  DEBUG(putstring_nl("in the middle"));
	  if ( ((uint8_t)crand()) & 0x1)
	    left_dest = left_keepout_top - PADDLE_H - 1;
	  else
	    left_dest = left_keepout_bot + 1;
	}
	if(DEBUGGING) {putstring("missL -> "); uart_putw_dec(left_dest); putstring_nl(""); }
      }
    } else {
      ticksremaining--;
    }
    // draw the keepout area (for debugging
    //glcdRectangle(LEFTPADDLE_X, left_keepout_top, PADDLE_W, left_keepout_bot - left_keepout_top);
    
    int8_t distance = abs(leftpaddle_y - left_dest);
    
    /*if(DEBUGGING) {
    putstring("\n\rdest dist: "); uart_putw_dec(abs(distance)); 
    putstring("\n\rtix: "); uart_putw_dec(ticksremaining);
    
    putstring("\n\rmax travel: "); uart_putw_dec(ticksremaining * MAX_PADDLE_SPEED);
    putstring_nl("");
    }*/

    //if(DEBUGGING){putstring("\n\rleft paddle @ "); uart_putw_dec(leftpaddle_y); putstring_nl("");}

    // if we have just enough time, move the paddle!
    if (distance > ((ticksremaining-1) * MAX_PADDLE_SPEED)) {
      if (left_dest > leftpaddle_y ) {
	if (distance > MAX_PADDLE_SPEED)
	  leftpaddle_y += MAX_PADDLE_SPEED;
	else
	  leftpaddle_y += distance;
      }
      if (left_dest < leftpaddle_y ) {
	if (distance > MAX_PADDLE_SPEED)
	  leftpaddle_y -= MAX_PADDLE_SPEED;
	else
	  leftpaddle_y -= distance;
      }
    } 
  }

  }

  // make sure the paddles dont hit the top or bottom
  if (leftpaddle_y < TOPBAR_H +1)
    leftpaddle_y = TOPBAR_H + 1;
  if (rightpaddle_y < TOPBAR_H + 1)
    rightpaddle_y = TOPBAR_H + 1;
  
  if (leftpaddle_y > (SCREEN_H - PADDLE_H - BOTBAR_H - 1))
    leftpaddle_y = (SCREEN_H - PADDLE_H - BOTBAR_H - 1);
  if (rightpaddle_y > (SCREEN_H - PADDLE_H - BOTBAR_H - 1))
    rightpaddle_y = (SCREEN_H - PADDLE_H - BOTBAR_H - 1);
}

void drawmidline(uint8_t inverted) {
  uint8_t i;
  for (i=0; i < (SCREEN_H/8 - 1); i++) { 
    glcdSetAddress((SCREEN_W-MIDLINE_W)/2, i);
    if (inverted) {
      glcdDataWrite(0xF0);
    } else {
      glcdDataWrite(0x0F);  
    }
  }
  glcdSetAddress((SCREEN_W-MIDLINE_W)/2, i);
  if (inverted) {
    glcdDataWrite(0x20);  
  } else {
    glcdDataWrite(0xCF);  
  }
}

void draw(uint8_t inverted) {

    // erase old ball
    glcdFillRectangle(oldball_x, oldball_y, ball_radius*2, ball_radius*2, inverted);

    // draw new ball
    glcdFillRectangle(ball_x, ball_y, ball_radius*2, ball_radius*2, ! inverted);

    // draw middle lines around where the ball may have intersected it?
    if  (intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
		       SCREEN_W/2-MIDLINE_W, 0, MIDLINE_W, SCREEN_H)) {
      // redraw it since we had an intersection
      drawmidline(inverted);
    }



    
    if (oldleftpaddle_y != leftpaddle_y) {
      // clear left paddle
      glcdFillRectangle(LEFTPADDLE_X, oldleftpaddle_y, PADDLE_W, PADDLE_H, inverted);
      // draw left paddle
      glcdFillRectangle(LEFTPADDLE_X, leftpaddle_y, PADDLE_W, PADDLE_H, !inverted);
    }

    if (oldrightpaddle_y != rightpaddle_y) {
      // clear right paddle
      glcdFillRectangle(RIGHTPADDLE_X, oldrightpaddle_y, PADDLE_W, PADDLE_H, inverted);
      // draw right paddle
      glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y, PADDLE_W, PADDLE_H, !inverted);
    }

    if (intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2, RIGHTPADDLE_X, rightpaddle_y, PADDLE_W, PADDLE_H)) {
      glcdFillRectangle(RIGHTPADDLE_X, rightpaddle_y, PADDLE_W, PADDLE_H, !inverted);
    }
   // draw time
   uint8_t redraw_digits;
   TIMSK2 = 0;	//Disable Timer 2 interrupt, to prevent a race condition.
   if(redraw_time)
   {
   	   redraw_digits = 1;
   	   redraw_time = 0;
   }
   TIMSK2 = _BV(TOIE2); //Race issue gone, renable.
    
    // redraw 10's of hours
    if (redraw_digits || intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
				      DISPLAY_H10_X, DISPLAY_TIME_Y, DISPLAY_DIGITW, DISPLAY_DIGITH)) {
      
			if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
				drawbigdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, ((left_score + 23)%12 + 1)/10, inverted);
      else 
	drawbigdigit(DISPLAY_H10_X, DISPLAY_TIME_Y, left_score/10, inverted);
    }
    
    // redraw 1's of hours
    if (redraw_digits || intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
		      DISPLAY_H1_X, DISPLAY_TIME_Y, DISPLAY_DIGITW, DISPLAY_DIGITH)) {
			if ((time_format == TIME_12H) && ((score_mode == SCORE_MODE_TIME) || (score_mode == SCORE_MODE_ALARM)))
				drawbigdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, ((left_score + 23)%12 + 1)%10, inverted);
      else
	drawbigdigit(DISPLAY_H1_X, DISPLAY_TIME_Y, left_score%10, inverted);
    }

    if (redraw_digits || intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
							DISPLAY_M10_X, DISPLAY_TIME_Y, DISPLAY_DIGITW, DISPLAY_DIGITH)) {
      drawbigdigit(DISPLAY_M10_X, DISPLAY_TIME_Y, right_score/10, inverted);
    }
    if (redraw_digits || intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
				      DISPLAY_M1_X, DISPLAY_TIME_Y, DISPLAY_DIGITW, DISPLAY_DIGITH)) {
      drawbigdigit(DISPLAY_M1_X, DISPLAY_TIME_Y, right_score%10, inverted);
    }
    
    redraw_digits = 0;
    // print 'alarm'
    /*
    if (intersectrect(oldball_x, oldball_y, ball_radius*2, ball_radius*2,
		      ALARMBOX_X, ALARMBOX_Y, ALARMBOX_W, ALARMBOX_H)) {
      
      glcdFillRectangle(ALARMBOX_X+2, ALARMBOX_Y+2, ALARMBOX_W-4, ALARMBOX_H-4, OFF);

      glcdFillRectangle(ALARMBOX_X, ALARMBOX_Y, 2, ALARMBOX_H, ON);
      glcdFillRectangle(ALARMBOX_X+ALARMBOX_W, ALARMBOX_Y, 2, ALARMBOX_H, ON);
    }

    if (!intersectrect(ball_x, ball_y, ball_radius*2, ball_radius*2,
		      ALARMBOX_X, ALARMBOX_Y, ALARMBOX_W, ALARMBOX_H)) {
    */

      //}
}

uint8_t intersectrect(uint8_t x1, uint8_t y1, uint8_t w1, uint8_t h1,
		      uint8_t x2, uint8_t y2, uint8_t w2, uint8_t h2) {
  // yer everyday intersection tester
  // check x coord first
  if (x1+w1 < x2)
    return 0;
  if (x2+w2 < x1)
    return 0;

  // check the y coord second
  if (y1+h1 < y2)
    return 0;
  if (y2+h2 < y1)
    return 0;

  return 1;
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
};


void drawbigdigit(uint8_t x, uint8_t y, uint8_t n, uint8_t inverted) {
  uint8_t i, j;
  
  for (i = 0; i < 4; i++) {
    uint8_t d = pgm_read_byte(BigFont+(n*4)+i);
    for (j=0; j<8; j++) {
      if (d & _BV(7-j)) {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, !inverted);
      } else {
	glcdFillRectangle(x+i*2, y+j*2, 2, 2, inverted);
      }
    }
  }
}

float random_angle_rads(void) {
   // create random vector MEME seed it ok???
    float angle = crand();

    //angle = 31930; // MEME DEBUG
    if(DEBUGGING){putstring("\n\rrand = "); uart_putw_dec(angle);}
    angle = (angle * (90.0 - MIN_BALL_ANGLE*2)  / RAND_MAX) + MIN_BALL_ANGLE;

    //pick the quadrant
    uint8_t quadrant = (crand() >> 8) % 4; 
    //quadrant = 2; // MEME DEBUG

    if(DEBUGGING){putstring(" quad = "); uart_putw_dec(quadrant);}
    angle += quadrant * 90;
    if(DEBUGGING){putstring(" new ejection angle = "); uart_putw_dec(angle); putstring_nl("");}

    angle *= 3.1415;
    angle /= 180;
    return angle;
}

uint8_t calculate_keepout(float theball_x, float theball_y, float theball_dx, float theball_dy, uint8_t *keepout1, uint8_t *keepout2) {
  // "simulate" the ball bounce...its not optimized (yet)
  float sim_ball_y = theball_y;
  float sim_ball_x = theball_x;
  float sim_ball_dy = theball_dy;
  float sim_ball_dx = theball_dx;
  
  uint8_t tix = 0, collided = 0;

  while ((sim_ball_x < (RIGHTPADDLE_X + PADDLE_W)) && ((sim_ball_x + ball_radius*2) > LEFTPADDLE_X)) {
    float old_sim_ball_x = sim_ball_x;
    float old_sim_ball_y = sim_ball_y;
    sim_ball_y += sim_ball_dy;
    sim_ball_x += sim_ball_dx;
	
    if (sim_ball_y  > (int8_t)(SCREEN_H - ball_radius*2 - BOTBAR_H)) {
      sim_ball_y = SCREEN_H - ball_radius*2 - BOTBAR_H;
      sim_ball_dy *= -1;
    }
	
    if (sim_ball_y <  TOPBAR_H) {
      sim_ball_y = TOPBAR_H;
      sim_ball_dy *= -1;
    }
    
    if ((((int8_t)sim_ball_x + ball_radius*2) >= RIGHTPADDLE_X) && 
	((old_sim_ball_x + ball_radius*2) < RIGHTPADDLE_X)) {
      // check if we collided with the right paddle
      
      // first determine the exact position at which it would collide
      float dx = RIGHTPADDLE_X - (old_sim_ball_x + ball_radius*2);
      // now figure out what fraction that is of the motion and multiply that by the dy
      float dy = (dx / sim_ball_dx) * sim_ball_dy;
	  
      if(DEBUGGING){putstring("RCOLL@ ("); uart_putw_dec(old_sim_ball_x + dx); putstring(", "); uart_putw_dec(old_sim_ball_y + dy);}
      
      *keepout1 = old_sim_ball_y + dy; 
      collided = 1;
    } else if (((int8_t)sim_ball_x <= (LEFTPADDLE_X + PADDLE_W)) && 
	       (old_sim_ball_x > (LEFTPADDLE_X + PADDLE_W))) {
      // check if we collided with the left paddle

      // first determine the exact position at which it would collide
      float dx = (LEFTPADDLE_X + PADDLE_W) - old_sim_ball_x;
      // now figure out what fraction that is of the motion and multiply that by the dy
      float dy = (dx / sim_ball_dx) * sim_ball_dy;
	  
      //if(DEBUGGING){putstring("LCOLL@ ("); uart_putw_dec(old_sim_ball_x + dx); putstring(", "); uart_putw_dec(old_sim_ball_y + dy);}
      
      *keepout1 = old_sim_ball_y + dy; 
      collided = 1;
    }
    if (!collided) {
      tix++;
    }
    
    //if(DEBUGGING){putstring("\tSIMball @ ["); uart_putw_dec(sim_ball_x); putstring(", "); uart_putw_dec(sim_ball_y); putstring_nl("]");}
  }
  *keepout2 = sim_ball_y;

  return tix;
}
      
