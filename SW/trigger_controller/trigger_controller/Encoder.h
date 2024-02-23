#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

typedef enum encoderChange {
	A_RISING,
	A_FALLING,
	B_RISING,
	B_FALLING
} encoderChange;

extern volatile uint32_t encoderBackcount;
extern volatile uint32_t encoderErrorcount;
void encoderStateMachine(uint8_t change);

#endif
