/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_frame.c                                                     **
**  Description:	MarsChat frame layer                                           **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Implements claude/MarsChat/PROTOCOL.md v0.3 sections 3-6
//
#include <string.h>

#include "mc_frame.h"

// Charset, Latin page (PROTOCOL.md 6.1)
// 0 END, 1-26 A-Z, 27-36 0-9, 37 space, 38-59 punctuation,
// 60 reserved, 61 SHIFT, 62 ESC, 63 reserved
const char mc_charset_latin[64] =
{
	'\0', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H',  'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P',  'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X',  'Y', 'Z', '0', '1', '2', '3', '4',
	'5',  '6', '7', '8', '9', ' ', '.', ',',
	'?',  '!', '/', '-', '+', '=', '@', ':',
	'\'', '(', ')', '"', '#', '$', '%', '&',
	'*',  ';', '<', '>', '\0', '\0', '\0', '\0'
};

// Charset, Cyrillic rendering page - Bulgarian Phonetic keyboard mapping
// (PROTOCOL.md 6.3). Pure UI hint, same on-air codes. UTF-8 strings so
// the host rig and any UTF-8 capable UI can render directly; codes 52-55
// overlay punctuation with the four letters that have no Latin key
const char *mc_charset_cyr_utf8[64] =
{
	"",  "А", "Б", "Ц", "Д", "Е", "Ф", "Г",
	"Х", "И", "Й", "К", "Л", "М", "Н", "О",
	"П", "Я", "Р", "С", "Т", "У", "Ж", "В",
	"Ь", "Ъ", "З", "0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9", " ", ".", ",",
	"?", "!", "/", "-", "+", "=", "@", ":",
	"'", "(", ")", "\"", "Ч", "Ш", "Щ", "Ю",
	"*", ";", "<", ">", "", "", "", ""
};

// Phrase LUT v1 (PROTOCOL.md 6.2), expanded by ESC + index
const char *mc_phrase_lut[MC_PHRASE_COUNT] =
{
	"HEY",								// 0
	"HOW COPY?",						// 1
	"GOING OFF AIR",					// 2  (QRT)
	"SIGNAL WEAK",						// 3
	"SIGNAL STRONG",					// 4
	"ALL OK",							// 5
	"NEED HELP",						// 6
	"LOCATION UNCHANGED",				// 7
	"YES",								// 8
	"NO",								// 9
	"STANDBY",							// 10 (QRX)
	"MOVING FREQ",						// 11 (QSY)
	"SEND AGAIN",						// 12 (QSZ)
	"INTERFERENCE HERE",				// 13 (QRM)
	"BACK IN 1 HOUR",					// 14
	"BACK TOMORROW SAME TIME"			// 15
};

