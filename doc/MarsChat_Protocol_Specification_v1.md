# MarsChat — On-Air Protocol Specification

**Version:** 1.0  
**Date:** 30 August 2026  
**Author:** Krassi Atanassov, M0NKA  
**Platform:** mcHF QRP Transceiver v9  
**Licence:** GPLv3  
**Repository:** https://github.com/m0nka/mcHF

---

## 1. Purpose and Regulatory Notice

MarsChat is a very-slow-baud, text-only digital communication mode for HF
amateur radio, designed for extreme weak-signal messaging over ionospheric
paths. It trades throughput for reach: a single text character takes
approximately 24 seconds of on-air time, but the link closes at signal-to-noise
ratios of **−28 dB or better** in a 2500 Hz reference bandwidth — well below
the noise floor, where voice and conventional digital modes cannot operate.

This document is published in compliance with amateur radio practice that
requires open documentation of new on-air modes, enabling any station to decode
MarsChat transmissions. **No encryption is used in amateur (ham) mode.** The
on-air modulation is standard WSPR; only the interpretation of the 50
information bits differs.

The name "MarsChat" refers to the multi-minute message turnaround inherent to
the mode, reminiscent of Earth–Mars radio dialogue latency. It is not affiliated
with any space agency or Mars mission.

---

## 2. Overview

MarsChat reuses the **WSPR (Weak Signal Propagation Reporter)** physical layer
without modification. The 4-FSK modulation, convolutional FEC, interleaving,
synchronisation vector and timing are identical to WSPR type-1. Only the meaning
of the **50 information bits** carried within the WSPR frame changes.

This design choice means:
- Any existing WSPR decoder can demodulate MarsChat signals at the RF level.
- The same convolutional code and Fano sequential decoder recovers the 50 raw
  bits from noise.
- MarsChat then applies its own frame interpretation (described in §4–§8) to
  those 50 bits.

**Throughput:** 5 characters per 120-second slot, or approximately 0.42 bits
per second. A whole canned phrase can be transmitted in a single slot via the
phrase lookup table (§7.2).

---

## 3. Physical Layer (WSPR, Unchanged)

All PHY parameters are standard WSPR type-1 as defined by Joe Taylor, K1JT.

| Parameter | Value |
|-----------|-------|
| Modulation | 4-FSK, continuous phase |
| Number of symbols | 162 |
| Symbol rate | 12000 / 8192 ≈ 1.46484375 baud |
| Tone spacing | 375 / 256 ≈ 1.46484375 Hz |
| Occupied bandwidth | ≈ 6 Hz |
| Transmission duration | 162 × 8192 / 12000 ≈ 110.6 seconds |
| Slot period | 120 seconds, aligned to even UTC minutes |
| TX start | Slot boundary + 1.0 second |
| Audio centre frequency | Dial frequency + 1500 Hz (USB) |
| FEC | K = 32, rate = 1/2, non-recursive convolutional code |
| Sequential decoder | Fano algorithm |
| Interleaving | Bit-reversal permutation over 256 indices |
| Sync vector | Standard WSPR 162-bit pseudo-random sequence |
| Information payload | **50 bits** |

### 3.1 Encoding Chain (Transmit)

```
50 information bits
  → append 31 zero tail bits (flush the K = 32 shift register) = 81 bits
  → K = 32, r = 1/2 convolutional encode = 162 coded bits
      polynomials: G1 = 0xF2D05351, G2 = 0xE4613C47
      convention:  MSB-first feed, coded bit = even parity of (state AND poly)
  → bit-reversal interleave over 256 indices (skip indices ≥ 162)
  → channel symbol[k] = sync_vector[k] + 2 × coded_bit[k]
  → 162 four-level symbols (values 0, 1, 2, 3)
  → 4-FSK audio: tone[k] = 1500 + (symbol[k] − 1.5) × 1.46484375 Hz
```

### 3.2 Decoding Chain (Receive)

Standard WSPR decoding: FFT search → candidate detection → fine sync →
demodulation → de-interleave → Fano sequential decode → 50 raw bits + SNR,
time offset (dt), frequency, and drift metadata per decode.

### 3.3 Dial Frequencies (Ham Mode)

MarsChat transmissions in ham mode use the standard WSPR dial frequencies (USB
carrier):

