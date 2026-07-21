/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_session.h                                                   **
**  Description:	MarsChat session layer - slot ownership and stop-and-wait      **
**					ARQ (PROTOCOL.md section 7). Pure state machine: it is         **
**					handed a slot parity and decides listen/transmit, it never     **
**					touches the RTC, the radio or FreeRTOS.                        **
**					No OS or HAL dependencies - host buildable for the test rig    **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MC_SESSION_H
#define __MC_SESSION_H

#include <stdint.h>

#include "mc_frame.h"

// Slot ownership - stations alternate 120 s slots, the caller owns the
// even slot indices (slot index = UTC minutes / 2), the peer the odd
#define MC_ROLE_CALLER			0
#define MC_ROLE_PEER			1

// Session state
#define MC_SESS_OFF				0					// not in a session
#define MC_SESS_ACTIVE			1
#define MC_SESS_LOST			2					// peer silent for MC_SESSION_LOST_SLOTS

// Action the caller must perform for the slot just started
#define MC_SLOT_IDLE			0					// nothing to do
#define MC_SLOT_TX_NEW			1					// transmit - offered payload consumed
#define MC_SLOT_LISTEN			2					// peer's slot - arm the receiver
#define MC_SLOT_TX_AGAIN		3					// transmit - retry or keep-alive, queue untouched

#define MC_SLOT_IS_TX(a)		(((a) == MC_SLOT_TX_NEW) || ((a) == MC_SLOT_TX_AGAIN))

// Outcome of the last frame that left the tx window (mc_session_take_result)
#define MC_XFER_NONE			0
#define MC_XFER_DELIVERED		1					// peer acknowledged it
#define MC_XFER_FAILED			2					// retry limit hit, frame dropped

// Retries of one un-acknowledged frame (its own slot each time), and the
// number of consecutive silent peer slots that ends the session
#ifndef MC_ARQ_RETRIES
#define MC_ARQ_RETRIES			5
#endif

#ifndef MC_SESSION_LOST_SLOTS
#define MC_SESSION_LOST_SLOTS	5
#endif

typedef struct
{
	uint8_t		state;								// MC_SESS_xxx
	uint8_t		role;								// MC_ROLE_xxx

	uint8_t		tx_seq;								// seq stamped on the next new frame
	uint8_t		last_rx_seq;						// ack field we send back
	uint8_t		have_rx;							// last_rx_seq holds a real frame

	uint8_t		pending;							// a frame is in flight, un-acked
	uint8_t		retries;							// transmissions of it so far
	MC_FRAME	pending_frame;						// re-sent bit identical (PROTOCOL 9)

	uint8_t		awaiting_rx;						// listened, nothing decoded yet
	uint8_t		empty_slots;						// consecutive silent peer slots

	uint8_t		last_result;						// MC_XFER_xxx, pending pickup
	uint8_t		last_result_seq;

} MC_SESSION;

// Enter a session in the given role. Sequence numbers restart; anything
// still in flight from a previous session is dropped without a result
void	mc_session_start(MC_SESSION *s, uint8_t role);
void	mc_session_stop(MC_SESSION *s);

// Slot boundary. slot_parity is (slot index & 1) of the slot that is
// starting; new_codes/new_flags describe the next queued payload, or
// new_codes == NULL when the caller has nothing new to send.
// Returns MC_SLOT_xxx; out is filled for both transmit actions.
//
// MC_SLOT_TX_NEW is the only return that consumes the offered payload -
// a retransmission or an idle keep-alive leaves the queue untouched
uint8_t	mc_session_slot(MC_SESSION *s, uint8_t slot_parity,
						const uint8_t *new_codes, uint8_t new_flags,
						MC_FRAME *out);

// Frame decoded in a listening slot. Returns 1 when it carries payload
// the application has not seen before (show it), 0 when it is an ACK
// keep-alive or a retransmission of the frame we already took
uint8_t	mc_session_on_rx(MC_SESSION *s, const MC_FRAME *f);

// Pick up (and clear) the delivery result of the last frame that left
// the tx window. Returns MC_XFER_NONE when there is nothing new
uint8_t	mc_session_take_result(MC_SESSION *s, uint8_t *seq);

// True when this slot parity belongs to us
uint8_t	mc_session_owns_slot(const MC_SESSION *s, uint8_t slot_parity);

#endif
