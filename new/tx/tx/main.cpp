// transmitter.pde
//
// Simple example of how to use VirtualWire to transmit messages
// Implements a simplex (one-way) transmitter with an TX-C1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: transmitter.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $
#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <string.h>

#include "VirtualWireTX.h"






void inline init() // Place uC init code here
{

//------------------Initialize clock settings
	/*CLKPR = (1<<CLKPCE);
	  	// A value = 128 sets CLKPCE Bit to 1 to enable the CLKPS bits
	CLKPR = 0x00;*/

//------------------initialize PORTS-------------

	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2) | (1<<DDB3) | (1<<DDB4);	// initialize PORTB Outputs

	PORTB = 0;//*(1<<PORTB0) |*/ (1<<PORTB1) | (1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4); // Enable pullups

/* Individual Bit manipulation

	DDRB &= ~(1<<DDB4);		// Define Port B4 as an input.(clears the bit)
	PORTD |= (1<<PD7);	// Enable Pullup on Button input Sets the bit

*/




}



//uint8_t count = 1;

int main(void) //Enter Main
{
sei();
init();
vw_setup(2000);

uint8_t msg[1] = {'s'};//, 20, 30, 40, 50};

for(;;) // Enter Loop
{
  

  //msg[6] = count;
//  digitalWrite(led_pin, 1); // Flash a light to show transmitting

  for (uint8_t i=0;i<5; i++)
  {

  msg[0] = (i+1)*10;
  vw_send((uint8_t *)msg, 1);

  vw_wait_tx(); // Wait until the whole message is gone
//  digitalWrite(led_pin, 0);
  _delay_ms(1000);
}
  //count = count + 1;
} // End Loop
} // End Main
