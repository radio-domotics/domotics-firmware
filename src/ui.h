/**
 * @file ui.h
 *
 * @date 17 dic 2017
 * @author Leonardo Ricupero
 */

#ifndef SRC_UI_H_
#define SRC_UI_H_

#include "micro.h"

#define LED_PORT PORTB
#define LED_PIN PB0


#define Ui__LedOn() {LED_PORT |= (1 << LED_PIN);}
#define Ui__LedOff() {LED_PORT &= ~(1 << LED_PIN);}

#define Ui__LedToggle() {PINB = (1 << PINB0);}

void Ui__Initialize(void);
void Ui__LedBlink500ms(uint8_t times);
void Ui__100msTask(void);

#endif /* SRC_UI_H_ */
