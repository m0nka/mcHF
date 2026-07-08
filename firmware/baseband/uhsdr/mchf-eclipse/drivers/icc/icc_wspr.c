/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_wspr.c                                                     **
**  Description:	WSPR rx audio capture streaming to the M7 core. Taps the       **
**					line level rx audio (post demod, fixed scaling, independent    **
**					of the volume control), decimates 48 kHz stereo to 12 kHz      **
**					mono and buffers it in chunks, which the M7 core polls out     **
**					with the ICC_WSPR_READ command and saves to the SD card        **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "icc_wspr.h"

// Wire format constants, duplicated from common/mchf_icc_def.h to keep
// this translation unit free of the wire protocol header (same approach
// as icc_spectrum.c)
#define WSPR_SIG					0x9E
#define WSPR_FLAG_ACTIVE			0x01
#define WSPR_FLAG_OVERRUN			0x02
#define WSPR_HDR_SIZE				4
#define WSPR_CHUNK_SAMPLES			512

// Chunk ring - 16 KB buys the M7 poll loop ~680 ms of slack
#define WSPR_RING_CHUNKS			16

// 48 kHz to 12 kHz boxcar decimation. The rx audio chain has already
// bandlimited the signal to the demod filter width, the WSPR energy
// sits at 1400..1600 Hz, so a plain average of four is good enough
#define WSPR_DECIM_FACTOR			4

// Safety stop if the M7 core never sends ICC_WSPR_STOP (120 s @ 12 kHz)
#define WSPR_MAX_SAMPLES			(120UL * 12000UL)

typedef struct
{
	int16_t				data[WSPR_RING_CHUNKS][WSPR_CHUNK_SAMPLES];

	// Chunk indices - producer is the audio DMA irq, consumer is the
	// superloop, single reader/single writer so no locking needed.
	// Only chunks behind wr_chunk are complete and readable
	volatile uint32_t	wr_chunk;
	volatile uint32_t	rd_chunk;

	uint32_t			wr_pos;			// sample position inside wr chunk
	volatile uint8_t	active;
	volatile uint8_t	overrun;

	// Decimator state
	int32_t				acc;
	uint32_t			phase;
	uint32_t			total;			// captured samples, for the safety stop

} icc_wspr_state_t;

static icc_wspr_state_t		iw;

//*----------------------------------------------------------------------------
//* Function Name       : icc_wspr_start
//* Object              : begin a new capture, idempotent - the M7 side
//*						: retries the start request until acknowledged
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_wspr_start(void)
{
	if(iw.active)
		return;

	memset(&iw, 0, sizeof(iw));
	iw.active = 1;

	printf("wspr capture start\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_wspr_stop
//* Object              : end the capture, buffered chunks stay readable
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_wspr_stop(void)
{
	if(!iw.active)
		return;

	iw.active = 0;

	printf("wspr capture stop, %u samples\r\n", (unsigned int)iw.total);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_wspr_collect
//* Object              : decimate one rx audio block into the chunk ring
//* Context    			: CONTEXT_IRQ (audio DMA block handler)
//*----------------------------------------------------------------------------
void icc_wspr_collect(volatile int16_t *src, uint32_t num_frames, uint32_t tx_mode)
{
	uint32_t	i;
	int16_t		sample;

	if(!iw.active)
		return;

	for(i = 0; i < num_frames; i++)
	{
		// Left channel carries the line level rx audio, silence on tx
		// keeps the two minute time base of the capture intact
		iw.acc += tx_mode ? 0 : (int32_t)(*src);
		src    += 2;

		if(++iw.phase < WSPR_DECIM_FACTOR)
			continue;

		sample   = (int16_t)(iw.acc / WSPR_DECIM_FACTOR);
		iw.acc   = 0;
		iw.phase = 0;

		// Ring full - drop the sample, flag it to the M7 core
		if(((iw.wr_chunk + 1) % WSPR_RING_CHUNKS) == iw.rd_chunk)
		{
			iw.overrun = 1;
			continue;
		}

		iw.data[iw.wr_chunk][iw.wr_pos] = sample;

		if(++iw.wr_pos >= WSPR_CHUNK_SAMPLES)
		{
			iw.wr_pos   = 0;
			iw.wr_chunk = (iw.wr_chunk + 1) % WSPR_RING_CHUNKS;
		}

		// Safety stop - never stream forever on a lost stop command
		if(++iw.total >= WSPR_MAX_SAMPLES)
		{
			iw.active = 0;
			return;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_wspr_get_buffer
//* Object              : fill the ICC_WSPR_READ response, one chunk per call
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint16_t icc_wspr_get_buffer(uint8_t *buffer)
{
	uint16_t	len = 0;
	uint8_t		flags = 0;

	if(buffer == NULL)
		return 0;

	if(iw.active)
		flags |= WSPR_FLAG_ACTIVE;

	if(iw.overrun)
		flags |= WSPR_FLAG_OVERRUN;

	// Complete chunk available ?
	if(iw.rd_chunk != iw.wr_chunk)
	{
		len = WSPR_CHUNK_SAMPLES * 2;
		memcpy(buffer + WSPR_HDR_SIZE, iw.data[iw.rd_chunk], len);
		iw.rd_chunk = (iw.rd_chunk + 1) % WSPR_RING_CHUNKS;
	}

	buffer[0] = WSPR_SIG;
	buffer[1] = flags;
	buffer[2] = (uint8_t)(len >> 0);
	buffer[3] = (uint8_t)(len >> 8);

	return (WSPR_HDR_SIZE + len);
}

#endif // H7_M4_CORE
