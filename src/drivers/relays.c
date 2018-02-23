/**
 * @file
 *
 * @date 21/11/2014
 *
 * @author Leonardo Ricupero
 */ 

#include "micro.h"
#include <avr/interrupt.h>
#include "relays.h"

#define RELAYS_ACTION_DELAY_MS 4

typedef enum {
	STATE_INIT = 0,
	STATE_WAIT_FOR_INIT_RESET,
	STATE_SET,
	STATE_RESET,
	STATE_WAIT_FOR_SET,
	STATE_WAIT_FOR_RESET,
} RELAYS_STATE_T;

typedef enum {
	EVENT_NO_EVENT,
	EVENT_RELAY_SET_REQUESTED,
	EVENT_RELAY_RESET_REQUESTED,
} RELAYS_EVENT_T;

static RELAY_T Current_Relay;
static RELAYS_STATE_T Relay0_State;
static RELAYS_STATE_T Relay1_State;
static RELAYS_EVENT_T Relays_Event;
static uint8_t Countdown_Timer_Ms;

static inline void BeginMovePinForSet(RELAY_T relay);
static inline void EndMovePinForSet(RELAY_T relay);
static inline void BeginMovePinForReset(RELAY_T relay);
static inline void EndMovePinForReset(RELAY_T relay);

/**
 * @brief	Relays module initialization
 *
 * @details Initializes the relays to a known state, as reported on the
 * 			silkscreen of the board, providing a 4ms pulse on the reset coil
 * 
 */
void Relays__Initialize(void)
{
	// Control pin as outputs
	DDRD |= (1 << DDD3) | (1 << DDD4) | (1 << DDD5) | (1 << DDD6);

	Relay0_State = STATE_INIT;
	Current_Relay = RELAY_0; // only RELAY_0 will have its state automatically changed
	Relay1_State = STATE_RESET; // so we put STATE_RESET as initial state for RELAY_1
	Relays_Event = EVENT_RELAY_RESET_REQUESTED;
}


void Relays__Set(RELAY_T relay)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        Current_Relay = relay;
        Relays_Event = EVENT_RELAY_SET_REQUESTED;
    }
}


void Relays__Reset(RELAY_T relay)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        Current_Relay = relay;
        Relays_Event = EVENT_RELAY_RESET_REQUESTED;
    }
}

void Relays__1msTask(void)
{
	RELAYS_STATE_T current_state, next_state;

	if (Countdown_Timer_Ms != 0)
	{
		Countdown_Timer_Ms--;
	}

	if (Relays_Event != EVENT_NO_EVENT)
	{
		if (Current_Relay == RELAY_0)
		{
			current_state = Relay0_State;
		}
		else
		{
			current_state = Relay1_State;
		}
		// by default, remains in the current state
		next_state = current_state;

		switch (current_state)
		{
			case STATE_INIT:
			{
				BeginMovePinForReset(RELAY_0);
				BeginMovePinForReset(RELAY_1);
				Countdown_Timer_Ms = RELAYS_ACTION_DELAY_MS;
				next_state = STATE_WAIT_FOR_INIT_RESET;
				break;
			}
			case STATE_WAIT_FOR_INIT_RESET:
			{
				if (Countdown_Timer_Ms == 0)
				{
					EndMovePinForReset(RELAY_0);
					EndMovePinForReset(RELAY_1);
					next_state = STATE_RESET;
				}
				break;
			}
			case STATE_WAIT_FOR_SET:
			{
				if (Countdown_Timer_Ms == 0)
				{
					EndMovePinForSet(Current_Relay);
					next_state = STATE_SET;
				}
				break;
			}
			case STATE_WAIT_FOR_RESET:
			{
				if (Countdown_Timer_Ms == 0)
				{
					EndMovePinForReset(Current_Relay);
					next_state = STATE_RESET;
				}
				break;
			}
			case STATE_SET:
			{
				if (Relays_Event == EVENT_RELAY_RESET_REQUESTED)
				{
					BeginMovePinForReset(Current_Relay);
					Countdown_Timer_Ms = RELAYS_ACTION_DELAY_MS;
					next_state = STATE_WAIT_FOR_RESET;
				}
				else
				{
					Relays_Event = EVENT_NO_EVENT;
				}
				break;
			}
			case STATE_RESET:
			{
				if (Relays_Event == EVENT_RELAY_SET_REQUESTED)
				{
					BeginMovePinForSet(Current_Relay);
					Countdown_Timer_Ms = RELAYS_ACTION_DELAY_MS;
					next_state = STATE_WAIT_FOR_SET;
				}
				else
				{
					Relays_Event = EVENT_NO_EVENT;
				}
				break;
			}
			default:
			{
				break;
			}
		}

		if (Current_Relay == RELAY_0)
        {
            Relay0_State = next_state;
        }
        else
        {
            Relay1_State = next_state;
        }
	}
}

static inline void BeginMovePinForSet(RELAY_T relay)
{
	if (relay == RELAY_0)
	{
		PORTD |= (1 << PD3);
	}
	else if (relay == RELAY_1)
	{
		PORTD |= (1 << PD5);
	}
}

static inline void EndMovePinForSet(RELAY_T relay)
{
	if (relay == RELAY_0)
	{
		PORTD &= ~(1 << PD3);
	}
	else if (relay == RELAY_1)
	{
		PORTD &= ~(1 << PD5);
	}
}

static inline void BeginMovePinForReset(RELAY_T relay)
{
	if (relay == RELAY_0)
	{
		PORTD |= (1 << PD4);
	}
	else if (relay == RELAY_1)
	{
		PORTD |= (1 << PD6);
	}
}

static inline void EndMovePinForReset(RELAY_T relay)
{
	if (relay == RELAY_0)
	{
		PORTD &= ~(1 << PD4);
	}
	else if (relay == RELAY_1)
	{
		PORTD &= ~(1 << PD6);
	}
}
