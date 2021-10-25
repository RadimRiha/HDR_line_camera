#ifndef STRING_H_
#define STRING_H_

#include <stdint.h>

uint8_t cmpString(volatile char *str1, const char *str2);	//compares USART input buffer with a string constant
char *intToString(uint16_t integer);

#endif