#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>

int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("Binking LED");
	Bool ledOn = False;
	// set the data direktion
	// Pin3 and pin 4 drive the led that is plugged in 
	// the signal of pin 3 is routed to PIN_5 that is connected to the status LED of the board
	// hence PIN_5 has to be an input!
	SetRegister(PortB.DDR, (PIN_3, DdrOutput), (PIN_4, DdrOutput), (PIN_5, DdrInput));

	// force pin 4 to 0; pin 3 will toggle; hence the initial value can be anything!
	// i use here a logic value ledOn to reflect the current value of the Led
	SetRegister(PortB.PORT, (PIN_3, ledOn), (PIN_4, 0));
	while (True)
	{
		// toggle the value of ledOn!
		ledOn = !ledOn;
		UpdateRegister(PortB.PORT, (PIN_3, ledOn));

		// this is a busy wait; the cpu will just loop a number of times
		// hence no other processing is possible during this time!
		// and the cpu uses current
        _delay_ms(1000);
	}
	return 0;
}

