/**
 * @file
 *
 * @date 26/12/2017
 * @author Leonardo Ricupero
 */ 

#include "onewire.h"
#include "temp_sensor.h"

#define SCRATCHPAD_SIZE		9

// ROM Commands
#define SEARCH_ROM			0xF0
#define READ_ROM			0x33
#define MATCH_ROM			0x55
#define SKIP_ROM			0xCC
#define ALARM_SEARCH		0xEC

// Function Commands
#define CONVERT_T			0x44
#define WRITE_SCRATCHPAD	0x4E
#define READ_SCRATCHPAD		0xBE
#define COPY_SCRATCHPAD		0x48
#define RECALL_E2			0xB8
#define READ_POWER_SUPPLY	0xB4

// Alarms and Configuration
#define T_ALARM_HIGH		0x32 // +50
#define T_ALARM_LOW			0x85 // -5 1000 0101b
#define RES_CONFIG			0x7F // 12 bit 0111 1111b


typedef enum {
	STATE_IDLE = 0,
	STATE_DETECT_PRESENCE,
	STATE_SKIP_ROM,
	STATE_CONFIG_PRE,
	STATE_CONFIG_T_ALARM0,
	STATE_CONFIG_T_ALARM1,
	STATE_CONFIG_RESOLUTION,
	STATE_CONVERT_TEMPERATURE,
	STATE_READ_SCRATCHPAD,
	STATE_ACQUIRING_SCRATCHPAD,
	STATE_ERROR_FOUND,
} TEMP_SENSOR_STATE_T;

typedef union {
    struct {
	    uint8_t configuring :1;
	    uint8_t reading_temp :1;
	    uint8_t conversion_finished :1;
	    uint8_t temperature_read :1;
	    uint8_t configured: 1;
	    uint8_t timeout_expired: 1;
    };

	uint8_t all;
} TEMP_SENSOR_EVENTS_T;

static uint8_t IsBusy(void);

static TEMP_SENSOR_STATE_T TempSensor_State;
static TEMP_SENSOR_EVENTS_T TempSensor_Events;
static uint8_t Scratchpad[SCRATCHPAD_SIZE];
static uint8_t Scratchpad_Read_Index;

/**
 * @brief Initialize the module
 * 
 */
void TempSensor__Initialize(void)
{
	uint8_t i;

	Onewire__Initialize();

	TempSensor_State = STATE_IDLE;
	TempSensor_Events.all = 0;
	
	for (i=0; i<SCRATCHPAD_SIZE; i++)
	{
		Scratchpad[i] = 0;
	}
	Scratchpad_Read_Index = 0;
}

void TempSensor__Configure(void)
{
	if (TempSensor_Events.configured != 1)
	{
		TempSensor_Events.configuring = 1;	
	}
}

void TempSensor__StartAcquisition(void)
{
	if (TempSensor_Events.configured == 1 &&
        IsBusy() == 0)
    {
        TempSensor_Events.reading_temp = 1;
    }
}

/**
 * @brief 	Get the last measured temperature
 *
 * @details The temperature is given in fixed point
 * 			format Q12.4
 *
 * @remarks Call this function in order to start a new conversion
 *
 */
int16_t TempSensor__GetTemperature(void)
{
	int16_t result;

	result = Scratchpad[1] << 8;
	result += Scratchpad[0];
	TempSensor_Events.temperature_read = 0;
	return result;
}

uint8_t TempSensor__IsTemperatureReady(void)
{
	uint8_t result = 0;
    if (TempSensor_Events.temperature_read)
    {
        TempSensor_Events.temperature_read = 0;
        result = 1;
    }
	return result;
}

