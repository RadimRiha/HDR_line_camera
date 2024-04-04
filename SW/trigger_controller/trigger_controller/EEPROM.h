#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>
#include <avr/interrupt.h>
#include "settings.h"

// addresses of the settings in EEPROM
#define ADDR_PUO 0
#define ADDR_PUP (ADDR_PUO + MAX_PULSE_CONFIGS+1)
#define ADDR_TRS (ADDR_PUP + 2*(MAX_PULSE_CONFIGS+1))
#define ADDR_TTP (ADDR_TRS + 1)
#define ADDR_HTP (ADDR_TTP + 2)
#define ADDR_SAVED (ADDR_HTP + 1)

extern void precomputePulseTimerParameters();
extern uint8_t setTimedTriggerPeriod(uint16_t period);
extern uint8_t setTriggerSource(triggerSources source);
extern uint8_t setHwTriggerPolarity(hwTriggerPolarities polarity);

// restores default settings 
// (1 output pulse to trigger, 1000us, triggering disabled)
void restoreDefaults();
// save current settings to EEPROM
void saveSettings();
// try to load settings from EEPROM
// returns true if successful, restores defaults and returns false if unsuccessful
uint8_t loadSettings();

#endif
