/*
 * ps.cpp
 *
 * Created: 10/26/2015 12:39:19 PM
 * Author : HELLO
 */ 
//#define SLEEP
#define F_CPU 1000000UL
#define SLEEP1

#include <avr/io.h>
#include <avr/sleep.h>

int main(void)
{
	DDRB=0;
	DDRC=0;
	DDRD=0;
	PORTB=~(1<<4);
	PORTC=0xff;
	PORTD=0xff;
	ACSR|=(1<<ACD);
	#ifdef SLEEP
	  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
	  sleep_enable();
	  sleep_mode();
	  #endif
    /* Replace with your application code */
	
	#ifdef SLEEP1
	MCUCR|=(1<<SE)|(1<<SM1);
	MCUCR&=~((1<<SM0)|(1<<SM2));
	asm("SLEEP");
	#endif
	
    while (1) 
    {
    }
}

