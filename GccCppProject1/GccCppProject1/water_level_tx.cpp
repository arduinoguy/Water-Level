

#define F_CPU 1000000UL
#include <Arduino.h>
#include <avr/sleep.h>

#define SLEEP 0
#define RATE 9000
#define TX_PIN 12
#define ADDRESS "1234567" 
#define ON		"0000001"
#define OFF     "0000000"
#define BOTTOM_SENSOR 2
#define TOP_SENSOR    3

bool start_flag=false;
bool stop_flag=false;

#include "VirtualWire.h"

void setup();
void loop();
void start();
void stop();
void real_start();
void real_stop();



void setup() 
{
  DDRB=0xff;
  DDRC=0xff;
  DDRD=0xff;
  DDRD&=~((1<<2)|(1<<3)|(1<<6));
  PORTB=0;
  PORTC=0;
  PORTD=(1<<3);

  /*pinMode(0,OUTPUT);
  pinMode(1,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);*/
   //pinMode(6,OUTPUT);
 // pinMode(7,OUTPUT);
  //digitalWrite(0,0);
  //digitalWrite(1,0);
  
  //pinMode(2,INPUT);
  //pinMode(3,INPUT_PULLUP);
  
  
  
  ACSR|=(1<<ACD);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
  
  
  vw_set_tx_pin(TX_PIN);
  vw_setup(RATE);

  
  real_stop();
  
  if(!digitalRead(BOTTOM_SENSOR)&&digitalRead(TOP_SENSOR))
  {
	  start_flag=true;
  }
  
  else attachInterrupt(0,start,LOW);
  
  
 
  }

void loop() 
{
   
   if(start_flag)
   real_start();
   
   else if(stop_flag)
   real_stop();
   
   #ifdef SLEEP
   sleep_enable(); 
   sleep_cpu();
//   sleep_disable();
   #endif
    
}


void start()
{    
	 detachInterrupt(0);
	 //digitalWrite(13,!digitalRead(13));
	 
	 if(!digitalRead(BOTTOM_SENSOR))
	 {
        if(digitalRead(TOP_SENSOR))
		{
	 	    start_flag=true;
			 
		}
	 }
	 
	 else attachInterrupt(0,start,LOW);
	
}

void stop()
{  
	detachInterrupt(1);
	
	//digitalWrite(13,!digitalRead(13));
	
    if(!digitalRead(TOP_SENSOR))
	{
		stop_flag=true;
		
	}
	
	else attachInterrupt(1,stop,LOW);
}

void real_start()
{
	start_flag=false;
	
	
	vw_send((uint8_t*)ADDRESS,strlen(ADDRESS));
	vw_wait_tx();
	vw_send((uint8_t*)ON,strlen(ON));
	vw_wait_tx();
	attachInterrupt(1,stop,LOW);
	
	
}

void real_stop()
{
	stop_flag=false;
	vw_send((uint8_t*)ADDRESS,strlen(ADDRESS));
	vw_wait_tx();
	vw_send((uint8_t*)OFF,strlen(OFF));
	vw_wait_tx();
	
	if(digitalRead(BOTTOM_SENSOR))
	attachInterrupt(0,start,LOW);
	
}



