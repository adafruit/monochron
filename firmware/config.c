#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>
#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "glcd.h"

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarm_h, alarm_m;
extern volatile uint8_t last_buttonstate, just_pressed, pressed;
extern volatile uint8_t buttonholdcounter;
extern volatile uint8_t region;
extern volatile uint8_t time_format;

extern volatile uint8_t displaymode;
// This variable keeps track of whether we have not pressed any
// buttons in a few seconds, and turns off the menu display
volatile uint8_t timeoutcounter = 0;

volatile uint8_t screenmutex = 0;

void display_menu(void) {
  DEBUGP("display menu");
  
  screenmutex++;

  glcdClearScreen();
  
  glcdSetAddress(0, 0);
  glcdPutStr("Configuration Menu", NORMAL);
  
  glcdSetAddress(MENU_INDENT, 1);
  glcdPutStr("Set Alarm:  ", NORMAL);
  print_alarmhour(alarm_h, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber(alarm_m, NORMAL);
  
  glcdSetAddress(MENU_INDENT, 2);
  glcdPutStr("Set Time: ", NORMAL);
  print_timehour(time_h, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber(time_m, NORMAL);
  glcdWriteChar(':', NORMAL);
  printnumber(time_s, NORMAL);
  if (time_format == TIME_12H) {
    glcdWriteChar(' ', NORMAL);
    if (time_h >= 12) {
      glcdWriteChar('P', NORMAL);
    } else {
      glcdWriteChar('A', NORMAL);
    }
  }
  
  print_date(date_m,date_d,date_y,SET_DATE);
  print_region_setting(NORMAL);
  
#ifdef BACKLIGHT_ADJUST
  glcdSetAddress(MENU_INDENT, 5);
  glcdPutStr("Set Backlight: ", NORMAL);
  printnumber(OCR2B>>OCR2B_BITSHIFT,NORMAL);
#endif
  
  glcdSetAddress(0, 6);
  glcdPutStr("Press MENU to advance", NORMAL);
  glcdSetAddress(0, 7);
  glcdPutStr("Press SET to set", NORMAL);

  screenmutex--;
}

void print_month(uint8_t inverted, uint8_t month) {
  switch(month)
  {
  	case 1:
  	  glcdPutStr("Jan ", inverted);
  	  break;
  	case 2:
  	  glcdPutStr("Feb ", inverted);
  	  break;
  	case 3:
  	  glcdPutStr("Mar ", inverted);
  	  break;
  	case 4:
  	  glcdPutStr("Apr ", inverted);
  	  break;
  	case 5:
  	  glcdPutStr("May ", inverted);
  	  break;
  	case 6:
  	  glcdPutStr("Jun ", inverted);
  	  break;
  	case 7:
  	  glcdPutStr("Jul ", inverted);
  	  break;
  	case 8:
  	  glcdPutStr("Aug ", inverted);
  	  break;
  	case 9:
  	  glcdPutStr("Sep ", inverted);
  	  break;
  	case 10:
  	  glcdPutStr("Oct ", inverted);
  	  break;
  	case 11:
  	  glcdPutStr("Nov ", inverted);
  	  break;
  	case 12:
  	  glcdPutStr("Dec ", inverted);
  	  break;
  }
}
void print_dow(uint8_t inverted, uint8_t mon, uint8_t day, uint8_t yr) {
  switch(dotw(mon,day,yr))
  {
    case 0:
      glcdPutStr("Sun ", inverted);
      break;
    case 1:
      glcdPutStr("Mon ", inverted);
      break;
    case 2:
      glcdPutStr("Tue ", inverted);
      break;
    case 3:
      glcdPutStr("Wed ", inverted);
      break;
    case 4:
      glcdPutStr("Thu ", inverted);
      break;
    case 5:
      glcdPutStr("Fri ", inverted);
      break;
    case 6:
      glcdPutStr("Sat ", inverted);
      break;
    
  }
}

void print_date(uint8_t month, uint8_t day, uint8_t year, uint8_t mode) {
  glcdSetAddress(MENU_INDENT, 3);
  glcdPutStr("Date: ", NORMAL);
  if (region == REGION_US) {
  	glcdPutStr("      ",NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if (region == REGION_EU) {
  	glcdPutStr("      ",NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if ( region == DOW_REGION_US) {
  	glcdPutStr("  ",NORMAL);
  	print_dow(NORMAL,month,day,year);
  	printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if ( region == DOW_REGION_EU) {
  	glcdPutStr("  ",NORMAL);
  	print_dow(NORMAL,month,day,year);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
    printnumber(month, (mode == SET_MONTH)?INVERTED:NORMAL);
    glcdWriteChar('/', NORMAL);
  } else if ( region == DATELONG) {
  	glcdPutStr("    ",NORMAL);
  	print_month((mode == SET_MONTH)?INVERTED:NORMAL,month);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	glcdWriteChar(',', NORMAL);
  	glcdWriteChar(' ', NORMAL);
  } else {
  	print_dow(NORMAL,month,day,year);
  	print_month((mode == SET_MONTH)?INVERTED:NORMAL,month);
  	printnumber(day, (mode == SET_DAY)?INVERTED:NORMAL);
  	glcdWriteChar(',', NORMAL);
  	glcdWriteChar(' ', NORMAL);
  }
  printnumber(year, (mode == SET_YEAR)?INVERTED:NORMAL);
}

void set_date(void) {
  uint8_t mode = SET_DATE;
  uint8_t day, month, year;
    
  day = date_d;
  month = date_m;
  year = date_y;

  display_menu();

  screenmutex++;
  // put a small arrow next to 'set date'
  drawArrow(0, 27, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if ((mode == SET_DATE) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_MONTH;

	// print the month inverted
	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change mon", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set mon.", NORMAL);
      } else if ((mode == SET_DATE) && ((region == REGION_EU) || (region == DOW_REGION_EU))) {
	DEBUG(putstring("Set date month"));
	// ok now its selected
	mode = SET_DAY;

	// print the day inverted
	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change day", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set date", NORMAL);
      } else if ((mode == SET_MONTH) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) {
	DEBUG(putstring("Set date day"));
	mode = SET_DAY;

	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change day", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set date", NORMAL);
      }else if ((mode == SET_DAY) && ((region == REGION_EU) || (region == DOW_REGION_EU))) {
	DEBUG(putstring("Set date month"));
	mode = SET_MONTH;

	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change mon", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set mon.", NORMAL);
      } else if ( ((mode == SET_DAY) && ((region == REGION_US) || (region == DOW_REGION_US) || (region == DATELONG) || (region == DATELONG_DOW))) ||
		  ((mode == SET_MONTH) && ((region == REGION_EU) || (region == DOW_REGION_EU))) )  {
	DEBUG(putstring("Set year"));
	mode = SET_YEAR;
	// print the date normal

	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change yr.", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set year", NORMAL);
      } else {
	// done!
	DEBUG(putstring("done setting date"));
	mode = SET_DATE;
	// print the seconds normal
	print_date(month,day,year,mode);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press MENU to advance", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set", NORMAL);
	
	date_y = year;
	date_m = month;
	date_d = day;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
	init_crand();
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;

      screenmutex++;

      if (mode == SET_MONTH) {
	month++;
	if (month >= 13)
	  month = 1;
	if(month == 2) {
	  if(leapyear(year) && (day > 29))
	  	day = 29;
	  else if (!leapyear(year) && (day > 28))
	  	day = 28;
	} else if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
      if(day > 30)
      	day = 30;
	}
	print_date(month,day,year,mode);
	
      }
      if (mode == SET_DAY) {
	day++;
	if (day > 31)
	  day = 1;
	if(month == 2) {
	  if(leapyear(year) && (day > 29))
	  	day = 1;
	  else if (!leapyear(year) && (day > 28))
	  	day = 1;
	} else if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
      if(day > 30)
      	day = 1;
	}
	print_date(month,day,year,mode);
      }
      if (mode == SET_YEAR) {
	year = (year+1) % 100;
	print_date(month,day,year,mode);
      }
      screenmutex--;

      if (pressed & 0x4)
	_delay_ms(200);  
    }
  }

}

