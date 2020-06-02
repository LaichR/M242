#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>
	

//static uint8_t timing[] = { 1,2,3,4,5,6,7,8,7,6,5,4,3,2 };
//static uint8_t timing[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 14,13,12,11,10,9,8,7,6,5,4,3,2,1 };
static uint8_t timing[] = { 0,1,2,3,4,5,4,3,2,1,};

static uint8_t timingCurrentIndex = 0;
static uint8_t nrOfSamples = 0;




void ConfigureAdc(void)
{
	SetRegister(Prr, (PRR_PRADC, 0));		 // disable power reduction for adc
	SetRegister(Adc.Admux, (ADMUX_MUX, MuxAdc0), (ADMUX_REFS, Internal1_1));
	SetRegister(Adc.Adcsra, (ADCSRA_ADEN, 1), // enable ADC
		(ADCSRA_ADPS, AdcDiv64),			  // prescaler selection; a conversion takes 13 clocks a 4us
		(ADCSRA_ADIE, True),			      // do not enable adc complte interrupt
		(ADCSRA_ADATE, 0));					  // enable adc trigger source
	SetRegister(Adc.Adcsrb, (ADCSRB_ADTS, FreeRunning));
	SetRegister(Adc.Didr0, (DIDR0_ADC0D, 1));
}

void ConfigurePins(void)
{
	SetRegister(PortB.DDR, (PIN_3, DdrOutput), (PIN_4, DdrOutput));
	SetRegister(PortB.PORT, (PIN_4, 0), (PIN_3, 1));

	SetRegister(PortD.DDR, (PIN_7, DdrOutput), (PIN_6, DdrOutput)); //two debug pins
	SetRegister(PortD.PORT, (PIN_7, 0), (PIN_6, 0));

	SetRegister(PortC.DDR, (PIN_0, DdrOutput));
	SetRegister(PortC.PORT, (PIN_0, 0));
}


void ConfigureTimerCounter(void)
{
	// configure timer counter 2
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, PwmNormal), (TCCRA_COMA, 2)); // compare match
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, CS_Div1)); // counting speed!
	Tcnt2.OCRA = 0xFF;

	// configure timer counter 0
	SetRegister(Tcnt0.TCCRA, (TCCRA_WGM, Normal));
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CsT1_Div1024));

	// enable interrupts
	SetRegister(Timsk0, (TIMSK_TOIE, True));   // enable overflow interrutp for tcnt1
}


int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("hello world 1");

	ConfigurePins();
	ConfigureTimerCounter();
	ConfigureAdc();
	while (True);
	
	return 0;
}


ISR_Tcnt0Overflow()
{
	static Bool overflowPin = False;
	
	static int updateRateCounter = 0;
	updateRateCounter++;
	if (updateRateCounter == 16)
	{
		nrOfSamples = 0;
		overflowPin = !overflowPin;
		UpdateRegister(PortD.PORT, (PIN_7, overflowPin));

		timingCurrentIndex = (timingCurrentIndex + 1) % countof(timing);
		updateRateCounter = 0;

		Tcnt2.OCRA =  (timing[timingCurrentIndex] * 11 );

	}
	if (updateRateCounter > 0)
	{
		//if ((updateRateCounter % 2) == 0)
		{

			//trigger a measurement!
			UpdateRegister(PortC.DDR, (PIN_0, DdrInput));


			// this gives enough time for the receiver to settle 
			// according the current light intensity! that's something to play with!

			_delay_us(25);

			// start the measurement!
			UpdateRegister(Adc.Adcsra, (ADCSRA_ADSC, True));
		}
	}

}



ISR_AdcComplete()
{
	//static uint16_t data[4];

	UpdateRegister(PortD.PORT, (PIN_6, True));
	// put the sample into a queue such that it can be processed in 
	// non interrupt context!
	uint16_t buf = Adc.Data;

	//data[nrOfSamples++] = buf;
	TRACE("@plot %16", buf);
	
	
	UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
	
	UpdateRegister(PortD.PORT, (PIN_6, False));
}

