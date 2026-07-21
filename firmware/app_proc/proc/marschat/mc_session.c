/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_session.c                                                   **
**  Description:	MarsChat session layer - slot ownership and stop-and-wait ARQ  **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Everything here is decided at slot boundaries, so the whole layer is a
// function of (slot parity, queued payload, frames decoded since the last
// slot). No timers, no radio, no OS - marschat_proc.c owns the RTC slot
// clock and the transmitter, this owns the protocol.
//
// Sequence numbers are mod 8 with a window of one (PROTOCOL.md section 7).
// last_rx_seq starts at 7 as a "nothing heard from the peer yet" value:
// our own first frame goes out with seq 0, so a peer's opening frame -
// which necessarily carries its own uninitialised ack - cannot be mistaken
// for an acknowledgement of it.
//
#include <string.h>

#include "mc_session.h"

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_start
//* Object              : enter a session in the given role
//*----------------------------------------------------------------------------
void mc_session_start(MC_SESSION *s, uint8_t role)
{
	if(s == NULL)
		return;

	memset(s, 0, sizeof(MC_SESSION));

	s->state		= MC_SESS_ACTIVE;
	s->role			= (role == MC_ROLE_PEER) ? MC_ROLE_PEER : MC_ROLE_CALLER;
	s->tx_seq		= 0;
	s->last_rx_seq	= 7;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_stop
//* Object              : leave the session, drop anything in flight
//*----------------------------------------------------------------------------
void mc_session_stop(MC_SESSION *s)
{
	if(s == NULL)
		return;

	s->state	= MC_SESS_OFF;
	s->pending	= 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_owns_slot
//* Object              : is this slot parity ours ?
//*----------------------------------------------------------------------------
uint8_t mc_session_owns_slot(const MC_SESSION *s, uint8_t slot_parity)
{
	if(s == NULL)
		return 0;

	return ((slot_parity & 1) == s->role) ? 1 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_take_result
//* Object              : pick up and clear the last delivery result
//*----------------------------------------------------------------------------
uint8_t mc_session_take_result(MC_SESSION *s, uint8_t *seq)
{
	uint8_t r;

	if(s == NULL)
		return MC_XFER_NONE;

	r = s->last_result;

	if(seq != NULL)
		*seq = s->last_result_seq;

	s->last_result = MC_XFER_NONE;

	return r;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_slot
//* Object              : slot boundary decision - listen, transmit or idle
//* Notes    			: a listening slot that produced no decode is only
//*						: counted when our own next slot starts, by which
//*						: time the decoder has long finished (it runs in
//*						: the gap at the end of the capture)
//*----------------------------------------------------------------------------
uint8_t mc_session_slot(MC_SESSION *s, uint8_t slot_parity,
						const uint8_t *new_codes, uint8_t new_flags,
						MC_FRAME *out)
{
	if((s == NULL) || (out == NULL))
		return MC_SLOT_IDLE;

	if(s->state != MC_SESS_ACTIVE)
		return MC_SLOT_IDLE;

	// Peer's slot - listen, and remember we are owed a decode
	if(!mc_session_owns_slot(s, slot_parity))
	{
		s->awaiting_rx = 1;
		return MC_SLOT_LISTEN;
	}

	// Our slot. Did the peer's last one stay silent ?
	if(s->awaiting_rx)
	{
		s->awaiting_rx = 0;
		s->empty_slots++;

		if(s->empty_slots >= MC_SESSION_LOST_SLOTS)
		{
			s->state = MC_SESS_LOST;

			if(s->pending)
			{
				s->pending			= 0;
				s->last_result		= MC_XFER_FAILED;
				s->last_result_seq	= s->pending_frame.seq;
			}

			return MC_SLOT_IDLE;
		}
	}

	memset(out, 0, sizeof(MC_FRAME));

	// A frame still waiting for its acknowledgement owns the slot
	if(s->pending)
	{
		if(s->retries >= MC_ARQ_RETRIES)
		{
			// Out of retries - drop it and tell the UI. The session
			// stays up, the next queued payload gets its own chance
			s->pending			= 0;
			s->last_result		= MC_XFER_FAILED;
			s->last_result_seq	= s->pending_frame.seq;
		}
		else
		{
			// Re-sent bit identical, including the ack field - the OTP
			// keystream is consumed per frame, not per transmission
			s->retries++;
			*out = s->pending_frame;

			return MC_SLOT_TX_AGAIN;
		}
	}

	// Something new to send ?
	if(new_codes != NULL)
	{
		out->ftype	= MC_FTYPE_DATA;
		out->seq	= s->tx_seq;
		out->ack	= s->last_rx_seq;
		out->flags	= new_flags;
		memcpy(out->codes, new_codes, MC_PAYLOAD_CHARS);

		s->tx_seq			= (uint8_t)((s->tx_seq + 1) & 7);
		s->pending_frame	= *out;
		s->pending			= 1;
		s->retries			= 1;

		return MC_SLOT_TX_NEW;
	}

	// Idle in an active session - keep the link alive and repeat the ack
	// (fire and forget, an ACK frame is never itself acknowledged)
	out->ftype	= MC_FTYPE_ACK;
	out->seq	= s->tx_seq;
	out->ack	= s->last_rx_seq;

	return MC_SLOT_TX_AGAIN;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_session_on_rx
//* Object              : frame decoded in a listening slot
//* Notes    			: the seq of a pure ACK frame is the sender's next
//*						: unused number, echoing it would acknowledge a
//*						: frame we have not seen - only payload carrying
//*						: frames advance our ack field.
//*						: A peer retransmits until it hears our ack, so
//*						: the same payload frame legitimately arrives
//*						: several times - it must be acknowledged every
//*						: time but handed to the application only once
//*----------------------------------------------------------------------------
uint8_t mc_session_on_rx(MC_SESSION *s, const MC_FRAME *f)
{
	uint8_t	fresh = 0;

	if((s == NULL) || (f == NULL))
		return 0;

	if(s->state != MC_SESS_ACTIVE)
		return 0;

	s->awaiting_rx	= 0;
	s->empty_slots	= 0;

	if(f->ftype != MC_FTYPE_ACK)
	{
		// Stop-and-wait: the peer cannot start a new frame before ours
		// acknowledged the previous one, so a repeated seq is a repeat
		if((!s->have_rx) || ((f->seq & 7) != s->last_rx_seq))
			fresh = 1;

		s->last_rx_seq	= (uint8_t)(f->seq & 7);
		s->have_rx		= 1;
	}

	// Does it acknowledge what we have in flight ?
	if((s->pending) && ((f->ack & 7) == s->pending_frame.seq))
	{
		s->pending			= 0;
		s->last_result		= MC_XFER_DELIVERED;
		s->last_result_seq	= s->pending_frame.seq;
	}

	return fresh;
}