#ifdef BACKLIGHT_ADJUST
void set_backlight(void) {
  uint8_t mode = SET_BRIGHTNESS;

  display_menu();
  
  screenmutex++;
  glcdSetAddress(0, 6);
  glcdPutStr("Press MENU to exit   ", NORMAL);

  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, 43, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_BRIGHTNESS) {
	DEBUG(putstring("Setting backlight"));
	// ok now its selected
	mode = SET_BRT;
	// print the region 
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber(OCR2B>>OCR2B_BITSHIFT,INVERTED);
	
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change   ", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to save   ", NORMAL);
      } else {
	mode = SET_BRIGHTNESS;
	// print the region normal
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber(OCR2B>>OCR2B_BITSHIFT,NORMAL);

	glcdSetAddress(0, 6);
	glcdPutStr("Press MENU to exit", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set   ", NORMAL);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_BRT) {
	    OCR2B += OCR2B_PLUS;
	    if(OCR2B > OCR2A_VALUE)
	      OCR2B = 0;
	screenmutex++;
	display_menu();
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change    ", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to save    ", NORMAL);

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 43, MENU_INDENT -1);
	glcdSetAddress(MENU_INDENT + 15*6, 5);
	printnumber(OCR2B>>OCR2B_BITSHIFT,INVERTED);
	
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_BRIGHT, OCR2B);
      }
    }
  }
}
#endif

