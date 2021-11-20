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
	UCSR0B = (1<<RXCIE0) | (1<<TXCIE0) | (1<<RXEN0) | (1<<TXEN0);	//enable transmitter, receiver and corresponding interrupts
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);	//8-bit character size
	UBRR0 = F_CPU/(16UL*9600) - 1;		//9600 baud
}

ISR(USART0_RX_vect) {
	if(USART0.receiveComplete) return;	//reject incoming data if the last message has not been processed yet
	char receiveChar = UDR0;
	USART0.inBuffer[USART0.inBufferIndex] = receiveChar;
	if((USART0.inBufferIndex >= USART_BUFFER_LENGTH-1) || (receiveChar == '\n')) {
		USART0.inBuffer[USART0.inBufferIndex] = '\0';
		USART0.receiveComplete = 1;
		USART0.inBufferIndex = 0;
	}
	else USART0.inBufferIndex++;
}

ISR(USART0_TX_vect) {
	if((USART0.outBufferIndex >= USART_BUFFER_LENGTH) || (USART0.outBuffer[USART0.outBufferIndex] == '\0')) {
		USART0.transmitComplete = 1;
		USART0.outBufferIndex = 0;	//new message start index reset
		return;
	}
	UDR0 = USART0.outBuffer[USART0.outBufferIndex];	//queue next character
	USART0.outBufferIndex++;
}

void usartAddToOutBuffer(const char *str) {		//strips ending \0
	//change this to ring buffer!
	for (uint8_t i = 0; i < 0xFF; i++) {
		if(str[i] == '\0') return;
		USART0.outBuffer[USART0.outBufferIndex] = str[i];
		USART0.outBufferIndex++;
		if(USART0.outBufferIndex >= USART_BUFFER_LENGTH) return;
	}
}

void usartSend() {
	if(!USART0.transmitComplete || stringEmpty(USART0.outBuffer)) return;
	USART0.outBuffer[USART0.outBufferIndex] = '\0';	//terminate string (adding ignores \0)
	USART0.transmitComplete = 0;
	USART0.outBufferIndex = 1;	//first TX complete interrupt starts at second char
	UDR0 = USART0.outBuffer[0];
}
