/**
 * @file radio.c
 *
 * @date 22/09/2014 18:30:05
 * @authors Stefan Engelke, Leonardo Ricupero
 */ 

#include "micro.h"
#include "usart.h"
#include "spi.h"
#include "radio.h"

#define DEFAULT_ADDRESS_SIZE 5
#define DEFAULT_NODE_ADDRESS {0x00, 0x01, 0x02, 0x03, 0x04}

#define DELAY_TPD2STBY 5 // milliseconds

#define RADIO_DRIVE_CE_LOW()  {PORTB &= ~(1<<PORTB1);}
#define RADIO_DRIVE_CE_HIGH() {PORTB |= (1<<PORTB1);}

typedef enum {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_READING,
    STATE_CONFIGURING,
    STATE_ERROR_FOUND,
} RADIO_STATE_T;

typedef union {
    struct {
        uint8_t turning_on: 1;
        uint8_t on :1;
        uint8_t transmitting :1;
        uint8_t receiving :1;
        uint8_t rx_complete :1;
    };

    uint8_t all;
} RADIO_EVENTS_T;

static RADIO_STATE_T Radio_State;
static RADIO_EVENTS_T Radio_Events;

static uint8_t Node_Address[DEFAULT_ADDRESS_SIZE] = DEFAULT_NODE_ADDRESS;

static void WriteRegister(uint8_t reg, uint8_t value);
static void InitializeIRQ(void);

uint8_t* data;

/**
 * Setup the RF24 module
 * 
 * @brief Edit this function in order to change the initial configuration
 *        of the radio module
 * 
 */
void Radio__Initialize(void)
{
	uint8_t val;
	// Set CE low to start with, because nothing has to be transmitted
    RADIO_DRIVE_CE_LOW();
	
	// EN_AA - (enable auto-acknowledgments)
	// Transmitter gets automatic response from receiver in case of successful transmission
	// It only works if the TX module has the same RF_Address on its channel. ex: RX_ADDR_P0 = TX_ADDR
    val = (1 << BIT_ENAA_P0);
	WriteRegister(REG_EN_AA, &val, 1);
	
	// SETUP_RETR (the setup for "EN_AA")
	// 0b0010 00011 "2" sets it up to 750uS delay between every retry (at least 500us at 250kbps and if payload >5bytes in 1Mbps, and if payload >15byte in 2Mbps) "F" is number of retries (1-15, now 15)
	val = (2 << BIT_ARD) | (15 << BIT_ARC); //0x2F;
	WriteRegister(REG_SETUP_RETR, &val, 1);
	
	// Choose the number of the enabled RX data pipe (0-5)
	val = (1 << BIT_ERX_P0);
	WriteRegister(REG_EN_RXADDR, &val, 1); // enable data pipe 0
	
	// RF_Address width setup: how many bytes is the receiver address
	val = (0x03 << BIT_AW);
	WriteRegister(REG_SETUP_AW, &val, 1); // 5byte RF Address
	
	// RF channel setup - choose frequency 2.401 - 2.527 GHz, 1 MHz/step
	val = 0x4C;
	WriteRegister(REG_RF_CH, &val, 1); // 0b1101 0100 = 2,476 GHz (same on TX and RX)
	
	//RF setup - choose power mode and data speed
	val = (0 << BIT_RF_DR_HIGH) | (3 << BIT_RF_PWR); //0x07;
	WriteRegister(REG_RF_SETUP, &val, 1); //0b0000 0111 bit 3="0" 1Mbps=longer range, bit 2-1 power mode ("11" = 0dB)
	
	// RX RF_Adress width setup 5 byte - set receiver address (set RX_ADDR_P0 = TX_ADDR if EN_AA is enabled)
	// Address = 0x01 00 01 00 01
	// Since we chose pipe 0 and 1 on EN_RXADDR, we give this address to those channels.
	// P1 is the primary receiver address.
	// It is possible to assign different addresses to different channels (if they are enabled in EN_RXADDR) in order
	// to listen on several different transmitters
	WriteRegister(REG_RX_ADDR_P0, Node_Address, DEFAULT_ADDRESS_SIZE);
	
	// TX RF_Adress setup 5 byte - set TX address
	// Not used in a receiver but can be set anyway. Equal to Rx address
	WriteRegister(REG_TX_ADDR, Node_Address, DEFAULT_ADDRESS_SIZE);
	
//	// Enable dynamic payload length and payload with ack packet
//	val[0] = 0x06;
//	WriteRegister(REG_FEATURE, val, 1); // 0b0000 0110: EN_DPL = 1, EN_ACK_PAY = 1
//
//	// Choose the pipes with dynamic payload length. 0 and 1.
//	val[0] = 0x03;
//	RF24ReadWrite(DYNPD, val, 1);
	
	/*
	// Payload width setup - how many bytes to send per transmission (1-32)
	val[0] = 32;	// 32 bytes per package (same on RX and TX)
	RF24ReadWrite(W, RX_PW_P0, val, 1);
	*/
	
	// CONFIG reg setup - boot up the nRF24L01 and choose if it is a transmitter or a receiver
	val = (0 << BIT_PRIM_RX) | (0 << BIT_PWR_UP) | (1 << BIT_MASK_MAX_RT);
	WriteRegister(REG_CONFIG, &val, 1);
	
	InitializeIRQ();

	Radio_State = STATE_INIT;
	Radio_Events.all = 0;
}

