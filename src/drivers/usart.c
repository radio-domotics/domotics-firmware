/*
 * \file USART.c
 *
 * Created: 02/10/2014 13:52:03
 * \author: Leonardo Ricupero
 */ 

#include "micro.h"
#include "usart.h"

#define BAUD_PRESCALE (uint16_t) ((F_CPU / (16.0f * USART_BAUDRATE)) -1)

#define TX_BUFFER_SIZE 16
#define RX_BUFFER_SIZE 16

static uint8_t Tx_Buffer[TX_BUFFER_SIZE];
static uint8_t Rx_Buffer[RX_BUFFER_SIZE];

static uint8_t Tx_Idx;
static uint8_t Rx_Idx;

/**
 * \brief Initializes the USART
 * 
 * Interrupt disabled
 *
 * \return void
 */
void Usart__Initialize(void)
{
	// Baud rate setting
	UBRR0H = (uint8_t) (BAUD_PRESCALE >> 8);
	UBRR0L = (uint8_t) BAUD_PRESCALE;

	// Frame format: 8 data bit, no parity, 1 stop bit
	UCSR0C = (3 << UCSZ00);

	// Interrupts enable - RX and data empty
	UCSR0B = 0;
	UCSR0B |= (1 << RXCIE0);

	// Enable transmitter and receiver
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    // Buffers initialization
    Tx_Idx = 0;
    Rx_Idx = 0;
}

uint8_t Usart__GetChar(void)
{
    uint8_t c = 0;

    c = Rx_Buffer[Rx_Idx];
    if (Rx_Idx >= 0)
    {
        Rx_Idx--;
    }

    return c;
}

void Usart__PutChar(uint8_t c)
{
    if (Tx_Idx >= TX_BUFFER_SIZE - 1)
    {
        Tx_Idx = 0;
    }
    Tx_Buffer[Tx_Idx] = c;
    Tx_Idx++;
}

BOOL_T Usart__IsRxBufferEmpty(void)
{
    uint8_t res = TRUE;

    if (Rx_Idx != 0)
    {
        res = FALSE;
    }

    return res;
}

BOOL_T Usart__IsTxBufferEmpty(void)
{
    uint8_t res = TRUE;

    if (Tx_Idx != 0)
    {
        res = FALSE;
    }

    return res;
}

ISR(USART_RX_vect)
{
    Rx_Buffer[Rx_Idx] = UDR0;
    if (Rx_Idx < RX_BUFFER_SIZE)
    {
        Rx_Idx++;
    }
    else
    {
        Rx_Idx = 0;
    }
}

void Usart__FastTask(void)
{
    if (Tx_Idx > 0)
    {
        if ((UCSR0A & (1<<UDRE0)) == 1)
        {
            Tx_Idx--;
            UDR0 = Tx_Buffer[Tx_Idx];
        }
    }
}
