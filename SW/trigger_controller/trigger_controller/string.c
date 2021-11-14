#include "string.h"
#include "settings.h"

uint8_t stringLength(volatile const char *str) {
	for(uint8_t i = 0; i < 0xFF; i++) {
		if(str[i] == '\0') return i;
	}
	return 0;
}

uint8_t stringEmpty(volatile const char *str) {
	if(str[0] == '\0') return 1;
	else return 0;
}

uint8_t cmpString(volatile const char *str1, const char *str2) {
	for(uint8_t i = 0; i < 0xFF; i++) {
		if(str1[i] == '\0' || str2[i] == '\0') break;	//end of either string with no previous mismatches counts as a match
		if(str1[i] != str2[i]) return 0;	//mismatch
	}
	return 1;	//match
}

char *intToString(uint16_t integer) {
	#define MAX_INT_LENGTH 5				//max uint16_t length is 5 digits
	static char output[MAX_INT_LENGTH+1];	//ATTENTION, function call overwrites last return value!
	
	uint8_t intLength = 1;
	uint16_t intScale = integer;
	while(1) {
		intScale = intScale/10;
		if(intScale > 0) intLength++;
		else break;
	}
	
	for(int8_t i = intLength-1; i >= 0; i--) {
		output[i] = (integer % 10) + '0';
		integer = integer / 10;
	}
	output[intLength] = '\0';
	
	return output;
}

uint16_t stringToInt(volatile const char *str) {
	if(stringEmpty(str)) return 0;
	
	uint16_t output = 0;
	uint8_t len = stringLength(str);
	uint16_t factor = 1;
	for(int16_t i = len-1; i >= 0; i--) {
		output += (str[i]-'0') * factor;
		factor = factor * 10;
	}
	
	return output;
}

uint16_t *stringToInts(volatile const char *str, char splitMarker) {
	if(stringEmpty(str)) return 0;
	
	#define MAX_NUM_OF_INTS MAX_PULSE_CONFIGS
	static uint16_t returnArray[MAX_NUM_OF_INTS+1];
	uint8_t returnArrayIndex = 0;
	
	#define MAX_INT_LENGTH 5				//max uint16_t length is 5 digits
	static char stringPart[MAX_INT_LENGTH+1];
	uint8_t stringPartIndex = 0;
	
	for(uint8_t i = 0; i < 0xFF; i++) {
		stringPart[stringPartIndex] = str[i];
		
		if(str[i] == splitMarker || str[i] == '\0') {
			stringPart[stringPartIndex] = '\0';	//overwrite split marker with termination
			returnArray[returnArrayIndex] = stringToInt(stringPart);
			stringPartIndex = 0;
			returnArrayIndex++;
			if(str[i] == '\0') break;	//converted all strings
			if(returnArrayIndex >= MAX_NUM_OF_INTS) break;
		}
		else {
			if(stringPartIndex >= MAX_INT_LENGTH) break;
			stringPartIndex++;
		}
	}
	
	returnArray[returnArrayIndex] = 0xFFFF;
	return returnArray;
}
