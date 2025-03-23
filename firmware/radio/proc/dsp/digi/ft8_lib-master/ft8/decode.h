#ifndef __DECODE_H
#define __DECODE_H

//#pragma once
//#include <stdint.h>

typedef struct Candidate {

	int16_t      score;
    int16_t      time_offset;
    int16_t      freq_offset;
    uint8_t      time_sub;
    uint8_t      freq_sub;

} Candidate;

// Localize top N candidates in frequency and time according to their sync strength (looking at Costas symbols)
// We treat and organize the candidate list as a min-heap (empty initially).
int find_sync(uint8_t *power, int num_blocks, int num_bins,uint8_t *sync_map, int num_candidates, Candidate *heap);


// Compute log likelihood log(p(1) / p(0)) of 174 message bits 
// for later use in soft-decision LDPC decoding
void extract_likelihood(uint8_t *power, int num_bins, Candidate *cand, uint8_t *code_map, float *log174);

#endif
