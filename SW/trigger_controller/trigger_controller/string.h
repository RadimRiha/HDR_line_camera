#ifndef STRING_H_
#define STRING_H_

#include <stdint.h>

// functions for operations on '/0' terminated strings

uint8_t stringLength(volatile const char *str);
uint8_t stringEmpty(volatile const char *str);
// returns true if strings match
uint8_t cmpString(volatile const char *str1, const char *str2);
// converts integer to string
char *intToString(int32_t integer);
// converts string to a single unsigned integer
uint16_t stringToInt(volatile const char *str);
// converts string to array of unsigned integers and terminates it with 0xFFFF
// integers in the source string must be delimited by splitMarker
uint16_t *stringToInts(volatile const char *str, char splitMarker);

#endif