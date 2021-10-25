#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "string.h"

#define MAX_HDR_EXP_TIMES 5		//maximum number of HDR exposure times
#define USART_BUFFER_LENGTH 20

enum triggerTypes{FREE, TIMED, HW, ENCODER};
enum hwTriggerPolarities{RISING, FALLING, HIGH, LOW};
	
struct acquisitionSettings {
	uint8_t rgbEnabled;				//acquisition with/without RGB light cycling
	uint8_t hdrEnabled;				//acquisition with/without HDR sequence
	enum triggerTypes trigger;		//triggering type
	uint8_t triggerWidthExposure;	//exposure control using trigger width/light duration
	uint8_t noRgbLight;				//use white light if RGB is disabled
	uint16_t noHdrExposureTime;		//use this exposure time if HDR is disabled
	uint16_t hdrExposureTime[MAX_HDR_EXP_TIMES+1];	//HDR exposure times
	uint16_t triggerPeriod;			//period [us] of the TIMED trigger
	enum hwTriggerPolarities hwTriggerPolarity;
};

struct acquisitionSettings acqSettings = {
	.rgbEnabled = 0,
	.hdrEnabled = 1,
	.trigger = FREE,
	.triggerWidthExposure = 1,
	.noRgbLight = 0,
	.noHdrExposureTime = 5,
	.hdrExposureTime[0] = 200,
	.hdrExposureTime[1] = 400,
	.hdrExposureTime[2] = 800,
	.hdrExposureTime[3] = 0xFFFF,
	.triggerPeriod = 1000,
	.hwTriggerPolarity = RISING,
};

uint8_t precomputedOCR0A[MAX_HDR_EXP_TIMES+1];
uint8_t precomputedTCCR0B[MAX_HDR_EXP_TIMES+1];

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t hdrPulseCount = 0;
volatile uint8_t cameraReady = 0;

struct USART {
	volatile uint8_t receiveComplete;
	volatile uint8_t transmitComplete;
	volatile char inBuffer[USART_BUFFER_LENGTH];
	volatile uint8_t inBufferIndex;
	volatile char outBuffer[USART_BUFFER_LENGTH];
	volatile uint8_t outBufferIndex;
};

struct USART USART0 = {
	.receiveComplete = 0,
	.transmitComplete = 1,
	.inBufferIndex = 0,
	.outBufferIndex = 0,
};

void cameraReadyInterrupt();
void cameraNotReadyInterrupt();
void startTriggerTimer();
void startNewLineTrigger();
void checkCameraReadyStatus();
void usartSend();
void usartAddToOutBuffer(const char *str);

ISR(TIMER0_COMPA_vect) {
	TCCR0B = 0;		//stop the timer
	if(!acqSettings.hdrEnabled) pulseTrainComplete = 1;
	else if(acqSettings.hdrExposureTime[hdrPulseCount] == 0xFFFF) pulseTrainComplete = 1;
}

ISR(INT0_vect) {			//line out 1 rising/falling edge
	if(PIND & (1<<2)) {		//rising edge or high level of LINE OUT 1 (line trigger wait)
		if(!cameraReady) {	//rising edge
			cameraReady = 1;
			if(!pulseTrainComplete) startTriggerTimer();				//another HDR trigger
			else if(acqSettings.trigger == FREE) startNewLineTrigger();	//FREE run mode triggering
		}
	}
	else {					//falling edge or low level of LINE OUT 1 (line trigger wait)
		cameraReady = 0;
	}
}

ISR(USART_RX_vect) {
	if(USART0.receiveComplete) return;
	char receiveChar = UDR0;
	USART0.inBuffer[USART0.inBufferIndex] = receiveChar;
	if((USART0.inBufferIndex >= USART_BUFFER_LENGTH-1) || (receiveChar == '\n')) {
		USART0.receiveComplete = 1;
		USART0.inBufferIndex = 0;
	}
	else USART0.inBufferIndex++;
}

ISR(USART_TX_vect) {
	if((USART0.outBufferIndex >= USART_BUFFER_LENGTH-1) || (USART0.outBuffer[USART0.outBufferIndex] == '\n')) {
		USART0.transmitComplete = 1;
		USART0.outBufferIndex = 0;	//new message start index reset
		return;
	}
	else USART0.outBufferIndex++;
	UDR0 = USART0.outBuffer[USART0.outBufferIndex];	//queue next character
}

