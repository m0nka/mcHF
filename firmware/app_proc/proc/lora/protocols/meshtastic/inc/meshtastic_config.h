/**
 * meshtastic_config.h — Modem presets, region definitions, and role enums.
 *
 * All values extracted from meshtastic/firmware @ develop (2025-03):
 *   - MeshRadio.h:100-149   (modem presets)
 *   - RadioInterface.cpp:41-215 (region table)
 *   - DisplayFormatters.cpp:3-44 (preset canonical names)
 *
 * Part of meshtastic-lite — a thin Meshtastic protocol library
 * for use with RadioLib on ESP32/ESP-IDF platforms.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

// ─── Modem Presets ─────────────────────────────────────────────────────────────

enum MeshModemPreset : uint8_t {
    MODEM_LONG_FAST = 0,     // SF11, BW250, CR4/5  (default)
    MODEM_LONG_SLOW,         // SF12, BW125, CR4/8
    MODEM_LONG_MODERATE,     // SF11, BW125, CR4/8
    MODEM_LONG_TURBO,        // SF11, BW500, CR4/8
    MODEM_MEDIUM_FAST,       // SF9,  BW250, CR4/5
    MODEM_MEDIUM_SLOW,       // SF10, BW250, CR4/5
    MODEM_SHORT_FAST,        // SF7,  BW250, CR4/5
    MODEM_SHORT_SLOW,        // SF8,  BW250, CR4/5
    MODEM_SHORT_TURBO,       // SF7,  BW500, CR4/5
    MODEM_PRESET_COUNT
};

struct ModemParams {
    float    bw_khz;         // bandwidth in kHz
    uint8_t  sf;             // spreading factor 7-12
    uint8_t  cr;             // coding rate denominator (5 = 4/5, 8 = 4/8)
};

/**
 * Get LoRa modem parameters for a given preset.
 * Values from MeshRadio.h modemPresetToParams(), sub-GHz (wideLora=false).
 */
static inline ModemParams meshPresetParams(MeshModemPreset preset) {
    switch (preset) {
        case MODEM_SHORT_TURBO:   return { 500.0f, 7, 5 };
        case MODEM_SHORT_FAST:    return { 250.0f, 7, 5 };
        case MODEM_SHORT_SLOW:    return { 250.0f, 8, 5 };
        case MODEM_MEDIUM_FAST:   return { 250.0f, 9, 5 };
        case MODEM_MEDIUM_SLOW:   return { 250.0f, 10, 5 };
        case MODEM_LONG_TURBO:    return { 500.0f, 11, 8 };
        case MODEM_LONG_MODERATE: return { 125.0f, 11, 8 };
        case MODEM_LONG_SLOW:     return { 125.0f, 12, 8 };
        default: /* LONG_FAST */  return { 250.0f, 11, 5 };
    }
}

/**
 * Canonical channel name for a preset (used in frequency hashing).
 * From DisplayFormatters.cpp — these MUST match exactly for interop.
 */
static inline const char* meshPresetName(MeshModemPreset preset) {
    switch (preset) {
        case MODEM_SHORT_TURBO:   return "ShortTurbo";
        case MODEM_SHORT_FAST:    return "ShortFast";
        case MODEM_SHORT_SLOW:    return "ShortSlow";
        case MODEM_MEDIUM_FAST:   return "MediumFast";
        case MODEM_MEDIUM_SLOW:   return "MediumSlow";
        case MODEM_LONG_TURBO:    return "LongTurbo";
        case MODEM_LONG_MODERATE: return "LongMod";
        case MODEM_LONG_SLOW:     return "LongSlow";
        default: /* LONG_FAST */  return "LongFast";
    }
}

// ─── Region Definitions ────────────────────────────────────────────────────────

