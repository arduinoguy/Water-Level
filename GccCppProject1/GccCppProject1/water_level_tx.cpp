
#define F_CPU 16000000UL
#define TX_PIN 12
#define ADDRESS "1234567" 
#define ON		"0000001"
#define OFF     "0000000"
#define BOTTOM_SENSOR 2
#define TOP_SENSOR    3

bool start_flag=false;
bool stop_flag=false;

#include <Arduino.h>
#include "VirtualWire.h"

void setup();
void loop();
void start();
void stop();
void real_start();
void real_stop();



void setup() 
{
  pinMode(13,1);
  DDRD&=~((1<<PORTD2)|(1<<PORTD3));
  PORTD|=(1<<PORTD2)|(1<<PORTD3);
  
  pinMode(BOTTOM_SENSOR,INPUT_PULLUP);
  pinMode(TOP_SENSOR,INPUT_PULLUP);
  
  attachInterrupt(0,start,RISING);
  attachInterrupt(1,stop,FALLING);
  
  vw_set_tx_pin(TX_PIN);
  vw_setup(2000);
  real_stop();
  
  if(digitalRead(BOTTOM_SENSOR)&&digitalRead(TOP_SENSOR))
  real_start();
  
  
}

void loop() 
{
   if(start_flag)
   real_start();
   
   else if(stop_flag)
   real_stop();
    
}


void start()
{
	 digitalWrite(13,!digitalRead(13));
	 
	 if(digitalRead(TOP_SENSOR))
	 start_flag=true;
	
}

void stop()
{digitalWrite(13,!digitalRead(13));
//	if(!digitalRead(BOTTOM_SENSOR))
	stop_flag=true;
	
}

void real_start()
{
	start_flag=false;
	vw_send((uint8_t*)ADDRESS,strlen(ADDRESS));
	vw_wait_tx();
	vw_send((uint8_t*)ON,strlen(ON));
	vw_wait_tx();
}

void real_stop()
{
	stop_flag=false;
	vw_send((uint8_t*)ADDRESS,strlen(ADDRESS));
	vw_wait_tx();
	vw_send((uint8_t*)OFF,strlen(OFF));
	vw_wait_tx();
}
