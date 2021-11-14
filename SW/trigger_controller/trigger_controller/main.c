#include "global.h"
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "string.h"
#include "USART.h"
#include "settings.h"

acquisitionSettings acqSettings = {
	.pulseOutput[0] = 0,
	.pulseOutput[1] = 0xFF,
	.pulsePeriod[0] = 1000,
	.pulsePeriod[1] = 0xFFFF,
	.triggerSource = NONE,
	.timedTriggerPeriod = 0xFFFF,
	.hwTriggerPolarity = RISING,
};

uint8_t precomputedOCR0A[MAX_PULSE_CONFIGS+1];
uint8_t precomputedTCCR0B[MAX_PULSE_CONFIGS+1];

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t pulseCount = 0;
volatile uint8_t cameraReady = 0;

void startPulseTimer();
void startNewPulseTrain();
void checkCameraReadyStatus();
uint8_t changeTriggerSource(triggerSources source);

ISR(TIMER0_COMPA_vect) {	//pulse timer end
	TCCR0B = 0;				//stop the timer
	if(acqSettings.pulseOutput[pulseCount] == 0xFF) pulseTrainComplete = 1;
}

ISR(TIMER1_COMPA_vect) {					//timed trigger end
	if(cameraReady && pulseTrainComplete) {	//camera can keep up
		startNewPulseTrain();
	}
	else {									//camera can't keep up
		changeTriggerSource(NONE);			//disable triggering
		usartAddToOutBuffer("OVERTRIGGER\n\0");
		usartSend();
	}
}

ISR(INT0_vect) {			//line out 1 rising/falling edge
	checkCameraReadyStatus();
}

void checkCameraReadyStatus() {
	if(PIND & (1<<2)) {		//rising edge or high level of LINE OUT 1 (line trigger wait)
		if(TCCR0B == 0) {	//pulse timer stopped (COMPA interrupt handled)
			cameraReady = 1;
			if(!pulseTrainComplete) startPulseTimer();	//another pulse
			else if(acqSettings.triggerSource == FREE) startNewPulseTrain();	//FREE run mode triggering
		}
	}
	else {		//falling edge or low level of LINE OUT 1 (line trigger wait)
		cameraReady = 0;
	}
}

void startPulseTimer() {
	OCR0A = precomputedOCR0A[pulseCount];
	uint8_t TCCR0B_val = precomputedTCCR0B[pulseCount];
	
	//pulse output select
	TCCR0A = (1<<COM0B1) | (1<<WGM01) | (1<<WGM00);	//pulse output to light
	switch (acqSettings.pulseOutput[pulseCount]) {
		case LINE_START:
			TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);	//pulse output to camera
		break;
		case L1:
		break;
		case L2:
		break;
		case L3:
		break;
		default:
		break;
	}
	
	pulseCount++;
	OCR0B = OCR0A;
	TCNT0 = 0xFF;
	TCCR0B = TCCR0B_val;	//start the timer
}

void startNewPulseTrain() {
	pulseTrainComplete = 0;
	pulseCount = 0;
	startPulseTimer();
}

//CALL THIS WHEN CHANGING PULSE PERIODS
void precomputePulseTimerParameters() {
	uint16_t pulseTime;
	for(uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		pulseTime = acqSettings.pulsePeriod[i];
		if(pulseTime < 125) {								//pulseTime < 125
			precomputedTCCR0B[i] = (1<<CS01);				//8 prescaler, 0.5us period
			precomputedOCR0A[i] = pulseTime*2;
		}else if(pulseTime < 1000) {						//125 < pulseTime < 1000
			precomputedTCCR0B[i] = (1<<CS01) | (1<<CS00);	//64 prescaler, 4us period
			precomputedOCR0A[i] = pulseTime/4;
		}else if(pulseTime < 4000) {						//1000 < pulseTime < 4000
			precomputedTCCR0B[i] = (1<<CS02);				//256 prescaler, 16us period
			precomputedOCR0A[i] = pulseTime/16;
		}else {												//4000 < pulseTime < 10000
			precomputedTCCR0B[i] = (1<<CS02) | (1<<CS00);	//1024 prescaler, 64us period
			precomputedOCR0A[i] = pulseTime/64;
		}
	}
}

uint8_t setTimedTriggerPeriod(uint16_t period) {
	//timer period for 64 prescaler = 1 / 16MHz * 64 = 4us
	cli();
	OCR1A = period / 4;
	if(OCR1A < 1) {
		OCR1A = 0xFFFF/4;
		acqSettings.timedTriggerPeriod = 0xFFFF;
		sei();
		return 0;
	}
	acqSettings.timedTriggerPeriod = period;
	sei();
	return 1;
}

uint8_t changeTriggerSource(triggerSources source) {
	cli();
	uint8_t retVal = 1;
	switch(source) {
		case FREE:
			if(PIND & (1<<2)) cameraReady = 0;	//simulate rising edge if camera is already ready
		break;
		case TIMED:
			TCNT1 = 0;	//reset timed trigger
			TCCR1B |= (1<<CS10) | (1<<CS11);	//start timed trigger with 64 prescaler
		break;
		case HW:
			//TODO
		break;
		case ENCODER:
			//TODO
		break;
		default:	//NONE or out of range
			if(source >= NUM_OF_TRIGGER_SOURCES)	{	//not in the list - fail
				source = NONE;
				retVal = 0;
			}
			TCCR1B &= ~((1<<CS10) | (1<<CS11));	//disable timed trigger
			TCCR0B = 0;	//stop pulse timer
			pulseTrainComplete = 1;
			TCCR0A = 0;	//release pulse timer outputs
			PORTD &= ~((1<<6) | (1<<5));	//OC0A, OC0B output zero
	}
	acqSettings.triggerSource = source;
	sei();
	return retVal;
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
	if(val >= MIN_EXP_TIME && val <= MAX_EXP_TIME) return 1;
	return 0;
}

