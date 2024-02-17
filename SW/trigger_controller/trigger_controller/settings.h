#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

#define MAX_PULSE_CONFIGS 10	//maximum number of pulse configurations
#define MIN_EXP_TIME 2
#define MAX_EXP_TIME 10000
#define MAX_TIMED_PERIOD 30000

typedef enum pulseOutputs {
	T,
	L1,
	L2,
	L3,
	L_ALL,
	L1T,
	L2T,
	L3T,
	LT_ALL,
	NUM_OF_PULSE_OUTPUTS,		//LEAVE THIS AS LAST
} pulseOutputs;

typedef enum triggerSources {
	NONE,
	FREE,
	TIMED,
	HW,
	ENCODER,
	NUM_OF_TRIGGER_SOURCES,		//LEAVE THIS AS LAST
} triggerSources;

typedef enum hwTriggerPolarities {
	RISING,
	FALLING,
	HIGH,
	LOW,
	NUM_OF_TRIGGER_POLARITIES,	//LEAVE THIS AS LAST
} hwTriggerPolarities;

typedef struct acquisitionSettings {
	pulseOutputs pulseOutput[MAX_PULSE_CONFIGS+1];	//output of each pulse
	uint16_t pulsePeriod[MAX_PULSE_CONFIGS+1];		//period [us] corresponding to each pulse output
	triggerSources triggerSource;					//source for individual trigger events
	uint16_t timedTriggerPeriod;					//period [us] of the TIMED trigger
	hwTriggerPolarities hwTriggerPolarity;
} acquisitionSettings;

#endif