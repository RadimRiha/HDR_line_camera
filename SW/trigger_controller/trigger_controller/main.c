#include "global.h"
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "string.h"
#include "USART.h"
#include "settings.h"
#include "EEPROM.h"

#define TCCR0A_FAST_PWM ((1<<WGM01) | (1<<WGM00))

acquisitionSettings acqSettings;

uint8_t precomputedOCR0A[MAX_PULSE_CONFIGS+1];
uint8_t precomputedTCCR0B[MAX_PULSE_CONFIGS+1];

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t pulseCount = 0;
volatile uint8_t cameraReady = 0;
volatile uint8_t cameraReadyRequest = 0;
volatile uint8_t timedTriggerRequest = 0;
volatile uint8_t hwTriggerRequest = 0;
void checkCameraReady();
uint8_t changeTriggerSource(triggerSources source);
void startPulseTimer();
uint8_t trigger();

uint16_t overtriggerCount = 0;
/*
ISR(TIMER0_COMPA_vect) {	//pulse timer overflow
	//TCCR0B = 0;				//stop the timer
	
	pulseCount++;
	if(acqSettings.pulseOutput[pulseCount] == 0xFF) pulseTrainComplete = 1;
	
}*/

ISR(INT0_vect) {		// line out 1 rising/falling edge
	
	if(PIND & (1<<2)){	// rising edge - camera ready
		TCCR0B = 0;				//stop the timer
		if(!pulseTrainComplete) startPulseTimer();
		else if(acqSettings.triggerSource == FREE) {
			trigger();
			//cameraReadyRequest = 0;	// FREE run mode triggering
		}
	}
	
	//checkCameraReady();
}

ISR(TIMER3_COMPA_vect) {	// timed trigger overflow
	trigger();
}

ISR(INT1_vect) {			//HW trigger rising/falling edge
	hwTriggerRequest = 1;
}

void checkCameraReady(){
	if(PIND & (1<<2)){
		cameraReadyRequest = 1;
		cameraReady = 1;
	}
	else cameraReady = 0;
}

void startPulseTimer() {
	//TCCR0A = 0;
	//uint8_t TCCR0B_val = precomputedTCCR0B[pulseCount];
	//TCCR0A = 0;
	//TCCR0A &= ~((1<<COM0A1) | (1<<COM0B1));	//release pulse timer outputs
	//PORTC &= ~((1<<0) | (1<<1));	//clear light select (output goes to L1)
	
	switch (acqSettings.pulseOutput[pulseCount]) {
		case T:
			//TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);	//pulse output to camera
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0A1);	//pulse output to camera
		break;
		case L1:
			PORTC &= ~((1<<0) | (1<<1));	//clear light select (output goes to L1)
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0B1);	// pulse output to light
		break;
		case L2:
			PORTC &= ~(1<<1);
			PORTC |= (1<<0);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0B1);
		break;
		case L3:
			PORTC &= ~(1<<0);
			PORTC |= (1<<1);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0B1);
		break;
		case L_ALL:
			PORTC |= (1<<0) | (1<<1);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0B1);
		break;
		case L1T:
			//keep light select cleared
			PORTC &= ~((1<<0) | (1<<1));
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0A1) | (1<<COM0B1);	//pulse output to camera and light
		break;
		case L2T:
			PORTC &= ~(1<<1);
			PORTC |= (1<<0);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0A1) | (1<<COM0B1);
		break;
		case L3T:
			PORTC &= ~(1<<0);
			PORTC |= (1<<1);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0A1) | (1<<COM0B1);
		break;
		case LT_ALL:
			PORTC |= (1<<0) | (1<<1);
			TCCR0A = TCCR0A_FAST_PWM | (1<<COM0A1) | (1<<COM0B1);
		break;
		default:
		break;
	}
	
	OCR0A = precomputedOCR0A[pulseCount];
	OCR0B = OCR0A;
	TCNT0 = 0xFF;
	GTCCR |= (1<<TSM);
	TCCR0B = precomputedTCCR0B[pulseCount];	//start the timer
	GTCCR &= ~(1<<TSM);
	
	pulseCount++;
	if(acqSettings.pulseOutput[pulseCount] == 0xFF) pulseTrainComplete = 1;
}

