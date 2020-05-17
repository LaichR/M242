#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>
	
void OverflowImpl(void);
void CompareMatchImpl(void);

extern uint8_t timing[];
extern uint8_t timingCurrentIndex;


void ConfigurePins(void)
{
	SetRegister(PortB.DDR, (PIN_3, DdrOutput), (PIN_4, DdrOutput), 
		(PIN_5, DdrInput));
	SetRegister(PortB.PORT, (PIN_4, 0), (PIN_3,1));

	// Demo configs
	SetRegister(PortD.DDR, (PIN_6, DdrOutput), (PIN_7, DdrOutput));
}


void ConfigureTimerCounter(void)
{
	// configure timer counter 1
	SetRegister(Tcnt0.TCCRA, (TCCRA_WGM, ClrTmrOnCmpMatch)); // compare match
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CsT1_Div1024)); // counting speed!
	Tcnt0.OCRA = 41;

	// configure timer counter 2
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, Normal));
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, CsT2_Div1024));

	// enable interrupts
	SetRegister(Timsk0, (TIMSK_OCIEA, True) ); // enable compare match interrupts for tcnt0
	SetRegister(Timsk2, (TIMSK_TOIE, True));   // enable overflow interrutp for tcnt1
}


int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("hello world 1");

	ConfigurePins();
	ConfigureTimerCounter();
	while (True);
	
	return 0;
}


ISR_Tcnt2Overflow()
{
	//TRACE("overflow timer counter 2");
	/*UpdateRegister(PortD.PORT, (PIN_6, True));
	_delay_us(5);
	UpdateRegister(PortD.PORT, (PIN_6, False));*/
	OverflowImpl();
}


ISR_Tcnt0CompareMatchA()
{
	//TRACE("Tcnt0 compare match a interrupt");
	/*UpdateRegister(PortD.PORT, (PIN_7, True));
	_delay_us(5);
	UpdateRegister(PortD.PORT, (PIN_7, False));*/
	CompareMatchImpl();
}
