
// VirtualWire.c
//
// Virtual Wire implementation for Arduino
// See the README file in this directory fdor documentation
// See also
// ASH Transceiver Software Designer's Guide of 2002.08.07
//   http://www.rfm.com/products/apnotes/tr_swg05.pdf
//
// Changes:
// 1.5 2008-05-25: fixed a bug that could prevent messages with certain
//  bytes sequences being received (false message start detected)
// 1.6 2011-09-10: Patch from David Bath to prevent unconditional reenabling of the receiver
//  at end of transmission.
//
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: VirtualWire.cpp,v 1.9 2013/02/14 22:02:11 mikem Exp mikem $
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

#include "VirtualWireTX.h"
#include <util/crc16.h>

uint8_t vw_send(uint8_t*,uint8_t);
void vw_setup(uint16_t);
void vw_wait_tx();

#define false 0
#define true 1


#define TX_PIN_HIGH	PORTB |= _BV(PORTB0)
#define TX_PIN_LOW  PORTB &= ~_BV(PORTB0)
#define TX_PTT_ENABLE	PORTB |= _BV(PORTB1)
#define TX_PTT_DISABLE	PORTB &= ~_BV(PORTB1)
#define TX_PTT_INVERT 0

/*
/ Add this section to your Init
// Set the output pin number for transmitter data

    DDRB |= _BV(DDB0); // Set TX Pin as output

    DDRB |= _BV(DDB1); // Set PTT Pin as output

*/

// Set the ptt pin inverted (low to transmit)
#if TX_PTT_INVERT == 1
 #define PTT_INVERT_HOLD TX_PTT_ENABLE
 #undef TX_PTT_ENABLE
 #define TX_PTT_ENABLE TX_PTT_DISABLE
 #undef TX_PTT_DISABLE
 #define TX_PTT_DISABLE PTT_INVERT_HOLD
#endif


static uint8_t vw_tx_buf[(VW_MAX_MESSAGE_LEN * 2) + VW_HEADER_LEN]
     = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c};

// Number of symbols in vw_tx_buf to be sent;
static uint8_t vw_tx_len = 0;

// Index of the next symbol to send. Ranges from 0 to vw_tx_len
static uint8_t vw_tx_index = 0;

// Bit number of next bit to send
static uint8_t vw_tx_bit = 0;

// Sample number for the transmitter. Runs 0 to 7 during one bit interval
static uint8_t vw_tx_sample = 0;

// Flag to indicated the transmitter is active
static volatile uint8_t vw_tx_enabled = 0;

// Total number of messages sent
static uint16_t vw_tx_msg_count = 0;

// 4 bit to 6 bit symbol converter table
// Used to convert the high and low nybbles of the transmitted data
// into 6 bit symbols for transmission. Each 6-bit symbol has 3 1s and 3 0s
// with at most 3 consecutive identical bits
static uint8_t symbols[] =
{
    0xd,  0xe,  0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
    0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34
};

/*
// Convert a 6 bit encoded symbol into its 4 bit decoded equivalent
uint8_t vw_symbol_6to4(uint8_t symbol)
{
    uint8_t i;

    // Linear search :-( Could have a 64 byte reverse lookup table?
    for (i = 0; i < 16; i++)
	if (symbol == symbols[i]) return i;
    return 0; // Not found
}
*/

// Common function for setting timer ticks @ prescaler values for speed
// Returns prescaler index into {0, 1, 8, 64, 256, 1024} array
// and sets nticks to compare-match value if lower than max_ticks
// returns 0 & nticks = 0 on fault
static uint8_t _timer_calc(uint16_t speed, uint16_t max_ticks, uint16_t *nticks)
{
	// Clock divider (prescaler) values - 0/3333: error flag
	uint16_t prescalers[] = {0, 1, 8, 64, 256, 1024, 3333};
	uint8_t prescaler=0; // index into array & return bit value
	unsigned long ulticks; // calculate by ntick overflow

	// Div-by-zero protection
	if (speed == 0)
	{
		// signal fault
		*nticks = 0;
		return 0;
	}

	// test increasing prescaler (divisor), decreasing ulticks until no overflow
	for (prescaler=1; prescaler < 7; prescaler += 1)
	{
		// Amount of time per CPU clock tick (in seconds)
		float clock_time = (1.0 / ((float)F_CPU / (float)prescalers[prescaler]));
		// Fraction of second needed to xmit one bit
		float bit_time = ((1.0 / (float)speed) / 8.0);
		// number of prescaled ticks needed to handle bit time @ speed
		ulticks = (long)(bit_time / clock_time);
		// Test if ulticks fits in nticks bitwidth (with 1-tick safety margin)
		if ((ulticks > 1) && (ulticks < max_ticks))
		{
			break; // found prescaler
		}
		// Won't fit, check with next prescaler value
	}

	// Check for error
	if ((prescaler == 6) || (ulticks < 2) || (ulticks > max_ticks))
	{
		// signal fault
		*nticks = 0;
		return 0;
	}

	*nticks = ulticks;
	return prescaler;
}

