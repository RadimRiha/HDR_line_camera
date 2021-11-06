#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

#define MAX_HDR_EXP_TIMES 5		//maximum number of HDR exposure times
#define MIN_EXP_TIME 2
#define MAX_EXP_TIME 10000

typedef enum triggerTypes {
	NONE,
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
	triggerTypes trigger;			//triggering type
	uint8_t triggerWidthExposure;	//exposure control using trigger width/light duration
	uint8_t noRgbLight;				//use white light if RGB is disabled
	uint16_t noHdrExposureTime;		//use this exposure time if HDR is disabled
	uint16_t hdrExposureTime[MAX_HDR_EXP_TIMES+1];	//HDR exposure times
	uint16_t triggerPeriod;			//period [us] of the TIMED trigger
	hwTriggerPolarities hwTriggerPolarity;
} acquisitionSettings;

#endif