/**
 * @file
 *
 * @date 25/09/2015 23:23:35
 * @author Leonardo Ricupero
 */ 

#include "micro.h"
#include "timer.h"

// User parameters
#define TIMER_PRESCALER 	64
#define TIMER_COMPARE_VALUE 250

#if (TIMER_PRESCALER == 1)
	#define TIMER_PRESC_SHIFT 0
#elif (TIMER_PRESCALER == 8)
	#define TIMER_PRESC_SHIFT 3
#elif (TIMER_PRESCALER == 64)
	#define TIMER_PRESC_SHIFT 6
#elif (TIMER_PRESCALER == 256)
	#define TIMER_PRESC_SHIFT 8
#elif (TIMER_PRESCALER == 1024)
	#define TIMER_PRESC_SHIFT 10
#else
	#error "Invalid timer prescaler value!!"
#endif

uint16_t Timer_Counter;

static uint32_t Timer_Frequency;


void Timer__Initialize(void)
{
	// Mode selection
	// CTC
	TCCR0A |= (1 << WGM01) | (0 << WGM00);

	// Top value for CTC mode
	OCR0A = TIMER_COMPARE_VALUE;

	// Interrupt enable
	TIMSK0 |= (1 << OCIE0A);

	Timer_Frequency = Micro__GetClockFrequency() >> TIMER_PRESC_SHIFT;
	Timer_Counter = 0;
	Timer__Start();

}

void Timer__Start(void)
{
    // Clock source selection -- Timer enable
    #if (TIMER_PRESCALER == 1)
        TCCR0B |= (0  << CS02) | (0 << CS01) | (1 << CS00);
    #elif (TIMER_PRESCALER == 8)
        TCCR0B |= (0  << CS02) | (1 << CS01) | (0 << CS00);
    #elif (TIMER_PRESCALER == 64)
        TCCR0B |= (0  << CS02) | (1 << CS01) | (1 << CS00);
    #elif (TIMER_PRESCALER == 256)
        TCCR0B |= (1  << CS02) | (0 << CS01) | (0 << CS00);
    #elif (TIMER_PRESCALER == 1024)
        TCCR0B |= (1  << CS02) | (0 << CS01) | (1 << CS00);
    #endif
}
