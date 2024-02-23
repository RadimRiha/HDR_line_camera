#include "main.h"
#include "global.h"
#include <avr/interrupt.h>
#include "string.h"
#include "USART.h"
#include "settings.h"
#include "EEPROM.h"
#include "Encoder.h"

#define TCCR1A_FAST_PWM (1<<WGM11)
#define TCCR1B_FAST_PWM ((1<<WGM12) | (1<<WGM13))

acquisitionSettings acqSettings;

uint16_t precomputedOCR1A[MAX_PULSE_CONFIGS+1];

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t pulseCount = 0;
volatile uint16_t overtriggerCount = 0;

void startPulseTimer();

ISR(INT0_vect) {		// line out 1 rising/falling edge
	if(PIND & (1<<2)) {	// rising edge - camera ready
		TCCR1B = TCCR1B_FAST_PWM;		// stop the timer
		if(!pulseTrainComplete) startPulseTimer();
		else if(acqSettings.triggerSource == FREE) trigger();
	}
}

ISR(TIMER3_COMPA_vect) {	// timed trigger overflow
	trigger();
}

ISR(INT1_vect) {			// HW trigger rising/falling edge
	trigger();
}

ISR(PCINT1_vect) {	// Encoder A rising/falling edge
	if(PINC & (1<<3)) {	// rising edge
		encoderStateMachine(A_RISING);
	}
	else {
		encoderStateMachine(A_FALLING);
	}
}

ISR(PCINT2_vect) {	// Encoder B rising/falling edge
	if(PIND & (1<<4)) {	// rising edge
		encoderStateMachine(B_RISING);
	}
	else {
		encoderStateMachine(B_FALLING);
	}
}

void startPulseTimer() {
	switch (acqSettings.pulseOutput[pulseCount]) {
		case T:
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1A1);	// pulse output to camera
		break;
		case L1:
			PORTC &= ~((1<<0) | (1<<1));	// clear light select (output routes to L1)
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1B1);	// pulse output to light
		break;
		case L2:
			PORTC &= ~(1<<1);
			PORTC |= (1<<0);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1B1);
		break;
		case L3:
			PORTC &= ~(1<<0);
			PORTC |= (1<<1);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1B1);
		break;
		case L_ALL:
			PORTC |= (1<<0) | (1<<1);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1B1);
		break;
		case L1T:
			PORTC &= ~((1<<0) | (1<<1));
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1A1) | (1<<COM1B1);	// pulse output to camera and light
		break;
		case L2T:
			PORTC &= ~(1<<1);
			PORTC |= (1<<0);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1A1) | (1<<COM1B1);
		break;
		case L3T:
			PORTC &= ~(1<<0);
			PORTC |= (1<<1);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1A1) | (1<<COM1B1);
		break;
		case LT_ALL:
			PORTC |= (1<<0) | (1<<1);
			TCCR1A = TCCR1A_FAST_PWM | (1<<COM1A1) | (1<<COM1B1);
		break;
		default:
		break;
	}
	
	OCR1A = precomputedOCR1A[pulseCount];	// set the output compare register (period)
	OCR1B = OCR1A;
	TCNT1 = 0xFFFF;		// counter starts one pulse below BOT (next clock sets output to 1)
	TCCR1B = TCCR1B_FAST_PWM | (1<<CS11);	// set the prescaler to 8 - start the timer
	
	pulseCount++;
	if(acqSettings.pulseOutput[pulseCount] == 0xFF) pulseTrainComplete = 1;
}