void Radio__TurnOn(void)
{
    uint8_t val;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        // fixme read config first
        val = (1 << BIT_PWR_UP);
        WriteRegister(REG_CONFIG, &val, 1);
        Radio_Events.turning_on = 1;
    }
}

void Radio__TurnOff(void)
{
    uint8_t val;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        // fixme read config first
        val = (0 << BIT_PWR_UP);
        WriteRegister(REG_CONFIG, &val, 1);
        Radio_Events.on = 0;
    }
}

void Radio__1msTask(void)
{
    static down_counter = 0;
    RADIO_STATE_T next_state = Radio_State;

    switch (Radio_State)
    {
        case STATE_INIT:
        {
            if (Spi__IsTxBufferEmpty())
            {
                next_state = STATE_IDLE;
            }
            break;
        }
        case STATE_IDLE:
        {
            if (Radio_Events.turning_on)
            {
                down_counter = DELAY_TPD2STBY;
                next_state = STATE_CONFIGURING;
            }
            break;
        }
        case STATE_CONFIGURING:
        {
            down_counter--;
            if (down_counter == 0)
            {
                if (Radio_Events.turning_on)
                {
                    Radio_Events.turning_on = 0;
                    Radio_Events.on = 1;
                    next_state = STATE_IDLE;
                }
            }
            break;
        }
        case STATE_READING:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    Radio_State = next_state;

}

/**
 * \brief Read a register value from nRF24L01
 *
 * \param reg
 *
 * \return uint8_t
 */
uint8_t RF24GetReg(uint8_t reg)
{
	_delay_us(10);
	Spi__PutChar(R_REGISTER + reg);	// R_REGISTER = set the RF24 to reading mode, "reg" is the register that will be read back
	_delay_us(10);
	Spi__WriteThenRead(NOP); // Send a NOP (dummy byte) in order to read the first byte of the register "reg"
	reg = Spi__GetChar();
	_delay_us(10);
	return reg;	// Return the read register
}

/**
 * \brief Write to or Read from nRF24L01
 *
 * \param ReadWrite		Specifies if we want to read ("R") or write ("W") to the register
 * \param reg			The register to read or write
 * \param val			array to read or write
 * \param nVal			size of the array
 *
 * \return uint8_t *	array of bytes read
 */
uint8_t *RF24ReadWrite(uint8_t ReadWrite, uint8_t reg, uint8_t *val, uint8_t nVal)
{
    //! A static uint8_t is needed to return an array
    static uint8_t ret[DATA_LEN];

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
	//! If "W" we want to write to nRF24. No need to "R" mode because R=0x00
	if (ReadWrite == W)
	{
		reg = W_REGISTER + reg;	//ex: reg = EN_AA: 0b0010 0000 + 0b0000 0001 = 0b0010 0001
	}

	//! Makes sure we wait a bit
	_delay_us(10);
	Spi__PutChar(reg);	// set to write or read mode the register "reg"
	_delay_us(10);

	int i;
	for(i=0; i < nVal; i++)
	{
		// We want to read a register
		if (ReadWrite == R && reg != W_TX_PAYLOAD)
		{
			// Send dummy bytes to read the data
		    Spi__WriteThenRead(NOP);
			ret[i] = Spi__GetChar();
			_delay_us(10);
		}
		else
		{
		    Spi__PutChar(val[i]);
			_delay_us(10);
		}
	}
    }

	return ret;
}



