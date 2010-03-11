/* ***************************************************************************
// buttons.c - the button debouncing/switches handling
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

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

// These store the current button states for all 3 buttons. We can 
// then query whether the buttons are pressed and released or pressed
// This allows for 'high speed incrementing' when setting the time
volatile uint8_t last_buttonstate = 0, just_pressed = 0, pressed = 0;
volatile uint8_t buttonholdcounter = 0;

// whether hte alarm is going off
extern volatile uint8_t alarming;

void initbuttons(void) {
  // alarm pin requires a pullup
  ALARM_DDR &= ~_BV(ALARM);
  ALARM_PORT |= _BV(ALARM);

  // alarm switching is detected by using the pin change interrupt
  PCICR =  _BV(PCIE0);
  PCMSK0 |= _BV(ALARM);

  // The buttons are totem pole'd together so we can read the buttons with one pin
  // set up ADC
  ADMUX = 2;      // listen to ADC2 for button presses
  ADCSRB = 0;     // free running mode
  // enable ADC and interrupts, prescale down to <200KHz
  ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1); 
  ADCSRA |= _BV(ADSC); // start a conversion
}

uint16_t readADC(void) {
  // basically just the busy-wait code to read the ADC and return the value
  ADCSRA &= ~_BV(ADIE); // no interrupt
  ADCSRA |= _BV(ADSC); // start a conversion
  while (! (ADCSRA & _BV(ADIF)));
  return ADC;
}

// Every time the ADC finishes a conversion, we'll see whether
// the buttons have changed
SIGNAL(ADC_vect) {
  uint16_t reading, reading2;
  sei();

  // We get called when ADC is ready so no need to request a conversion
  reading = ADC;

  if (reading > 735) {
    // no presses
    pressed = 0;
    last_buttonstate = 0;
    
    ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  
    return;
  } else if (reading > 610) {
    // button 3 "+" pressed
    if (! (last_buttonstate & 0x4)) { // was not pressed before
      // debounce by taking a second reading 
      _delay_ms(10);
      reading2 = readADC();
      if ( (reading2 > 735) || (reading2 < 610)) {
	// was a bounce, ignore it
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }

      buttonholdcounter = 2;          // see if we're press-and-holding
      // the buttonholdcounter is decremented by a timer!
      while (buttonholdcounter) {
	reading2 = readADC();
	if ( (reading2 > 735) || (reading2 < 610)) {
	  // button was released
	  last_buttonstate &= ~0x4;
  
	  DEBUG(putstring_nl("b3"));
	  just_pressed |= 0x4;
	  ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	  return;
	}
      }
      // 2 seconds later...
      last_buttonstate |= 0x4;
      pressed |= 0x4;                 // The button was held down (fast advance)
    }

  } else if (reading > 270) {
    // button 2 "SET" pressed
    if (! (last_buttonstate & 0x2)) { // was not pressed before
      // debounce by taking a second reading 
      _delay_ms(10);
      reading2 = readADC();
      if ( (reading2 > 610) || (reading2 < 270)) {
	// was a bounce, ignore it
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }
      DEBUG(putstring_nl("b2"));
      just_pressed |= 0x2;
    }
    last_buttonstate |= 0x2;
    pressed |= 0x2;                 // held down

  } else {
    // button 1 "MENU" pressed
    if (! (last_buttonstate & 0x1)) { // was not pressed before
      // debounce by taking a second reading 
      _delay_ms(10);
      reading2 = readADC();
      if (reading2 > 270) {
	// was a bounce, ignore it
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }
      DEBUG(putstring_nl("b1"));
      just_pressed |= 0x1;
    }
    last_buttonstate |= 0x1;
    pressed |= 0x1;                 // held down

  }
  ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  
}

// We use the pin change interrupts to detect when alarm changes
SIGNAL(PCINT0_vect) {
  // allow interrupts while we're doing this
  sei();
  setalarmstate();
}