uint8_t trigger() {
	if (!cameraReady || !pulseTrainComplete || TCCR0B != 0) {
		if (overtriggerCount < 0xFFFF) overtriggerCount++;
		return 0;
	}
	pulseTrainComplete = 0;
	pulseCount = 0;
	startPulseTimer();
	return 1;
}

//CALL THIS WHEN CHANGING PULSE PERIODS
void precomputePulseTimerParameters() {
	uint16_t pulseTime;
	for(uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		pulseTime = acqSettings.pulsePeriod[i];
		if(pulseTime < 125) {								//pulseTime < 125
			precomputedTCCR0B[i] = (1<<CS01);				//8 prescaler, 0.5us period
			precomputedOCR0A[i] = pulseTime*2-1;
		}else if(pulseTime < 1000) {						//125 < pulseTime < 1000
			precomputedTCCR0B[i] = (1<<CS01) | (1<<CS00);	//64 prescaler, 4us period
			precomputedOCR0A[i] = pulseTime/4-1;
		}else if(pulseTime < 4000) {						//1000 < pulseTime < 4000
			precomputedTCCR0B[i] = (1<<CS02);				//256 prescaler, 16us period
			precomputedOCR0A[i] = pulseTime/16-1;
		}else {												//4000 < pulseTime < 10000
			precomputedTCCR0B[i] = (1<<CS02) | (1<<CS00);	//1024 prescaler, 64us period
			precomputedOCR0A[i] = pulseTime/64-1;
		}
	}
}

void restoreDefaults() {
	acqSettings.pulseOutput[0] = 0;
	acqSettings.pulseOutput[1] = 0xFF;
	acqSettings.pulsePeriod[0] = 1000;
	acqSettings.pulsePeriod[1] = 0xFFFF;
	acqSettings.triggerSource = NONE;
	acqSettings.timedTriggerPeriod = MAX_TIMED_PERIOD;
	acqSettings.hwTriggerPolarity = RISING;
	precomputePulseTimerParameters();
}

uint8_t setTimedTriggerPeriod(uint16_t period) {
	//timer period for 8 prescaler = 1 / 16MHz * 8 = 0.5us
	if (period < 4 || period > MAX_TIMED_PERIOD) return 0;
	cli();
	OCR3A = period * 2 - 1;
	acqSettings.timedTriggerPeriod = period;
	sei();
	return 1;
}

