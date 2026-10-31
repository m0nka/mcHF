/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		radio_ini.c                                                    **
**  Description:	radio settings in a text .ini file on the SD card - long term  **
**					copy of the backup SRAM, editable on a PC, restore source when **
**					the SRAM is lost. Own small parser, FF_USE_STRFUNC is off and  **
**					there is no ReadProfileString here: the whole file goes into   **
**					one buffer, lines are cut to INI_LINE_MAX, nothing is trusted  **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_SD

#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "mchf_icc_def.h"

#include "radio_init.h"
#include "radio_ini.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

#define INI_BUF_SIZE		(16 * 1024)		// written file is ~6 KB
#define INI_LINE_MAX		96
#define INI_SECTION_MAX		24

static char		ini_buf[INI_BUF_SIZE];
static ulong	ini_len;
static uchar	ini_overflow;
static FIL		ini_fil;

// Section names, indexed by BAND_MODE_xx
static const char * const ini_band_names[MAX_BANDS] =
{
	"2200m", "630m", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"
};

// Indexed by DEMOD_xx
static const char * const ini_mode_names[DEMOD_MAX_MODE + 1] =
{
	"USB", "LSB", "CW", "AM", "FM", "DIGI"
};

// Indexed by AUDIO_xx
static const char * const ini_filter_names[AUDIO_WIDE + 1] =
{
	"300", "500", "1k8", "2k3", "3k6", "wide"
};

// Indexed by AGC_xx
static const char * const ini_agc_names[AGC_MAX_MODE + 1] =
{
	"slow", "med", "fast", "custom", "off"
};

static const char * const ini_vfo_names[2] =
{
	"A", "B"
};

// Indexed by FKEY_APP_xx
static const char * const ini_fkey_app_names[FKEY_APP_COUNT] =
{
	"ft8", "meshcore", "marschat", "wspr", "none"
};

// Indexed by key, tsu.fkey_app[]
static const char * const ini_fkey_names[FKEY_COUNT] =
{
	"fkey_f2", "fkey_f3", "fkey_f4"
};

// Baseband menu settings, the X-macro ids lower cased in the file
#define DSP_SET_NAME(id, def, min, max)		#id,
static const char * const ini_dsp_names[DSP_SET_COUNT] = { DSP_SET_LIST(DSP_SET_NAME) };

// -----------------------------------------------------------------------------
// Helpers
//
static char ini_lower(char c)
{
	if((c >= 'A') && (c <= 'Z'))
		return (c - 'A' + 'a');

	return c;
}

// Case insensitive compare
static uchar ini_eq(const char *a, const char *b)
{
	while(*a && *b)
	{
		if(ini_lower(*a++) != ini_lower(*b++))
			return 0;
	}

	return (*a == *b);
}

static char *ini_trim(char *s)
{
	char *e;

	while((*s == ' ') || (*s == '\t'))
		s++;

	e = s + strlen(s);
	while((e > s) && ((*(e - 1) == ' ') || (*(e - 1) == '\t')))
		e--;
	*e = 0;

	return s;
}

static ulong ini_fnv(const char *d, ulong n)
{
	ulong h = 2166136261UL;

	while(n--)
	{
		h ^= (uchar)*d++;
		h *= 16777619UL;
	}

	return h;
}

// -----------------------------------------------------------------------------
// Writer
//
static void ini_puts(const char *s)
{
	while(*s)
	{
		if(ini_len >= (INI_BUF_SIZE - 1))
		{
			ini_overflow = 1;
			return;
		}
		ini_buf[ini_len++] = *s++;
	}
}

static void ini_put_lower(const char *s)
{
	char c[2] = { 0, 0 };

	while(*s)
	{
		c[0] = ini_lower(*s++);
		ini_puts(c);
	}
}

// No printf here, the tiny one drops whole lines on anything it does not know
static void ini_put_num(long v)
{
	char	t[12], c[2] = { 0, 0 };
	int		i = 0;
	ulong	u;

	if(v < 0)
	{
		ini_puts("-");
		u = (ulong)(-v);
	}
	else
		u = (ulong)v;

	do
	{
		t[i++] = '0' + (u % 10);
		u /= 10;
	}
	while(u);

	while(i)
	{
		c[0] = t[--i];
		ini_puts(c);
	}
}

static void ini_kv_num(const char *k, long v)
{
	ini_puts(k);
	ini_puts("=");
	ini_put_num(v);
	ini_puts("\r\n");
}

static void ini_kv_str(const char *k, const char *v)
{
	ini_puts(k);
	ini_puts("=");
	ini_puts(v);
	ini_puts("\r\n");
}

