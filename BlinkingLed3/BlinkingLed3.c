#include <avr/io.h>
#include <Atmega328P.h>
#include <avrlib.h>
#include <RegisterAccess.h>
#define F_CPU 16000000
#include <util/delay.h>

//#define NR_OF_CLASSES 16
//#define MAX_INTENSITY 75
//#define NR_OF_BITS  (4)

#define NR_OF_CLASSES 4
#define MAX_INTENSITY 75
#define NR_OF_BITS  (2)

#define BIT_MASK	(NR_OF_CLASSES-1)

#define SIGNAL_STEP		(MAX_INTENSITY/(NR_OF_CLASSES-1))
#define CurrentIntensity 108
static uint16_t minMeasurement = 0;
static uint16_t maxMeasurement = 0x3FF;
static uint16_t div = 0;


//static uint8_t timing[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1 };
//static uint8_t timing[] = { 0,1,2,3,2,1};
//static uint8_t timing[] = { 0,1 };
static uint16_t threasholds[NR_OF_CLASSES];


typedef struct _ReceiverFsm_tag
{
	Fsm Base;
}ReceiverFsm_T;

void Swap(uint8_t* left, uint8_t* right);
void Sort(uint8_t* buf, int8_t leftMost, int8_t rightMost);
void DoSample(const Message* msg);
uint8_t LookupSampleClass(uint16_t sampleValue);

void TriggerMeasurement(void);


static ReceiverFsm_T _receiver = {
	.Base = {.RxMask = 0xFF, .Next = 0, .CurrentState = DoSample }
};


uint8_t LookupSampleClass(uint16_t sample)
{
	uint8_t i = 1;
	while ((i < countof(threasholds)) &&
		sample > threasholds[i]) {
		i++;
	}
	return i-1;
}


void DoSample(const Message* msg)
{
	static uint8_t byte = 0;
	static uint8_t decodeIndex = 0; // 8/NR_OF_CLASSES = #bits in the message
	
	uint8_t buffer[1];
	uint8_t halfDiv = div - (div >> 2);
	decodeIndex = msg->MsgParamLow;

	//uint8_t halfDiv = 0;
	UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
	UpdateRegister(PortD.PORT, (PIN_6, True));
	
	uint8_t i = 0;
	for (; i < countof(buffer); i++)
	{
		TriggerMeasurement();
		while (Adc.Adcsra & ADCSRA_ADSC_mask);
		UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
		
		uint16_t val = Adc.Data + halfDiv;

		//TRACE("measurement %8= %16", index, val);
		
		buffer[i] = val / div;

		//buffer[i] = LookupSampleClass(val);

		// sort the entries
		//*****************
		/*uint8_t j = i;
		while (j >= 1)
		{
			if (buffer[j] > buffer[j - 1])
			{
				Swap(&buffer[j], &buffer[j - 1]);
			}
			j--;
		}*/
	}

	uint8_t v1 = buffer[countof(buffer)/2]&(BIT_MASK);
	byte <<= NR_OF_BITS;
	byte |= v1;
	
	decodeIndex++;
	if (decodeIndex == (NR_OF_CLASSES-1))
	{
		TRACE("@plot %8", byte);
		decodeIndex = 0;
		byte = 0;
	}
	

	UpdateRegister(PortD.PORT, (PIN_6, False));

}

void Swap(uint8_t* left, uint8_t* right)
{
	uint8_t tmp = *left;
	*left = *right;
	*right = tmp;
}

void Sort(uint8_t* buf, int8_t leftMost, int8_t rightMost)
{
	//TRACE("sorting from %8 to %8", leftMost, rightMost);
	if (leftMost < rightMost)
	{
		uint8_t med = buf[(rightMost + leftMost) / 2];
		//TRACE("current med = %8", med);
		int8_t i = leftMost;
		int8_t j = rightMost;
		do
		{
			while (buf[i] < med) i++;
			while (buf[j] > med) j--;
			if (i <= j)
			{
				if (i != j)
				{
					uint8_t tmp = buf[i]; buf[i] = buf[j]; buf[j] = tmp;
				}
				i++; j--;
			}
			
		} while (i < j);
		if( j > leftMost) Sort(buf, leftMost, j);
		if( i < rightMost ) Sort(buf, i, rightMost);
	}
}


void ConfigureAdc(void)
{
	SetRegister(Prr, (PRR_PRADC, 0));		 // disable power reduction for adc
	SetRegister(Adc.Admux, (ADMUX_MUX, MuxAdc0), (ADMUX_REFS, Internal1_1));
	SetRegister(Adc.Adcsra, (ADCSRA_ADEN, 1), // enable ADC
		(ADCSRA_ADPS, AdcDiv64),			  // prescaler selection; a conversion takes 13 clocks a 4us
		(ADCSRA_ADIE, False),			      // do not enable adc complte interrupt
		(ADCSRA_ADATE, 0));					  // enable adc trigger source
	SetRegister(Adc.Adcsrb, (ADCSRB_ADTS, FreeRunning));
	SetRegister(Adc.Didr0, (DIDR0_ADC0D, 1));
}

void ConfigurePins(void)
{
	SetRegister(PortB.DDR, (PIN_3, DdrOutput), (PIN_4, DdrOutput));
	SetRegister(PortB.PORT, (PIN_4, 0), (PIN_3, 0));

	SetRegister(PortD.DDR, (PIN_7, DdrOutput), (PIN_6, DdrOutput)); //two debug pins
	SetRegister(PortD.PORT, (PIN_7, 0), (PIN_6, 0));

	SetRegister(PortC.DDR, (PIN_0, DdrOutput));
	SetRegister(PortC.PORT, (PIN_0, 0));
}


