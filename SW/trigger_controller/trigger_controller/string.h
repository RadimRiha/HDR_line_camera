#ifndef STRING_H_
#define STRING_H_

#include <stdint.h>

uint8_t stringLength(volatile const char *str);
uint8_t stringEmpty(volatile const char *str);
uint8_t cmpString(volatile const char *str1, const char *str2);	//compares USART input buffer with a string constant
char *intToString(int32_t integer);
uint16_t stringToInt(volatile const char *str);
uint16_t *stringToInts(volatile const char *str, char splitMarker);	//converts string to array of integers and terminates it with 0xFFFF

#endif