enum MeshRegion : uint8_t {
    REGION_US = 0,
    REGION_EU_433,
    REGION_EU_868,
    REGION_CN,
    REGION_JP,
    REGION_ANZ,
    REGION_ANZ_433,
    REGION_KR,
    REGION_TW,
    REGION_IN,
    REGION_NZ_865,
    REGION_TH,
    REGION_RU,
    REGION_UNSET,   // Falls back to US band plan
    REGION_COUNT
};

struct RegionDef {
    MeshRegion region;
    float      freq_start;    // MHz
    float      freq_end;      // MHz
    uint8_t    duty_cycle;    // percent (100 = no limit)
    uint8_t    power_limit;   // dBm (0 = use default 17)
    const char *name;
};

// Subset of commonly-used regions. From RadioInterface.cpp:41-215.
// Extend as needed; these are the ones you'll encounter in practice.
static const RegionDef MESH_REGIONS[] = {
    { REGION_US,      902.0f,  928.0f,  100, 30, "US"     },
    { REGION_EU_433,  433.0f,  434.0f,  10,  10, "EU_433" },
    { REGION_EU_868,  869.4f,  869.65f, 10,  27, "EU_868" },
    { REGION_CN,      470.0f,  510.0f,  100, 19, "CN"     },
    { REGION_JP,      920.5f,  923.5f,  100, 13, "JP"     },
    { REGION_ANZ,     915.0f,  928.0f,  100, 30, "ANZ"    },
    { REGION_ANZ_433, 433.05f, 434.79f, 100, 14, "ANZ433" },
    { REGION_KR,      920.0f,  923.0f,  100, 23, "KR"     },
    { REGION_TW,      920.0f,  925.0f,  100, 27, "TW"     },
    { REGION_IN,      865.0f,  867.0f,  100, 30, "IN"     },
    { REGION_NZ_865,  864.0f,  868.0f,  100, 36, "NZ_865" },
    { REGION_TH,      920.0f,  925.0f,  100, 16, "TH"     },
    { REGION_RU,      868.7f,  869.2f,  100, 20, "RU"     },
    { REGION_UNSET,   902.0f,  928.0f,  100, 30, "UNSET"  },
};

static inline const RegionDef* meshGetRegion(MeshRegion r) {
    for (size_t i = 0; i < sizeof(MESH_REGIONS) / sizeof(MESH_REGIONS[0]); i++) {
        if (MESH_REGIONS[i].region == r) return &MESH_REGIONS[i];
    }
    return &MESH_REGIONS[sizeof(MESH_REGIONS) / sizeof(MESH_REGIONS[0]) - 1]; // UNSET
}

// ─── Device Roles (subset we support) ──────────────────────────────────────────

enum MeshRole : uint8_t {
    ROLE_CLIENT      = 0,   // Normal node: TX + RX, rebroadcast with SNR-weighted delay
    ROLE_CLIENT_MUTE = 1,   // RX only, no rebroadcast, no TX
    ROLE_ROUTER_LATE = 2,   // TX + RX, rebroadcast with LONG delay (after routers)
};

// ─── Radio Constants ───────────────────────────────────────────────────────────

static constexpr uint8_t  MESH_SYNC_WORD       = 0x2B;    // RadioLibInterface.h:84
static constexpr uint16_t MESH_PREAMBLE_LENGTH  = 16;      // RadioInterface.h:98
static constexpr uint8_t  MESH_HOP_MAX          = 7;       // MeshTypes.h:38
static constexpr uint8_t  MESH_HOP_RELIABLE     = 3;       // MeshTypes.h:41
static constexpr uint16_t MESH_MAX_PAYLOAD      = 255;     // RadioInterface.h:20
static constexpr uint8_t  MESH_HEADER_LEN       = 16;      // RadioInterface.h:21

// CSMA/CA parameters (RadioInterface.h:95-103)
static constexpr uint8_t  MESH_CW_MIN           = 3;
static constexpr uint8_t  MESH_CW_MAX           = 8;
static constexpr uint8_t  MESH_NUM_SYM_CAD      = 2;

