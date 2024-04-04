#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include <avr/interrupt.h>

#define USART_BUFFER_LENGTH 70

typedef struct USART {
	// receive complete flag (got /n)
	// if this is true, inBuffer can be processed and will not be overwritten
	// after processing inBuffer, reset this to enable receiving again
	volatile uint8_t receiveComplete;
	// transmit complete flag (outBuffer empty)
	volatile uint8_t transmitComplete;
	// buffer for incoming data
	volatile char inBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t inBufferIndex;
	// buffer for outgoing data
	volatile char outBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t outBufferIndex;
} USART;

extern USART USART0;

// initialize USART at 9600 baud, enable interrupts
void usartInit();
// add string to output buffer (last string added before sending should be /n terminated)
void usartAddToOutBuffer(const char *str);
// send output buffer to PC
void usartSend();

#endif