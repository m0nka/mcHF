
#ifndef __WAVE_H
#define __WAVE_H
//#pragma once

// Save signal in floating point format (-1 .. +1) as a WAVE file using 16-bit signed integers.
void save_wav(float *signal, int num_samples, int sample_rate, char *path);

// Load signal in floating point format (-1 .. +1) as a WAVE file using 16-bit signed integers.
int load_wav(float *signal, int *num_samples, int *sample_rate, char *path);

#endif
