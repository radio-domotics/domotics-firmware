/**
 * @file
 *
 * @brief 1-Wire driver for single device
 *
 * @details 	This driver provides support for 1-wire communication with
 * 				a single device in a non-blocking way.
 * 				The driver uses the TC1 in CTC mode and a state machine in order
 * 				to implement the required delays without blocking the SW execution.
 *
 * @date 24/12/2017
 * @author Leonardo Ricupero
 */ 

#include "micro.h"
#include "onewire.h"

// Ticks for a delay of 1 microsecond timer clocked at 2 MHz
#define TICKS_PER_MICROSECOND 2 // 2 * 0.5 us = 1 us

#define DELAY_1_US (1 * TICKS_PER_MICROSECOND)
#define DELAY_2_US (2 * TICKS_PER_MICROSECOND)
#define DELAY_6_US (6 * TICKS_PER_MICROSECOND)
#define DELAY_15_US (15 * TICKS_PER_MICROSECOND)
#define DELAY_64_US (64 * TICKS_PER_MICROSECOND)
#define DELAY_60_US (60 * TICKS_PER_MICROSECOND)
#define DELAY_10_US (10 * TICKS_PER_MICROSECOND)
#define DELAY_9_US (9 * TICKS_PER_MICROSECOND)
#define DELAY_55_US (55 * TICKS_PER_MICROSECOND)
#define DELAY_480_US (480 * TICKS_PER_MICROSECOND)
#define DELAY_70_US (70 * TICKS_PER_MICROSECOND)
#define DELAY_410_US (410 * TICKS_PER_MICROSECOND)

#define DELAY_PRESENCE_INIT     DELAY_480_US
#define DELAY_PRESENCE_SAMPLE   DELAY_60_US
#define DELAY_PRESENCE_RECOVERY DELAY_410_US

#define DELAY_READ_INIT         DELAY_6_US
#define DELAY_READ_SAMPLE       (DELAY_15_US - DELAY_READ_INIT)
#define DELAY_READ_RECOVERY     (DELAY_70_US - DELAY_READ_INIT - DELAY_READ_SAMPLE)

#define DELAY_WRITE0_INIT       DELAY_60_US
#define DELAY_WRITE0_RECOVERY   DELAY_10_US

#define DELAY_WRITE1_INIT       DELAY_6_US
#define DELAY_WRITE1_RECOVERY   DELAY_64_US

// Start the counter at F_CPU / 2 MHz
#define TIMER1__START() {TCCR1B |= (0 << CS02) | (1 << CS01) | (0 << CS00);}
#define TIMER1__STOP()  {TCCR1B &= 0b11111000;}
#define TIMER1__GET_COUNTER() TCNT1
#define TIMER1__RESET_COUNTER() {TCNT1 = 0;}
#define TIMER1__SET_DELAY(delay) {OCR1A = delay;}
#define TIMER1__TRIGGER_DELAY(delay) {TIMER1__RESET_COUNTER(); TIMER1__SET_DELAY(delay); TIMER1__START();}

#define DELAY_BLOCKING(x) Micro__WaitFourClockCycles(x << 1)

typedef enum {
	ONEWIRE_IDLE = 0,
	ONEWIRE_PRESENCE_SAMPLE,
	ONEWIRE_PRESENCE_DRIVE_LOW,
	ONEWIRE_PRESENCE_RECOVERY,
	ONEWIRE_WRITE1_RECOVERY,
	ONEWIRE_WRITE0_RECOVERY,
	ONEWIRE_READBIT_RECOVERY,
} ONEWIRE_STATE_T;

ONEWIRE_SAMPLE_T Last_Sample;
uint8_t Byte_Read;

uint16_t Debug_Counter = 0;

static volatile ONEWIRE_STATE_T Onewire_State;
static uint8_t Byte_To_Write;
static uint8_t Last_Byte;
static uint8_t Remaining_Bits;

void Onewire__Initialize(void)
{
    // Timer initialization
    TIMER1__STOP();
    // Select Normal mode
	TCCR1B |= (1 << WGM12);
	// Enable ISR at compare
	TIMSK1 |= (1 << OCIE1A);

	TIMER1__SET_DELAY(0xFFFF);
	TIMER1__RESET_COUNTER();

    ONEWIRE_RELEASE_BUS();

	Onewire_State = ONEWIRE_IDLE;
	Last_Sample = ONEWIRE_DATA_NOT_READY;
	Last_Byte = ONEWIRE_DATA_NOT_READY;
	Remaining_Bits = 0;
	Byte_To_Write = 0xFF;
	Byte_Read = 0xFF;
}


