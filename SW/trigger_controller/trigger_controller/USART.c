#include "USART.h"
#include "string.h"
#include "global.h"

USART USART0 = {
	.receiveComplete = 0,
	.transmitComplete = 1,
	.inBufferIndex = 0,
	.outBufferIndex = 0,
};

void usartInit() {
	UCSR0B = (1<<RXCIE0) | (1<<TXCIE0) | (1<<RXEN0) | (1<<TXEN0);	// enable transmitter, receiver and corresponding interrupts
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);	// 8-bit character size
	UBRR0 = F_CPU/(16UL*9600) - 1;		// 9600 baud
}

// USART byte received interrupt
ISR(USART0_RX_vect) {
	if(USART0.receiveComplete) return;	// reject incoming data if the last message has not been processed yet
	char receiveChar = UDR0;
	// load received byte into inBuffer
	USART0.inBuffer[USART0.inBufferIndex] = receiveChar;
	if((USART0.inBufferIndex >= USART_BUFFER_LENGTH-1) || (receiveChar == '\n')) {
		// end of message
		USART0.inBuffer[USART0.inBufferIndex] = '\0';
		USART0.receiveComplete = 1;
		USART0.inBufferIndex = 0;
	}
	else if (receiveChar == '\r') {}	// discard carriage return
	else USART0.inBufferIndex++;
}

// USART byte sent interrupt
ISR(USART0_TX_vect) {
	if((USART0.outBufferIndex >= USART_BUFFER_LENGTH) || (USART0.outBuffer[USART0.outBufferIndex] == '\0')) {
		// end of message
		USART0.transmitComplete = 1;
		USART0.outBufferIndex = 0;
		return;
	}
	UDR0 = USART0.outBuffer[USART0.outBufferIndex];	// queue next character
	USART0.outBufferIndex++;
}

void usartAddToOutBuffer(const char *str) {
	for (uint8_t i = 0; i < 0xFF; i++) {
		if(str[i] == '\0') return;
		// copy byte to out buffer (skip /0)
		USART0.outBuffer[USART0.outBufferIndex] = str[i];
		USART0.outBufferIndex++;
		if(USART0.outBufferIndex >= USART_BUFFER_LENGTH) return;
	}
}

void usartSend() {
	if(!USART0.transmitComplete || stringEmpty(USART0.outBuffer)) return;
	USART0.outBuffer[USART0.outBufferIndex] = '\0';	// terminate string (because usartAddToOutBuffer skips \0)
	USART0.transmitComplete = 0;
	USART0.outBufferIndex = 1;	// first byte sent interrupt triggers at second char
	UDR0 = USART0.outBuffer[0];
}
