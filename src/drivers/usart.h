/*
 * \file USART.h
 *
 * Created: 02/10/2014 13:52:21
 * \author: Leonardo Ricupero
 */ 


#ifndef USART_H_
#define USART_H_

#include "micro.h"

#define USART_BAUDRATE 9600

void Usart__Initialize(void);
uint8_t Usart__GetChar(void);
void Usart__PutChar(uint8_t c);
BOOL_T Usart__IsRxBufferEmpty(void);
BOOL_T Usart__IsTxBufferEmpty(void);
void Usart__FastTask(void);

#endif /* USART_H_ */
