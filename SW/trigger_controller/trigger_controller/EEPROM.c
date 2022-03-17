#include "EEPROM.h"
#include "settings.h"
#include "global.h"

extern acquisitionSettings acqSettings;

void save(uint8_t val, uint16_t addr) {
	while (EECR & (1<<EEPE)) {}
	EEAR = addr;
	EEDR = val;			//save low byte first
	EECR = (EECR | (1<<EEMPE)) & ~(1<<EEPE);
	EECR |= (1<<EEPE);
}

void save16(uint16_t val, uint16_t addr) {
	save(val, addr);
	save(val>>8, addr+1);
}

uint8_t load(uint16_t addr) {
	EEAR = addr;				//setup address
	EECR |= (1<<EERE);			//read enable
	return EEDR;
}

uint16_t load16(uint16_t addr) {
	uint16_t readVal = load(addr);
	readVal |= load(addr+1) << 8;
	return readVal;
}

void saveSettings() {
	//save PUO
	for (uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		save(acqSettings.pulseOutput[i], ADDR_PUO+i);
	}
	
	//save PUP
	for (uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		save16(acqSettings.pulsePeriod[i], ADDR_PUP+i*2);
	}
	
	//save TRS
	save(acqSettings.triggerSource, ADDR_TRS);
	
	//save TTP
	save16(acqSettings.timedTriggerPeriod, ADDR_TTP);
	
	//save HTP
	save(acqSettings.hwTriggerPolarity, ADDR_HTP);
	
	//data saved at least once
	save(0, ADDR_SAVED);
}

uint8_t loadSettings() {
	while(EECR & (1<<EEPE)) {}	//wait for write complete
	
	//check if settings have been saved before
	if(load(ADDR_SAVED) == 0xFF) return 0;	//data not saved yet, fail
	
	//load PUO
	for(uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		acqSettings.pulseOutput[i] = load(ADDR_PUO+i);
	}
	
	//load PUP
	for(uint8_t i = 0; i < MAX_PULSE_CONFIGS+1; i++) {
		acqSettings.pulsePeriod[i] = load16(ADDR_PUP+i*2);
	}
	precomputePulseTimerParameters();
	
	//load TRS
	acqSettings.triggerSource = load(ADDR_TRS);
	
	//load TTP
	acqSettings.timedTriggerPeriod = load16(ADDR_TTP);
	
	//load HTP
	acqSettings.hwTriggerPolarity = load(ADDR_HTP);
	
	return 1;
}
