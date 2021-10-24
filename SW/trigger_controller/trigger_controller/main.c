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
	.noHdrExposureTime = 200,
	.hdrExposureTime[0] = 100,
	.hdrExposureTime[1] = 200,
	.hdrExposureTime[2] = 2000,
	.hdrExposureTime[3] = 5000,
	.hdrExposureTime[4] = 0xFF,
	.triggerPeriod = 1000,
	.hwTriggerPolarity = RISING
};

//volatile uint8_t precomputedExpTimes[MAX_HDR_EXP_TIMES] = {200, 50, 70, 64};

void cameraReadyInterrupt();
void cameraNotReadyInterrupt();
void startTriggerTimer();
void startNewLineTrigger();
void checkCameraReadyStatus();

ISR(TIMER0_COMPA_vect) {
	TCCR0B = 0;		//stop the timer
	if(!acqSettings.hdrEnabled) pulseTrainComplete = 1;
	else if(acqSettings.hdrExposureTime[hdrPulseCount] == 0xFF) pulseTrainComplete = 1;
}

ISR(INT0_vect) {	//line out 1 rising/falling edge
	checkCameraReadyStatus();
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

void startTriggerTimer() {
	uint16_t expTimeToSet;
	uint8_t TCCR0B_val;
	
	if(acqSettings.hdrEnabled) {
		expTimeToSet = acqSettings.hdrExposureTime[hdrPulseCount];
		//OCR0A = precomputedExpTimes[hdrPulseCount];
		hdrPulseCount++;
	}
	else expTimeToSet = acqSettings.noHdrExposureTime;
	
	if(expTimeToSet < 125) {				//exp time < 125
		TCCR0B_val = (1<<CS01);				//8 prescaler, 0.5us period
		OCR0A = expTimeToSet*2;
	}else if(expTimeToSet < 1000) {			//125 < exp time < 1000
		TCCR0B_val = (1<<CS01) | (1<<CS00);	//64 prescaler, 4us period
		OCR0A = expTimeToSet/4;
	}else if(expTimeToSet < 4000) {			//1000 < exp time < 4000
		TCCR0B_val = (1<<CS02);				//256 prescaler, 16us period
		OCR0A = expTimeToSet/16;
	}else {									//4000 < exp time < 10000
		TCCR0B_val = (1<<CS02) | (1<<CS00);	//1024 prescaler, 64us period
		OCR0A = expTimeToSet/64;
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
	sei();
	
	//checkCameraReadyStatus();
	
    while(1) {
		checkCameraReadyStatus();
    }
}