/**
 * \brief Transmit a data packet with the radio module
 * 
 * \param WBuff
 * 
 * \return void
 */
void RF24TransmitPayload(uint8_t *WBuff)
{
	// Sends 0xE1 to flush the register from old data
	RF24ReadWrite(R, FLUSH_TX, WBuff, 0);
	// Sends data in WBuff to the module
	// Note that FLUSH_TX and W_TX_PAYLOAD are sent with "R" instead of "W" because
	// they are on the highest byte-level in the nRF
	RF24ReadWrite(R, W_TX_PAYLOAD, WBuff, 5);
	
	_delay_ms(10);
	// CE high = transmit the data
	PORTB |= (1 << PORTB1);
	_delay_us(20);
	// CE low = stop transmitting
	PORTB &= ~(1 << PORTB1);
	_delay_ms(10);
}

/**
 * \brief Receive a data packet with the radio module
 * 
 * 
 * \return void
 */
void RF24ReceivePayload(void)
{
	// CE high = listens for data
	PORTB |= (1 << PORTB1);
	// listens for 1 sec
	_delay_ms(1000);
	// CE low = stop listening
	PORTB &= ~(1 << PORTB1);
}

/**
 * \brief Reset IRQ on the nRF24L01
 *
 * 
 * \return void
 */
void RF24ResetIRQ(void)
{
	// Write to STATUS register
	Spi__PutChar(W_REGISTER + STATUS);
	_delay_us(10);
	// Reset all IRQ in STATUS register
	Spi__PutChar(0x70);
	_delay_us(10);
}

static void WriteRegister(uint8_t reg, uint8_t* val, uint8_t n_val)
{
    uint8_t i;

    reg = CMD_W_REGISTER + reg;
    Spi__PutChar(reg);
    for (i = 0; i < n_val; i++)
    {
        Spi__PutChar(val[i]);
    }
}

static void InitializeIRQ(void)
{
    // INT0 init
    DDRD &= ~(1 << DDD2);
    // INT0 falling edge PD2
    EICRA |=  (1<<ISC01);
    EICRA  &=  ~(1<<ISC00);
    // Enable IRQ INT0
    EIMSK |=  (1<<INT0);
}

/**
 * @brief ISR on INT0
 *
 * This is called when successful data receive or transmission
 * happened
 *
 * @todo Avoid 0.5 sec delay
 *
 * @return void
 */
ISR(INT0_vect)
{
    PORTB &= ~(1 << PORTB1);

    // LED to show success
    PORTB |= (1 << PORTB0);
    _delay_ms(500);
    PORTB &= ~(1 << PORTB0);
    // Read data from RX FIFO
    data = RF24ReadWrite(R, R_RX_PAYLOAD, data, 32);
    // Print data to console
    for (int i = 0; i < 32; i++)
    {
        Usart__PutChar(data[i]);
    }
    // Clear IRQ masks in STATUS register
    RF24ResetIRQ();
}
