/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

static const float pwr10[] = {1e36,  1e33,  1e30,  1e27,  1e24,  1e21,  1e18,
                              1e15,  1e12,  1e9,   1e6,   1e3,   1e0,   1e-3,
                              1e-6,  1e-9,  1e-12, 1e-15, 1e-18, 1e-21, 1e-24,
                              1e-27, 1e-30, 1e-33, 1e-36};

//
// https://stackoverflow.com/questions/61841890/printf-for-float-in-c
//
void ftoa(float f, char *buf, size_t bufsiz)
{
	char sign[2] = {0};
	uint32_t p;

	if (f < 0)
	{
		f = -f;
		sign[0] = '-';
	}

	if (f == 0)
		p = 12;
	else
	{
		for (p = 0; p < sizeof(pwr10) / sizeof(pwr10[0]) - 1; p++)
		{
			if (f >= pwr10[p])
				break;
		}
	}

	uint32_t 	exponent = 36 - 3 * p;
	float 		mantissa = f / pwr10[p];

	mantissa += 0.00005; // 4 digit precision

	uint32_t digits = mantissa;
	uint32_t fraction = (mantissa - digits) * 100.0;	// 10000.0

	if(exponent)
		snprintf(buf, bufsiz, "%s%d.%04de%d", sign, (int)digits, (int)fraction, (int)exponent);
	else
		snprintf(buf, bufsiz, "%s%d.%d", sign, (int)digits, (int)fraction);
}