void print_region_setting(uint8_t inverted) {
  glcdSetAddress(MENU_INDENT, 4);
  glcdPutStr("Region: ", NORMAL);
  if ((region == REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr("     US 12hr", inverted);
  } else if ((region == REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr("     US 24hr", inverted);
  } else if ((region == REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr("     EU 12hr", inverted);
  } else if ((region == REGION_EU) && (time_format == TIME_24H)){
    glcdPutStr("     EU 24hr", inverted);
  } else if ((region == DOW_REGION_US) && (time_format == TIME_12H)) {
    glcdPutStr(" US 12hr DOW", inverted);
  } else if ((region == DOW_REGION_US) && (time_format == TIME_24H)) {
    glcdPutStr(" US 24hr DOW", inverted);
  } else if ((region == DOW_REGION_EU) && (time_format == TIME_12H)) {
    glcdPutStr(" EU 12hr DOW", inverted);
  } else if ((region == DOW_REGION_EU) && (time_format == TIME_24H)){
    glcdPutStr(" EU 24hr DOW", inverted);
  } else if ((region == DATELONG) && (time_format == TIME_12H)) {
    glcdPutStr("   12hr LONG", inverted);
  } else if ((region == DATELONG) && (time_format == TIME_24H)) {
    glcdPutStr("   24hr LONG", inverted);
  } else if ((region == DATELONG_DOW) && (time_format == TIME_12H)) {
    glcdPutStr("12h LONG DOW", inverted);
  } else if ((region == DATELONG_DOW) && (time_format == TIME_24H)){
    glcdPutStr("24h LONG DOW", inverted);
  }
}

void set_region(void) {
  uint8_t mode = SET_REGION;

  display_menu();
  
  screenmutex++;
  
#ifndef BACKLIGHT_ADJUST
  glcdSetAddress(0, 6);
  glcdPutStr("Press MENU to exit   ", NORMAL);
#endif

  // put a small arrow next to 'set 12h/24h'
  drawArrow(0, 35, MENU_INDENT -1);
  screenmutex--;
  
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
  
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_REGION) {
	DEBUG(putstring("Setting region"));
	// ok now its selected
	mode = SET_REG;
	// print the region 
	print_region_setting(INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change    ", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to save    ", NORMAL);
      } else {
	mode = SET_REGION;
	// print the region normal
	print_region_setting(NORMAL);

	glcdSetAddress(0, 6);
#ifdef BACKLIGHT_ADJUST
	glcdPutStr("Press MENU to advance", NORMAL);
#else
	glcdPutStr("Press MENU to exit   ", NORMAL);
#endif
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set     ", NORMAL);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      
      if (mode == SET_REG) {
	    if(time_format) {        
	      region++;
	      if(region > DATELONG_DOW)
	        region = 0;
		  time_format = !time_format;
		} else {
		  time_format = !time_format;
		}
	screenmutex++;
	display_menu();
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change    ", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to save    ", NORMAL);

	// put a small arrow next to 'set 12h/24h'
	drawArrow(0, 35, MENU_INDENT -1);

	print_region_setting(INVERTED);
	screenmutex--;

	eeprom_write_byte((uint8_t *)EE_REGION, region);
	eeprom_write_byte((uint8_t *)EE_TIME_FORMAT, time_format);    
      }
    }
  }
}

