/**
 * meshtastic.h — Thin Meshtastic protocol library for RadioLib.
 *
 * Include this single header to get everything. Define MESH_CRYPTO_USE_MBEDTLS=1
 * before including if you're on ESP-IDF (recommended for hardware AES).
 *
 * All protocol parameters extracted from meshtastic/firmware @ develop (2025-03).
 * See individual headers for exact source references.
 *
 * License: MIT (this is a clean-room reimplementation, not a fork)
 */
#pragma once

#include "meshtastic_config.h"
#include "meshtastic_packet.h"
#include "meshtastic_crypto.h"
#include "meshtastic_channel.h"
#include "meshtastic_pb.h"
#include "meshtastic_pki.h"
#include "meshtastic_radio.h"
