/*
 * \file adc.c
 *
 * \date: 22/11/2014 19:28:36
 * \author: Leonardo Ricupero
 */ 

#include "adc.h"

/**
 * \brief Initializes the ADC
 * 
 * \return void
 */
void ADCinit(void)
{
	// To-Do: complete initialization
	// Reference selection: AVCC
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);
	// Channel selection: ADC0
	ADMUX &= (1 << MUX0) & (1 << MUX1) & (1 << MUX2) & (1 << MUX3);
	// Left Adjust (8 bit resolution)
	ADMUX |= (1 << ADLAR);
	// 
	ADCSRA &= ~(1 << ADPS0);
	ADCSRA |= (1 << ADPS1) | (1 << ADPS2);
}
