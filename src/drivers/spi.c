/*
 * SPI.c
 *
 * Created: 22/09/2014 17:18:08
 *  \author: Leonardo Ricupero
 */ 

#include "micro.h"
#include "spi.h"

#define DDR_SPI DDRB
#define DDR_MOSI DDB3
#define DDR_SCK DDB5
#define DDR_CSN DDB2

#define PORT_SPI PORTB
#define PIN_CSN PORTB2

#define TX_BUFFER_SIZE 16
#define RX_BUFFER_SIZE 16

#define SPI_DRIVE_CSN_LOW() {PORT_SPI &= ~(1 << PIN_CSN);}
#define SPI_DRIVE_CSN_HIGH() {PORT_SPI |= (1 << PIN_CSN);}

static uint8_t Tx_Buffer[TX_BUFFER_SIZE];
static uint8_t Rx_Buffer[RX_BUFFER_SIZE];
static uint8_t Tx_Idx;
static uint8_t Rx_Idx;

static BOOL_T Pending_Write;
static BOOL_T Pending_Read;

/**
 * Initialize SPI in master mode
 * 
 */
void Spi__Initialize(void)
{
	// Set MOSI ,SCK, and CSN as output, all other pin as input
	DDR_SPI = (1 << DDR_SCK) | (1 << DDR_MOSI) | (1 << DDR_CSN);
	// Enable SPI, Master, set clock rate fck/16, IRQ enabled
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPIE);
	// Set CSN high to start with, because nothing has to be transmitted
	SPI_DRIVE_CSN_HIGH();

	// Buffers initialization
    Tx_Idx = 0;
    Rx_Idx = 0;
    Pending_Write = FALSE;
    Pending_Read = FALSE;
}

void Spi__PutChar(uint8_t c)
{
    if (Tx_Idx >= TX_BUFFER_SIZE - 1)
    {
        Tx_Idx = 0;
    }
    Tx_Buffer[Tx_Idx] = c;
    Tx_Idx++;
}

uint8_t Spi__GetChar(void)
{
    uint8_t c = 0;

    c = Rx_Buffer[Rx_Idx];
    if (Rx_Idx >= 0)
    {
        Rx_Idx--;
    }

    return c;
}

void Spi__WriteThenRead(uint8_t c)
{
    Pending_Read = TRUE;
    Spi__PutChar(c);
}

BOOL_T Spi__IsRxBufferEmpty(void)
{
    uint8_t res = TRUE;

    if (Rx_Idx != 0)
    {
        res = FALSE;
    }

    return res;
}

BOOL_T Spi__IsTxBufferEmpty(void)
{
    uint8_t res = TRUE;

    if (Tx_Idx != 0)
    {
        res = FALSE;
    }

    return res;
}

void Spi__FastTask(void)
{
    if (Tx_Idx > 0)
    {
        if (!Pending_Write)
        {
            Tx_Idx--;
            SPI_DRIVE_CSN_LOW();
            SPDR = Tx_Buffer[Tx_Idx];
            Pending_Write = TRUE;
        }
    }
}

ISR(SPI_STC_vect)
{
    if (Pending_Read)
    {
        Rx_Buffer[Rx_Idx] = SPDR;
        if (Rx_Idx < RX_BUFFER_SIZE)
        {
            Rx_Idx++;
        }
        else
        {
            Rx_Idx = 0;
        }
        Pending_Read = FALSE;
    }
    Pending_Write = FALSE;
    SPI_DRIVE_CSN_HIGH();
}
