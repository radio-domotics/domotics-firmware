/**
 * @file thermostat.c
 *
 * @date 02 gen 2018
 * @author Leonardo Ricupero
 */

#include "micro.h"
#include "temp_sensor.h"
#include "relays.h"
#include "parameters.h"
#include "thermostat.h"

#define THERMOSTAT_SAMPLE_RATE_100MS 50 // 5 seconds
#define THERMOSTAT_TIMEOUT_100MS 10 // 1 seconds

#define THERMOSTAT_LOAD_ON()  {Relays__Set(RELAY_0); Thermostat_Status.load_active = 1;}
#define THERMOSTAT_LOAD_OFF() {Relays__Reset(RELAY_0); Thermostat_Status.load_active = 0;}

typedef enum {
    STATE_IDLE,
    STATE_WAIT_FOR_TEMPERATURE,
    STATE_ERROR_FOUND,
} TEMP_READING_STATE_T;

typedef union {
    struct {
        uint8_t temperature_ready :1;
        uint8_t load_active :1;
    };
    uint8_t all;
} THERMOSTAT_STATUS_T;

typedef enum {
    MODE_WINTER,
    MODE_SUMMER,
} THERMOSTAT_MODE_T;


static uint8_t Sample_Counter;
static uint8_t Timeout_Counter;
static TEMP_READING_STATE_T Temperature_Reading_State;
static THERMOSTAT_STATUS_T Thermostat_Status;
static THERMOSTAT_MODE_T Thermostat_Mode;
static int16_t Last_Temperature; // Q12.4 format

static inline void TemperatureReadingStateMachine(void);

void Thermostat__Initialize(void)
{
    Sample_Counter = 0;
    Temperature_Reading_State = STATE_IDLE;
    Thermostat_Status.all = 0;
    Thermostat_Mode = MODE_WINTER;

    Last_Temperature = 0xFFFF;
    TempSensor__Configure();
}


void Thermostat__100msTask(void)
{
    TemperatureReadingStateMachine();

    if (Thermostat_Status.temperature_ready)
    {
        Thermostat_Status.temperature_ready = 0;

        if (Last_Temperature <= THERMOSTAT_TEMPERATURE_SET - THERMOSTAT_TEMPERATURE_HISTERESYS)
        {
            if (Thermostat_Status.load_active == 0)
            {
                THERMOSTAT_LOAD_ON();
            }
        }
        else if (Last_Temperature >= THERMOSTAT_TEMPERATURE_SET)
        {
            if (Thermostat_Status.load_active == 1)
            {
                THERMOSTAT_LOAD_OFF();
            }
        }
    }
}

static inline void TemperatureReadingStateMachine(void)
{
    TEMP_READING_STATE_T next_state;

    next_state = Temperature_Reading_State;

    if (Sample_Counter < THERMOSTAT_SAMPLE_RATE_100MS)
    {
        Sample_Counter++;
    }

    switch (Temperature_Reading_State)
    {
        case STATE_IDLE:
        {
            if (Sample_Counter == THERMOSTAT_SAMPLE_RATE_100MS)
            {
                Sample_Counter = 0;
                TempSensor__StartAcquisition();
                Timeout_Counter = THERMOSTAT_TIMEOUT_100MS;
                next_state = STATE_WAIT_FOR_TEMPERATURE;
            }
            break;
        }
        case STATE_WAIT_FOR_TEMPERATURE:
        {
            if (TempSensor__IsTemperatureReady())
            {
                Last_Temperature = TempSensor__GetTemperature();
                Thermostat_Status.temperature_ready = 1;
                next_state = STATE_IDLE;
            }
            else
            {
                Timeout_Counter--;
                if (Timeout_Counter == 0)
                {
                    next_state = STATE_ERROR_FOUND;
                }
            }
            break;
        }
        case STATE_ERROR_FOUND:
        {
            // todo better error handling
            next_state = STATE_IDLE;
            break;
        }
        default:
        {
            break;
        }
    }

    Temperature_Reading_State = next_state;
}