static const char *ini_name_of(const char * const *names, uchar cnt, uchar idx)
{
	if(idx < cnt)
		return names[idx];

	return "";
}

static void ini_build(void)
{
	struct BAND_INFO	*b;
	int					i;

	ini_len 	 = 0;
	ini_overflow = 0;

	ini_puts("; mcHF settings - written by the radio at power off.\r\n");
	ini_puts("; Fine to edit on a PC, a changed file is loaded on the next boot.\r\n");
	ini_puts("; Unknown keys are ignored, out of range values are clamped or skipped.\r\n");
	ini_puts("; mode: USB LSB CW AM FM DIGI, filter: 300 500 1k8 2k3 3k6 wide,\r\n");
	ini_puts("; agc_mode: slow med fast custom off, frequencies in Hz.\r\n");
	ini_puts("; fkey_f2..fkey_f4: ft8 meshcore marschat wspr none.\r\n");

	ini_puts("\r\n[radio]\r\n");
	ini_kv_str("band",			ini_name_of(ini_band_names, MAX_BANDS, tsu.curr_band));
	ini_kv_num("brightness",	tsu.brightness);
	ini_kv_num("smeter_type",	tsu.smet_type);
	ini_kv_str("agc_mode",		ini_name_of(ini_agc_names, AGC_MAX_MODE + 1, tsu.agc_mode));
	ini_kv_num("rf_gain",		tsu.rf_gain);
	ini_kv_num("bluetooth",		tsu.bt_enabled);
	ini_kv_num("demo_mode",		tsu.demo_mode);

	ini_puts("\r\n[ui]\r\n");
	for(i = 0; i < FKEY_COUNT; i++)
		ini_kv_str(ini_fkey_names[i], ini_name_of(ini_fkey_app_names, FKEY_APP_COUNT, tsu.fkey_app[i]));

	for(i = 0; i < MAX_BANDS; i++)
	{
		b = &tsu.band[i];

		ini_puts("\r\n[band_");
		ini_puts(ini_band_names[i]);
		ini_puts("]\r\n");

		ini_kv_num("vfo_a",			b->vfo_a);
		ini_kv_num("vfo_b",			b->vfo_b);
		ini_kv_str("active_vfo",	ini_name_of(ini_vfo_names, 2, b->active_vfo));
		ini_kv_str("mode",			ini_name_of(ini_mode_names, DEMOD_MAX_MODE + 1, b->demod_mode));
		ini_kv_str("filter",		ini_name_of(ini_filter_names, AUDIO_WIDE + 1, b->filter));
		ini_kv_num("step",			b->step);
		ini_kv_num("volume",		b->volume);
		ini_kv_num("nco_freq",		b->nco_freq);
		ini_kv_num("fixed_mode",	b->fixed_mode);
		ini_kv_num("span",			b->span);
		ini_kv_num("atten",			b->atten);
		ini_kv_num("st_volume",		b->st_volume);
		ini_kv_num("audio_balance",	b->audio_balance);
		ini_kv_num("tx_power",		b->tx_power);
		ini_kv_num("power_factor",	b->power_factor);
	}

	ini_puts("\r\n[dsp]\r\n");
	for(i = 0; i < DSP_SET_COUNT; i++)
	{
		ini_put_lower(ini_dsp_names[i]);
		ini_puts("=");
		ini_put_num(dsp_settings[i]);
		ini_puts("\r\n");
	}
}

// -----------------------------------------------------------------------------
// Reader
//
static uchar ini_read(void)
{
	UINT	br;
	FRESULT	res;

	if(f_open(&ini_fil, RADIO_INI_PATH, FA_READ) != FR_OK)
		return RADIO_INI_NO_FILE;

	if(f_size(&ini_fil) >= INI_BUF_SIZE)
	{
		f_close(&ini_fil);
		printf("ini: file too big \r\n");
		return RADIO_INI_ERROR;
	}

	res = f_read(&ini_fil, ini_buf, (INI_BUF_SIZE - 1), &br);
	f_close(&ini_fil);

	if(res != FR_OK)
		return RADIO_INI_ERROR;

	ini_len = br;
	ini_buf[ini_len] = 0;

	return RADIO_INI_OK;
}

static uchar ini_num(const char *v, long *out)
{
	char *end;
	long x = strtol(v, &end, 10);

	if((end == v) || (*end != 0))
		return 0;

	*out = x;
	return 1;
}

// A name from the list, or its plain index
static uchar ini_name(const char *v, const char * const *names, uchar cnt, long *out)
{
	uchar i;

	for(i = 0; i < cnt; i++)
	{
		if(ini_eq(v, names[i]))
		{
			*out = i;
			return 1;
		}
	}

	if(!ini_num(v, out))
		return 0;

	return ((*out >= 0) && (*out < cnt));
}