void processUsart() {
	if(!USART0.receiveComplete) return;	//only process if message is complete
	//message[0] = S(set)/G(get)
	switch(USART0.inBuffer[0]) {
		case 'S':
			//message[1-3] = XYZ - acronym for setting parameter
			if(cmpString(USART0.inBuffer+1, "PUO\0")) {	//pulse output
				triggerSources triggerMem = acqSettings.triggerSource;
				changeTriggerSource(NONE);	//disable triggering for safety
				uint16_t *values = stringToInts(USART0.inBuffer+4, ',');
				for(uint8_t i = 0; i < 0xFF; i++) {
					acqSettings.pulseOutput[i] = values[i];
					if(values[i] == 0xFFFF) {	//reached end of array successfully
						usartAddToOutBuffer("OK");
						acqSettings.pulseOutput[i] = 0xFF;	//terminate array
						break;
					}
					if(values[i] >= NUM_OF_PULSE_OUTPUTS) {	//value out of range
						usartAddToOutBuffer("FAIL");
						acqSettings.pulseOutput[i] = 0xFF;	//terminate array
						break;
					}
				}
				acqSettings.triggerSource = triggerMem;	//restart previous trigger
			}else if(cmpString(USART0.inBuffer+1, "PUP\0")) {	//pulse period
				triggerSources triggerMem = acqSettings.triggerSource;
				changeTriggerSource(NONE);	//disable triggering for safety
				uint16_t *values = stringToInts(USART0.inBuffer+4, ',');
				for(uint8_t i = 0; i < 0xFF; i++) {
					acqSettings.pulsePeriod[i] = values[i];
					if(values[i] == 0xFFFF) {	//reached end of array successfully
						usartAddToOutBuffer("OK");
						break;
					}
					if(!passFailExpRange(values[i])) {	//value out of range
						usartAddToOutBuffer("FAIL");
						acqSettings.pulsePeriod[i] = 0xFFFF;	//terminate array
						break;
					}
				}
				precomputePulseTimerParameters();
				acqSettings.triggerSource = triggerMem;	//restart previous trigger
			}else if(cmpString(USART0.inBuffer+1, "TRS\0")) {	//trigger source
				if(changeTriggerSource(USART0.inBuffer[4] - '0')) usartAddToOutBuffer("OK");
				else usartAddToOutBuffer("FAIL");
			}else if(cmpString(USART0.inBuffer+1, "TTP\0")) {	//timed trigger period
				if(setTimedTriggerPeriod(stringToInt(USART0.inBuffer+4))) usartAddToOutBuffer("OK");
				else usartAddToOutBuffer("FAIL");
			}else if(cmpString(USART0.inBuffer+1, "HTP\0")) {	//HW trigger polarity
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
			if(cmpString(USART0.inBuffer+1, "PUO\0")) {	//pulse output
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					if(acqSettings.pulseOutput[i] == 0xFF) break;
					usartAddToOutBuffer(intToString(acqSettings.pulseOutput[i]));
					if(acqSettings.pulseOutput[i+1] != 0xFF) usartAddToOutBuffer(",");
				}
			}else if(cmpString(USART0.inBuffer+1, "PUP\0")) {	//pulse period
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					if(acqSettings.pulsePeriod[i] == 0xFFFF) break;
					usartAddToOutBuffer(intToString(acqSettings.pulsePeriod[i]));
					if(acqSettings.pulsePeriod[i+1] != 0xFFFF) usartAddToOutBuffer(",");
				}
			}else if(cmpString(USART0.inBuffer+1, "TRS\0")) {	//trigger source
				usartAddToOutBuffer(intToString(acqSettings.triggerSource));
			}else if(cmpString(USART0.inBuffer+1, "TTP\0")) {	//timed trigger period
				usartAddToOutBuffer(intToString(acqSettings.timedTriggerPeriod));
			}else if(cmpString(USART0.inBuffer+1, "HTP\0")) {	//HW trigger polarity
				usartAddToOutBuffer(intToString(acqSettings.hwTriggerPolarity));
			}else if(cmpString(USART0.inBuffer+1, "DBG\0")) {	//DEBUG
				usartAddToOutBuffer(intToString(pulseTrainComplete));
				usartAddToOutBuffer(intToString(pulseCount));
				usartAddToOutBuffer(intToString(cameraReady));
			}else usartAddToOutBuffer("UNRECOGNIZED");
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

int main(void) {
	cli();
	//timer 0 setup (pulse timer)
	DDRD |= (1<<6) | (1<<5);			//OC0A, OC0B output
	TCCR0A = (1<<WGM01) | (1<<WGM00);	//Fast PWM
	TCCR0B = 0;							//stop the timer
	TIMSK0 = (1<<OCIE0A);				//enable COMPA interrupt
	
	//timer 1 setup (timed trigger)
	TCCR1B = (1<<WGM12);	//CTC mode
	TIMSK1 = (1<<OCIE1A);	//enable COMPA interrupt
	setTimedTriggerPeriod(acqSettings.timedTriggerPeriod);
	
	//line out 1 interrupt
	EICRA = (1<<ISC00);		//INT0 on logical change
	EIMSK = (1<<INT0);		//enable INT0
	
	usartInit();
	precomputePulseTimerParameters();
	sei();
	
	DDRD |= (1<<7);		//DEBUG
	
    while(1) {
		processUsart();
		checkCameraReadyStatus();	//check status periodically (camera can be ready at startup - does not generate rising edge interrupt)
    }
}
