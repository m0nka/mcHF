// ******************************************
//
// ft8_lib by Karlis Goba, YL3JG
// https://github.com/kgoba/ft8_lib
//
// ******************************************
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_DSP

#include "wave.h"

#include "C:\Projects\wip\mcHF\firmware\STM32CubeH7\Middlewares\Third_Party\FatFs\src\ff.h"

// Save signal in floating point format (-1 .. +1) as a WAVE file using 16-bit signed integers.
void save_wav(float *signal, int num_samples, int sample_rate, char *path)
{
	char 		subChunk1ID[4] = {'f', 'm', 't', ' '};
    uint32_t 	subChunk1Size = 16;    // 16 for PCM
    uint16_t 	audioFormat = 1;       // PCM = 1
    uint16_t 	numChannels = 1;
    uint16_t 	bitsPerSample = 16;
    uint32_t 	sampleRate = sample_rate;
    uint16_t 	blockAlign = numChannels * bitsPerSample / 8;
    uint32_t 	byteRate = sampleRate * blockAlign;

    FIL 		f;
    uint 		written;

    char 		subChunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t 	subChunk2Size = num_samples * blockAlign;

    char 		chunkID[4] = {'R', 'I', 'F', 'F'};
    uint32_t 	chunkSize = 4 + (8 + subChunk1Size) + (8 + subChunk2Size);
    char 		format[4] = {'W', 'A', 'V', 'E'};

    if(f_open(&f,path, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
    	printf("error create file, exit!\r\n");
    	return;
    }

    // NOTE: works only on little-endian architecture
    f_write(&f,chunkID, sizeof(chunkID), &written);
    f_write(&f,&chunkSize, sizeof(chunkSize), &written);
    f_write(&f,format, sizeof(format), &written);
    f_write(&f,subChunk1ID, sizeof(subChunk1ID), &written);
    f_write(&f,&subChunk1Size, sizeof(subChunk1Size), &written);
    f_write(&f,&audioFormat, sizeof(audioFormat), &written);
    f_write(&f,&numChannels, sizeof(numChannels), &written);
    f_write(&f,&sampleRate, sizeof(sampleRate), &written);
    f_write(&f,&byteRate, sizeof(byteRate), &written);
    f_write(&f,&blockAlign, sizeof(blockAlign), &written);
    f_write(&f,&bitsPerSample, sizeof(bitsPerSample), &written);
    f_write(&f,subChunk2ID, sizeof(subChunk2ID), &written);
    f_write(&f,&subChunk2Size, sizeof(subChunk2Size), &written);

    // Write as words, to save on stack usage
    for (int i = 0; i < num_samples; i++)
    {
            float x = signal[i];

            if (x > 1.0)
            	x = 1.0;
            else
            {
            	if (x < -1.0)
            		x = -1.0;
            }

            int16_t raw_data = (int)(0.5 + (x * 32767.0));
            f_write(&f,&raw_data, sizeof(raw_data), &written);
    }

    f_close(&f);

    printf("-- file saved --\r\n");
}

// Load signal in floating point format (-1 .. +1) as a WAVE file using 16-bit signed integers.
int load_wav(float *signal, int *num_samples, int *sample_rate, char *path)
{
    char 		subChunk1ID[4];    	// = {'f', 'm', 't', ' '};
    uint32_t 	subChunk1Size; 		// = 16;    // 16 for PCM
    uint16_t 	audioFormat;   		// = 1;     // PCM = 1
    uint16_t 	numChannels;   		// = 1;
    uint16_t 	bitsPerSample; 		// = 16;
    uint32_t 	sampleRate;
    uint16_t 	blockAlign;    		// = numChannels * bitsPerSample / 8;
    uint32_t 	byteRate;      		// = sampleRate * blockAlign;

    char 		subChunk2ID[4];    	// = {'d', 'a', 't', 'a'};
    uint32_t 	subChunk2Size; 		// = num_samples * blockAlign;

    char 		chunkID[4];        	// = {'R', 'I', 'F', 'F'};
    uint32_t 	chunkSize;     		// = 4 + (8 + subChunk1Size) + (8 + subChunk2Size);
    char 		format[4];         	// = {'W', 'A', 'V', 'E'};

    uchar 		raw_data[512];
    short 		curr;
    ulong		loc_size;

    FIL 		f;
    FSIZE_t 	ofs = 0;
    uint 		bytes_read;

    if(f_open(&f,path,FA_READ) != FR_OK)
    {
        printf("error open file!\r\n");
        return -1;
    }

    f_lseek(&f, ofs);
    f_read(&f,(void *)chunkID,sizeof(chunkID),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&chunkSize,sizeof(chunkSize),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)format,sizeof(format),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)subChunk1ID,sizeof(subChunk1ID),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&subChunk1Size,sizeof(subChunk1Size),&bytes_read);
    ofs += bytes_read;

    //printf("subChunk1Size %d \r\n",subChunk1Size);

    if (subChunk1Size != 16)
    	return -2;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&audioFormat,sizeof(audioFormat),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&numChannels,sizeof(numChannels),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&sampleRate,sizeof(sampleRate),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&byteRate,sizeof(byteRate),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&blockAlign,sizeof(blockAlign),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&bitsPerSample,sizeof(bitsPerSample),&bytes_read);
    ofs += bytes_read;

    if (audioFormat != 1 || numChannels != 1 || bitsPerSample != 16)
    	return -1;

    f_lseek(&f, ofs);
    f_read(&f,(void *)subChunk2ID,sizeof(subChunk2ID),&bytes_read);
    ofs += bytes_read;

    f_lseek(&f, ofs);
    f_read(&f,(void *)&subChunk2Size,sizeof(subChunk2Size),&bytes_read);
    ofs += bytes_read;

    //printf("subChunk2Size %d \r\n",subChunk2Size);
    //printf("blockAlign %d \r\n",blockAlign);

    if (subChunk2Size / blockAlign > *num_samples)
    	return -3;
    
    *num_samples = subChunk2Size / blockAlign;
    *sample_rate = sampleRate;

    //printf("num_samples %d \r\n",*num_samples);
    printf("sample_rate %d \r\n",*sample_rate);

    // Original, probably fast, but lots of extra RAM needed
	#if 0
    int16_t *raw_data = (int16_t *)malloc(num_samples * blockAlign);
    fread((void *)raw_data, blockAlign, num_samples, f);
    for (int i = 0; i < num_samples; i++) {
        signal[i] = raw_data[i] / 32768.0f;
    }
    free(raw_data);
    //fclose(f);
	#endif

    loc_size = (*num_samples) * blockAlign;

    printf("reading file...\r\n");

    // ToDo: This loop is still not working, fix it!
    for (int i = 0; i < loc_size/512; i++)
    {
    	f_lseek(&f, ofs);
    	f_read(&f,raw_data,512,&bytes_read);
    	ofs += bytes_read;

    	if(bytes_read == 512)
    	{
    		int k = 0;
    		for (int j = 0; j < 512; j++)
    		{
    			curr = raw_data[k + 1] | (raw_data[k + 0] << 8);
    			signal[i*512 + j] = (float)(curr/32768.0f);
    			if(k < 512) k += 2;
    		}
    	}
    	else
    	{
    		printf("bytes left %d\r\n",bytes_read);

    		// Do read them...

    		break;
    	}
    }

    f_close(&f);
    printf("-- file read ok --\r\n");

    return 0;
}
#endif
