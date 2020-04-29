#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>
	
#include <avr/interrupt.h>


uint16_t timing[] = { 1,2,4,16,32, 64, 128, 256, 512, 768, 896, 960, 1023, 512, 256, 256, 128, 64, 32, 16, 8, 4, 2, 1 };


#define PULS_WIDTH_1MS 64

#define SERVO_MIN  50
#define SERVO_MAX 205


#define MYUBRR F_CPU/16/BAUD-1

void LedConfigurePin(void)
{

	SetRegister(PortB.DDR, (PIN_3, DdrOutput), (PIN_4, DdrOutput), (PIN_5, DdrInput));
	//SetRegister(Mcucr, (MCUCR_PUD, True));
	SetRegister(PortB.PORT, (PIN_4, 0));
	SetRegister(PortB.PORT, (PIN_3, 1));
}

void LedControl(void)
{
	TRACE("LED control");
	SetRegister( PortB.DDR, (PIN_3,1) , (PIN_5,0));
	SetRegister(PortB.PORT, (PIN_5, 0));

	Tcnt2.OCRA = 127; // corresponds to roughly 1ms when using diveder 1024
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, 1), (TCCRA_COMA, 2));
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, 2), (TCCRB_WGM, 0)); //prescaler = 1024
	
	while (1)
	{
		/*uint8_t i = 0;
		while (i++ < 255)
		{
			Tcnt2.OCRA++;
			_delay_ms(10);
		}
		i = 0;
		while (i++ < 255)
		{
			Tcnt2.OCRA--;
			_delay_ms(10);
		}*/
	}
}

void LedSignal(void)
{
	TRACE("LED control");
	PRR &= ~1;									// make sure that adc is not gated!
	SetRegister(PortC.PORT, (PIN_0, 0),(PIN_1,0));
	SetRegister(PortC.DDR, (PIN_0, 1), (PIN_1,1));	// force pin 0 to 0!

	SetRegister(PortB.PORT, (PIN_3, 1), (PIN_4, 0)); // switch led on!
	SetRegister(PortB.DDR, (PIN_3, 1), (PIN_4, 1));
	

	Tcnt2.OCRA = 0; // corresponds to roughly 1ms when using diveder 1024
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, FastPwm), (TCCRA_COMA,2));
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, 1)); //prescaler = 1024
	SetRegister(Timsk2, (TIMSK_TOIE, 1));


	SetRegister(PortD.PORT, (PIN_7, 1));
	sei();

	while (1)
	{
		/*uint8_t i = 0;
		while (i++ < 255)
		{
			Tcnt2.OCRA++;
			_delay_ms(10);
		}
		i = 0;
		while (i++ < 255)
		{
			Tcnt2.OCRA--;
			_delay_ms(10);
		}*/
	}
}


void AdcReadout(void)
{
	SetRegister(Tcnt1.TCCRA, (TCCRA1_WGM, Normal));
	SetRegister(Tcnt1.TCCRB, (TCCRB1_CS, CsT1_Div64 )); //prescaler = 1024

	//TCCR1A = 0;
	//TCCR1B = 5;
		
	SetRegister(Timsk1, (TIMSK_TOIE, 1));
	SetRegister(PortB.PORT, (PIN_5, 1));
	//TRACE("ADC readout");
	PortB.DDR = 0xFF;
	SetRegister(PortD.DDR, (PIN_7, DdrOutput));
	PRR &= ~1;
	SetRegister(Adc.Admux, (ADMUX_MUX, 5), (ADMUX_REFS, 1));
	SetRegister(Adc.Adcsra, (ADCSRA_ADEN, 1), (ADCSRA_ADPS, 6), (ADCSRA_ADIE,1), (ADCSRA_ADATE,1));
	SetRegister(Adc.Adcsrb, (ADCSRB_ADTS, Tc1Overflow));
	SetRegister(PortD.PORT, (PIN_7, 1));
	sei();
	while (1);
}


int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("hello world 1");

	//LedConfigurePin();
	//Bool status = True;
	while (True)
	{
		TRACE("hello world 1");
		/*UpdateRegister(PortB.PORT, (PIN_3, status));
		status = !status;
		_delay_ms(1000);*/
	}
	return 0;
}


ISR(ADC_vect)
{
	static uint16_t nrOfMeasurements = 0;
	static uint16_t min = 0xFFFF;
	static uint16_t max = 0x0;
	static uint16_t range = 0;

	static uint16_t data = 0;
	static uint8_t rxByte = 0;
	data = Adc.Data;

	//TRACE("%16, %16", nrOfMeasurements, );

	

	if (nrOfMeasurements < 32)
	{
		if (min > data)
		{
			min = data;
		}
		if (max < data)
		{
			max = data;
		}
		
	}
	else
	{
		if (nrOfMeasurements == 32)
		{
			range = max - min;
			TRACE("range = %16", range);
		}

		//uint8_t rx = (data*4)/range;
		//uint8_t pos = (nrOfMeasurements % 4)<<1;
		uint8_t rx = (data*16)/range;
		uint8_t pos = (nrOfMeasurements % 2)<<2;

		
		rxByte = rxByte | (rx << pos);
		//TRACE("rx = %8, %8 => rxbyte = %8", rx, pos, rxByte);
		//if (pos == 6)
		if( pos == 4)
		{
			TRACE("received %8", rxByte);
			rxByte = 0;
		}

	}
	nrOfMeasurements++;
	Tifr1 |= TIFR_TOV_mask;
}


ISR(TIMER1_OVF_vect)
{}


ISR(TIMER2_OVF_vect)
{
	static uint8_t bitrate = 0;
	static uint8_t nibbleValue = 0;
	static uint8_t pos = 0;
	static uint8_t bit = 0;
	static uint8_t txt[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
	static Bool triggerMeasurement = False;
	bitrate++;
	
	if (bitrate == 17 )
	{
		if (triggerMeasurement)
		{
			SetRegister(Adc.Admux, (ADMUX_MUX, 0), (ADMUX_REFS, 3)); // chose 
			SetRegister(Adc.Adcsra, (ADCSRA_ADSC, 1), (ADCSRA_ADEN, 1), (ADCSRA_ADPS, 6), (ADCSRA_ADATE, 0), (ADCSRA_ADIE, 1));
			triggerMeasurement = False;
		}
	}
	//if (bitrate == 20)
	//{
	//	UpdateRegister(PortC.DDR, (PIN_0, 1)); // force the pin to a defined value
	//}
	else if (bitrate ==21 )
	{
		UpdateRegister(PortC.DDR, (PIN_0, 1)); // force the pin to a defined value
		/*if (!repeat)
		{*/
			//nibbleValue = ((txt[pos] >> bit) & 0x3);
			nibbleValue = ((txt[pos] >> bit) & 0x0F);
			//bit += 2;
			bit += 4;
			if (bit == 8)
			{
				bit = 0;
				pos++;
				pos %= 8;

			}
		//}
		
		//TRACE("data value = %8", nibbleValue);
		
		//repeat = !repeat;
		Tcnt2.OCRA = nibbleValue * 50;
		Tcnt2.OCRA = nibbleValue*15;
		

		//UpdateRegister(PortB.PORT, (PIN_3, val));
		UpdateRegister(PortC.DDR, (PIN_0, 0)); // release the pin again
	
	}
	
	else if (bitrate == 26)
	{
		triggerMeasurement = True;
		bitrate = 0;
	}
}