uint8_t trigger() {
	if (!pulseTrainComplete || TCCR1B != TCCR1B_FAST_PWM) {	// TCCR1B = TCCR1B_FAST_PWM ensures timer is completely stopped
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
	for(uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		precomputedOCR1A[i] = acqSettings.pulsePeriod[i]*2-1;
	}
}

uint8_t setTimedTriggerPeriod(uint16_t period) {
	// timer period for 8 prescaler = 1 / 16MHz * 8 = 0.5us
	if (period < 4 || period > MAX_TIMED_PERIOD) return 0;
	cli();
	OCR3A = period * 2 - 1;
	acqSettings.timedTriggerPeriod = period;
	sei();
	return 1;
}

uint8_t setTriggerSource(triggerSources source) {
	TCCR3B &= ~(1<<CS31);	// disable timed trigger
	TCCR1B = TCCR1B_FAST_PWM;	// stop pulse timer
	TCCR1A &= ~((1<<COM1A1) | (1<<COM1B1));	// release pulse timer outputs
	PORTD &= ~((1<<6) | (1<<5));	// OC0A, OC0B output zero
	EIMSK &= ~(1<<INT1);		// disable INT1 (HW trigger)
	PCICR &= ~((1<<PCIE1) | (1<<PCIE2));	// disable pin change interrupts for ports C and D
	pulseTrainComplete = 1;
	
	uint8_t retVal = 1;
	switch(source) {
		case NONE:
		break;
		case FREE:
			if(PIND & (1<<2)) {		// trigger if camera is already ready
				trigger();
			}
		break;
		case TIMED:
			TCNT3 = 0;	// reset timed trigger timer
			TCCR3B |= (1<<CS31);	// start timed trigger timer with 8 prescaler
		break;
		case HW_TTL:
			PORTC &= ~(1<<2);		// select TTL input
			EIMSK |= (1<<INT1);		// enable INT1
		break;
		case HW_DIFFERENTIAL:
			PORTC |= (1<<2);		// select differential input
			EIMSK |= (1<<INT1);		// enable INT1
		break;
		case ENCODER:
			PCICR |= (1<<PCIE1) | (1<<PCIE2);	// enable pin change interrupts for ports C and D
		break;
		default:	// out of range
			source = NONE;
			retVal = 0;
		break;
	}
	
	acqSettings.triggerSource = source;
	return retVal;
}

uint8_t setHwTriggerPolarity(hwTriggerPolarities polarity) {
	uint8_t retVal = 1;
	switch(polarity) {
		default:
			retVal = 0;
		case RISING:
			EICRA |= (1<<ISC10) | (1<<ISC11);	// INT1 on rising edge
		break;
		case FALLING:
			EICRA &= ~(1<<ISC10);	// INT1 on falling edge
			EICRA |= (1<<ISC11);
		break;
	}
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
	if(!USART0.receiveComplete) return;	// only process if message is complete
	// message[0] = S(set)/G(get)
	switch(USART0.inBuffer[0]) {
		// set command
		case 'S':
			// message[1-3] = XYZ - acronym for setting parameter
			// pulse output
			if(cmpString(USART0.inBuffer+1, "PUO\0")) {
				// store the current trigger and stop triggering for safety
				triggerSources tmpSource = acqSettings.triggerSource;
				setTriggerSource(NONE);
				
				uint16_t *values = stringToInts(USART0.inBuffer+4, ',');
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					acqSettings.pulseOutput[i] = values[i];
					if(values[i] == 0xFFFF) {	// reached end of array successfully
						usartAddToOutBuffer("OK");
						acqSettings.pulseOutput[i] = 0xFF;	// terminate array
						break;
					}
					if(values[i] >= NUM_OF_PULSE_OUTPUTS) {	// value out of range
						usartAddToOutBuffer("FAIL");
						acqSettings.pulseOutput[i] = 0xFF;	// terminate array
						break;
					}
					if(!passFailExpRange(acqSettings.pulsePeriod[i])) {	// pulse period for this output is invalid, set to default
						acqSettings.pulsePeriod[i] = 1000;
					}
				}
				precomputePulseTimerParameters();
				
				setTriggerSource(tmpSource);	// start triggering again
			}
			// pulse period
			else if(cmpString(USART0.inBuffer+1, "PUP\0")) {
				// store the current trigger and stop triggering for safety
				triggerSources tmpSource = acqSettings.triggerSource;
				setTriggerSource(NONE);
				
				uint16_t *values = stringToInts(USART0.inBuffer+4, ',');
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					acqSettings.pulsePeriod[i] = values[i];
					if(values[i] == 0xFFFF) {	// reached end of array successfully
						usartAddToOutBuffer("OK");
						break;
					}
					if(!passFailExpRange(values[i])) {	// value out of range
						acqSettings.pulsePeriod[i] = 1000;
						usartAddToOutBuffer("FAIL");
						break;
					}
					if(acqSettings.pulseOutput[i] >= NUM_OF_PULSE_OUTPUTS) {	// pulse output for this value missing
						acqSettings.pulseOutput[i] = T;			// default value - output to camera trigger
						acqSettings.pulseOutput[i+1] = 0xFF;	// terminate
					}
				}
				precomputePulseTimerParameters();
				
				setTriggerSource(tmpSource);	// start triggering again
			}
			// trigger source
			else if(cmpString(USART0.inBuffer+1, "TRS\0")) {
				if(setTriggerSource(USART0.inBuffer[4] - '0')) usartAddToOutBuffer("OK");
				else usartAddToOutBuffer("FAIL");
			}
			// timed trigger period
			else if(cmpString(USART0.inBuffer+1, "TTP\0")) {
				if(setTimedTriggerPeriod(stringToInt(USART0.inBuffer+4))) usartAddToOutBuffer("OK");
				else usartAddToOutBuffer("FAIL");
			}
			// HW trigger polarity
			else if(cmpString(USART0.inBuffer+1, "HTP\0")) {
				if(setHwTriggerPolarity(USART0.inBuffer[4] - '0')) usartAddToOutBuffer("OK");
				else usartAddToOutBuffer("FAIL");
			}
			// set over trigger counter to 0
			else if(cmpString(USART0.inBuffer+1, "OTR\0")) {
				overtriggerCount = 0;
				usartAddToOutBuffer("OK");
			}
			// set encoder error and back counter to 0
			else if(cmpString(USART0.inBuffer+1, "ENC\0")) {
				encoderErrorcount = 0;
				encoderErrorcount = 0;
				usartAddToOutBuffer("OK");
			}
			// restore defaults
			else if(cmpString(USART0.inBuffer+1, "RST\0")) {	
				restoreDefaults();
				usartAddToOutBuffer("OK");
			}else usartAddToOutBuffer("UNRECOGNIZED");
			usartAddToOutBuffer("\n\0");
			usartSend();
			saveSettings();
		break;
		// get command
		case 'G':
			// message[1-3] = XYZ - acronym for getting parameter
			// pulse output
			if(cmpString(USART0.inBuffer+1, "PUO\0")) {
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					if(acqSettings.pulseOutput[i] == 0xFF) break;
					usartAddToOutBuffer(intToString(acqSettings.pulseOutput[i]));
					if(acqSettings.pulseOutput[i+1] != 0xFF) usartAddToOutBuffer(",");
				}
			}
			// pulse period
			else if(cmpString(USART0.inBuffer+1, "PUP\0")) {
				for(uint8_t i = 0; i < MAX_PULSE_CONFIGS; i++) {
					if(acqSettings.pulsePeriod[i] == 0xFFFF) break;
					usartAddToOutBuffer(intToString(acqSettings.pulsePeriod[i]));
					if(acqSettings.pulsePeriod[i+1] != 0xFFFF) usartAddToOutBuffer(",");
				}
			}
			// trigger source
			else if(cmpString(USART0.inBuffer+1, "TRS\0")) {
				usartAddToOutBuffer(intToString(acqSettings.triggerSource));
			}
			// timed trigger period
			else if(cmpString(USART0.inBuffer+1, "TTP\0")) {
				usartAddToOutBuffer(intToString(acqSettings.timedTriggerPeriod));
			}
			// HW trigger polarity
			else if(cmpString(USART0.inBuffer+1, "HTP\0")) {
				usartAddToOutBuffer(intToString(acqSettings.hwTriggerPolarity));
			}
			// over trigger counter
			else if(cmpString(USART0.inBuffer+1, "OTR\0")) {
				usartAddToOutBuffer(intToString(overtriggerCount));
			}
			// Encoder info
			else if(cmpString(USART0.inBuffer+1, "ENC\0")) {
				usartAddToOutBuffer(intToString(encoderErrorcount));
				usartAddToOutBuffer(",");
				usartAddToOutBuffer(intToString(encoderBackcount));
			}
			// DEBUG
			else if(cmpString(USART0.inBuffer+1, "DBG\0")) {	
				usartAddToOutBuffer(intToString(pulseTrainComplete));
				usartAddToOutBuffer(",");
				usartAddToOutBuffer(intToString(pulseCount));
				usartAddToOutBuffer(",");
				usartAddToOutBuffer(intToString(TCCR1B));
			}
			// for controller identification and communication test
			else if(cmpString(USART0.inBuffer+1, "ID\0")) {
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
	USART0.receiveComplete = 0;	// ACK message
}

int main(void) {
	cli();
	// timer 1 setup (pulse timer)
	DDRB |= (1<<0);		// this is the ICP1 pin. Set it to output to prevent accidental ICR1 capture
	DDRB |= (1<<1) | (1<<2);	// OC1A, OC1B output
	ICR1 = 0xFFFF;	// set TOP to maximum
	
	// timer 3 setup (timed trigger)
	TCCR3B = (1<<WGM32);	// CTC mode
	TIMSK3 = (1<<OCIE3A);	// enable COMPA interrupt
	
	// line out 1 interrupt
	EICRA |= (1<<ISC00);	// INT0 on logical change
	EIMSK |= (1<<INT0);		// enable INT0
	
	// GPIO setup
	DDRC |= (1<<0) | (1<<1);	// light select output
	DDRC |= (1<<2);				// HW trigger select output
	
	// Encoder setup
	PCMSK1 |= (1<<PCINT11);	// enable PCINT11 - Encoder A signal
	PCMSK2 |= (1<<PCINT20);	// enable PCINT20 - Encoder B signal
	
	loadSettings();
	usartInit();
	sei();
	
    while(1) {
		processUsart();
    }
}
