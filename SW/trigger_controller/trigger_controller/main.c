#include <xc.h>
#define F_CPU 16000000UL
#include <util/delay.h>

int main(void) {
	DDRD |= (1<<2);
	
    while(1) {
		PORTD |= (1<<2);
        _delay_ms(1000);
		PORTD &= ~(1<<2);
		_delay_ms(1000);
    }
}
