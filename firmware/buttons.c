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

extern volatile uint8_t alarming;

void initbuttons(void) {
  ALARM_DDR &= ~_BV(ALARM);
  ALARM_PORT |= _BV(ALARM);

  PCICR =  _BV(PCIE0);
  PCMSK0 |= _BV(ALARM);

  // setup ADC
  ADMUX = 2;      // listen to ADC2 for button presses
  ADCSRB = 0;     // free running mode
  // enable ADC and interrupts, prescale down to <200KHz
  ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1); 
  ADCSRA |= _BV(ADSC); // start a conversion
}

uint16_t readADC(void) {
  ADCSRA &= ~_BV(ADIE); // no interrupt
  ADCSRA |= _BV(ADSC); // start a conversion
  while (! (ADCSRA & _BV(ADIF)));
  return ADC;
}

// we listen to the ADC to determine what button is pressed
SIGNAL(ADC_vect) {
  uint16_t reading, reading2;
  sei();

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
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }

      buttonholdcounter = 2;          // see if we're press-and-holding
      while (buttonholdcounter) {
	reading2 = readADC();
	if ( (reading2 > 735) || (reading2 < 610)) {
	  // released
	  last_buttonstate &= ~0x4;

	  DEBUGP("b3");
	  just_pressed |= 0x4;
	  ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	  return;
	}
      }
      last_buttonstate |= 0x4;
      pressed |= 0x4;                 // held down   
    }

  } else if (reading > 270) {
    // button 2 "SET" pressed
    if (! (last_buttonstate & 0x2)) { // was not pressed before

      // debounce by taking a second reading 
      _delay_ms(10);
      reading2 = readADC();
      if ( (reading2 > 610) || (reading2 < 270)) {
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }
      DEBUGP("b2");
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
	ADCSRA |= _BV(ADIE) | _BV(ADSC); // start next conversion  	
	return;
      }
      DEBUGP("b1");
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