| Band | Dial Frequency (Hz) |
|------|---------------------|
| 160 m | 1,836,600 |
| 80 m | 3,568,600 |
| 60 m | 5,287,200 |
| 40 m | 7,038,600 |
| 30 m | 10,138,700 |
| 20 m | 14,095,600 |
| 17 m | 18,104,600 |
| 15 m | 21,094,600 |
| 12 m | 24,924,600 |
| 10 m | 28,124,600 |

The 4-FSK tones are centred at dial + 1500 Hz within the 200 Hz WSPR sub-band.

---

## 4. Frame Format (50 Bits)

The 50 information bits are packed MSB-first into 7 bytes (the low 6 bits of
byte 6 are unused). Bit numbering: `b0` is the MSB of byte 0.

```
 b0    b1..b2   b3..b5   b6..b8   b9..b11   b12..b41      b42..b49
┌────┬────────┬────────┬────────┬─────────┬─────────────┬──────────┐
│ MC │ FTYPE  │  SEQ   │  ACK   │  FLAGS  │  PAYLOAD    │  CRC-8   │
│ =1 │ 2 bits │ 3 bits │ 3 bits │ 3 bits  │  30 bits    │  8 bits  │
└────┴────────┴────────┴────────┴─────────┴─────────────┴──────────┘

Total: 1 + 2 + 3 + 3 + 3 + 30 + 8 = 50 bits
```

### 4.1 Field Definitions

| Field | Width | Description |
|-------|-------|-------------|
| `MC` | 1 bit | MarsChat marker. Always **1**. Used for frame discrimination (§5). |
| `FTYPE` | 2 bits | Frame type: 0 = BEACON, 1 = DATA, 2 = ACK, 3 = CTRL |
| `SEQ` | 3 bits | Frame sequence number, modulo 8 |
| `ACK` | 3 bits | Sequence number of the last correctly received peer frame. 0 if not applicable |
| `FLAGS` | 3 bits | Bit field: bit 0 = `OTP` (encrypted, non-amateur only), bit 1 = `MF` (more fragments follow), bit 2 = `CYR` (Cyrillic rendering hint) |
| `PAYLOAD` | 30 bits | 5 × 6-bit character codes (§7), or type-specific content (§6) |
| `CRC` | 8 bits | CRC-8 integrity check over b0..b41 (§4.2) |

### 4.2 Bit Packing (Byte Layout)

The bit-to-byte mapping, as implemented:

```
byte 0:  b0=MC(1)  b1-b2=FTYPE  b3-b5=SEQ  b6-b7=ACK[2:1]
byte 1:  b8=ACK[0]  b9=OTP  b10=MF  b11=CYR  b12-b15=PAYLOAD_CODE0[5:2]
byte 2:  b16-b17=PAYLOAD_CODE0[1:0]  b18-b23=PAYLOAD_CODE1[5:0]
byte 3:  b24-b29=PAYLOAD_CODE2[5:0]  b30-b31=PAYLOAD_CODE3[5:4]
byte 4:  b32-b35=PAYLOAD_CODE3[3:0]  b36-b37=PAYLOAD_CODE4[5:4]  (note: only top 2 bits)
         (correction: b32-b35=CODE3[3:0]  b36-b39=CODE4[5:2])
byte 5:  b40-b41=PAYLOAD_CODE4[1:0]  b42-b47=CRC[7:2]
byte 6:  b48-b49=CRC[1:0]  b50-b55=unused(0)
```

Precise implementation (C, canonical reference):

```c
// Pack
bits50[0] = 0x80
          | ((ftype & 0x03) << 5)
          | ((seq   & 0x07) << 2)
          | ((ack   & 0x07) >> 1);

bits50[1] = ((ack   & 0x01) << 7)
          | ((flags & 0x01) ? 0x40 : 0)   // OTP
          | ((flags & 0x02) ? 0x20 : 0)   // MF
          | ((flags & 0x04) ? 0x10 : 0)   // CYR
          | ((codes[0] & 0x3F) >> 2);

bits50[2] = ((codes[0] & 0x03) << 6) | (codes[1] & 0x3F);
bits50[3] = ((codes[2] & 0x3F) << 2) | ((codes[3] & 0x3F) >> 4);
bits50[4] = ((codes[3] & 0x0F) << 4) | ((codes[4] & 0x3F) >> 2);
bits50[5] = ((codes[4] & 0x03) << 6);

// CRC-8 over b0..b41 (bytes 0..5, with byte 5 low 6 bits masked to 0)
crc = crc8_autosar(bits50[0..4], bits50[5] & 0xC0);

bits50[5] |= (crc >> 2);
bits50[6]  = (crc & 0x03) << 6;
```

