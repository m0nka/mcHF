/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_frame.h                                                     **
**  Description:	MarsChat frame layer - public API                              **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// MarsChat 50 bit frame pack/unpack per claude/MarsChat/PROTOCOL.md v0.3
//
//  b0    b1..b2   b3..b5   b6..b8   b9..b11   b12..b41      b42..b49
// [ MC | FTYPE  |  SEQ   |  ACK   |  FLAGS  |  PAYLOAD    |  CRC-8   ]
//
// Peer consumer/producer of the raw 50 bit PHY interface in proc/wspr
// (wspr_decoder_run_raw / wspr_encode_raw). Personality logic only -
// nothing in here may touch symbols, FFT or Fano internals.
//
// No OS or HAL dependencies - builds on a PC for the host test rig
//
#ifndef __MC_FRAME_H
#define __MC_FRAME_H

#include <stdint.h>

// Frame types
#define MC_FTYPE_BEACON			0
#define MC_FTYPE_DATA			1
#define MC_FTYPE_ACK			2
#define MC_FTYPE_CTRL			3

// FLAGS bits
#define MC_FLAG_OTP				0x01				// payload encrypted (gorilla builds only)
#define MC_FLAG_MF				0x02				// more fragments follow
#define MC_FLAG_CYR				0x04				// Cyrillic rendering page (UI hint only)

// Payload geometry
#define MC_PAYLOAD_CHARS		5					// 5 x 6 bit codes per frame

// Special charset codes
#define MC_CODE_END				0					// end of text / padding
#define MC_CODE_SHIFT			61					// reserved
#define MC_CODE_ESC				62					// next code is a phrase LUT index
#define MC_CODE_EXT				63					// reserved (version escape in first position)

// One frame, unpacked form
typedef struct
{
	uint8_t	ftype;									// MC_FTYPE_xxx
	uint8_t	seq;									// 0..7
	uint8_t	ack;									// 0..7, last correctly received peer seq
	uint8_t	flags;									// MC_FLAG_xxx
	uint8_t	codes[MC_PAYLOAD_CHARS];				// 6 bit charset codes

} MC_FRAME;

// Charset rendering tables, indexed by 6 bit code
// Latin page: single ASCII chars ('\0' for END and reserved codes)
// Cyrillic page: UTF-8 strings (Bulgarian Phonetic mapping, PROTOCOL.md 6.3)
extern const char	mc_charset_latin[64];
extern const char	*mc_charset_cyr_utf8[64];

// Phrase lookup table (ESC expansion), v1 defines 16 entries
#define MC_PHRASE_COUNT			16
extern const char	*mc_phrase_lut[MC_PHRASE_COUNT];

// Generic CRC-8/AUTOSAR (poly 0x2F, init 0xFF, xorout 0xFF, no reflection)
uint8_t	mc_crc8(const uint8_t *data, int len);

// Frame -> canonical raw 50 bit payload (7 bytes, MSB first, CRC filled in)
void	mc_frame_pack(const MC_FRAME *f, uint8_t bits50[7]);

// Raw 50 bit payload -> frame. Returns 0 ok, 1 not a MarsChat frame
// (MC bit clear or CRC mismatch - caller falls back to WSPR type 1)
int		mc_frame_unpack(const uint8_t bits50[7], MC_FRAME *f);

// Text -> 6 bit codes (Latin page, lowercase folded to upper)
// Returns number of codes written, -1 on unmappable char
int		mc_text_to_codes(const char *text, uint8_t *codes, int max_codes);

// 6 bit codes -> printable ASCII (Latin page), END stops, ESC expands
// phrases from mc_phrase_lut. Returns chars written (excl. terminator)
int		mc_codes_to_text(const uint8_t *codes, int num_codes, char *text, int text_size);

#endif
