// ******************************************
//
// ft8_lib by Karlis Goba, YL3JG9
// https://github.com/kgoba/ft8_lib
//
// ******************************************
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_DSP

#include "ft8/unpack_v2.h"
#include "ft8/ldpc.h"
#include "ft8/decode.h"
#include "ft8/ft8_constants.h"
#include "common/wave.h"

// test only!
#include "gen_ft8.h"

//#include "common/debug.h"
#include "fft/kiss_fftr.h"

//#define LOG_LEVEL   LOG_INFO

//void usage() {
//    fprintf(stderr, "Decode a 15-second WAV file.\n");
//}

float hann_i(int i, int N)
{
    float x = sinf((float)M_PI * i / (N - 1));
    return x*x;
}

float hamming_i(int i, int N)
{
    const float a0 = (float)25 / 46;
    const float a1 = 1 - a0;

    float x1 = cosf(2 * (float)M_PI * i / (N - 1));
    return a0 - a1*x1;
}

float blackman_i(int i, int N)
{
    const float alpha = 0.16f; // or 2860/18608
    const float a0 = (1 - alpha) / 2;
    const float a1 = 1.0f / 2;
    const float a2 = alpha / 2;

    float x1 = cosf(2 * (float)M_PI * i / (N - 1));
    //float x2 = cosf(4 * (float)M_PI * i / (N - 1));
    float x2 = 2*x1*x1 - 1; // Use double angle formula

    return a0 - a1*x1 + a2*x2;
}

// Compute FFT magnitudes (log power) for each timeslot in the signal
//
float   		ep_window[15360];
kiss_fft_scalar ep_timedata[15360];
kiss_fft_cpx    ep_freqdata[15361];
float           ep_mag_db[7681];
//
void extract_power(float *signal, int num_blocks, int num_bins, uint8_t *power)
{
    const int 		block_size = 2 * num_bins;      // Average over 2 bins per FSK tone
    const int 		nfft = 2 * block_size;          // We take FFT of two blocks, advancing by one
    const float 	fft_norm = 2.0f / nfft;
    // Stack usage									   bytes
    //float   		ep_window[nfft];				// 15360
    size_t  		fft_work_size;					// 4
    //kiss_fft_scalar ep_timedata[nfft];			// 15360
    //kiss_fft_cpx    ep_freqdata[nfft/2 + 1];		// 15361
    //float           ep_mag_db[nfft/2 + 1];		// 7681
    int 			offset = 0;						// 4
    float 			max_mag = -100.0f;				// 4
    float 			mag2;							// 4
    float	 		db1;							// 4
    float 			db2;							// 4
    float 			db;								// 4
    int 			scaled;							// 4
    int				i,j,time_sub,freq_sub;			// 16
    // End of stack usage							   53810 total

    //printf("extract_power->in\r\n");

    for (int i = 0; i < nfft; ++i)
    	ep_window[i] = blackman_i(i, nfft);

    kiss_fftr_alloc(nfft, 0, 0, &fft_work_size);

    //printf("nfft ram usage: %d bytes\r\n", nfft);
    //printf("FFT work area = %d bytes\r\n", fft_work_size);

    void        *fft_work = pvPortMalloc(fft_work_size);
    kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(nfft, 0, fft_work, &fft_work_size);

    for (i = 0; i < num_blocks; ++i)
    {
        // Loop over two possible time offsets (0 and block_size/2)
        for (time_sub = 0; time_sub <= block_size/2; time_sub += block_size/2)
        {
            // Extract windowed signal block
            for (j = 0; j < nfft; ++j)
            	ep_timedata[j] = ep_window[j] * signal[(i * block_size) + (j + time_sub)];

            kiss_fftr(fft_cfg, ep_timedata, ep_freqdata);

            // Compute log magnitude in decibels
            for (j = 0; j < nfft/2 + 1; ++j)
            {
                mag2 = (ep_freqdata[j].i * ep_freqdata[j].i + ep_freqdata[j].r * ep_freqdata[j].r);
                ep_mag_db[j] = 10.0f * log10f(1E-10f + mag2 * fft_norm * fft_norm);
            }

            // Loop over two possible frequency bin offsets (for averaging)
            for (freq_sub = 0; freq_sub < 2; ++freq_sub)
            {
                for (j = 0; j < num_bins; ++j)
                {
                    db1 = ep_mag_db[j * 2 + freq_sub];
                    db2 = ep_mag_db[j * 2 + freq_sub + 1];
                    db = (db1 + db2) / 2;

                    // Scale decibels to unsigned 8-bit range and clamp the value
                    scaled = (int)(2 * (db + 120));
                    power[offset] = (scaled < 0) ? 0 : ((scaled > 255) ? 255 : scaled);
                    ++offset;

                    if (db > max_mag) max_mag = db;
                }
            }
        }
    }

    //LOG(LOG_INFO, "Max magnitude: %.1f dB\n", max_mag);
    vPortFree(fft_work);
    //printf("extract_power->out\r\n");
}

