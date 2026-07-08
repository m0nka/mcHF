# WSPR subsystem

WSPR decoder and background monitor for the mcHF. The CM4 baseband core taps
the receiver audio, streams it to the CM7 application core over the ICC link,
the CM7 saves it as a capture file on the SD card and decodes it after every
two minute rx cycle.

Status 2026-07-08: decoder verified on host (claude/wspr_test, decodes to
-26 dB SNR), capture pipeline builds clean on both cores, **not yet tested
on hardware**.

## Files

| File | Purpose |
|------|---------|
| `wspr_proc.c/h` | FreeRTOS task: even-minute capture scheduler, SD card writer, decode requests |
| `wspr_decoder.c/h` | WSPR decoder front end (12 kHz PCM in, spots out) |
| `wspr_fano.c/h` | Fano sequential decoder for the K=32 r=1/2 convolutional code |
| `baseband/uhsdr/mchf-eclipse/drivers/icc/icc_wspr.c/h` | M4 side: audio tap, decimation, chunk ring |

## Data flow

```
M4 (baseband, uhsdr)                        M7 (app proc)
--------------------                        -------------
SAI DMA block handler
  uhsdr_hw_i2s.c taps line level rx
  audio AFTER the UHSDR chain wrote
  the audio out buffer (fixed LINE_OUT
  scaling - independent of the volume
  knob; silence during TX keeps the
  time base intact)
        |
  icc_wspr_collect()
  boxcar-4 decimation 48k stereo
  -> 12 kHz mono
        |
  chunk ring, 16 x 512 samples              icc task (osPriorityAboveNormal)
  (~680 ms of slack)                          while a capture runs it wakes every
        |                                     ICC_WSPR_POLL_TIME (20 ms) instead of
  ICC_WSPR_READ response      <------------   sleeping forever, drains chunks with
  0x9E + flags + len + <=1024B PCM   ---->    ICC_WSPR_READ (memcpy only, no FatFS!)
                                                |
                                              wspr_capture_push()
                                              64 KB staging ring in .wspr_mem
                                              (~2.7 s of slack for SD card stalls)
                                                |  WSPR_NOTIFY_DATA
                                              wspr task (tskIDLE_PRIORITY)
                                              owns ALL SD card I/O:
                                              drains ring -> 0://wspr/capture.raw,
                                              closes at 114 s, decodes, appends
                                              spots to 0://wspr/decodes.txt
```

## Wire protocol (common/mchf_icc_def.h)

- `ICC_WSPR_START` (13) - reset ring, begin capture. Idempotent, the M7
  retries the request until acknowledged (the notification value scheme on
  the icc task can lose a race against a spectrum broadcast).
- `ICC_WSPR_STOP` (14) - end capture, buffered chunks stay readable.
- `ICC_WSPR_READ` (15) - response: `[0]` = 0x9E signature, `[1]` = flags
  (`ACTIVE`, `OVERRUN`), `[2..3]` = payload bytes LE (0 = ring empty),
  `[4..]` = 16 bit signed LE mono PCM @ 12 kHz. One 1024 byte chunk per
  call, fits the 1100 byte rpmsg buffers.
- Safety: the M4 auto-stops after 120 s if the stop command is ever lost.

## Capture scheduler (wspr_proc.c)

Arm with `wspr_proc_monitor_set(1)` (any task - UI hook not wired yet), or
uncomment `WSPR_MONITOR_AUTO_START` in `config/proc_conf.h` to arm at boot
for bench testing.

State machine: WAIT (poll RTC every 100 ms) -> at second :00 of an even
minute open the capture file, remember the dial frequency (active VFO) ->
STARTING (retry start request, give up after 5 s) -> CAPTURE (114 s;
WSPR tx is 110.6 s starting 1 s past the even minute) -> stop, drain the
tails, close, decode inline -> WAIT. A cycle that begins while a decode is
still crunching is skipped - the machine only arms at second zero.

Capture file contract: `0://wspr/capture.raw`, 16 bit signed LE mono PCM,
12000 Hz, no header, recorded from the start of an even minute.

## Fixes that rode along (2026-07-08)

- `WSPR_PROC_PRIORITY` was `osPriorityLow` (-3), which wraps through raw
  `xTaskCreate` to the HIGHEST FreeRTOS priority - a decode would have
  frozen the whole radio for seconds every cycle. Now `tskIDLE_PRIORITY`.
- The target `snprintf`/`printf` is `common/print_f.c`, which drops the
  whole line on any specifier outside `%d %u %x %s %c` - the decode log
  formatter used `%ld/%lu` and would have written garbage on hardware.
  All formats now use `%d/%u` with casts. Watch for this in new code.

## Bench test checklist

1. Uncomment `WSPR_MONITOR_AUTO_START` in `config/proc_conf.h`, build, flash.
2. RTC must be set (GPS sync or manually) - the scheduler works off it.
3. Tune USB to a WSPR dial frequency, e.g. 14.0956 MHz (20 m), standard
   2.3 kHz filter.
4. Watch the debug UART: "wspr: capture start" at the even minute,
   "capture done, 2736000 bytes, overrun(0)", then the decode report.
5. Spots land in `0://wspr/decodes.txt`:
   `YYMMDD HHMM SNR DT FREQ DRIFT CALL GRID PWR`.

## Open items

- Only the **uhsdr** baseband implements the M4 side. On the clint core the
  start request never acks and the M7 gives up cleanly after 5 s.
- No UI hook for `wspr_proc_monitor_set()` yet.
- Logged spot frequency assumes audio 0 Hz = active VFO frequency - verify
  against a known spot on air (NCO/iq_freq_mode offset assumed transparent).
- Capture level/clipping at LINE_OUT scaling and decode duration on target
  are unmeasured.