void ConfigureTimerCounter(void)
{
	// configure timer counter 2
	SetRegister(Tcnt2.TCCRA, (TCCRA_WGM, FastPwm), (TCCRA_COMA, 2)); // compare match
	SetRegister(Tcnt2.TCCRB, (TCCRB_CS, CS_Div1)); // counting speed!
	Tcnt2.OCRA = 0;

	// configure timer counter 0
	SetRegister(Tcnt0.TCCRA, (TCCRA_WGM, ClrTmrOnCmpMatch));

	//
	// initially this needs to be switched off
	// otherwise it will disturb the calibration!
	//
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CS_Off));

	// enable interrupts
	//SetRegister(Timsk0, (TIMSK_TOIE, True));   // enable overflow interrutp for tcnt1
	SetRegister(Timsk0, (TIMSK_OCIEA, True));
	Tcnt0.OCRA = 0x7F; // 2 bit config
	//Tcnt0.OCRA = 0xBF; // 4 bit config
}

void TriggerMeasurement(void)
{
	UpdateRegister(PortC.DDR, (PIN_0, DdrInput));

	//  this gives enough time for the receiver to settle 
	//  according the current light intensity! that's something to play with!

	_delay_us(25);

	UpdateRegister(Adc.Adcsra, (ADCSRA_ADSC, True));
}

//
// Compute the signal range; this is important in 
// order to know the step size to reconstruct the signal
//
void Calibrate(void)
{
	uint8_t i = 0;

	TRACE("Calibrate");
	Tcnt2.OCRA = 0; // min value

	// just discard the first values, since the first
	// values of the ADC are not reliable!
	
	for (; i < 2; i++)
	{
		TriggerMeasurement();
		while (Adc.Adcsra & ADCSRA_ADSC_mask);	
	}
	UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
	minMeasurement = 0;
	//
	// less might be sufficient!
	for (i=0; i < 4; i++)
	{
		TriggerMeasurement();
		while (Adc.Adcsra & ADCSRA_ADSC_mask);
		int16_t data = Adc.Data;
		minMeasurement += data;
		TRACE("measurement when led is off %16", data);
		UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
	}
	minMeasurement =0;

	maxMeasurement = 0;
	
	uint8_t j = 1;
	for (; j < NR_OF_CLASSES; j ++)
	{
		Tcnt2.OCRA = j * SIGNAL_STEP; // max value
		uint16_t measurement = 0;
		uint8_t diff = 0;
		for (i = 0; i < 16; i++)
		{
			TriggerMeasurement();
			while (Adc.Adcsra & ADCSRA_ADSC_mask);
			UpdateRegister(PortC.DDR, (PIN_0, DdrOutput));
			measurement += Adc.Data;
		}
		measurement /= i;
		threasholds[j - 1] = measurement;
		diff = measurement - maxMeasurement;
		maxMeasurement = measurement;
		TRACE("signal step %8=%16", j, diff);
	}
	//
	// it is important that we can use the full measurement
	// range of the ADC!
	// one could try to use the 5V reference voltage 
	//
	
	TRACE("min value = %16", minMeasurement);
	TRACE("max value = %16", maxMeasurement);
	div = (maxMeasurement - minMeasurement) / (NR_OF_CLASSES-1);

	int16_t v1 = maxMeasurement - (div + 1) * (NR_OF_CLASSES - 1);
	v1 = v1 > 0 ? v1 : -v1;
	int16_t v2 = maxMeasurement - div * (NR_OF_CLASSES - 1);
	v2 = v2 > 0 ? v2 : -v2;
	if (v1 < v2)
	{
		div++;
	}
	for (i = 0; i < countof(threasholds); i++)
	{
		threasholds[i] = (i)*div;
	}

	TRACE("step size = %16", div);
	//
	// now it's time to start the interrupt machinery
	// UpdateRegister(Adc.Adcsra, (ADCSRA_ADIE, True));
	//SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CsT2_Div32));
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CS_Div8));
}

int main(void)
{
	Usart_Init(250000); // higher is to fast; cannot be consumed reliably anymore!
	
	TRACE("** main()");
	
	RegisterFsm(&_receiver.Base);

	ConfigurePins();

	ConfigureAdc();

	ConfigureTimerCounter();

	Calibrate();

	TRACE("Calibrate done");
	
	//
	// if we want to try out, we can limit this
	// and just run it a couple of times
	//
	//while ( True );
	InitializeStateEventFramework();
	SetRegister(Tcnt0.TCCRB, (TCCRB_CS, CS_Off));
	SetRegister(Timsk0, (TIMSK_TOIE, False));
	return 0;
}


//ISR_Tcnt0Overflow()
ISR_Tcnt0CompareMatchA()
{
	static Bool overflowPin = False;
	
	static uint8_t txByte = 0;
	static uint8_t currentBitIndex = 0;
	uint8_t shift = (8 - NR_OF_BITS) - currentBitIndex * NR_OF_BITS;
	static int updateRateCounter = 0;
	updateRateCounter++;
	if (updateRateCounter == 5)
	{
		overflowPin = !overflowPin;
		
		UpdateRegister(PortD.PORT, (PIN_7, overflowPin));

		updateRateCounter = 0;
		
		uint8_t val = (txByte >> shift) & BIT_MASK;
		currentBitIndex++;
		if (currentBitIndex == NR_OF_CLASSES)
		{
			currentBitIndex = 0;
			txByte++;
			//TRACE("next to transmit = %8, val = %8", txByte, val);
			
		}
		Tcnt2.OCRA = (val * SIGNAL_STEP);
	}
	else if (updateRateCounter == 1)
	{
		SendMessage(1, 0x10, currentBitIndex, 0);
		
	}
}