void Onewire__DetectPresence(void)
{
    TIMER1__RESET_COUNTER();
	TIMER1__SET_DELAY(DELAY_PRESENCE_INIT);

	ONEWIRE_DRIVE_BUS_LOW();
	TIMER1__START();

	Onewire_State = ONEWIRE_PRESENCE_DRIVE_LOW;
}


ONEWIRE_SAMPLE_T Onewire__GetPresence(void)
{
	uint8_t result = ONEWIRE_PRESENCE_NOK;
	if (Last_Sample == 0)
	{
		result = ONEWIRE_PRESENCE_OK;
	}

	Last_Sample = ONEWIRE_DATA_NOT_READY;

	return result;
}


void Onewire__WriteBit(uint8_t bit)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (bit)
        {
            ONEWIRE_DRIVE_BUS_LOW();
            DELAY_BLOCKING(DELAY_WRITE1_INIT);
            ONEWIRE_RELEASE_BUS();
            Onewire_State = ONEWIRE_WRITE1_RECOVERY;
            TIMER1__TRIGGER_DELAY(DELAY_WRITE1_RECOVERY);
        }
        else
        {
            ONEWIRE_DRIVE_BUS_LOW();
            DELAY_BLOCKING(DELAY_WRITE0_INIT);
            ONEWIRE_RELEASE_BUS();
            Onewire_State = ONEWIRE_WRITE0_RECOVERY;
            TIMER1__TRIGGER_DELAY(DELAY_WRITE0_RECOVERY);
        }
    }
}

uint8_t Onewire__ReadBit(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ONEWIRE_DRIVE_BUS_LOW();
        DELAY_BLOCKING(DELAY_READ_INIT);
        ONEWIRE_RELEASE_BUS();
        DELAY_BLOCKING(DELAY_READ_SAMPLE);
        Last_Sample = ONEWIRE_SAMPLE_BUS();
        Onewire_State = ONEWIRE_READBIT_RECOVERY;
        TIMER1__TRIGGER_DELAY(DELAY_READ_RECOVERY);
    }
    return Last_Sample;
}

void Onewire__WriteByte(uint8_t data)
{
	Byte_To_Write = data;
	Remaining_Bits = 7;
	Onewire__WriteBit(Byte_To_Write & 0x1);
}

void Onewire__StartReadByte(void)
{
    Byte_Read = 0;
    Remaining_Bits = 7;
	Onewire__ReadBit();
}


uint8_t Onewire__IsIdle(void)
{
	uint8_t result = 0;
	if (Onewire_State == ONEWIRE_IDLE)
	{
		result = 1;
	}
	return result;
}


/**
 * Timer 1 compare match ISR
 *
 */
ISR(TIMER1_COMPA_vect)
{
    TIMER1__STOP();
	switch (Onewire_State)
	{
	    case ONEWIRE_PRESENCE_DRIVE_LOW:
	    {
	        ONEWIRE_RELEASE_BUS();
	        TIMER1__TRIGGER_DELAY(DELAY_70_US);
	        Onewire_State = ONEWIRE_PRESENCE_SAMPLE;
	        break;
	    }
	    case ONEWIRE_PRESENCE_SAMPLE:
	    {
            Last_Sample = ONEWIRE_SAMPLE_BUS();
            TIMER1__TRIGGER_DELAY(DELAY_410_US);
            Onewire_State = ONEWIRE_PRESENCE_RECOVERY;
	        break;
	    }
	    case ONEWIRE_PRESENCE_RECOVERY:
	    {
	        Onewire_State = ONEWIRE_IDLE;
	        break;
	    }
	    case ONEWIRE_READBIT_RECOVERY:
	    {
	        Byte_Read |= (Last_Sample << 7);
	        if (Remaining_Bits != 0)
            {
	            Byte_Read >>= 1;
                Onewire__ReadBit();
                Remaining_Bits--;
            }
            else
            {
                Onewire_State = ONEWIRE_IDLE;
            }
	        break;
	    }
	    case ONEWIRE_WRITE0_RECOVERY:
	    case ONEWIRE_WRITE1_RECOVERY:
	    {
	        if (Remaining_Bits != 0)
            {
                Remaining_Bits--;
                Byte_To_Write = Byte_To_Write >> 1;
                Onewire__WriteBit(Byte_To_Write & 0x1);
            }
            else
            {
                Onewire_State = ONEWIRE_IDLE;
            }
	        break;
	    }
	    default:
	    {
	        break;
	    }
	}
}

