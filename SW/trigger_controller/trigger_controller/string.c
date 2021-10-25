#include "string.h"

uint8_t cmpString(volatile char *str1, const char *str2) {
	for(uint8_t i = 0; i < 0xFF; i++) {
		if(str2[i] == '\0') break;
		if(str1[i] != str2[i]) return 0;	//mismatch
	}
	return 1;	//match
}
/*

char *concatStrings() {
	
}*/

char *intToString(uint16_t integer) {
	#define MAX_INT_LENGTH 5				//max uint16_t length is 5 digits
	static char output[MAX_INT_LENGTH+1];	//ATTENTION, function call overwrites last return value!
	
	uint8_t intLength = 1;
	
	uint8_t intScale = integer;
	while(1) {
		intScale = intScale/10;
		if(intScale > 0) intLength++;
		else break;
	}
	if(intLength > MAX_INT_LENGTH) intLength = MAX_INT_LENGTH;
	
	for (int16_t i = intLength-1; i >= 0; i--) {
		output[i] = (integer % 10) + '0';
		integer = integer / 10;
	}
	output[intLength] = '\0';
	
	return output;
}
