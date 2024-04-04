#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

#define MAX_PULSE_CONFIGS 10	// maximum number of pulse configurations
#define MIN_EXP_TIME 2			// minimum settable exposure time [us]
#define MAX_EXP_TIME 10000		// maximum settable exposure time [us]
#define MIN_TIMED_PERIOD 4		// minimum period of timed trigger [us]
#define MAX_TIMED_PERIOD 30000	// maximum period of timed trigger [us]

typedef enum pulseOutputs {
	T,		// cam trigger
	L1,		// light 1
	L2,		// light 2
	L3,		// light 3
	L_ALL,	// all lights
	L1T,	// trigger + light 1
	L2T,	// trigger + light 2
	L3T,	// trigger + light 3
	LT_ALL,	// trigger + all lights
	NUM_OF_PULSE_OUTPUTS,		//LEAVE THIS AS LAST
} pulseOutputs;

typedef enum triggerSources {
	NONE,	// triggering disabled
	FREE,	// free triggering (as fast as possible)
	TIMED,	// timed periodic trigger
	HW_TTL,	// hardware TTL trigger
	HW_DIFFERENTIAL,	// hardware RS422 trigger
	ENCODER,	// encoder triggering (A, B)
	NUM_OF_TRIGGER_SOURCES,		//LEAVE THIS AS LAST
} triggerSources;

typedef enum hwTriggerPolarities {
	RISING,		// hardware trigger on rising edge
	FALLING,	// hardware trigger on falling edge
	NUM_OF_TRIGGER_POLARITIES,	//LEAVE THIS AS LAST
} hwTriggerPolarities;

// settings structure for the whole project
typedef struct acquisitionSettings {
	pulseOutputs pulseOutput[MAX_PULSE_CONFIGS+1];	// output of each pulse
	uint16_t pulsePeriod[MAX_PULSE_CONFIGS+1];		// period [us] corresponding to each pulse output
	triggerSources triggerSource;					// source of triggering
	uint16_t timedTriggerPeriod;					// period [us] of the TIMED trigger
	hwTriggerPolarities hwTriggerPolarity;			// polarity of the hardware trigger (TTL or RS422)
} acquisitionSettings;

#endif