/**
 * @file ui.c
 *
 * @date 17 dic 2017
 * @author Leonardo Ricupero
 */

#include "micro.h"
#include "ui.h"

#define UI_BLINK_RATE_500MS 5 // 500ms

static uint8_t Countdown_Timer;
static uint8_t Blinks_Remaining;

void Ui__Initialize(void)
{
	DDRB |= (1 << DDB0);
	Ui__LedOff();

	Blinks_Remaining = 0;
	Countdown_Timer = 0;
}

void Ui__LedBlink500ms(uint8_t times)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        Blinks_Remaining = (times << 1) + 1;
        Countdown_Timer = UI_BLINK_RATE_500MS;
        Ui__LedOn();
    }
}

void Ui__100msTask(void)
{
    if (Blinks_Remaining != 0)
    {
        if (Countdown_Timer == 0)
        {
            Ui__LedToggle();
            Countdown_Timer = UI_BLINK_RATE_500MS;
            Blinks_Remaining--;
        }
        else
        {
            Countdown_Timer--;
        }
    }
}