uint8_t changeTriggerSource(triggerSources source) {
	TCCR3B &= ~(1<<CS31);	//disable timed trigger
	TCCR0B = 0;	//stop pulse timer
	TCCR0A &= ~((1<<COM0A1) | (1<<COM0B1));	//release pulse timer outputs
	PORTD &= ~((1<<6) | (1<<5));	//OC0A, OC0B output zero
	EIMSK &= ~(1<<INT1);		//disable INT1 (HW trigger)
	
	uint8_t retVal = 1;
	switch(source) {
		case NONE:
		break;
		case FREE:
			if(PIND & (1<<2)) {		//simulate rising edge if camera is already ready
				trigger();
			}
		break;
		case TIMED:
			TCNT3 = 0;	//reset timed trigger
			TCCR3B |= 1<<CS31;	//start timed trigger with 8 prescaler
		break;
		case HW_TTL:
			PORTC &= ~(1<<2);		// select TTL input
			EIMSK |= (1<<INT1);		//enable INT1
			
			
			EICRA |= (1<<ISC10) | (1<<ISC11);		//INT1 on rising edge
		break;
		case HW_DIFFERENTIAL:
			PORTC |= (1<<2);		// select differential input
			EIMSK |= (1<<INT1);		//enable INT1
			
			
			EICRA |= (1<<ISC10) | (1<<ISC11);		//INT1 on rising edge
		break;
		case ENCODER:
			//TODO
		break;
		default:	// out of range
			source = NONE;
			retVal = 0;
		break;
	}
	pulseTrainComplete = 1;
	acqSettings.triggerSource = source;
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
				//triggerSources triggerMem = acqSettings.triggerSource;
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
				//acqSettings.triggerSource = triggerMem;	//restart previous trigger
			}else if(cmpString(USART0.inBuffer+1, "PUP\0")) {	//pulse period
				//triggerSources triggerMem = acqSettings.triggerSource;
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
				//acqSettings.triggerSource = triggerMem;	//restart previous trigger
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
			}else if(cmpString(USART0.inBuffer+1, "OTR\0")) {	//set overtrigger counter to 0
				overtriggerCount = 0;
				usartAddToOutBuffer("OK");
			}else if(cmpString(USART0.inBuffer+1, "RST\0")) {	//restore defaults
				restoreDefaults();
				usartAddToOutBuffer("OK");
			}else usartAddToOutBuffer("UNRECOGNIZED");
			usartAddToOutBuffer("\n\0");
			usartSend();
			saveSettings();
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
			}else if(cmpString(USART0.inBuffer+1, "OTR\0")) {	//overtrigger counter
				usartAddToOutBuffer(intToString(overtriggerCount));
			}else if(cmpString(USART0.inBuffer+1, "DBG\0")) {	//DEBUG
				usartAddToOutBuffer(intToString(pulseTrainComplete));
				usartAddToOutBuffer(intToString(pulseCount));
				usartAddToOutBuffer(intToString(cameraReady));
				usartAddToOutBuffer(intToString(cameraReadyRequest));
			}else if(cmpString(USART0.inBuffer+1, "ID\0")) {	//for controller identification and communication test
				usartAddToOutBuffer("CONTROLLER");
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
	TCCR0A = TCCR0A_FAST_PWM;			//Fast PWM
	TCCR0B = 0;							//stop the timer
	//TIMSK0 = (1<<OCIE0A);				//enable COMPA interrupt
	
	//timer 3 setup (timed trigger)
	TCCR3B = (1<<WGM32);	//CTC mode
	TIMSK3 = (1<<OCIE3A);	//enable COMPA interrupt
	
	//line out 1 interrupt
	EICRA |= (1<<ISC00);	//INT0 on logical change
	EIMSK |= (1<<INT0);		//enable INT0
	
	//GPIO setup
	DDRC |= (1<<0) | (1<<1);//light select output
	DDRC |= (1<<2);			//HW trigger select output
	
	if(!loadSettings()) restoreDefaults();
	usartInit();
	sei();
	
	setTimedTriggerPeriod(acqSettings.timedTriggerPeriod);
	changeTriggerSource(acqSettings.triggerSource);
	checkCameraReady();
	
	//triggerNextExposure();
	
    while(1) {
		/*
		if(TCCR0B == 0){	// pulse timer COMPA interrupt executed
			if(cameraReadyRequest){
				if (!pulseTrainComplete){
					//startPulseTimer();
					cameraReadyRequest = 0;
				}
				else if (acqSettings.triggerSource == FREE){
					trigger();
					cameraReadyRequest = 0;	// FREE run mode triggering
				}
			}
			switch(acqSettings.triggerSource) {
				case TIMED:
					if (timedTriggerRequest){
						trigger();
						timedTriggerRequest = 0;
					}
				break;
				case HW_TTL:
				case HW_DIFFERENTIAL:
					if (hwTriggerRequest){
						trigger();
						hwTriggerRequest = 0;
					}
				case ENCODER:
				//TODO
				break;
				default:
				break;
			}
		}*/
		processUsart();
		
			/*
			if(cameraReadyRequest){
				if (!pulseTrainComplete) {
					startPulseTimer();
					cameraReadyRequest = 0;
				}
				else if(acqSettings.triggerSource == FREE) {
					trigger();
					cameraReadyRequest = 0;	// FREE run mode triggering
				}
			}*/
			/*
			if (timedTriggerRequest){
				trigger();
				timedTriggerRequest = 0;
			}*/
			/*
			if (hwTriggerRequest){
				trigger();
				hwTriggerRequest = 0;
			}*/
		
		//triggerNextExposure();	//DO NOT DO THIS, MARGINAL BEHAVIOUR
    }
}
