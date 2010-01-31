/*! \file ks0108conf.h \brief Graphic LCD driver configuration. */
//*****************************************************************************
//
// File Name	: 'ks0108conf.h'
// Title		: Graphic LCD driver for HD61202/KS0108 displays
// Author		: Pascal Stang - Copyright (C) 2001-2003
// Date			: 10/19/2001
// Revised		: 5/1/2003
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


#ifndef KS0108CONF_H
#define KS0108CONF_H

// define LCD hardware interface
// -LCD_MEMORY_INTERFACE assumes that the registers of the LCD have been mapped
// into the external memory space of the AVR processor memory bus
// -LCD_PORT_INTERFACE is a direct-connection interface from port pins to LCD
// SELECT (UNCOMMENT) ONLY ONE!

// *** NOTE: memory interface is not yet fully supported, but it might work

//#define GLCD_MEMORY_INTERFACE
#define GLCD_PORT_INTERFACE

// GLCD_PORT_INTERFACE specifics
#ifdef GLCD_PORT_INTERFACE
	// make sure these parameters are not already defined elsewhere
	#ifndef GLCD_CTRL_PORT

#define GLCD_CTRL_RS 7
#define GLCD_CTRL_RS_PORT PORTB
#define GLCD_CTRL_RS_DDR DDRB

#define GLCD_CTRL_RW 5
#define GLCD_CTRL_RW_PORT PORTB
#define GLCD_CTRL_RW_DDR DDRB

#define GLCD_CTRL_E 4
#define GLCD_CTRL_E_PORT PORTB
#define GLCD_CTRL_E_DDR DDRB

#define GLCD_CTRL_CS0 0
#define GLCD_CTRL_CS0_PORT PORTC
#define GLCD_CTRL_CS0_DDR DDRC

#define GLCD_CTRL_CS1 2
#define GLCD_CTRL_CS1_PORT PORTD
#define GLCD_CTRL_CS1_DDR DDRD

  // this too
  //#define GLCD_CTRL_RESET PC4
  //#define GLCD_CTRL_RESET_PORT PORTC
  //#define GLCD_CTRL_RESET_DDR DDRC
		// (*) NOTE: additonal controller chip selects are optional and 
		// will be automatically used per each step in 64 pixels of display size
		// Example: Display with 128 hozizontal pixels uses 2 controllers
	#endif
	#ifndef GLCD_DATA_PORT
  // we split the data port into 2 4byte chunks so we dont bash the SPI or RXTX pins
  //#define GLCD_DATA_PORT	PORTD	// PORT for LCD data signals
  //		#define GLCD_DATA_DDR	DDRD	// DDR register of LCD_DATA_PORT
  //		#define GLCD_DATA_PIN	PIND	// PIN register of LCD_DATA_PORT


		#define GLCD_DATAH_PORT	PORTD	// PORT for LCD data signals
		#define GLCD_DATAH_DDR	DDRD	// DDR register of LCD_DATA_PORT
		#define GLCD_DATAH_PIN	PIND	// PIN register of LCD_DATA_PORT
		#define GLCD_DATAL_PORT	PORTB	// PORT for LCD data signals
		#define GLCD_DATAL_DDR	DDRB	// DDR register of LCD_DATA_PORT
		#define GLCD_DATAL_PIN	PINB	// PIN register of LCD_DATA_PORT
	#endif
#endif

// GLCD_MEMORY_INTERFACE specifics
#ifdef GLCD_MEMORY_INTERFACE
	// make sure these parameters are not already defined elsewhere
	#ifndef GLCD_CONTROLLER0_CTRL_ADDR
		// absolute address of LCD Controller #0 CTRL and DATA registers
		#define GLCD_CONTROLLER0_CTRL_ADDR	0x1000
		#define GLCD_CONTROLLER0_DATA_ADDR	0x1001
		// offset of other controllers with respect to controller0
		#define GLCD_CONTROLLER_ADDR_OFFSET	0x0002
	#endif
#endif


// LCD geometry defines (change these definitions to adapt code/settings)
#define GLCD_XPIXELS			128		// pixel width of entire display
#define GLCD_YPIXELS			64		// pixel height of entire display
#define GLCD_CONTROLLER_XPIXELS	64		// pixel width of one display controller

// Set text size of display
// These definitions are not currently used and will probably move to glcd.h
#define GLCD_TEXT_LINES           8     // visible lines
#define GLCD_TEXT_LINE_LENGTH    22     // internal line length

#endif
