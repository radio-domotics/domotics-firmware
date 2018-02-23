/*
 * relays.h
 *
 * Created: 21/11/2014 13:02:43
 *  Author: Leo
 */ 


#ifndef RELAYS_H_
#define RELAYS_H_

typedef enum {
	RELAY_0,
	RELAY_1,
} RELAY_T;

void Relays__Initialize(void);
void Relays__Set(RELAY_T relay);
void Relays__Reset(RELAY_T relay);
void Relays__1msTask(void);

#endif /* RELAYS_H_ */
