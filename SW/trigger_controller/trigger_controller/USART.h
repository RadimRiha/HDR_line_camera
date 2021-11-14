#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include <avr/interrupt.h>

#define USART_BUFFER_LENGTH 70

typedef struct USART {
	volatile uint8_t receiveComplete;
	volatile uint8_t transmitComplete;
	volatile char inBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t inBufferIndex;
	volatile char outBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t outBufferIndex;
} USART;

extern USART USART0;

void usartInit();
void usartAddToOutBuffer(const char *str);
void usartSend();

#endif