void vw_setup(uint16_t speed)
{
	uint16_t nticks; // number of prescaled ticks needed
	uint8_t prescaler; // Bit values for CS0[2:0]

    DDRB |= _BV(DDB0); // Set TX Pin as output

    DDRB |= _BV(DDB1); // Set PTT Pin as output
// Speed is in bits per sec RF rate

	prescaler = _timer_calc(speed, (uint16_t)-1, &nticks);
	 if (!prescaler)
	 {
		 return; // fault
	 }
	 
	  TCCR1A = 0; // Output Compare pins disconnected
	  TCCR1B = _BV(WGM12); // Turn on CTC mode

	  // convert prescaler index to TCCRnB prescaler bits CS10, CS11, CS12
	  TCCR1B |= prescaler;

	  // Caution: special procedures for setting 16 bit regs
	  // is handled by the compiler
	  OCR1A = nticks;
	  // Enable interrupt
	  #ifdef TIMSK1
	  // atmega168
	  TIMSK1 |= _BV(OCIE1A);
	  #else
	  // others
	  TIMSK |= _BV(OCIE1A);
	  #endif // TIMSK1
	 
	 
   /* TCCR0A = 0;
    TCCR0A |= _BV(WGM01); // Turn on CTC mode / Output Compare pins disconnected

    // convert prescaler index to TCCRnB prescaler bits CS00, CS01, CS02
    TCCR0B = 0;
    TCCR0B |= _BV(CS01); //prescaler; // set CS00, CS01, CS02 (other bits not needed)

    // Number of ticks to count before firing interrupt
    OCR0A = 124; //uint8_t(nticks);

    // Set mask to fire interrupt when OCF0A bit is set in TIFR0
    TIMSK |= _BV(OCIE0A);*/

	TX_PTT_DISABLE;
}


// Start the transmitter, call when the tx buffer is ready to go and vw_tx_len is
// set to the total number of symbols to send
void vw_tx_start()
{
    vw_tx_index = 0;
    vw_tx_bit = 0;
    vw_tx_sample = 0;

    // Enable the transmitter hardware
    TX_PTT_ENABLE;

    // Next tick interrupt will send the first bit
    vw_tx_enabled = true;
}

// Stop the transmitter, call when all bits are sent
void vw_tx_stop()
{
    // Disable the transmitter hardware
    TX_PTT_DISABLE;
    TX_PIN_LOW;
	//bit_clear(DDRB, DDRB0);

    // No more ticks for the transmitter
    vw_tx_enabled = false;
}

// Return true if the transmitter is active
uint8_t vx_tx_active()
{
    return vw_tx_enabled;
}

// Wait for the transmitter to become available
// Busy-wait loop until the ISR says the message has been sent
void vw_wait_tx()
{
    while (vw_tx_enabled);
}


// Wait until transmitter is available and encode and queue the message
// into vw_tx_buf
// The message is raw bytes, with no packet structure imposed
// It is transmitted preceded a byte count and followed by 2 FCS bytes
uint8_t vw_send(uint8_t* buf, uint8_t len)
{
    uint8_t i;
    uint8_t index = 0;
    uint16_t crc = 0xffff;
    uint8_t *p = vw_tx_buf + VW_HEADER_LEN; // start of the message area
    uint8_t count = len + 3; // Added byte count and FCS to get total number of bytes

    if (len > VW_MAX_PAYLOAD)
	return false;

    // Wait for transmitter to become available
    vw_wait_tx();

    // Encode the message length
    crc = _crc_ccitt_update(crc, count);
    p[index++] = symbols[count >> 4];
    p[index++] = symbols[count & 0xf];

    // Encode the message into 6 bit symbols. Each byte is converted into
    // 2 6-bit symbols, high nybble first, low nybble second
    for (i = 0; i < len; i++)
    {
	crc = _crc_ccitt_update(crc, buf[i]);
	p[index++] = symbols[buf[i] >> 4];
	p[index++] = symbols[buf[i] & 0xf];
    }

    // Append the fcs, 16 bits before encoding (4 6-bit symbols after encoding)
    // Caution: VW expects the _ones_complement_ of the CCITT CRC-16 as the FCS
    // VW sends FCS as low byte then hi byte
    crc = ~crc;
    p[index++] = symbols[(crc >> 4)  & 0xf];
    p[index++] = symbols[crc & 0xf];
    p[index++] = symbols[(crc >> 12) & 0xf];
    p[index++] = symbols[(crc >> 8)  & 0xf];

    // Total number of 6-bit symbols to send
    vw_tx_len = index + VW_HEADER_LEN;

    // Start the low level interrupt handler sending symbols
    vw_tx_start();

    return true;
}

ISR(TIMER1_COMPA_vect)
{

// Do transmitter stuff first to reduce transmitter bit jitter due 
    // to variable receiver processing
    if (vw_tx_enabled && vw_tx_sample++ == 0)
    {
	// Send next bit
	// Symbols are sent LSB first
	// Finished sending the whole message? (after waiting one bit period 
	// since the last bit)
	if (vw_tx_index >= vw_tx_len)
	{
	    vw_tx_stop();
	    vw_tx_msg_count++;
	}
	else
	{
	    if (!(vw_tx_buf[vw_tx_index] & (1 << vw_tx_bit++)))
		{
			TX_PIN_LOW;
		}
		else
		{
			TX_PIN_HIGH;
		}
		
	    if (vw_tx_bit >= 6)
	    {
		vw_tx_bit = 0;
		vw_tx_index++;
	    }
	}
    }
    if (vw_tx_sample > 7)
	vw_tx_sample = 0;
}