static uchar ini_range(const char *v, long lo, long hi, long *out)
{
	if(!ini_num(v, out))
		return 0;

	if(*out < lo) *out = lo;
	if(*out > hi) *out = hi;

	return 1;
}

static uchar ini_valid_step(long s)
{
	switch(s)
	{
		case T_STEP_1HZ:
		case T_STEP_10HZ:
		case T_STEP_100HZ:
		case T_STEP_500HZ:
		case T_STEP_1KHZ:
		case T_STEP_10KHZ:
		case T_STEP_100KHZ:
		case T_STEP_1MHZ:
		case T_STEP_10MHZ:
			return 1;
		default:
			return 0;
	}
}

static void ini_apply_radio(const char *key, const char *val)
{
	long v;

	if(ini_eq(key, "band"))
	{
		if(ini_name(val, ini_band_names, MAX_BANDS, &v))
			tsu.curr_band = (uchar)v;
	}
	else if(ini_eq(key, "brightness"))
	{
		if(ini_range(val, 0, 100, &v))
			tsu.brightness = (uchar)v;
	}
	else if(ini_eq(key, "smeter_type"))
	{
		if(ini_range(val, 0, 1, &v))
			tsu.smet_type = (uchar)v;
	}
	else if(ini_eq(key, "agc_mode"))
	{
		if(ini_name(val, ini_agc_names, AGC_MAX_MODE + 1, &v))
			tsu.agc_mode = (uchar)v;
	}
	else if(ini_eq(key, "rf_gain"))
	{
		if(ini_range(val, 0, 50, &v))
			tsu.rf_gain = (uchar)v;
	}
	else if(ini_eq(key, "bluetooth"))
	{
		if(ini_range(val, 0, 1, &v))
			tsu.bt_enabled = (uchar)v;
	}
	else if(ini_eq(key, "demo_mode"))
	{
		if(ini_range(val, 0, 1, &v))
			tsu.demo_mode = (uchar)v;
	}
}

static void ini_apply_ui(const char *key, const char *val)
{
	long  v;
	uchar i;

	for(i = 0; i < FKEY_COUNT; i++)
	{
		if(ini_eq(key, ini_fkey_names[i]))
		{
			if(ini_name(val, ini_fkey_app_names, FKEY_APP_COUNT, &v))
				tsu.fkey_app[i] = (uchar)v;
			return;
		}
	}
}

static void ini_apply_band(struct BAND_INFO *b, const char *key, const char *val)
{
	long v;

	if(ini_eq(key, "vfo_a"))
	{
		if(ini_num(val, &v) && (v > 0) && (v <= 100000000))
			b->vfo_a = (ulong)v;
	}
	else if(ini_eq(key, "vfo_b"))
	{
		if(ini_num(val, &v) && (v > 0) && (v <= 100000000))
			b->vfo_b = (ulong)v;
	}
	else if(ini_eq(key, "active_vfo"))
	{
		if(ini_name(val, ini_vfo_names, 2, &v))
			b->active_vfo = (uchar)v;
	}
	else if(ini_eq(key, "mode"))
	{
		if(ini_name(val, ini_mode_names, DEMOD_MAX_MODE + 1, &v))
			b->demod_mode = (uchar)v;
	}
	else if(ini_eq(key, "filter"))
	{
		if(ini_name(val, ini_filter_names, AUDIO_WIDE + 1, &v))
			b->filter = (uchar)v;
	}
	else if(ini_eq(key, "step"))
	{
		if(ini_num(val, &v) && ini_valid_step(v))
			b->step = (ulong)v;
	}
	else if(ini_eq(key, "volume"))
	{
		if(ini_range(val, 0, MAX_AUDIO_LEVEL, &v))
			b->volume = (uchar)v;
	}
	else if(ini_eq(key, "nco_freq"))
	{
		if(ini_range(val, -20000, 20000, &v))
			b->nco_freq = (short)v;
	}
	else if(ini_eq(key, "fixed_mode"))
	{
		if(ini_range(val, 0, 1, &v))
			b->fixed_mode = (uchar)v;
	}
	else if(ini_eq(key, "span"))
	{
		if(ini_num(val, &v) && (v > 0) && (v <= 1000000))
			b->span = (ulong)v;
	}
	else if(ini_eq(key, "atten"))
	{
		if(ini_range(val, 0, (ATTEN_MAX - 1), &v))
			b->atten = (uchar)v;
	}
	else if(ini_eq(key, "st_volume"))
	{
		if(ini_range(val, 0, MAX_SIDETONE_LEVEL, &v))
			b->st_volume = (uchar)v;
	}
	else if(ini_eq(key, "audio_balance"))
	{
		if(ini_range(val, 0, 16, &v))
			b->audio_balance = (uchar)v;
	}
	else if(ini_eq(key, "tx_power"))
	{
		if(ini_range(val, 0, 255, &v))
			b->tx_power = (uchar)v;
	}
	else if(ini_eq(key, "power_factor"))
	{
		if(ini_range(val, 0, 255, &v))
			b->power_factor = (uchar)v;
	}
}