//*----------------------------------------------------------------------------
//* Function Name       : mc_crc8
//* Object              : CRC-8/AUTOSAR over a byte buffer
//* Notes    			: poly 0x2F, init 0xFF, xorout 0xFF, no reflection
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
uint8_t mc_crc8(const uint8_t *data, int len)
{
	uint8_t	crc = 0xFF;
	int		i, b;

	for(i = 0; i < len; i++)
	{
		crc ^= data[i];

		for(b = 0; b < 8; b++)
		{
			if(crc & 0x80)
				crc = (uint8_t)((crc << 1) ^ 0x2F);
			else
				crc = (uint8_t)(crc << 1);
		}
	}

	return crc ^ 0xFF;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_crc8_header
//* Object              : CRC over the frame's b0..b41 (header + payload)
//* Notes    			: input is the packed 7 byte frame; b42..b47 (low 6
//* Notes    			: bits of byte 5) hold CRC bits and are masked off
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
static uint8_t mc_crc8_header(const uint8_t bits50[7])
{
	uint8_t	buf[6];

	memcpy(buf, bits50, 5);
	buf[5] = bits50[5] & 0xC0;

	return mc_crc8(buf, 6);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_frame_pack
//* Object              : frame -> canonical raw 50 bit payload
//* Notes    			: layout per PROTOCOL.md section 3, CRC per section 4
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
void mc_frame_pack(const MC_FRAME *f, uint8_t bits50[7])
{
	uint8_t	crc;

	// b0 MC=1, b1-2 ftype, b3-5 seq, b6-7 ack high bits
	bits50[0] = (uint8_t)(0x80
	          | ((f->ftype & 0x03) << 5)
	          | ((f->seq   & 0x07) << 2)
	          | ((f->ack   & 0x07) >> 1));

	// b8 ack low bit, b9 OTP, b10 MF, b11 CYR, b12-15 codes[0] high bits
	bits50[1] = (uint8_t)(((f->ack   & 0x01) << 7)
	          | ((f->flags & MC_FLAG_OTP) ? 0x40 : 0)
	          | ((f->flags & MC_FLAG_MF)  ? 0x20 : 0)
	          | ((f->flags & MC_FLAG_CYR) ? 0x10 : 0)
	          | ((f->codes[0] & 0x3F) >> 2));

	// b16-41 remaining payload codes
	bits50[2] = (uint8_t)(((f->codes[0] & 0x03) << 6) | (f->codes[1] & 0x3F));
	bits50[3] = (uint8_t)(((f->codes[2] & 0x3F) << 2) | ((f->codes[3] & 0x3F) >> 4));
	bits50[4] = (uint8_t)(((f->codes[3] & 0x0F) << 4) | ((f->codes[4] & 0x3F) >> 2));
	bits50[5] = (uint8_t)((f->codes[4] & 0x03) << 6);

	// b42-49 CRC over b0..b41
	crc = mc_crc8_header(bits50);

	bits50[5] |= (uint8_t)(crc >> 2);
	bits50[6]  = (uint8_t)((crc & 0x03) << 6);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_frame_unpack
//* Object              : raw 50 bit payload -> frame
//* Notes    			: returns 0 ok, 1 not a MarsChat frame (discrimination
//* Notes    			: per PROTOCOL.md 2.2 - caller falls back to WSPR)
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
int mc_frame_unpack(const uint8_t bits50[7], MC_FRAME *f)
{
	uint8_t	crc_rx, crc_calc;

	// MC marker bit must be set
	if((bits50[0] & 0x80) == 0)
		return 1;

	// CRC must verify
	crc_rx   = (uint8_t)(((bits50[5] & 0x3F) << 2) | ((bits50[6] >> 6) & 0x03));
	crc_calc = mc_crc8_header(bits50);

	if(crc_rx != crc_calc)
		return 1;

	f->ftype = (uint8_t)((bits50[0] >> 5) & 0x03);
	f->seq   = (uint8_t)((bits50[0] >> 2) & 0x07);
	f->ack   = (uint8_t)(((bits50[0] & 0x03) << 1) | ((bits50[1] >> 7) & 0x01));

	f->flags = 0;
	if(bits50[1] & 0x40)	f->flags |= MC_FLAG_OTP;
	if(bits50[1] & 0x20)	f->flags |= MC_FLAG_MF;
	if(bits50[1] & 0x10)	f->flags |= MC_FLAG_CYR;

	f->codes[0] = (uint8_t)(((bits50[1] & 0x0F) << 2) | ((bits50[2] >> 6) & 0x03));
	f->codes[1] = (uint8_t)(bits50[2] & 0x3F);
	f->codes[2] = (uint8_t)((bits50[3] >> 2) & 0x3F);
	f->codes[3] = (uint8_t)(((bits50[3] & 0x03) << 4) | ((bits50[4] >> 4) & 0x0F));
	f->codes[4] = (uint8_t)(((bits50[4] & 0x0F) << 2) | ((bits50[5] >> 6) & 0x03));

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_text_to_codes
//* Object              : ASCII text -> 6 bit charset codes (Latin page)
//* Notes    			: lowercase folded to upper, returns count or -1 on
//* Notes    			: unmappable char
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
int mc_text_to_codes(const char *text, uint8_t *codes, int max_codes)
{
	int	n = 0;

	while((*text != 0) && (n < max_codes))
	{
		char	c = *text++;
		int		i, found = -1;

		if((c >= 'a') && (c <= 'z'))
			c = (char)(c - 'a' + 'A');

		for(i = 1; i < 60; i++)
		{
			if(mc_charset_latin[i] == c)
			{
				found = i;
				break;
			}
		}

		if(found < 0)
			return -1;

		codes[n++] = (uint8_t)found;
	}

	return n;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_codes_to_text
//* Object              : 6 bit codes -> printable ASCII (Latin page)
//* Notes    			: END stops output, ESC + index expands a phrase,
//* Notes    			: reserved codes render as '#'. Returns chars written
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
int mc_codes_to_text(const uint8_t *codes, int num_codes, char *text, int text_size)
{
	int	i, n = 0;

	for(i = 0; (i < num_codes) && (n < text_size - 1); i++)
	{
		uint8_t	c = codes[i] & 0x3F;

		if(c == MC_CODE_END)
			break;

		if(c == MC_CODE_ESC)
		{
			// Expand phrase from the LUT
			const char	*p;

			if(++i >= num_codes)
				break;

			if((codes[i] & 0x3F) >= MC_PHRASE_COUNT)
				continue;

			p = mc_phrase_lut[codes[i] & 0x3F];

			while((*p != 0) && (n < text_size - 1))
				text[n++] = *p++;

			continue;
		}

		if(mc_charset_latin[c] != 0)
			text[n++] = mc_charset_latin[c];
	}

	text[n] = 0;

	return n;
}
