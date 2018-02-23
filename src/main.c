/**
 * \file SMART_NODE.c
 *
 * \brief SMART NODE v0.1
 *
 * Firmware of the radio module SMART NODE, based on MCU ATMEGA328P
 * and radio module nRF24L01+, with temperature sensor DS18B20
 *
 * @todo Need to update the while loop to get rid of the delays.
 * Could use the WDT or timer in order to wait for acquisition to start.
 * This could be done once the 1 wire com driver gets updated.
 *	
 * \date: 22/09/2014 12:40:22
 * \author: Leonardo Ricupero
 */ 


#include "micro.h"
#include "timer.h"
#include "usart.h"
#include "spi.h"
#include "radio.h"
#include "temp_sensor.h"
#include "thermostat.h"
#include "parameters.h"
#include "relays.h"
#include "ui.h"
#include "main.h"

int main(void)
{
	// Initialization routines
	Timer__Initialize();
	Usart__Initialize();
	Relays__Initialize();
	Ui__Initialize();
	TempSensor__Initialize();
	Thermostat__Initialize();
	Micro__EnableInterrupts();

	Ui__LedBlink500ms(5);

	// Endless loop
	while(1)
    {
	    Usart__FastTask();
    }
}

/**
 * Timer 0 compare match ISR
 *
 * This shall be triggered every 1 ms
 */
ISR(TIMER0_COMPA_vect, ISR_NOBLOCK)
{
    uint8_t prescaler;
    
	// Increment the base counter
    prescaler = Timer__GetCounter()++;

	// Execute the 1ms tasks
    TempSensor__1msTask();
    Relays__1msTask();
    
	// Execute the 100ms tasks
	if (prescaler == 100)
	{
	    Timer__ResetCounter();
        Thermostat__100msTask();
	    Ui__100msTask();
	}

}