static void ini_apply_dsp(const char *key, const char *val)
{
	long	v;
	uchar	i;

	for(i = 0; i < DSP_SET_COUNT; i++)
	{
		if(!ini_eq(key, ini_dsp_names[i]))
			continue;

		if(ini_num(val, &v))
		{
			dsp_settings[i] = radio_init_dsp_setting_clamp(i, (short)v);
			dsp_settings_dirty = 1;
		}
		return;
	}
}

static void ini_apply(const char *sec, const char *key, const char *val)
{
	uchar i;

	if(ini_eq(sec, "radio"))
	{
		ini_apply_radio(key, val);
		return;
	}

	if(ini_eq(sec, "dsp"))
	{
		ini_apply_dsp(key, val);
		return;
	}

	if(ini_eq(sec, "ui"))
	{
		ini_apply_ui(key, val);
		return;
	}

	// [band_xx]
	if(strncmp(sec, "band_", 5) != 0)
		return;

	for(i = 0; i < MAX_BANDS; i++)
	{
		if(ini_eq(sec + 5, ini_band_names[i]))
		{
			ini_apply_band(&tsu.band[i], key, val);
			return;
		}
	}
}

static void ini_parse(void)
{
	char	line[INI_LINE_MAX];
	char	section[INI_SECTION_MAX];
	char	*p = ini_buf, *key, *val, *e;
	ulong	n;

	section[0] = 0;

	while(*p)
	{
		// One line, anything past INI_LINE_MAX is dropped
		n = 0;
		while(*p && (*p != '\n') && (*p != '\r'))
		{
			if(n < (sizeof(line) - 1))
				line[n++] = *p;
			p++;
		}
		while((*p == '\r') || (*p == '\n'))
			p++;
		line[n] = 0;

		// Comments
		e = strchr(line, ';');
		if(e) *e = 0;
		e = strchr(line, '#');
		if(e) *e = 0;

		key = ini_trim(line);
		if(*key == 0)
			continue;

		if(*key == '[')
		{
			e = strchr(key, ']');
			if(e) *e = 0;

			strncpy(section, ini_trim(key + 1), (sizeof(section) - 1));
			section[sizeof(section) - 1] = 0;
			continue;
		}

		val = strchr(key, '=');
		if(val == NULL)
			continue;
		*val++ = 0;

		ini_apply(section, ini_trim(key), ini_trim(val));
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_ini_file_sig
//* Object              : fingerprint of the file on the card, compared with
//*						: the one stored in the SRAM to spot a PC edit
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
uchar radio_ini_file_sig(ulong *sig)
{
	uchar res = ini_read();

	if(res != RADIO_INI_OK)
		return res;

	*sig = ini_fnv(ini_buf, ini_len);
	return RADIO_INI_OK;
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_ini_load
//* Object              : apply the file over the current tsu/dsp settings,
//*						: whatever is missing in it stays as it was
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
uchar radio_ini_load(void)
{
	uchar res = ini_read();

	if(res != RADIO_INI_OK)
		return res;

	ini_parse();
	return RADIO_INI_OK;
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_ini_save
//* Object              : write the current settings out, via a temp file so
//*						: a power cut mid write leaves the old one in place
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
uchar radio_ini_save(ulong *sig)
{
	UINT	bw;
	FRESULT	res;

	ini_build();
	if(ini_overflow)
	{
		printf("ini: buffer overflow \r\n");
		return RADIO_INI_ERROR;
	}

	f_mkdir("0://system");										// ok if it exists

	if(f_open(&ini_fil, RADIO_INI_TMP_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return RADIO_INI_NO_FILE;

	res = f_write(&ini_fil, ini_buf, ini_len, &bw);
	if(f_close(&ini_fil) != FR_OK)
		res = FR_DISK_ERR;

	if((res != FR_OK) || (bw != ini_len))
	{
		f_unlink(RADIO_INI_TMP_PATH);
		return RADIO_INI_ERROR;
	}

	f_unlink(RADIO_INI_PATH);
	if(f_rename(RADIO_INI_TMP_PATH, RADIO_INI_PATH) != FR_OK)
		return RADIO_INI_ERROR;

	*sig = ini_fnv(ini_buf, ini_len);
	return RADIO_INI_OK;
}

#endif
