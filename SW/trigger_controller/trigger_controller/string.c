#include "string.h"

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

uint8_t stringEmpty(volatile const char *str) {
	if(str[0] == '\0') return 1;
	else return 0;
}