void processUsart() {
	if(!USART0.receiveComplete) return;	//only process if message is complete
	//message[0] = S(set)/G(get)
	switch(USART0.inBuffer[0]) {
		case 'S':
			//message[1-3] = XYZ - acronym for setting parameter
			//if(cmpString(USART0.inBuffer+1, "RGE", 3)) {
			//	acqSettings.rgbEnabled = USART0.inBuffer[4] - '0';
			//}
		break;
		case 'G':
			//message[1-3] = XYZ - acronym for getting parameter
			if(cmpString(USART0.inBuffer+1, "RGE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.rgbEnabled));
				usartAddToOutBuffer("\n");
				usartSend();
			}
		break;
		default:
			usartAddToOutBuffer("UNRECOGNIZED\n\0");
			usartSend();
		break;
	}
	USART0.receiveComplete = 0;	//ack message
}

void usartAddToOutBuffer(const char *str) {
	for (uint8_t i = 0; i < 0xFF; i++) {
		if(str[i] == '\0') break;
		USART0.outBuffer[USART0.outBufferIndex] = str[i];
		USART0.outBufferIndex++;
		if(USART0.outBufferIndex >= USART_BUFFER_LENGTH) return;
	}
}

void usartSend() {
	if(!USART0.transmitComplete) return;
	USART0.transmitComplete = 0;
	USART0.outBufferIndex = 0;	//send index reset
	UDR0 = USART0.outBuffer[USART0.outBufferIndex];
}

void checkCameraReadyStatus() {
	if(PIND & (1<<2)) {	//rising edge or high level of LINE OUT 1 (line trigger wait)
		if(!cameraReady) {	//rising edge
			cameraReady = 1;
			if(!pulseTrainComplete) startTriggerTimer();				//another HDR trigger
			else if(acqSettings.trigger == FREE) startNewLineTrigger();	//FREE run mode triggering
		}
	}
	else {				//falling edge or low level of LINE OUT 1 (line trigger wait)
		cameraReady = 0;
	}
}

void precomputeTriggerTimerParameters() {
	for(uint8_t i = 0; i < MAX_HDR_EXP_TIMES+1; i++) {
		uint16_t expTime;
		if(i != MAX_HDR_EXP_TIMES) expTime = acqSettings.hdrExposureTime[i];
		else expTime = acqSettings.noHdrExposureTime;
		
		if(expTime < 125) {									//exp time < 125
			precomputedTCCR0B[i] = (1<<CS01);				//8 prescaler, 0.5us period
			precomputedOCR0A[i] = expTime*2;
		}else if(expTime < 1000) {							//125 < exp time < 1000
			precomputedTCCR0B[i] = (1<<CS01) | (1<<CS00);	//64 prescaler, 4us period
			precomputedOCR0A[i] = expTime/4;
		}else if(expTime < 4000) {							//1000 < exp time < 4000
			precomputedTCCR0B[i] = (1<<CS02);				//256 prescaler, 16us period
			precomputedOCR0A[i] = expTime/16;
		}else {												//4000 < exp time < 10000
			precomputedTCCR0B[i] = (1<<CS02) | (1<<CS00);	//1024 prescaler, 64us period
			precomputedOCR0A[i] = expTime/64;
		}
	}
}

void startTriggerTimer() {
	uint8_t TCCR0B_val;
	if(acqSettings.hdrEnabled) {
		OCR0A = precomputedOCR0A[hdrPulseCount];
		TCCR0B_val = precomputedTCCR0B[hdrPulseCount];
		hdrPulseCount++;
	}
	else {
		OCR0A = precomputedOCR0A[MAX_HDR_EXP_TIMES];
		TCCR0B_val = precomputedTCCR0B[MAX_HDR_EXP_TIMES];
	}
	OCR0B = OCR0A;
	TCNT0 = 0xFF;
	TCCR0B = TCCR0B_val;	//start the timer
}

void startNewLineTrigger() {
	if(!pulseTrainComplete || !cameraReady) return;
	pulseTrainComplete = 0;
	hdrPulseCount = 0;
	if(acqSettings.triggerWidthExposure) TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);	//trigger output to camera
	else TCCR0A = (1<<COM0B1) | (1<<WGM01) | (1<<WGM00);	//trigger output to RGB light
	startTriggerTimer();
}

int main(void) {
	cli();
	//timer 0 setup
	DDRD |= (1<<6) | (1<<5);			//OC0A, OC0B output
	TCCR0A = (1<<WGM01) | (1<<WGM00);	//Fast PWM
	TCCR0B = 0;							//stop the timer
	TIMSK0 = (1<<OCIE0A);				//enable COMPA interrupt
	
	//line out 1 interrupt
	EICRA = (1<<ISC00);		//INT0 on logical change
	EIMSK = (1<<INT0);		//enable INT0
	
	//setup UART
	UCSR0B = (1<<RXCIE0) | (1<<TXCIE0) | (1<<RXEN0) | (1<<TXEN0);	//enable transmitter, receiver and corresponding interrupts
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);	//8-bit character size
	UBRR0 = F_CPU/(16UL*9600) - 1;		//9600 baud
	
	precomputeTriggerTimerParameters();
	sei();
	
	DDRD |= (1<<7);		//DEBUG
	
	//char msg[] = "Dominik smrdi\n";
	//usartSend(msg);
	
    while(1) {
		processUsart();
		checkCameraReadyStatus();
    }
}
