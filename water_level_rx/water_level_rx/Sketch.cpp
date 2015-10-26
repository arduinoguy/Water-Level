
#define F_CPU 1000000UL
#define RATE  9000
#define RX_PIN  12
#define STR_LEN  8
#define MOTOR   11
#define ADDRESS "1234567"
#define ON      "0000001"
#define OFF     "0000000"

#include <Arduino.h>
#include "VirtualWire.h"


void setup();
void loop();


void setup() 
{
	pinMode(MOTOR,1);
	Serial.begin(4800);
	Serial.println("Hi");
	vw_set_rx_pin(12);
	vw_setup(RATE);
	vw_rx_start();
 
}

void loop() {
    uint8_t rec[VW_MAX_MESSAGE_LEN];
    uint8_t msglen=VW_MAX_MESSAGE_LEN;
	
	vw_wait_rx();
	
	if(vw_get_message(rec,&msglen))
	{
	   
	   if(strcmp((char*)rec,ADDRESS)==0)
	   {
		   Serial.println((char*)rec);
		   //vw_wait_rx_max(5000);
		   vw_wait_rx();
		   if(vw_get_message(rec,&msglen))
		   {
			   Serial.println((char*)rec);
			   
			   if(strcmp((char*)rec,ON)==0)
			   digitalWrite(MOTOR,1);
			   
			   else if(strcmp((char*)rec,OFF)==0)
			        digitalWrite(MOTOR,0);
		   }
		   
	   }
	   
	}
	
	
}