void normalize_signal(float *signal, int num_samples)
{
    float max_amp = 1E-5f;

    for (int i = 0; i < num_samples; ++i)
    {
        float amp = fabsf(signal[i]);
        if (amp > max_amp)
        {
            max_amp = amp;
        }
    }

    for (int i = 0; i < num_samples; ++i)
    {
        signal[i] /= max_amp;
    }    
}

void print_tones(const uint8_t *code_map, const float *log174)
{
    for (int k = 0; k < 3 * FT8_ND; k += 3)
    {
        uint8_t max = 0;
        if (log174[k + 0] > 0) max |= 4;
        if (log174[k + 1] > 0) max |= 2;
        if (log174[k + 2] > 0) max |= 1;
        //LOG(LOG_DEBUG, "%d", code_map[max]);
    }
    //LOG(LOG_DEBUG, "\n");
}

#if 0
int main(int argc, char **argv) {
    // Expect one command-line argument
    if (argc < 2) {
        usage();
        return -1;
    }

    const char *wav_path = argv[1];

    int sample_rate = 12000;
    int num_samples = 15 * sample_rate;
    float signal[num_samples];

    int rc = load_wav(signal, num_samples, sample_rate, wav_path);
    if (rc < 0) {
        return -1;
    }
    normalize_signal(signal, num_samples);

    const float fsk_dev = 6.25f;    // tone deviation in Hz and symbol rate

    // Compute DSP parameters that depend on the sample rate
    const int num_bins = (int)(sample_rate / (2 * fsk_dev));
    const int block_size = 2 * num_bins;
    const int num_blocks = (num_samples - (block_size/2) - block_size) / block_size;

    LOG(LOG_INFO, "%d blocks, %d bins\n", num_blocks, num_bins);

    // Compute FFT over the whole signal and store it
    uint8_t power[num_blocks * 4 * num_bins];

    extract_power(signal, num_blocks, num_bins, power);

    // Find top candidates by Costas sync score and localize them in time and frequency
    Candidate candidate_list[kMax_candidates];
    int num_candidates = find_sync(power, num_blocks, num_bins, kCostas_map, kMax_candidates, candidate_list);

    // TODO: sort the candidates by strongest sync first?

    // Go over candidates and attempt to decode messages
    char    decoded[kMax_decoded_messages][kMax_message_length];
    int     num_decoded = 0;
    for (int idx = 0; idx < num_candidates; ++idx) {
        Candidate &cand = candidate_list[idx];
        float freq_hz  = (cand.freq_offset + cand.freq_sub / 2.0f) * fsk_dev;
        float time_sec = (cand.time_offset + cand.time_sub / 2.0f) / fsk_dev;

        float   log174[FT8_N];
        extract_likelihood(power, num_bins, cand, kGray_map, log174);

        // bp_decode() produces better decodes, uses way less memory
        uint8_t plain[FT8_N];
        int     n_errors = 0;
        bp_decode(log174, kLDPC_iterations, plain, &n_errors);
        //ldpc_decode(log174, kLDPC_iterations, plain, &n_errors);

        if (n_errors > 0) {
            //printf("ldpc_decode() = %d\n", n_errors);
            continue;
        }
        
        // Extract payload + CRC (first FT8_K bits)
        uint8_t a91[12];
        pack_bits(plain, FT8_K, a91);

        // TODO: check CRC

        // printf("%03d: score = %d freq = %.1f time = %.2f\n", idx, 
        //         cand.score, freq_hz, time_sec);
        // print_tones(kGray_map, log174);
        // for (int i = 0; i < 12; ++i) {
        //     printf("%02x ", a91[i]);
        // }
        // printf("\n");

        char message[kMax_message_length];
        unpack77(a91, message);

        // Check for duplicate messages (TODO: use hashing)
        bool found = false;
        for (int i = 0; i < num_decoded; ++i) {
            if (0 == strcmp(decoded[i], message)) {
                found = true;
                break;
            }
        }

        if (!found && num_decoded < kMax_decoded_messages) {
            strcpy(decoded[num_decoded], message);
            ++num_decoded;

            // Fake WSJT-X-like output for now
            int snr = 0;    // TODO: compute SNR
            printf("000000 %3d %4.1f %4d ~  %s\n", cand.score, time_sec, (int)(freq_hz + 0.5f), message);
        }
    }
    LOG(LOG_INFO, "Decoded %d messages\n", num_decoded);

    return 0;
}

#endif

const float fsk_dev = 6.25f;    // tone deviation in Hz and symbol rate

const int 	kMax_candidates 		= 100;
const int 	kLDPC_iterations 		= 20;

const int 	kMax_decoded_messages 	= 50;
const int 	kMax_message_length 	= 20;

