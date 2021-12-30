#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>
#include <avr/interrupt.h>

#define ADDR_PUO 0
#define ADDR_PUP (ADDR_PUO + MAX_PULSE_CONFIGS+1)
#define ADDR_TRS (ADDR_PUP + 2*(MAX_PULSE_CONFIGS+1))
#define ADDR_TTP (ADDR_TRS + 1)
#define ADDR_HTP (ADDR_TTP + 2)
#define ADDR_SAVED (ADDR_HTP + 1)

void saveSettings();
uint8_t loadSettings();

#endif
