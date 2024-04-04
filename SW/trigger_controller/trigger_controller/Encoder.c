#include "Encoder.h"
#include "main.h"

// state machine current state
volatile uint8_t encoderState = 0;
// flag for clockwise rotation sequence start
volatile uint8_t cwSequenceStarted = 0;
// flag for counterclockwise rotation sequence start
volatile uint8_t ccwSequenceStarted = 0;
// error counter
volatile uint32_t encoderErrorcount = 0;
// back counter (is 0 during forward motion)
volatile uint32_t encoderBackcount = 0;

void error(){
	cwSequenceStarted = 0;
	ccwSequenceStarted = 0;
	if(encoderErrorcount < 0xFFFFFFFF) encoderErrorcount++;
}

void encoderStateMachine(uint8_t change) {
	switch(encoderState) {
		// state 0 - both signals low
		case 0:
			switch (change) {
				case A_RISING:
					cwSequenceStarted = 1;
					encoderState = 1;
				break;
				case A_FALLING:
					error();
				break;
				case B_RISING:
					ccwSequenceStarted = 1;
					encoderState = 3;
				break;
				case B_FALLING:
					error();
				break;
			}
		break;
		// state 1 - A high, B low
		case 1:
			switch (change) {
				case A_RISING:
					error();
				break;
				case A_FALLING:
					encoderState = 0;
					cwSequenceStarted = 0;
					if (ccwSequenceStarted) {
						// CCW sequence completed
						ccwSequenceStarted = 0;
						if (encoderBackcount < 0xFFFFFFFF) encoderBackcount++;
					}
				break;
				case B_RISING:
					encoderState = 2;
				break;
				case B_FALLING:
					error();
				break;
			}
		break;
		// state 2 - both signals high
		case 2:
			switch (change) {
				case A_RISING:
					error();
				break;
				case A_FALLING:
					encoderState = 3;
				break;
				case B_RISING:
					error();
				break;
				case B_FALLING:
					encoderState = 1;
				break;
			}
		break;
		// state 3 - A low, B high
		case 3:
			switch (change) {
				case A_RISING:
					encoderState = 2;
				break;
				case A_FALLING:
					error();
				break;
				case B_RISING:
					error();
				break;
				case B_FALLING:
					encoderState = 0;
					ccwSequenceStarted = 0;
					if (cwSequenceStarted) {
						// cw sequence completed
						if (encoderBackcount == 0) trigger();
						else encoderBackcount--;
						cwSequenceStarted = 0;
					}
				break;
			}
		break;
	}
}