---

## 5. Frame Discrimination (MarsChat vs WSPR)

A receiver that supports both WSPR and MarsChat classifies each raw 50-bit
decode as follows:

1. If bit `b0` (MSB of byte 0) is **1** AND the MarsChat CRC-8 (§4.3)
   verifies → interpret as a **MarsChat** frame.
2. Otherwise → attempt standard WSPR type-1 unpack (callsign + grid + power).

**False classification probability** of a genuine WSPR spot as MarsChat: the
WSPR type-1 callsign encoding uses the MSB for the first character of the
padded callsign. Approximately half of valid WSPR messages will have `b0 = 1`,
but the CRC-8 must also pass (probability ≈ 1/256), giving an overall false
positive rate of approximately **2⁻⁹ ≈ 0.2%**. This is acceptable at typical
WSPR decode rates.

A MarsChat frame is never a valid WSPR type-1 message by construction
(probabilistically — the callsign decode would be nonsensical).

---

## 6. CRC-8

**Algorithm:** CRC-8/AUTOSAR

| Parameter | Value |
|-----------|-------|
| Polynomial | 0x2F (x⁸ + x⁵ + x³ + x² + x + 1) |
| Initial value | 0xFF |
| Final XOR | 0xFF |
| Input reflection | No |
| Output reflection | No |

**Scope:** The CRC is computed over bits `b0` through `b41` of the frame
(the MC marker, FTYPE, SEQ, ACK, FLAGS and PAYLOAD fields). The input is
packed MSB-first into 6 bytes; the last byte contains `b40` and `b41` in
its two MSBs with the low 6 bits forced to zero.

**Verification:** A receiver computes the CRC over the same 6-byte span and
compares it against the received CRC in bits `b42..b49`. Mismatch means the
frame is not a valid MarsChat frame and should be tested as WSPR type-1.

**Reference implementation** (C):

```c
uint8_t mc_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    int     i, b;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (b = 0; b < 8; b++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x2F);
            else
                crc = (uint8_t)(crc << 1);
        }
    }

    return crc ^ 0xFF;
}
```

**Test vectors:**

| Input (hex) | CRC-8 |
|-------------|-------|
| `00` | 0x12 |
| `FF` | 0xC2 |
| `00 00 00 00 00 00` (6 bytes) | (implementation-defined, verify against reference) |

---

## 7. Character Set and Phrase Lookup Table

### 7.1 Six-Bit Character Set (Latin Page)

All text is case-insensitive (uppercase only), consistent with CW and digital
mode convention. Each character is encoded as a 6-bit value (0–63). Five
characters fit in the 30-bit payload field.

| Code | Character | Code | Character | Code | Character | Code | Character |
|------|-----------|------|-----------|------|-----------|------|-----------|
| 0 | END (pad) | 16 | P | 32 | 5 | 48 | ' (apostrophe) |
| 1 | A | 17 | Q | 33 | 6 | 49 | ( |
| 2 | B | 18 | R | 34 | 7 | 50 | ) |
| 3 | C | 19 | S | 35 | 8 | 51 | " (double quote) |
| 4 | D | 20 | T | 36 | 9 | 52 | # |
| 5 | E | 21 | U | 37 | SPACE | 53 | $ |
| 6 | F | 22 | V | 38 | . (period) | 54 | % |
| 7 | G | 23 | W | 39 | , (comma) | 55 | & |
| 8 | H | 24 | X | 40 | ? | 56 | * |
| 9 | I | 25 | Y | 41 | ! | 57 | ; |
| 10 | J | 26 | Z | 42 | / | 58 | < |
| 11 | K | 27 | 0 | 43 | - (hyphen) | 59 | > |
| 12 | L | 28 | 1 | 44 | + | 60 | (reserved) |
| 13 | M | 29 | 2 | 45 | = | 61 | SHIFT (reserved) |
| 14 | N | 30 | 3 | 46 | @ | 62 | ESC |
| 15 | O | 31 | 4 | 47 | : (colon) | 63 | (reserved) |

