/**
 * @file timer.h
 *
 * @date 25/09/2015 23:23:52
 * @author Leonardo Ricupero
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include "micro.h"

extern uint16_t Timer_Counter;

#define Timer__Stop() {TCCR0B &= 0b11111000;}
#define Timer__GetCounter() Timer_Counter
#define Timer__ResetCounter() {Timer_Counter = 0;}

void Timer__Initialize(void);
void Timer__Start(void);


#endif /* TIMER_H_ */
