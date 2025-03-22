/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
// Credits - SDR cube!!!

// Common
#include "mchf_board.h"

#include <math.h>

#include "dds_table.h"
#include "softdds.h"

// Software DDS public
struct SoftDds	sdd;

// Step in the public structure - don't work for some crazy reason
ulong 	step;

//*----------------------------------------------------------------------------
//* Function Name       : softdds_setfreq
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void softdds_setfreq(float freq,ulong samp_rate,uchar smooth)
{
	float f = (float)(samp_rate);

	//printf("freq %d\n\r",(int)freq);

	// Reset accumulator, if need smooth tone
	// transition, do not reset it (e.g. wspr)
	if(!smooth)
		sdd.acc = 0;

	// Calculate new step
	f   	 /= 65536.0;
	step   = (ulong)(freq / f);

	//printf("step %d\n\r",step);
}

//*----------------------------------------------------------------------------
//* Function Name       : softdds_runf
//* Object              : use two float buffer
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void softdds_runf(float *i_buff,float *q_buff,ushort size)
{
	ulong 	i,k;

	for(i = 0; i < size; i++)
	{
		// Calculate next sample
		sdd.acc += step;
		k    = sdd.acc >> DDS_ACC_SHIFT;

		// Fix ptr overload
		k &= (DDS_TBL_SIZE - 1);

		// Load I value
		*i_buff = (float)(DDS_TABLE[k]);

		// 90 degrees shift
		k += (DDS_TBL_SIZE/4);

		// Fix ptr overload
		k &= (DDS_TBL_SIZE - 1);

		// Load Q value
		*q_buff = (float)(DDS_TABLE[k]);

		// Next ptr
		i_buff++;
		q_buff++;
	}
}
