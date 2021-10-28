#include "global.h"
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "string.h"
#include "USART.h"
#include "settings.h"

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

void startTriggerTimer();
void startNewLineTrigger();

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

//CALL THIS WHEN CHANGING EXPOSURE TIMES
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

uint8_t passFailBool(uint8_t val) {
	if(val == 0 || val == 1) {
		usartAddToOutBuffer("OK");
		return 1;
	}
	usartAddToOutBuffer("FAIL");
	return 0;
}

uint8_t passFailExpRange(uint16_t val) {
	if(val >= MIN_EXP_TIME && val <= MAX_EXP_TIME) {
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
				acqSettings.noHdrExposureTime = stringToInt(USART0.inBuffer+4);
				if(!passFailExpRange(acqSettings.noHdrExposureTime)) acqSettings.noHdrExposureTime = MIN_EXP_TIME;
				precomputeTriggerTimerParameters();
			}else if(cmpString(USART0.inBuffer+1, "HET\0")) {
				uint16_t *values = stringsToInts(USART0.inBuffer+4, ',');
				for(uint8_t i = 0; i < 0xFF; i++) {
					acqSettings.hdrExposureTime[i] = values[i];
					if(values[i] == 0xFFFF) break;
					if(!passFailExpRange(values[i])) {
						acqSettings.hdrExposureTime[i] = 0xFFFF;
						break;
					}
				}
				precomputeTriggerTimerParameters();
			}else if(cmpString(USART0.inBuffer+1, "TPE\0")) {
				acqSettings.triggerPeriod = stringToInt(USART0.inBuffer+4);
				usartAddToOutBuffer("OK");
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
	
	usartInit();
	precomputeTriggerTimerParameters();
	sei();
	
	DDRD |= (1<<7);		//DEBUG
	
    while(1) {
		processUsart();
		checkCameraReadyStatus();
    }
}