void set_alarm(void) {
  uint8_t mode = SET_ALARM;

  display_menu();
  screenmutex++;
  // put a small arrow next to 'set alarm'
  drawArrow(0, 11, MENU_INDENT -1);
  screenmutex--;
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_ALARM) {
	DEBUG(putstring("Set alarm hour"));
	// ok now its selected
	mode = SET_HOUR;

	// print the hour inverted
	print_alarmhour(alarm_h, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change hr.", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set hour", NORMAL);
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set alarm min"));
	mode = SET_MIN;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 12*6, 1);
	print_alarmhour(alarm_h, NORMAL);
	// and the minutes inverted
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber(alarm_m, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change min", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set mins", NORMAL);

      } else {
	mode = SET_ALARM;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 12*6, 1);
	print_alarmhour(alarm_h, NORMAL);
	// and the minutes inverted
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber(alarm_m, NORMAL);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press MENU to advance", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set", NORMAL);
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_HOUR) {
	alarm_h = (alarm_h+1) % 24;
	// print the hour inverted
	print_alarmhour(alarm_h, INVERTED);
	eeprom_write_byte((uint8_t *)EE_ALARM_HOUR, alarm_h);    
      }
      if (mode == SET_MIN) {
	alarm_m = (alarm_m+1) % 60;
	glcdSetAddress(MENU_INDENT + 15*6, 1);
	printnumber(alarm_m, INVERTED);
	eeprom_write_byte((uint8_t *)EE_ALARM_MIN, alarm_m);    
      }
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void set_time(void) {
  uint8_t mode = SET_TIME;

  uint8_t hour, min, sec;
    
  hour = time_h;
  min = time_m;
  sec = time_s;

  display_menu();
  
  screenmutex++;
  // put a small arrow next to 'set time'
  drawArrow(0, 19, MENU_INDENT -1);
  screenmutex--;
 
  timeoutcounter = INACTIVITYTIMEOUT;  

  while (1) {
    if (just_pressed & 0x1) { // mode change
      return;
    }
    if (just_pressed || pressed) {
      timeoutcounter = INACTIVITYTIMEOUT;  
      // timeout w/no buttons pressed after 3 seconds?
    } else if (!timeoutcounter) {
      //timed out!
      displaymode = SHOW_TIME;     
      return;
    }
    if (just_pressed & 0x2) {
      just_pressed = 0;
      screenmutex++;

      if (mode == SET_TIME) {
	DEBUG(putstring("Set time hour"));
	// ok now its selected
	mode = SET_HOUR;

	// print the hour inverted
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, INVERTED);
	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (hour >= 12) {
	    glcdWriteChar('P', INVERTED);
	  } else {
	    glcdWriteChar('A', INVERTED);
	  }
	}

	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change hr.", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set hour", NORMAL);
      } else if (mode == SET_HOUR) {
	DEBUG(putstring("Set time min"));
	mode = SET_MIN;
	// print the hour normal
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, NORMAL);
	// and the minutes inverted
	glcdWriteChar(':', NORMAL);
	printnumber(min, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change min", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set mins", NORMAL);

	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (hour >= 12) {
	    glcdWriteChar('P', NORMAL);
	  } else {
	    glcdWriteChar('A', NORMAL);
	  }
	}
      } else if (mode == SET_MIN) {
	DEBUG(putstring("Set time sec"));
	mode = SET_SEC;
	// and the minutes normal
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 13*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 15*6, 2);
	}
	printnumber(min, NORMAL);
	glcdWriteChar(':', NORMAL);
	// and the seconds inverted
	printnumber(sec, INVERTED);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press + to change sec", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set secs", NORMAL);
      } else {
	// done!
	DEBUG(putstring("done setting time"));
	mode = SET_TIME;
	// print the seconds normal
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 16*6, 2);
	} else {
  	  glcdSetAddress(MENU_INDENT + 18*6, 2);
	}
	printnumber(sec, NORMAL);
	// display instructions below
	glcdSetAddress(0, 6);
	glcdPutStr("Press MENU to advance", NORMAL);
	glcdSetAddress(0, 7);
	glcdPutStr("Press SET to set", NORMAL);
	
	time_h = hour;
	time_m = min;
	time_s = sec;
	writei2ctime(time_s, time_m, time_h, 0, date_d, date_m, date_y);
	init_crand();
      }
      screenmutex--;
    }
    if ((just_pressed & 0x4) || (pressed & 0x4)) {
      just_pressed = 0;
      screenmutex++;
      if (mode == SET_HOUR) {
	hour = (hour+1) % 24;
	time_h = hour;
	
	glcdSetAddress(MENU_INDENT + 10*6, 2);
	print_timehour(hour, INVERTED);
	glcdSetAddress(MENU_INDENT + 18*6, 2);
	if (time_format == TIME_12H) {
	  glcdWriteChar(' ', NORMAL);
	  if (time_h >= 12) {
	    glcdWriteChar('P', INVERTED);
	  } else {
	    glcdWriteChar('A', INVERTED);
	  }
	}
      }
      if (mode == SET_MIN) {
	min = (min+1) % 60;
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 13*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 15*6, 2);
	}
	printnumber(min, INVERTED);
      }
      if (mode == SET_SEC) {
	sec = (sec+1) % 60;
	if(time_format == TIME_12H) {
	  glcdSetAddress(MENU_INDENT + 16*6, 2);
	} else {
	  glcdSetAddress(MENU_INDENT + 18*6, 2);
	}
	printnumber(sec, INVERTED);
      }
      screenmutex--;
      if (pressed & 0x4)
	_delay_ms(200);
    }
  }
}

void print_timehour(uint8_t h, uint8_t inverted) {
  if (time_format == TIME_12H) {
    if (((h + 23)%12 + 1) >= 10 ) {
      printnumber((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
  } else {
    glcdWriteChar(' ', NORMAL);
    glcdWriteChar(' ', NORMAL);
    printnumber(h, inverted);
  }
}

void print_alarmhour(uint8_t h, uint8_t inverted) {
  if (time_format == TIME_12H) {
    glcdSetAddress(MENU_INDENT + 18*6, 1);
    if (h >= 12) 
      glcdWriteChar('P', NORMAL);
    else
      glcdWriteChar('A', NORMAL);
    glcdWriteChar('M', NORMAL);
    glcdSetAddress(MENU_INDENT + 12*6, 1);

    if (((h + 23)%12 + 1) >= 10 ) {
      printnumber((h + 23)%12 + 1, inverted);
    } else {
      glcdWriteChar(' ', NORMAL);
      glcdWriteChar('0' + (h + 23)%12 + 1, inverted);
    }
   } else {
    glcdSetAddress(MENU_INDENT + 12*6, 1);
    printnumber(h, inverted);
  }
}
