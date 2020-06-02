#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>

//
// implementation of simple queue to buffer the samples of the adc
// before transmitting them to PC
//
uint16_t buffer[8];		// buffer memory
uint8_t in = 0;			// input index
uint8_t out = 0;		// output index

/**
* @brief Put a sample into the queue
*/
void Enqueue(uint16_t data)
{
	uint8_t nextin = 0;
	nextin = (in + 1) % countof(buffer);
	if (nextin == out) // we cannot wait in case of an overflow but just discard the sample!
	{
		TRACE("overflow occurred");
		return;
	}
	buffer[in] = data;
	in = nextin;
}

Bool Dequeue(uint16_t* data)
{
	if (out == in)
	{
		return False;
	}
	*data = buffer[out++];
	out %= countof(buffer);
	return True;
}


void ConfigurePins(void)
{
	//debug pin
	UpdateRegister(PortD.DDR, (PIN_2, DdrOutput), (PIN_3, DdrOutput));
	UpdateRegister(PortD.PORT, (PIN_2, False), (PIN_3, False));

	// configure pins for sender LED
	SetRegister(PortB.DDR, (PIN_0, DdrOutput), (PIN_1, DdrOutput) );
	SetRegister(PortB.PORT, (PIN_1, 0), (PIN_0,1));

	// configure pins for receiver LED
	SetRegister( PortC.DDR, (PIN_0, DdrInput), (PIN_1, DdrOutput));
	//
	// the anode of the receive LED has to go on PIN_0; the kathode
	// can go on the 5V Header; 
	// If we want to use a LED as receiver and transmitter
	// we have to be able to switch the setting for the kathode from 0 to 1
	//
	SetRegister( PortC.PORT, (PIN_1, True)); // reverse bias; this is the pin for the kathode;
}


void ConfigureAdc(void)
{
	SetRegister(Prr, (PRR_PRADC, 0));		 // disable power reduction for adc
	SetRegister(Adc.Admux, (ADMUX_MUX, MuxAdc0 ), (ADMUX_REFS, Internal1_1));
	SetRegister(Adc.Adcsra, (ADCSRA_ADEN, 1), // enable ADC
		(ADCSRA_ADPS, AdcDiv64 ),			  // prescaler selection; a conversion takes 13 clocks a 4us
		(ADCSRA_ADIE, True),			      // do not enable adc complte interrupt
		(ADCSRA_ADATE, 0));					  // enable adc trigger source
	SetRegister(Adc.Adcsrb, (ADCSRB_ADTS, FreeRunning));
	SetRegister(Adc.Didr0, (DIDR0_ADC0D, 1));
	
}



void ConfigureTimerCounter(void)
{
	// configure timer counter 1
	SetRegister(Tcnt0.TCCRA, (TCCRA_WGM, ClrTmrOnCmpMatch));
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CsT1_Div1024)); // one tick = 64 us
	Tcnt0.OCRA = 200;
	SetRegister(Timsk0, (TIMSK_OCIEA, True)); // enable timer 1 compare match interrupt

	// configure timer counter 2
	// higher sampling rate leads to errors in the serial communication!
	// but is in principle possible!
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, ClrTmrOnCmpMatch));
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, CsT2_Div32)); // one tick = 2 us
	Tcnt2.OCRA = 200;
	SetRegister(Timsk2, (TIMSK_OCIEA, True));
}

int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("hello world");

	ConfigurePins();
	ConfigureTimerCounter();
	ConfigureAdc();
	TRACE("init done");
	
	while (True)
	{
		static uint16_t data = 0;
		if (Dequeue(&data))
		{
			TRACE("@plot %16", data);
		}
	}
	
	return 0;
}


/**
* @brief used to send a pattern!
*/
ISR_Tcnt0CompareMatchA()
{
	
	static Bool ledOn = False;

	ledOn = !ledOn;
	UpdateRegister(PortB.PORT, (PIN_0, ledOn));		
	
}


/**
* @brief readout the converted analog value from the ADC
*/

ISR_AdcComplete()
{
	
	// debug pin to measure the time between starting conversion
	// and conversion completion
	UpdateRegister(PortD.PORT, (PIN_2, False) );
	
	// put the sample ino a queue such that it can be processed in 
	// non interrupt context!
	Enqueue(Adc.Data);

	// discharge the any remaining charge in the LED
	// a weak pull down would be an alternative solution! 
	// But requires addditional wires
	UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
	
	// just to see the difference between 
	// doing this here or in the timer interrupt!
	UpdateRegister(PortC.DDR, (PIN_0, DdrInput)); // comment this line after the torch exeperiment!
	
}

/**
* @brief Trigger the  next ADC measurement
*
* We do not use the auto trigger configuration of the ADC because
* we need to discharge the LED in a controlled way!
*
*/
ISR_Tcnt2CompareMatchA()
{
	// just for debugging; measure time of ISR
	UpdateRegister(PortD.PORT, (PIN_3, True));
	
	
	// uncomment the two lines below after the torch experiment
	UpdateRegister(PortC.DDR, (PIN_0, DdrInput));

	
	// this gives enough time for the receiver to settle 
	// according the current light intensity! that's something to play with!
	
	_delay_us(20);

	// start the measurement!
	UpdateRegister(Adc.Adcsra, (ADCSRA_ADSC, True));

	// PIN 2 used to measure time for ADC measurement!
	UpdateRegister(PortD.PORT, (PIN_2, True), (PIN_3, False));
	
}


