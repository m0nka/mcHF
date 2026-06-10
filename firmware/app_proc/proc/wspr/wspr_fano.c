/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_fano.c                                                    **
**  Description:	Fano sequential decoder for the WSPR convolutional code        **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Sequential (Fano) decoding of the rate 1/2, constraint length 32
// Layland-Lushbaugh convolutional code used by WSPR
//
// Algorithm after Phil Karn KA9Q (fano.c, GPL), restructured for static
// allocation (no malloc) so it can run inside a FreeRTOS task
//
#include "wspr_fano.h"

// One node per decoded info bit, plus root
struct fano_node
{
	uint32_t	encstate;					// encoder state, hypothesis bit in LSB
	int32_t		gamma;						// cumulative path metric to this node
	int			metrics[4];					// branch metric per 2 bit codeword
	int			tm[2];						// sorted branch metrics (best first)
	int			i;							// branch currently being explored
};

// 81 info bits + root, ~3KB - static, decoder runs in one task only
static struct fano_node fano_nodes[WSPR_FANO_MAX_NBITS + 1];

#ifdef _MSC_VER
// Portable parity for the host test build
static __inline int wspr_parity(uint32_t x)
{
	x ^= x >> 16;
	x ^= x >> 8;
	x ^= x >> 4;
	x ^= x >> 2;
	x ^= x >> 1;
	return (int)(x & 1u);
}
#else
#define wspr_parity(x)	__builtin_parity(x)
#endif

//*----------------------------------------------------------------------------
//* Function Name       : fano_encode_sym
//* Object              : codeword for encoder state (hypothesis bit in LSB)
//* Notes    			: POLY1 output in bit 1, POLY2 output in bit 0
//* Context    			: any
//*----------------------------------------------------------------------------
static inline int fano_encode_sym(uint32_t state)
{
	return (wspr_parity(state & WSPR_POLY1) << 1) | wspr_parity(state & WSPR_POLY2);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_fano
//* Object              : sequential decode, see header for parameters
//* Notes    			: last 31 bits are assumed to be the all zero tail
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
int wspr_fano(const uint8_t *symbols, uint8_t *data, int nbits,
              int mettab[2][256], int delta, uint32_t maxcycles)
{
	struct fano_node	*np;
	struct fano_node	*lastnode;
	struct fano_node	*tail;
	int					t, m0, m1, lsym;
	int32_t				ngamma;
	uint32_t			i;

	if((nbits < 32) || (nbits > WSPR_FANO_MAX_NBITS))
		return -2;

	lastnode = &fano_nodes[nbits - 1];
	tail     = &fano_nodes[nbits - 31];

	// Precompute branch metrics for every node from its symbol pair
	// Codeword index: (first coded bit << 1) | second coded bit
	for(np = fano_nodes; np <= lastnode; np++)
	{
		np->metrics[0] = mettab[0][symbols[0]] + mettab[0][symbols[1]];
		np->metrics[1] = mettab[0][symbols[0]] + mettab[1][symbols[1]];
		np->metrics[2] = mettab[1][symbols[0]] + mettab[0][symbols[1]];
		np->metrics[3] = mettab[1][symbols[0]] + mettab[1][symbols[1]];
		symbols += 2;
	}

	// Root node - try the 0 branch, the 1 branch codeword is the
	// complement (both polynomials have their LSB set)
	np = fano_nodes;
	np->encstate = 0;

	lsym = fano_encode_sym(np->encstate);
	m0 = np->metrics[lsym];
	m1 = np->metrics[3 ^ lsym];

	if(m0 > m1)
	{
		np->tm[0] = m0;
		np->tm[1] = m1;
	}
	else
	{
		np->tm[0] = m1;
		np->tm[1] = m0;
		np->encstate++;
	}
	np->i = 0;
	np->gamma = 0;
	t = 0;

	maxcycles *= (uint32_t)nbits;

	// Fano search loop
	for(i = 1; i <= maxcycles; i++)
	{
		// Look forward
		ngamma = np->gamma + np->tm[np->i];
		if(ngamma >= t)
		{
			// First visit of this node - tighten threshold
			if(np->gamma < t + delta)
			{
				while(ngamma >= t + delta)
					t += delta;
			}

			// Move forward
			np[1].gamma    = ngamma;
			np[1].encstate = np->encstate << 1;
			if(++np == (lastnode + 1))
				break;								// decoded all bits

			// Compute and sort branch metrics of the new node
			lsym = fano_encode_sym(np->encstate);
			if(np >= tail)
			{
				// Tail is known to be zero, only the 0 branch exists
				np->tm[0] = np->metrics[lsym];
			}
			else
			{
				m0 = np->metrics[lsym];
				m1 = np->metrics[3 ^ lsym];
				if(m0 > m1)
				{
					np->tm[0] = m0;
					np->tm[1] = m1;
				}
				else
				{
					np->tm[0] = m1;
					np->tm[1] = m0;
					np->encstate++;
				}
			}
			np->i = 0;
			continue;
		}

		// Threshold violated, can not go forward
		for(;;)
		{
			// Look backward
			if((np == fano_nodes) || (np[-1].gamma < t))
			{
				// Can not back up either - relax threshold and
				// revisit the best branch from here
				t -= delta;
				if(np->i != 0)
				{
					np->i = 0;
					np->encstate ^= 1;
				}
				break;
			}

			// Back up
			if((--np < tail) && (np->i != 1))
			{
				// Try the next best branch of this node
				np->i++;
				np->encstate ^= 1;
				break;
			}
		}
	}

	if(i > maxcycles)
		return -1;									// ran out of cycles

	// Extract decoded bits - node[8k+7] holds info bits 8k..8k+7
	// in its low byte, MSB first
	{
		int nbytes = nbits >> 3;
		np = &fano_nodes[7];
		while(nbytes-- != 0)
		{
			*data++ = (uint8_t)(np->encstate & 0xff);
			np += 8;
		}
	}

	return 0;
}
