#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_HDR_EXP_TIMES 5		//maximum number of HDR exposure times

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t hdrPulseCount = 0;
volatile uint8_t cameraReady = 0;

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
	.hwTriggerPolarity = RISING
};

volatile uint8_t precomputedOCR0A[MAX_HDR_EXP_TIMES+1];
volatile uint8_t precomputedTCCR0B[MAX_HDR_EXP_TIMES+1];

void cameraReadyInterrupt();
void cameraNotReadyInterrupt();
void startTriggerTimer();
void startNewLineTrigger();
void checkCameraReadyStatus();

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
	
	precomputeTriggerTimerParameters();
	sei();
	
    while(1) {
		checkCameraReadyStatus();
    }
}
