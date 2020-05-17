#include "Atmega328P.h"
#include "AvrLib.h"
#include "inttypes.h"
#include <RegisterAccess.h>


uint8_t timing[] = { 1, 2, 4, 8, 16, 32, 64, 128, 255, 245, 235, 225, 215, 190, 160, 80, 40, 20, 10, 5, 2, 1, 1 };
uint8_t timingCurrentIndex = 0;

static Bool LedOn = False;


void CompareMatchImpl(void)
{
	LedOn = !LedOn;
	UpdateRegister(PortB.PORT, (PIN_3, LedOn));
	Tcnt0.OCRA = 255 - Tcnt0.OCRA;
}

void OverflowImpl(void)
{
	static uint8_t advance = 0;

	advance++;
	advance %= 16;
	if (advance == 0)
	{
		timingCurrentIndex++;
		if (timingCurrentIndex == countof(timing))
		{
			timingCurrentIndex = 0;
		}
		Tcnt0.OCRA = timing[timingCurrentIndex];
		Tcnt0.TCNT = 0;
		LedOn = True;
		UpdateRegister(PortB.PORT, (PIN_3, LedOn));
	}
}