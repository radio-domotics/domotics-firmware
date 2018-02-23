/**
 * @file onewire.h
 *
 * @date 21/10/2014 13:19:47
 * @author Leonardo Ricupero
 */ 


#ifndef OW_H_
#define OW_H_

#include "micro.h"

#define ONEWIRE_DRIVE_BUS_LOW() {DDRD |= (1 << DDD7); PORTD &= ~(1 << PORTD7);}
#define ONEWIRE_RELEASE_BUS() {DDRD &= ~(1 << DDD7);}
#define ONEWIRE_SAMPLE_BUS() ((PIND & (1 << PIND7)) >> PIND7)

typedef enum {
	ONEWIRE_BIT_0 = 0,
	ONEWIRE_BIT_1 = 1,
	ONEWIRE_PRESENCE_OK = 2,
	ONEWIRE_PRESENCE_NOK = 3,
	ONEWIRE_DATA_NOT_READY = 0xFF,
} ONEWIRE_SAMPLE_T;

extern ONEWIRE_SAMPLE_T Last_Sample;
extern uint8_t Byte_Read;

void Onewire__Initialize(void);
void Onewire__DetectPresence(void);
ONEWIRE_SAMPLE_T Onewire__GetPresence(void);
void Onewire__WriteBit(uint8_t bit);
uint8_t Onewire__ReadBit(void);
void Onewire__WriteByte(uint8_t data);
void Onewire__StartReadByte(void);
uint8_t Onewire__IsIdle(void);

#define Onewire__GetLastSample() Last_Sample
#define Onewire__GetLastByte() Byte_Read

#endif /* OW_H_ */
