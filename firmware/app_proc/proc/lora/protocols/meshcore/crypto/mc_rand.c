/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_rand.c                                                      **
**  Description:	Random bytes for MeshCore key generation                       **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The H7 has a true RNG but HAL_RNG_MODULE is not enabled in this build,
// so the peripheral is driven directly - it is four registers. Whatever
// it produces is then run through SHA-512 together with the die id, the
// cycle counter and the RTC, so a partially working TRNG cannot make the
// output worse than the conditioning
//
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include <string.h>

#include "rtc.h"
#include "sha512.h"

#include "mc_rand.h"

// Unique device id (RM0399 - 64.1, "Unique device ID register")
#define MC_RAND_UID_BASE		0x1FF1E800UL

//*----------------------------------------------------------------------------
//* Function Name       : mc_rand_trng_start
//* Object              : bring up HSI48 (the RNG kernel clock by reset
//*						: default) and enable the peripheral
//* Notes    			: returns 0 when the RNG is running
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static uint8_t mc_rand_trng_start(void)
{
	uint32_t	guard;

	// HSI48 on - it may already be running for USB
	if((RCC->CR & RCC_CR_HSI48ON) == 0)
		RCC->CR |= RCC_CR_HSI48ON;

	guard = 100000;
	while(((RCC->CR & RCC_CR_HSI48RDY) == 0) && (guard--))
		;

	if((RCC->CR & RCC_CR_HSI48RDY) == 0)
		return 1;

	// Kernel clock select = hsi48_ker_ck (reset value, set explicitly so
	// an earlier configuration cannot leave us without a clock)
	RCC->D2CCIP2R &= ~RCC_D2CCIP2R_RNGSEL;

	__HAL_RCC_RNG_CLK_ENABLE();

	// Clock error detection off - the RNG is clocked from HSI48 while
	// the core runs at 480 MHz, which trips CECS on some samples
	RNG->CR = RNG_CR_CED;
	RNG->CR |= RNG_CR_RNGEN;

	// Discard the first word, as the reference manual requires
	guard = 100000;
	while(((RNG->SR & RNG_SR_DRDY) == 0) && (guard--))
		;

	if((RNG->SR & RNG_SR_DRDY) == 0)
		return 1;

	(void)RNG->DR;

	return 0;
}

static uint8_t mc_rand_trng_word(uint32_t *out)
{
	uint32_t	guard = 100000;

	while(((RNG->SR & RNG_SR_DRDY) == 0) && (guard--))
	{
		// A seed or clock error latches here - clear and keep trying
		if(RNG->SR & (RNG_SR_SECS | RNG_SR_CECS))
			RNG->SR = 0;
	}

	if((RNG->SR & RNG_SR_DRDY) == 0)
		return 1;

	*out = RNG->DR;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rand_bytes
//* Object              : fill a buffer with conditioned random bytes
//* Notes    			: entropy is gathered in blocks and squeezed
//*						: through SHA-512, so the caller never sees raw
//*						: TRNG words
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
uint8_t mc_rand_bytes(uint8_t *buf, size_t len)
{
	uint8_t			pool[128];
	uint8_t			digest[SHA512_DIGEST_SIZE];
	uint32_t		counter = 0;
	uint8_t			degraded = 0;
	sha512_ctx		ctx;
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	size_t			take;
	int				i;

	if((buf == NULL) || (len == 0))
		return 1;

	// The cycle counter is only running when the cpu_trace build switch
	// is on, and it is one of our entropy sources - start it here so it
	// contributes either way
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL		 |= DWT_CTRL_CYCCNTENA_Msk;

	if(mc_rand_trng_start())
		degraded = 1;

	while(len)
	{
		size_t	fill = 0;

		memset(pool, 0, sizeof(pool));

		// 16 TRNG words, or dead air if it never came up
		for(i = 0; i < 16; i++)
		{
			uint32_t	w = 0;

			if((!degraded) && mc_rand_trng_word(&w))
				degraded = 1;

			memcpy(pool + fill, &w, sizeof(w));
			fill += sizeof(w);
		}

		// Die id - constant, but it separates two radios that would
		// otherwise share a degraded pool
		memcpy(pool + fill, (const void *)MC_RAND_UID_BASE, 12);
		fill += 12;

		// Cycle counter, sampled across a short spin so the jitter of
		// the OS tick and the SDRAM refresh gets in
		for(i = 0; i < 8; i++)
		{
			uint32_t	c = DWT->CYCCNT;

			memcpy(pool + fill, &c, sizeof(c));
			fill += sizeof(c);

			osDelay(1);
		}

		k_GetTime(&tm);
		k_GetDate(&dt);

		pool[fill++] = tm.Hours;
		pool[fill++] = tm.Minutes;
		pool[fill++] = tm.Seconds;
		pool[fill++] = dt.Date;
		pool[fill++] = dt.Month;
		pool[fill++] = dt.Year;

		memcpy(pool + fill, &tm.SubSeconds, sizeof(tm.SubSeconds));
		fill += sizeof(tm.SubSeconds);

		// Conditioning - the block counter keeps successive squeezes
		// from repeating if every source happened to be static
		sha512_init(&ctx);
		sha512_update(&ctx, pool, fill);
		sha512_update(&ctx, (const uint8_t *)&counter, sizeof(counter));
		sha512_final(&ctx, digest);

		counter++;

		take = (len < SHA512_DIGEST_SIZE) ? len : SHA512_DIGEST_SIZE;

		memcpy(buf, digest, take);

		buf += take;
		len -= take;
	}

	memset(pool, 0, sizeof(pool));
	memset(digest, 0, sizeof(digest));

	return degraded;
}

#endif