void TempSensor__1msTask(void)
{
	TEMP_SENSOR_STATE_T next_state;
	
	next_state = TempSensor_State;
	uint8_t temp;
	// todo implementation of a timer for timeout
	switch(TempSensor_State)
	{
		case STATE_IDLE:
		{
			if (TempSensor_Events.configuring ||
				TempSensor_Events.reading_temp)
			{
				Onewire__DetectPresence();
				next_state = STATE_DETECT_PRESENCE;
			}
			break;
		}
		case STATE_DETECT_PRESENCE:
		{
			next_state = STATE_DETECT_PRESENCE;

			if (Onewire__IsIdle())
			{
				temp = Onewire__GetPresence();
				if (temp == ONEWIRE_PRESENCE_OK)
				{
					Onewire__WriteByte(SKIP_ROM);
					next_state = STATE_SKIP_ROM;
				}
				else
				{
					next_state = STATE_ERROR_FOUND;
				}
			}
			break;
		}
		case STATE_SKIP_ROM:
		{
			next_state = STATE_SKIP_ROM;

			if (Onewire__IsIdle())
			{
				if (TempSensor_Events.configuring)
				{
                    Onewire__WriteByte(WRITE_SCRATCHPAD);
                    next_state = STATE_CONFIG_PRE;
				}
				else if (TempSensor_Events.reading_temp)
				{
					Onewire__WriteByte(CONVERT_T);
					next_state = STATE_CONVERT_TEMPERATURE;
				}
				else if (TempSensor_Events.conversion_finished)
				{
				    TempSensor_Events.conversion_finished = 0;
					Onewire__WriteByte(READ_SCRATCHPAD);
					next_state = STATE_READ_SCRATCHPAD;
				}
			}
			break;
		}
		case STATE_CONVERT_TEMPERATURE:
		{
			next_state = STATE_CONVERT_TEMPERATURE;

			if (Onewire__IsIdle())
			{
				if (Onewire__ReadBit())
				{
				    TempSensor_Events.conversion_finished = 1;// = EVENT_CONVERSION_FINISHED;
                    TempSensor_Events.reading_temp = 0;
                    Onewire__DetectPresence();
                    next_state = STATE_DETECT_PRESENCE;
                }
			}
			break;
		}
		case STATE_READ_SCRATCHPAD:
		{
			next_state = STATE_READ_SCRATCHPAD;

			if (Onewire__IsIdle())
			{
				Onewire__StartReadByte();
				next_state = STATE_ACQUIRING_SCRATCHPAD;
			}
			break;
		}
		case STATE_ACQUIRING_SCRATCHPAD:
		{
			next_state = STATE_ACQUIRING_SCRATCHPAD;

			if (Scratchpad_Read_Index < SCRATCHPAD_SIZE)
			{
				if (Onewire__IsIdle())
				{
					Scratchpad[Scratchpad_Read_Index] = Onewire__GetLastByte();
					Scratchpad_Read_Index++;
					Onewire__StartReadByte();
				}
			}
			else
			{
			    Scratchpad_Read_Index = 0;
                TempSensor_Events.reading_temp = 0;
				TempSensor_Events.temperature_read = 1;
                next_state = STATE_IDLE;
			}
			break;
		}
        case STATE_CONFIG_PRE:
        {
            if (Onewire__IsIdle())
            {
                Onewire__WriteByte(T_ALARM_HIGH);
                next_state = STATE_CONFIG_T_ALARM0;
            }
            break;
        }
		case STATE_CONFIG_T_ALARM0:
		{
			next_state = STATE_CONFIG_T_ALARM0;

			if (Onewire__IsIdle())
			{
				Onewire__WriteByte(T_ALARM_LOW);
				next_state = STATE_CONFIG_T_ALARM1;
			}
			break;
		}
		case STATE_CONFIG_T_ALARM1:
		{
			next_state = STATE_CONFIG_T_ALARM1;

			if (Onewire__IsIdle())
			{
				Onewire__WriteByte(RES_CONFIG);
				next_state = STATE_CONFIG_RESOLUTION;
			}
			break;
		}
		case STATE_CONFIG_RESOLUTION:
		{
			next_state = STATE_CONFIG_RESOLUTION;

			if (Onewire__IsIdle())
			{
                TempSensor_Events.configuring = 0;
				TempSensor_Events.configured = 1;
                next_state = STATE_IDLE;
			}
			break;
		}
		default:
		{
			break;
		}
	}

	TempSensor_State = next_state;
}

static uint8_t IsBusy(void)
{
    if (TempSensor_Events.configuring == 0 && 
        TempSensor_Events.conversion_finished == 0 &&
        TempSensor_Events.reading_temp == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
