// ******************************************
//
// ft8_lib by Karlis Goba, YL3JG
// https://github.com/kgoba/ft8_lib
//
// ******************************************

#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_DSP

#include "common/wave.h"
#include "ft8/pack_v2.h"
#include "ft8/encode_v2.h"
#include "ft8/ft8_constants.h"

#include "gen_ft8.h"

// Convert a sequence of symbols (tones) into a sinewave of continuous phase (FSK).
// Symbol 0 gets encoded as a sine of frequency f0, the others are spaced in increasing
// fashion.
void synth_fsk(	uint8_t *symbols,
				int num_symbols,
				float f0,
				float spacing,
                float symbol_rate,
				float signal_rate,
				float *signal)
{
    float phase = 0;
    float dt = 1/signal_rate;
    float dt_sym = 1/symbol_rate;
    float t = 0;
    int j = 0;
    int i = 0;

    while (j < num_symbols)
    {
        float f = f0 + symbols[j] * spacing;
        phase += 2 * M_PI * f / signal_rate;

        signal[i] = sin(phase);

        t += dt;
        if (t >= dt_sym) {
            // Move to the next symbol
            t -= dt_sym;
            ++j;
        }
        ++i;
    }
}

void encode_ft8_message(char *msg,uchar save)
{
	const int 	sample_rate = 12000;
	const float symbol_rate = 6.25f;
	const int 	num_samples = (int)(0.5f + FT8_NN / symbol_rate * sample_rate);
	const int 	num_silence = (15 * sample_rate - num_samples) / 2;

	//float 	signal[num_silence + num_samples + num_silence]; - sorry baby, not a chance in a million years!
	float 		*ft8_signal = (float *)0xc0277000;	// layer 1 in ext sdram

	uint8_t 	packed[10];
	uint8_t 	tones[FT8_NN];          // FT8_NN = 79, lack of better name at the moment

	//printf("encode_ft8_message ram usage=%d bytes\r\n",(num_silence + num_samples + num_silence)*4);

    // First, pack the text data into binary message
	int rc = pack77(msg, packed);
	if (rc < 0)
	{
	     printf("Cannot parse message!\r\n");
	     printf("RC = %d\n", rc);
	     return;
	}

	#if 0
	printf("Packed data: ");
	for (int j = 0; j < 10; ++j)
	{
	   printf("%02x ", packed[j]);
	}
	printf("\r\n");
	#endif

	// Second, encode the binary message as a sequence of FSK tones
	genft8(packed, tones);

	#if 0
	printf("FSK tones: ");
	for (int j = 0; j < FT8_NN; ++j)
	{
	   printf("%d", tones[j]);
	}
	printf("\r\n");
	#endif

	// Clear buffer
	for (int i = 0; i < (num_silence + num_samples + num_silence); i++)
		ft8_signal[i] = 0;

	// Third, convert the FSK tones into an audio signal
	synth_fsk(tones, FT8_NN, 1000, symbol_rate, symbol_rate, sample_rate, (ft8_signal + num_silence));

	// Save to card
	if(save) save_wav(ft8_signal, (num_silence + num_samples + num_silence), sample_rate, "sample.wav");
}
#endif