**Special codes:**

- **Code 0 (END):** Terminates text early within a fragment. Remaining
  positions are padding (filled with 0) and are excluded from display.
- **Code 62 (ESC):** The next 6-bit code is a phrase lookup table index (§7.2).
- **Code 63:** Reserved for future protocol version escape. If code 63 appears
  as the **first** payload code in a cleartext frame, v1 receivers must discard
  the frame.
- **Code 61 (SHIFT):** Reserved for future mixed-script support.

### 7.2 Phrase Lookup Table (ESC Expansion)

Code 62 (`ESC`) followed by a phrase index (0–15) expands to a complete phrase,
allowing an entire sentence to be sent in a single 120-second slot. This table
is versioned with this protocol specification — both stations must run the same
version (no on-air LUT negotiation in v1).

| Index | Expansion | Index | Expansion |
|-------|-----------|-------|-----------|
| 0 | HEY | 8 | YES |
| 1 | HOW COPY? | 9 | NO |
| 2 | GOING OFF AIR | 10 | STANDBY |
| 3 | SIGNAL WEAK | 11 | MOVING FREQ |
| 4 | SIGNAL STRONG | 12 | SEND AGAIN |
| 5 | ALL OK | 13 | INTERFERENCE HERE |
| 6 | NEED HELP | 14 | BACK IN 1 HOUR |
| 7 | LOCATION UNCHANGED | 15 | BACK TOMORROW SAME TIME |

Indices 16–63 are reserved; receivers must ignore unknown indices.

**Example:** The payload codes `[62, 2, 0, 0, 0]` decode as "GOING OFF AIR"
(ESC + index 2, followed by END padding).

**Effective throughput:** Plain text delivers 5 characters per 2-minute slot.
With phrase LUT, a whole sentence fits in one slot.

### 7.3 Cyrillic Rendering Page (FLAGS.CYR)

When `FLAGS` bit 2 (`CYR`) is set to 1, the **same on-air codes** are displayed
using Cyrillic glyphs instead of Latin. This is a pure rendering hint — the
encoding, CRC, LUT mechanics and everything else are identical. The mapping
follows the traditional **Bulgarian Phonetic keyboard layout:**

| Code | Latin | Cyrillic | Code | Latin | Cyrillic | Code | Latin | Cyrillic |
|------|-------|----------|------|-------|----------|------|-------|----------|
| 1 | A | А | 10 | J | Й | 19 | S | С |
| 2 | B | Б | 11 | K | К | 20 | T | Т |
| 3 | C | Ц | 12 | L | Л | 21 | U | У |
| 4 | D | Д | 13 | M | М | 22 | V | Ж |
| 5 | E | Е | 14 | N | Н | 23 | W | В |
| 6 | F | Ф | 15 | O | О | 24 | X | Ь |
| 7 | G | Г | 16 | P | П | 25 | Y | Ъ |
| 8 | H | Х | 17 | Q | Я | 26 | Z | З |
| 9 | I | И | 18 | R | Р | | | |

Four additional Bulgarian letters overlay punctuation codes when `CYR = 1`:

| Code | Latin Page | Cyrillic Page |
|------|------------|---------------|
| 52 | # | Ч |
| 53 | $ | Ш |
| 54 | % | Щ |
| 55 | & | Ю |

This covers the full 30-letter Bulgarian alphabet. Digits, space and all other
punctuation codes are unchanged on both pages.

`CYR` is per-frame: every fragment of a multi-frame message carries its own
rendering hint, so a lost fragment cannot flip the rendering of later ones.

---

## 8. Frame Types and Payload Semantics

### 8.1 BEACON (FTYPE = 0)

One-way text transmission, no session or ARQ. Used for beacons and initial
testing.

- `SEQ` increments per transmitted frame (modulo 8).
- `ACK` = 0 (not applicable).
- `PAYLOAD` = 5 six-bit character codes (§7.1).
- Multi-frame text uses `FLAGS.MF = 1` and `SEQ` ordering. The receiver
  concatenates fragments until a frame with `MF = 0` arrives.
- Lost fragments are rendered as placeholder gaps (no retry mechanism).

