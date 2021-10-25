#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "string.h"

#define MAX_HDR_EXP_TIMES 5		//maximum number of HDR exposure times
#define USART_BUFFER_LENGTH 20

typedef enum triggerTypes {
	FREE,
	TIMED,
	HW,
	ENCODER,
	NUM_OF_TRIGGER_TYPES,	//LEAVE THIS AS LAST
} triggerTypes;

typedef enum hwTriggerPolarities {
	RISING,
	FALLING,
	HIGH,
	LOW,
	NUM_OF_TRIGGER_POLARITIES,	//LEAVE THIS AS LAST
} hwTriggerPolarities;
	
typedef struct acquisitionSettings {
	uint8_t rgbEnabled;				//acquisition with/without RGB light cycling
	uint8_t hdrEnabled;				//acquisition with/without HDR sequence
	triggerTypes trigger;		//triggering type
	uint8_t triggerWidthExposure;	//exposure control using trigger width/light duration
	uint8_t noRgbLight;				//use white light if RGB is disabled
	uint16_t noHdrExposureTime;		//use this exposure time if HDR is disabled
	uint16_t hdrExposureTime[MAX_HDR_EXP_TIMES+1];	//HDR exposure times
	uint16_t triggerPeriod;			//period [us] of the TIMED trigger
	hwTriggerPolarities hwTriggerPolarity;
} acquisitionSettings;

acquisitionSettings acqSettings = {
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

typedef struct USART {
	volatile uint8_t receiveComplete;
	volatile uint8_t transmitComplete;
	volatile char inBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t inBufferIndex;
	volatile char outBuffer[USART_BUFFER_LENGTH+1];
	volatile uint8_t outBufferIndex;
} USART;

USART USART0 = {
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
	if(USART0.receiveComplete) return;	//reject incoming data if the last message has not been processed yet
	char receiveChar = UDR0;
	USART0.inBuffer[USART0.inBufferIndex] = receiveChar;
	if((USART0.inBufferIndex >= USART_BUFFER_LENGTH-1) || (receiveChar == '\n')) {
		USART0.inBuffer[USART0.inBufferIndex+1] = '\0';
		USART0.receiveComplete = 1;
		USART0.inBufferIndex = 0;
	}
	else USART0.inBufferIndex++;
}

ISR(USART_TX_vect) {
	if((USART0.outBufferIndex >= USART_BUFFER_LENGTH) || (USART0.outBuffer[USART0.outBufferIndex] == '\0')) {
		USART0.transmitComplete = 1;
		USART0.outBufferIndex = 0;	//new message start index reset
		return;
	}
	UDR0 = USART0.outBuffer[USART0.outBufferIndex];	//queue next character
	USART0.outBufferIndex++;
}

uint8_t passFailBool(uint8_t val) {
	if(val == 0 || val == 1) {
		usartAddToOutBuffer("OK");
		return 1;
	}
	usartAddToOutBuffer("FAIL");
	return 0;
}

void processUsart() {
	if(!USART0.receiveComplete) return;	//only process if message is complete
	//message[0] = S(set)/G(get)
	switch(USART0.inBuffer[0]) {
		case 'S':
			//message[1-3] = XYZ - acronym for setting parameter
			if(cmpString(USART0.inBuffer+1, "RGE\0")) {
				acqSettings.rgbEnabled = USART0.inBuffer[4] - '0';
				if(!passFailBool(acqSettings.rgbEnabled)) acqSettings.rgbEnabled = 0;
			}else if(cmpString(USART0.inBuffer+1, "HDE\0")) {
				acqSettings.hdrEnabled = USART0.inBuffer[4] - '0';
				if(!passFailBool(acqSettings.hdrEnabled)) acqSettings.hdrEnabled = 0;
			}else if(cmpString(USART0.inBuffer+1, "TRI\0")) {
				acqSettings.trigger = USART0.inBuffer[4] - '0';
				if(acqSettings.trigger < NUM_OF_TRIGGER_TYPES) usartAddToOutBuffer("OK");
				else {
					usartAddToOutBuffer("FAIL");
					acqSettings.trigger = FREE;
				}
			}else if(cmpString(USART0.inBuffer+1, "TWE\0")) {
				acqSettings.triggerWidthExposure = USART0.inBuffer[4] - '0';
				if(!passFailBool(acqSettings.triggerWidthExposure)) acqSettings.triggerWidthExposure = 0;
			}else if(cmpString(USART0.inBuffer+1, "NRL\0")) {
				acqSettings.noRgbLight = USART0.inBuffer[4] - '0';
				if(!passFailBool(acqSettings.noRgbLight)) acqSettings.noRgbLight = 0;
			}else if(cmpString(USART0.inBuffer+1, "NHE\0")) {
				//TODO
			}else if(cmpString(USART0.inBuffer+1, "HET\0")) {
				//TODO
			}else if(cmpString(USART0.inBuffer+1, "TPE\0")) {
				//TODO
			}else if(cmpString(USART0.inBuffer+1, "TPO\0")) {
				acqSettings.hwTriggerPolarity = USART0.inBuffer[4] - '0';
				if(acqSettings.hwTriggerPolarity < NUM_OF_TRIGGER_POLARITIES) usartAddToOutBuffer("OK");
				else {
					usartAddToOutBuffer("FAIL");
					acqSettings.hwTriggerPolarity = RISING;
				}
			}else usartAddToOutBuffer("UNRECOGNIZED");
			usartAddToOutBuffer("\n\0");
			usartSend();
		break;
		case 'G':
			//message[1-3] = XYZ - acronym for getting parameter
			if(cmpString(USART0.inBuffer+1, "RGE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.rgbEnabled));
			}else if(cmpString(USART0.inBuffer+1, "HDE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.hdrEnabled));
			}else if(cmpString(USART0.inBuffer+1, "TRI\0")) {
				usartAddToOutBuffer(intToString(acqSettings.trigger));
			}else if(cmpString(USART0.inBuffer+1, "TWE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.triggerWidthExposure));
			}else if(cmpString(USART0.inBuffer+1, "NRL\0")) {
				usartAddToOutBuffer(intToString(acqSettings.noRgbLight));
			}else if(cmpString(USART0.inBuffer+1, "NHE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.noHdrExposureTime));
			}else if(cmpString(USART0.inBuffer+1, "HET\0")) {
				for(uint8_t i = 0; i < MAX_HDR_EXP_TIMES; i++) {
					if(acqSettings.hdrExposureTime[i] == 0xFFFF) break;
					usartAddToOutBuffer(intToString(acqSettings.hdrExposureTime[i]));
					if(acqSettings.hdrExposureTime[i+1] != 0xFFFF) usartAddToOutBuffer(",");
				}
			}else if(cmpString(USART0.inBuffer+1, "TPE\0")) {
				usartAddToOutBuffer(intToString(acqSettings.triggerPeriod));
			}else if(cmpString(USART0.inBuffer+1, "TPO\0")) {
				usartAddToOutBuffer(intToString(acqSettings.hwTriggerPolarity));
			} else usartAddToOutBuffer("UNRECOGNIZED");
			usartAddToOutBuffer("\n\0");
			usartSend();
		break;
		default:
			usartAddToOutBuffer("UNRECOGNIZED\n\0");
			usartSend();
		break;
	}
	USART0.receiveComplete = 0;	//ack message
}

void usartAddToOutBuffer(const char *str) {		//strips ending \0
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
	
    while(1) {
		processUsart();
		checkCameraReadyStatus();
    }
}