// --------------------------------------
// Static RAM usage
char    	decoded[50][20];
char 		message[20];
uint8_t 	a91[12];
float   	log174[FT8_N];
uint8_t 	plain[FT8_N];
// --------------------------------------

void decode_ft8_message(char *msg)
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// Minimised stack use
	int     	num_decoded = 0;
	int 		sample_rate = 12000;
	int 		num_samples = 15 * sample_rate;
	int 		num_bins;
	int 		block_size,num_blocks,num_candidates,n_errors;
	bool 		found;
	float 		freq_hz,time_sec;
	Candidate 	*cand;
	// ----------------------------------------------
	// External and internal RAM
	float 		*ft8_signal = (float *)0xc0277000; 					// float signal[num_samples]
	Candidate 	*candidate_list = (Candidate *)0x30000000;			// Candidate candidate_list[kMax_candidates] - needs to be aligned!!
	uint8_t 	*power = (uint8_t *)(0xc0277000 + num_samples*4); 	// uint8_t power[num_blocks * 4 * num_bins];
	// --------------------------------------------------------------------------------------------------------------------------------

	#if 1
	// Get file data - not working
	int rc = load_wav(ft8_signal, &num_samples, &sample_rate, "sample.wav");
	if (rc < 0)
	{
		//printf("Cannot load file!\r\n");
		//printf("RC = %d\n", rc);
		return;
	}
	#endif

	#if 0
	// Use local generation, to public RAM, instead of file values - works with the decoder
	encode_ft8_message("CQ M0NKA IO92",0);
	#endif

	normalize_signal(ft8_signal, num_samples);

	// Compute DSP parameters that depend on the sample rate
	num_bins = (int)(sample_rate / (2 * fsk_dev));
	block_size = 2 * num_bins;
	num_blocks = (num_samples - (block_size/2) - block_size) / block_size;

	//printf("%d blocks, %d bins\r\n", num_blocks, num_bins);
	//printf("power ram usage: %d bytes\r\n", num_blocks * 4 * num_bins);

	// Compute FFT over the whole signal and store it
	extract_power(ft8_signal, num_blocks, num_bins, power);

	//printf("candidate_list ram usage: %d bytes\r\n", kMax_candidates * sizeof(Candidate));

	// Find top candidates by Costas sync score and localize them in time and frequency
	num_candidates = find_sync(power, num_blocks, num_bins, (uchar *)kCostas_map, kMax_candidates, candidate_list);

	printf("num_candidates: %d\r\n", num_candidates);

	// TODO: sort the candidates by strongest sync first?
	//...

	// Go over candidates and attempt to decode messages
	for (int idx = 0; idx < num_candidates; ++idx)
	{
		cand = &candidate_list[idx];

		freq_hz  = (cand->freq_offset + cand->freq_sub / 2.0f) * fsk_dev;
		time_sec = (cand->time_offset + cand->time_sub / 2.0f) / fsk_dev;
		//float   log174[FT8_N];

		//printf("try: %d\r\n", idx);

		extract_likelihood(power, num_bins, cand, (uint8_t *)kGray_map, log174);

		//uint8_t plain[FT8_N];
		n_errors = 0;

		// bp_decode() produces better decodes, uses way less memory
		bp_decode(log174, kLDPC_iterations, plain, &n_errors);
		//ldpc_decode(log174, kLDPC_iterations, plain, &n_errors);

		if (n_errors > 0)
		{
			//printf("ldpc_decode() = %d\r\n", n_errors);
			continue;
		}

		#if 0
		printf("FSK tones: ");
		for (int j = 0; j < FT8_NN/2; ++j)
		{
			printf("%02x", plain[j]);
		}
		printf("\r\n");
		#endif

		// Extract payload + CRC (first FT8_K bits)
		//uint8_t a91[12];
		pack_bits(plain, FT8_K, a91);

		// TODO: check CRC
		//...

		#if 1
		//printf("%03d: score = %d freq = %.1f time = %.2f\n", idx, cand.score, freq_hz, time_sec);
		//print_tones(kGray_map, log174);
		for (int i = 0; i < 12; ++i)
		     printf("%02x ", a91[i]);
		printf("\r\n");
		#endif

		unpack77(a91, message);

		// Check for duplicate messages (TODO: use hashing)
		found = false;

		for (int i = 0; i < num_decoded; ++i)
		{
			if (0 == strcmp(decoded[i], message))
			{
				printf("found: %s\r\n",message);
				strcpy(msg,message);
				found = true;
				break;
			}
		}

		if (!found && num_decoded < kMax_decoded_messages)
		{
			strcpy(decoded[num_decoded], message);
			++num_decoded;

            // Fake WSJT-X-like output for now
            //int snr = 0;    // TODO: compute SNR
            //printf("000000 %3d %4.1f %4d ~  %s\n", cand.score, time_sec, (int)(freq_hz + 0.5f), message);
            //printf("%s\r\n", message);
        }
	}
	printf("Decoded %d messages\r\n", num_decoded);
}
#endif