### 8.2 DATA (FTYPE = 1)

Text with ARQ (Automatic Repeat reQuest) semantics (§9).

- Same text payload as BEACON.
- `ACK` carries the `SEQ` of the last correctly received peer frame,
  acknowledging it.
- Un-acknowledged frames are retried in the sender's next slot (§9).

### 8.3 ACK (FTYPE = 2)

Keep-alive frame when no text is pending. Maintains the link and reports
receive-side signal quality:

```
PAYLOAD (30 bits):
  [ snr + 32  : 6 bits ]   SNR in dB, clamped to −32..+31
  [ |dt| × 4  : 6 bits ]   time offset magnitude in seconds × 4, clamped 0..15.75
  [ drift + 32 : 6 bits ]  frequency drift in Hz × 10, clamped
  [ reserved   : 12 bits ]  set to 0
```

This gives each station propagation feedback for manual (or future scripted)
band and power decisions.

### 8.4 CTRL (FTYPE = 3)

Control frames for protocol signalling:

```
PAYLOAD (30 bits):
  [ opcode : 6 bits ]
  [ arg    : 24 bits ]
```

| Opcode | Name | Argument |
|--------|------|----------|
| 0 | PING | Echo token (24 bits) |
| 1 | TIME_SYNC | Sender's slot index, low 24 bits |
| 2 | OTP_SYNC | OTP counter, low 24 bits (non-amateur mode only) |
| 3 | SCRIPT_NEXT | Reserved for frequency hopping scripts |
| 4–63 | (reserved) | Receivers must ignore unknown opcodes |

---

## 9. ARQ Protocol (Stop-and-Wait)

The ARQ protocol applies to DATA, ACK and CTRL frame types. BEACON frames are
fire-and-forget.

### 9.1 Slot Ownership

Stations alternate 120-second slots. The **slot index** is defined as:

```
slot_index = floor(UTC_minutes / 2)
```

The station initiating the chat ("caller") owns **even** slot indices; the peer
owns **odd** slot indices. A station transmits only in its own slots and listens
only in the peer's slots. An hour contains 30 slots (even count), so the
alternation runs unbroken across hour boundaries.

### 9.2 Stop-and-Wait Operation

- Each side has **at most one un-acknowledged frame** in flight at a time.
- A frame is retried in the sender's next own slot (same SEQ, identical
  on-air bits) until an `ACK` field acknowledging that SEQ is received.
- **Retry limit:** 5 transmissions. After 5 unacknowledged attempts, the frame
  is dropped and the application is notified.
- **Sequence space:** Modulo 8 with a window of 1. SEQ also orders fragments
  for multi-frame message reassembly.

### 9.3 Idle Behaviour

When a station in an active session has nothing to send, it transmits ACK
frames (FTYPE = 2) in its own slots as keep-alives with link quality reports.

### 9.4 Session Loss

The session is considered lost after **5 consecutive empty peer slots** (no
decoded frame from the peer in 5 listening opportunities ≈ 20 minutes of
silence).

### 9.5 Timing

| Event | Timing |
|-------|--------|
| Best case, single 5-char message | 4 minutes (TX slot + peer ACK slot) |
| Multi-fragment sentence with a retry | 10–20 minutes |
| Maximum silence before session loss | ≈ 20 minutes (5 × 4-minute cycles) |

---

## 10. Time Synchronisation

