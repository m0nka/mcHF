/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#ifndef __UNIT_CONFIG_H
#define __UNIT_CONFIG_H

// Time for initial time lock(GNGGA) when LCD uses GPIO or PWM control:
// gpio - 18210mS
// pwm  - 24252mS
//
// Run the backlight in GPIO mode to limit GPS noise
#define SWITCH_TO_GPIO_CNTR

// Un-comment to arm the WSPR monitor automatically at boot (bench testing,
// no UI hook needed) - captures every even minute and decodes to SD
//
//#define WSPR_MONITOR_AUTO_START

// Un-comment for the closed-loop bench test: sends a MarsChat "HELLO"
// beacon over the CLK1 loopback injector at every even minute +1s.
// Arm WSPR_MONITOR_AUTO_START too - the radio then captures and decodes
// its own signal (no emissions, PA never keyed)
//
//#define MARSCHAT_LOOPBACK_BEACON

// Un-comment for the radiated tx bench test: sends the same "HELLO"
// beacon through the REAL tx chain (M4 symbol streamer keys the tx
// exciter) every even minute +1s. Tx mixer bench only until the PA
// exists. Set the dial and USB mode from the UI first. Mutually
// exclusive with MARSCHAT_LOOPBACK_BEACON
//
//#define MARSCHAT_RADIATED_BEACON

// Un-comment for the SINGLE-RADIO two-station test: a second, fully
// simulated MarsChat station runs inside this firmware in the opposite
// role, and the two converse through the real session/ARQ code over the
// CLK1 loopback injector. Stop-and-wait ARQ means exactly one station
// keys per 120 s slot, so the one injector and one decoder are simply
// time-shared by slot ownership - no emissions, PA never keyed. Start a
// session from the chat UI (CALLER or PEER); the emulated peer adopts the
// other role automatically and starts talking. Mutually exclusive with
// both beacon modes and needs a session, not WSPR_MONITOR_AUTO_START
//
//#define MARSCHAT_LOOPBACK_PEER

// Simulated packet loss for the loopback peer test, percent. A clean
// loopback channel decodes every frame (~-6 dB), so nothing is ever lost
// and the ARQ retry / session-lost paths never run. Raise this (e.g. 25)
// to make each station randomly skip an injection - the other side then
// hears silence and the retry / delivered-after-N-tries / session-lost
// logic gets exercised. 0 = clean happy-path conversation
#define MARSCHAT_LOOPBACK_LOSS_PCT		0

// CW id appended to every radiated MarsChat tx (ham legality - our
// payload is not standard WSPR). Empty string = no CW id segment
#define MARSCHAT_CW_ID					""
#define MARSCHAT_CW_WPM					25

// Uncomment to keep the pre-session bench behaviour available: with no
// session running, a message sent from the chat UI is transmitted as
// soon as the exciter is free instead of waiting for a slot. Normally
// off - SEND starts a session, and every burst is slot timed
//#define MARSCHAT_IMMEDIATE_SEND

// Second of the odd minute at which the slot scheduler arms the receiver
// for an upcoming peer slot - the WSPR monitor starts its capture on the
// even minute itself, so the request has to be in before that
#define MARSCHAT_ARM_SEC				50

// Depth of the decoded-frame queue drained by the chat UI dialog
#define MARSCHAT_RX_QUEUE_LEN			8

// Depth of the pending outgoing chunk queue, in MC_PAYLOAD_CHARS-sized
// chunks (5 chars each) - a slot-mode tx burst runs ~110 s+ per chunk,
// so a whole free-text message is split and queued here rather than
// sent (or dropped) all at once. 20 chunks = up to 100 buffered chars
#define MARSCHAT_TX_QUEUE_LEN			20

#endif
