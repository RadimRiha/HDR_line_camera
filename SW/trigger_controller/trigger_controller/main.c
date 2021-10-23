#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_HDR_EXP_TIMES 5		//maximum number of HDR exposure times

volatile uint8_t pulseTrainComplete = 1;
volatile uint8_t hdrPulseCount = 0;
volatile uint8_t cameraReady = 1;

enum triggerTypes{FREE, TIMED, HW, ENCODER};
enum hwTriggerPolarities{RISING, FALLING, HIGH, LOW};
	
struct acquisitionSettings {
	uint8_t rgbEnabled;				//acquisition with/without RGB light cycling
	uint8_t hdrEnabled;				//acquisition with/without HDR sequence
	enum triggerTypes trigger;		//triggering type
	uint8_t noRgbLight;				//use white light if RGB is disabled
	uint8_t noHdrExposureTime;		//use this exposure time if HDR is disabled
	uint8_t hdrExposureTime[MAX_HDR_EXP_TIMES+1];	//HDR exposure times
	uint16_t triggerPeriod;			//period [us] of the TIMED trigger
	enum hwTriggerPolarities hwTriggerPolarity;
};

struct acquisitionSettings acqSettings = {
	.rgbEnabled = 0,
	.hdrEnabled = 1,
	.trigger = FREE,
	.noRgbLight = 0,
	.noHdrExposureTime = 2,
	.hdrExposureTime[0] = 2,
	.hdrExposureTime[1] = 5,
	.hdrExposureTime[2] = 10,
	.hdrExposureTime[3] = 0xFF,
	.triggerPeriod = 1000,
	.hwTriggerPolarity = RISING
};

void cameraReadyInterrupt();
void cameraNotReadyInterrupt();

ISR(TIMER0_COMPA_vect) {
	TCCR0B = 0;	//stop the timer
	if(!acqSettings.hdrEnabled) pulseTrainComplete = 1;
	else if(acqSettings.hdrExposureTime[hdrPulseCount] == 0xFF) pulseTrainComplete = 1;
	cameraReadyInterrupt();	//DEBUG!!!!!!!!
}

void pulseTimerStart() {
	if(acqSettings.hdrEnabled) {
		OCR0A = acqSettings.hdrExposureTime[hdrPulseCount]*2;
		hdrPulseCount++;
	}
	else OCR0A = acqSettings.noHdrExposureTime*2;
	TCNT0 = 0xFF;
	TCCR0B = 1<<CS01;	//start the timer
	cameraNotReadyInterrupt();	//DEBUG!!!!!!!!!!
}

void startPulse() {
	if(!pulseTrainComplete || !cameraReady) return;
	pulseTrainComplete = 0;
	hdrPulseCount = 0;
	pulseTimerStart();
}

void cameraNotReadyInterrupt() {	//PLACEHOLDER, replace with a call from camera
	cameraReady = 0;
}

void cameraReadyInterrupt() {	//PLACEHOLDER, replace with a call from camera
	cameraReady = 1;
	if(!pulseTrainComplete) {
		pulseTimerStart();
	}
}

int main(void) {
	cli();
	DDRD |= (1<<6);
	TCCR0A = (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);	//Clear OC0A on compare match, set OC0A at BOTTOM, Fast PWM
	TCCR0B = 0;				//stop the timer
	TIMSK0 = (1<<OCIE0A);	//enable COMPA interrupt
	sei();
	
    while(1) {
		startPulse();
    }
}