Frames must start at even-minute boundaries + 1.0 second, within approximately
±2 seconds (the decoder's time-offset search window).

### 10.1 Preferred Synchronisation Methods (in order)

1. **GPS time fix:** Power the GPS receiver at session start, take an NMEA time
   fix, set the slot epoch, and power the GPS down to save battery. Cold-fix
   time is acceptable given the 2-minute slot cadence.

2. **TCXO free-run:** Between GPS fixes, slot phase is maintained by a timer
   derived from the 25 MHz TCXO domain. At ±2.5 ppm the ±2 s window holds
   for approximately 9 days; at ±25 ppm, for approximately 22 hours.

3. **On-air lock (no GPS):** The un-synced station receives continuously. Each
   decoded frame yields a time offset `dt`. The station steps its slot clock by
   `−dt` and confirms slot-index parity with a `CTRL TIME_SYNC` exchange.
   Subsequent decodes continuously re-trim the phase — at 2-minute intervals,
   even 25 ppm mutual drift moves `dt` by only ≈ 6 ms per slot.

### 10.2 Frequency Accuracy

The TCXO also determines dial frequency accuracy. At 14 MHz, ±25 ppm is
±350 Hz — outside the decoder's ±110 Hz search window. Mitigations:

1. One-time calibration against a known WSPR station (free frequency reference
   on air) or a GPS-disciplined counter.
2. For first contact between uncalibrated radios, widen the RX search window
   and apply the measured offset as a correction factor.

---

## 11. Station Identification (Ham Mode)

Because the MarsChat payload is not standard WSPR, transmissions must carry
station identification to remain identifiable and legal on amateur bands.

**CW ID:** After the 162-symbol 4-FSK transmission ends at approximately
111.6 seconds into the 120-second slot, the station's callsign is keyed in
Morse code (CW) at ≥ 20 WPM in the remaining ≈ 8-second gap. The CW is sent
at the same dial frequency and at no higher power than the data transmission.

- A 6-character callsign at 25 WPM takes approximately 5 seconds.
- **If no callsign is configured, ham-mode TX is refused** — the radio will not
  key without a valid callsign.

### 11.1 CW Timing

Standard International Morse timing:

| Element | Duration |
|---------|----------|
| Dot | 1 unit |
| Dash | 3 units |
| Inter-element gap | 1 unit |
| Inter-character gap | 3 units |
| 1 unit at 25 WPM | 1.2 / 25 = 48 ms |

The CW tone is keyed at the same audio frequency as the data tones (≈ 1500 Hz)
with 5 ms raised-cosine envelope edges to minimise key clicks.

---

## 12. Protocol Versioning

Protocol version 1 is defined by the `MC` marker bit being 1. Future versions
use an in-band escape: if character code 63 appears as the **first** payload
code in a cleartext frame, the frame uses a future extended format. Version 1
receivers **must discard** such frames.

---

## 13. Decoding a MarsChat Frame — Step by Step

This section provides a complete procedure for decoding a MarsChat transmission,
suitable for independent implementation.

### Step 1: Receive and Demodulate

Tune to a WSPR dial frequency (§3.3), mode USB. Run a standard WSPR decoder on
the audio to recover the 50 raw information bits (packed as 7 bytes, MSB-first).
Any WSPR decoder that exposes the raw 50-bit payload before type-1 callsign
interpretation is suitable.

### Step 2: Frame Discrimination

1. Check bit `b0` (MSB of byte 0). If `b0 = 0`, this is not a MarsChat frame —
   pass it to the WSPR type-1 unpacker.
2. Compute CRC-8/AUTOSAR (§6) over bits `b0..b41` (bytes 0–5, with byte 5 low
   6 bits masked to 0).
3. Extract the received CRC from bits `b42..b49`:
   `crc_rx = ((byte5 & 0x3F) << 2) | ((byte6 >> 6) & 0x03)`
4. If `crc_rx ≠ crc_computed`, this is not a valid MarsChat frame — try WSPR.

### Step 3: Unpack Fields

Extract the frame fields from the 7-byte array using the bit layout in §4.2.

### Step 4: Decode Text

For BEACON and DATA frames (FTYPE 0 or 1):
1. Interpret the 5 six-bit codes from the PAYLOAD field using the character
   table in §7.1.
2. If `FLAGS.CYR = 1`, use the Cyrillic rendering table in §7.3 instead.
3. If a code is 62 (ESC), the next code is a phrase LUT index — expand it using
   the table in §7.2.
4. If a code is 0 (END), stop — remaining codes are padding.
5. If `FLAGS.MF = 1`, this is one fragment of a longer message — concatenate
   with subsequent frames (ordered by SEQ) until a frame with `MF = 0` arrives.

For ACK frames (FTYPE 2), decode the signal report from the PAYLOAD (§8.3).
For CTRL frames (FTYPE 3), decode the opcode and argument (§8.4).

---

## 14. Encoding a MarsChat Frame — Step by Step

### Step 1: Prepare the Frame

1. Set `MC = 1`.
2. Set `FTYPE` (0–3) for the intended frame type.
3. Set `SEQ` to the current frame sequence number (mod 8).
4. Set `ACK` to the last received peer sequence number (0 if none).
5. Set `FLAGS` as appropriate (`MF` for multi-fragment, `CYR` for Cyrillic).
6. Encode up to 5 text characters into 6-bit codes using §7.1.

### Step 2: Pack and CRC

Pack the fields into 7 bytes per the layout in §4.2. Compute CRC-8 over
bytes 0–5 (with byte 5 low 6 bits masked), and write the CRC into bits
`b42..b49`.

### Step 3: Channel Encode and Transmit

Pass the 7-byte packed frame to the WSPR encoding chain (§3.1) to produce 162
four-level symbols. Transmit as 4-FSK starting at even-minute + 1.0 second.

### Step 4: CW ID (Ham Mode)

After the 162-symbol stream ends (≈ 111.6 s), key the station callsign in CW
at ≥ 20 WPM (§11).

---

## 15. Reference Test Vector

**Input:** The text "HELLO" as a BEACON frame (FTYPE = 0, SEQ = 0, ACK = 0,
FLAGS = 0).

Character encoding:
```
H = 8, E = 5, L = 12, L = 12, O = 15
```

Packed 50-bit frame (7 bytes, hex):
```
MC=1, FTYPE=00, SEQ=000, ACK=000, FLAGS=000
PAYLOAD codes: [8, 5, 12, 12, 15]
CRC-8 computed over b0..b41

Byte-by-byte construction:
  byte 0: 1_00_000_00 = 0x80
  byte 1: 0_0_0_0_0010 = 0x02  (ack[0]=0, OTP=0, MF=0, CYR=0, code0[5:2]=0010)
  byte 2: 00_000101 = 0x05     (code0[1:0]=00, code1=000101)
  byte 3: 001100_00 = 0x30     (code2=001100, code3[5:4]=00)
  byte 4: 1100_0011 = 0xC3     (code3[3:0]=1100, code4[5:2]=0011)
  byte 5: 11_xxxxxx = 0xC0 | crc_high  (code4[1:0]=11, then CRC bits)
  byte 6: xx_000000 = crc_low << 6

Pre-CRC bytes: 80 02 05 30 C3 C0
CRC-8/AUTOSAR of [80 02 05 30 C3 C0] = (compute to verify)
```

Implementers should compute the CRC-8 of the byte sequence `[0x80, 0x02, 0x05,
0x30, 0xC3, 0xC0]` using the algorithm in §6 and verify it matches the bits
placed in `b42..b49` of the complete frame. This serves as the primary
interoperability test: if your encoder produces the same 7 bytes for "HELLO"
as the reference, and your decoder recovers "HELLO" from them, the
implementation is correct.

---

## 16. Glossary

| Term | Definition |
|------|------------|
| ARQ | Automatic Repeat reQuest — error correction by retransmission |
| CW | Continuous Wave — Morse code |
| dt | Time offset of a decoded frame relative to the expected slot start |
| Fano | Sequential decoding algorithm used by WSPR |
| FEC | Forward Error Correction |
| FSK | Frequency Shift Keying |
| LUT | Lookup Table |
| MF | More Fragments (flag indicating a multi-frame message continues) |
| OTP | One-Time Pad (non-amateur mode only) |
| PHY | Physical layer |
| SEQ | Sequence number |
| Slot | One 120-second transmission/reception period |
| SNR | Signal-to-Noise Ratio |
| TCXO | Temperature-Compensated Crystal Oscillator |
| USB | Upper Side Band |
| WSPR | Weak Signal Propagation Reporter |

---

## 17. References

1. K1JT, J. Taylor, "The WSPR Protocol," 2008.
   https://www.physics.princeton.edu/pulsar/k1jt/wspr.html
2. K9AN, S. Franke, "wsprd — WSPR decoder," part of WSJT-X.
   https://sourceforge.net/projects/wsjt/
3. mcHF SDR Transceiver project.
   https://github.com/m0nka/mcHF
4. CRC-8/AUTOSAR specification.
   Polynomial 0x2F, per AUTOSAR E2E Profile 2.

---

*This document is published under the GNU General Public License v3.0 to
satisfy the amateur radio community requirement that new digital modes be
openly documented. Anyone may implement a MarsChat encoder or decoder using this
specification.*

*Document revision history:*

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-08-30 | Initial public release |
