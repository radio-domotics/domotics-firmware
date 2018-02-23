/*
 * SPI.h
 *
 * Created: 22/09/2014 17:18:32
 *  Author: Leo
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <avr/io.h>

void Spi__Initialize(void);
void Spi__PutChar(uint8_t c);
uint8_t Spi__GetChar(void);
void Spi__WriteThenRead(uint8_t c);
BOOL_T Spi__IsRxBufferEmpty(void);
BOOL_T Spi__IsTxBufferEmpty(void);
void Spi__FastTask(void);

#endif /* SPI_H_ */
