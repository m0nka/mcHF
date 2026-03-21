/**
 * meshtastic_pb.h — Minimal protobuf wire-format decoder for Meshtastic payloads.
 *
 * Zero external dependencies. Decodes the subset of messages needed for
 * a Meshtastic receiver: Data envelope, then TEXT, POSITION, NODEINFO, TELEMETRY.
 *
 * Protobuf field definitions derived from meshtastic/protobufs (mesh.proto,
 * telemetry.proto) via the generated nanopb headers.
 *
 * Part of meshtastic-lite.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

// ─── Port Numbers (portnums.pb.h) ──────────────────────────────────────────────

enum MeshPortNum : uint16_t {
    PORT_UNKNOWN          = 0,
    PORT_TEXT_MESSAGE      = 1,
    PORT_POSITION          = 3,
    PORT_NODEINFO          = 4,
    PORT_ROUTING           = 5,
    PORT_ADMIN             = 6,
    PORT_WAYPOINT          = 8,
    PORT_TELEMETRY         = 67,
    PORT_TRACEROUTE        = 70,
    PORT_NEIGHBORINFO      = 71,
};

// ─── Protobuf Wire Format Primitives ───────────────────────────────────────────

/**
 * Lightweight read cursor for decoding protobuf wire format.
 */
struct PbCursor {
    const uint8_t *buf;
    size_t pos;
    size_t len;

    bool exhausted() const { return pos >= len; }
    size_t remaining() const { return (pos < len) ? (len - pos) : 0; }

    bool readByte(uint8_t *out) {
        if (pos >= len) return false;
        *out = buf[pos++];
        return true;
    }

    bool readVarint(uint64_t *out) {
        *out = 0;
        int shift = 0;
        uint8_t b;
        do {
            if (!readByte(&b)) return false;
            *out |= (uint64_t)(b & 0x7F) << shift;
            shift += 7;
            if (shift > 63) return false; // overflow
        } while (b & 0x80);
        return true;
    }

    bool readFixed32(uint32_t *out) {
        if (remaining() < 4) return false;
        memcpy(out, buf + pos, 4);
        pos += 4;
        return true;
    }

    bool readFixed64(uint64_t *out) {
        if (remaining() < 8) return false;
        memcpy(out, buf + pos, 8);
        pos += 8;
        return true;
    }

    bool readFloat(float *out) {
        uint32_t bits;
        if (!readFixed32(&bits)) return false;
        memcpy(out, &bits, 4);
        return true;
    }

    /**
     * Read a protobuf field tag. Returns field number and wire type.
     */
    bool readTag(uint32_t *field_num, uint8_t *wire_type) {
        uint64_t tag;
        if (!readVarint(&tag)) return false;
        *wire_type = tag & 0x07;
        *field_num = tag >> 3;
        return true;
    }

    /**
     * Skip a field value based on wire type.
     */
    bool skipField(uint8_t wire_type) {
        switch (wire_type) {
            case 0: { // varint
                uint64_t dummy;
                return readVarint(&dummy);
            }
            case 1: { // 64-bit
                if (remaining() < 8) return false;
                pos += 8; return true;
            }
            case 2: { // length-delimited
                uint64_t slen;
                if (!readVarint(&slen)) return false;
                if (remaining() < slen) return false;
                pos += slen; return true;
            }
            case 5: { // 32-bit
                if (remaining() < 4) return false;
                pos += 4; return true;
            }
            default:
                return false; // unsupported wire type
        }
    }

    /**
     * Read a length-delimited field's length prefix, returning a sub-cursor.
     */
    bool readLengthDelimited(PbCursor *sub) {
        uint64_t slen;
        if (!readVarint(&slen)) return false;
        if (remaining() < slen) return false;
        sub->buf = buf + pos;
        sub->pos = 0;
        sub->len = (size_t)slen;
        pos += slen;
        return true;
    }

    /**
     * Read a length-delimited field into a buffer.
     */
    bool readBytes(uint8_t *out, size_t max_len, size_t *out_len) {
        uint64_t slen;
        if (!readVarint(&slen)) return false;
        if (remaining() < slen) return false;
        size_t copy_len = (slen > max_len) ? max_len : (size_t)slen;
        memcpy(out, buf + pos, copy_len);
        *out_len = copy_len;
        pos += slen;
        return true;
    }

    /**
     * Read a string field (length-delimited bytes interpreted as UTF-8).
     */
    bool readString(char *out, size_t max_len, size_t *out_len) {
        size_t slen;
        if (!readBytes((uint8_t *)out, max_len - 1, &slen)) return false;
        out[slen] = '\0';
        *out_len = slen;
        return true;
    }
};

static inline PbCursor pbCursor(const uint8_t *buf, size_t len) {
    return { buf, 0, len };
}

// ─── Meshtastic Data Envelope ──────────────────────────────────────────────────

/**
 * Decoded meshtastic.Data protobuf.
 * Fields: portnum(1), payload(2), want_response(3), dest(4), source(5),
 *         request_id(6), reply_id(7), emoji(8), bitfield(9)
 */
struct MeshData {
    MeshPortNum portnum;
    uint8_t     payload[240];     // inner payload bytes
    size_t      payload_len;
    bool        want_response;
    uint32_t    dest;
    uint32_t    source;
    uint32_t    request_id;
};

/**
 * Decode a meshtastic.Data protobuf from raw bytes.
 * Returns true on success. Validates that portnum != 0 (UNKNOWN).
 *
 * This is the validation step equivalent to Router.cpp:499-503.
 */
static inline bool meshDecodeData(const uint8_t *buf, size_t len, MeshData *out) {
    memset(out, 0, sizeof(MeshData));
    PbCursor c = pbCursor(buf, len);

    while (!c.exhausted()) {
        uint32_t field; uint8_t wtype;
        if (!c.readTag(&field, &wtype)) break;

        switch (field) {
            case 1: { // portnum (varint)
                uint64_t v;
                if (!c.readVarint(&v)) return false;
                out->portnum = (MeshPortNum)(v & 0xFFFF);
                break;
            }
            case 2: { // payload (bytes)
                if (!c.readBytes(out->payload, sizeof(out->payload), &out->payload_len))
                    return false;
                break;
            }
            case 3: { // want_response (varint/bool)
                uint64_t v;
                if (!c.readVarint(&v)) return false;
                out->want_response = (v != 0);
                break;
            }
            case 4: { // dest (fixed32)
                if (!c.readFixed32(&out->dest)) return false;
                break;
            }
            case 5: { // source (fixed32)
                if (!c.readFixed32(&out->source)) return false;
                break;
            }
            case 6: { // request_id (fixed32)
                if (!c.readFixed32(&out->request_id)) return false;
                break;
            }
            default:
                if (!c.skipField(wtype)) return false;
                break;
        }
    }

    // Reject UNKNOWN portnum (bad PSK produces garbage)
    return out->portnum != PORT_UNKNOWN;
}

/**
 * Protobuf validation callback for MeshChannelTable::tryDecrypt().
 * Returns true if the decrypted bytes parse as a valid Data message.
 */
static inline bool meshValidateData(const uint8_t *plaintext, size_t len) {
    MeshData tmp;
    return meshDecodeData(plaintext, len, &tmp);
}

// ─── Position ──────────────────────────────────────────────────────────────────

struct MeshPosition {
    int32_t  latitude_i;      // degrees * 1e7  (sfixed32, field 1)
    int32_t  longitude_i;     // degrees * 1e7  (sfixed32, field 2)
    int32_t  altitude;        // meters          (int32, field 3)
    uint32_t time;            // Unix epoch      (fixed32, field 4)
    uint32_t altitude_hae;    // field 13 in some versions
    uint32_t precision_bits;  // field 12

    double latitude()  const { return latitude_i / 1e7; }
    double longitude() const { return longitude_i / 1e7; }
};

static inline bool meshDecodePosition(const uint8_t *buf, size_t len, MeshPosition *out) {
    memset(out, 0, sizeof(MeshPosition));
    PbCursor c = pbCursor(buf, len);

    while (!c.exhausted()) {
        uint32_t field; uint8_t wtype;
        if (!c.readTag(&field, &wtype)) break;

        switch (field) {
            case 1: { // latitude_i (sfixed32)
                uint32_t v;
                if (!c.readFixed32(&v)) return false;
                out->latitude_i = (int32_t)v;
                break;
            }
            case 2: { // longitude_i (sfixed32)
                uint32_t v;
                if (!c.readFixed32(&v)) return false;
                out->longitude_i = (int32_t)v;
                break;
            }
            case 3: { // altitude (int32 varint)
                uint64_t v;
                if (!c.readVarint(&v)) return false;
                // zigzag decode for sint32
                out->altitude = (int32_t)((v >> 1) ^ -(int32_t)(v & 1));
                break;
            }
            case 4: { // time (fixed32)
                if (!c.readFixed32(&out->time)) return false;
                break;
            }
            case 12: { // precision_bits (uint32 varint)
                uint64_t v;
                if (!c.readVarint(&v)) return false;
                out->precision_bits = (uint32_t)v;
                break;
            }
            default:
                if (!c.skipField(wtype)) return false;
                break;
        }
    }
    return true;
}

// ─── User / NodeInfo ───────────────────────────────────────────────────────────

struct MeshUser {
    char     id[16];         // e.g., "!aabbccdd"   (string, field 1)
    char     long_name[40];  // human name            (string, field 2)
    char     short_name[5];  // 3-char abbreviation   (string, field 3)
    uint16_t hw_model;       // HardwareModel enum    (varint, field 6)
    uint8_t  public_key[32]; //                        (bytes, field 8)
    uint8_t  public_key_len;
};

static inline bool meshDecodeUser(const uint8_t *buf, size_t len, MeshUser *out) {
    memset(out, 0, sizeof(MeshUser));
    PbCursor c = pbCursor(buf, len);

    while (!c.exhausted()) {
        uint32_t field; uint8_t wtype;
        if (!c.readTag(&field, &wtype)) break;

        size_t slen;
        switch (field) {
            case 1: // id
                if (!c.readString(out->id, sizeof(out->id), &slen)) return false;
                break;
            case 2: // long_name
                if (!c.readString(out->long_name, sizeof(out->long_name), &slen)) return false;
                break;
            case 3: // short_name
                if (!c.readString(out->short_name, sizeof(out->short_name), &slen)) return false;
                break;
            case 6: { // hw_model
                uint64_t v;
                if (!c.readVarint(&v)) return false;
                out->hw_model = (uint16_t)v;
                break;
            }
            case 8: { // public_key
                if (!c.readBytes(out->public_key, sizeof(out->public_key),
                                  &slen)) return false;
                out->public_key_len = (uint8_t)slen;
                break;
            }
            default:
                if (!c.skipField(wtype)) return false;
                break;
        }
    }
    return true;
}

// ─── Telemetry (Device Metrics subset) ─────────────────────────────────────────

struct MeshDeviceMetrics {
    uint32_t battery_level;      // 0-100       (varint, field 1)
    float    voltage;            //             (float, field 2)
    float    channel_utilization; //            (float, field 3)
    float    air_util_tx;        //             (float, field 4)
    uint32_t uptime_seconds;     //             (varint, field 5)
};

struct MeshTelemetry {
    uint32_t          time;      // Unix epoch  (fixed32, field 1)
    bool              has_device_metrics;
    MeshDeviceMetrics device_metrics;
};

static inline bool meshDecodeDeviceMetrics(const uint8_t *buf, size_t len,
                                            MeshDeviceMetrics *out)
{
    memset(out, 0, sizeof(MeshDeviceMetrics));
    PbCursor c = pbCursor(buf, len);

    while (!c.exhausted()) {
        uint32_t field; uint8_t wtype;
        if (!c.readTag(&field, &wtype)) break;

        switch (field) {
            case 1: { uint64_t v; if (!c.readVarint(&v)) return false;
                      out->battery_level = (uint32_t)v; break; }
            case 2: if (!c.readFloat(&out->voltage)) return false; break;
            case 3: if (!c.readFloat(&out->channel_utilization)) return false; break;
            case 4: if (!c.readFloat(&out->air_util_tx)) return false; break;
            case 5: { uint64_t v; if (!c.readVarint(&v)) return false;
                      out->uptime_seconds = (uint32_t)v; break; }
            default: if (!c.skipField(wtype)) return false; break;
        }
    }
    return true;
}

static inline bool meshDecodeTelemetry(const uint8_t *buf, size_t len,
                                        MeshTelemetry *out)
{
    memset(out, 0, sizeof(MeshTelemetry));
    PbCursor c = pbCursor(buf, len);

    while (!c.exhausted()) {
        uint32_t field; uint8_t wtype;
        if (!c.readTag(&field, &wtype)) break;

        switch (field) {
            case 1: // time
                if (!c.readFixed32(&out->time)) return false;
                break;
            case 2: { // device_metrics (submessage)
                PbCursor sub;
                if (!c.readLengthDelimited(&sub)) return false;
                out->has_device_metrics = meshDecodeDeviceMetrics(
                    sub.buf, sub.len, &out->device_metrics);
                break;
            }
            default:
                if (!c.skipField(wtype)) return false;
                break;
        }
    }
    return true;
}

// ─── Minimal Protobuf Encoder (for TX) ─────────────────────────────────────────

struct PbWriter {
    uint8_t *buf;
    size_t   pos;
    size_t   capacity;

    bool full() const { return pos >= capacity; }
    size_t written() const { return pos; }

    bool writeByte(uint8_t b) {
        if (pos >= capacity) return false;
        buf[pos++] = b;
        return true;
    }

    bool writeVarint(uint64_t val) {
        do {
            uint8_t b = val & 0x7F;
            val >>= 7;
            if (val) b |= 0x80;
            if (!writeByte(b)) return false;
        } while (val);
        return true;
    }

    bool writeTag(uint32_t field, uint8_t wire_type) {
        return writeVarint((uint64_t)(field << 3 | wire_type));
    }

    bool writeFixed32(uint32_t val) {
        if (pos + 4 > capacity) return false;
        memcpy(buf + pos, &val, 4);
        pos += 4;
        return true;
    }

    bool writeBytes(uint32_t field, const uint8_t *data, size_t len) {
        if (!writeTag(field, 2)) return false;
        if (!writeVarint(len)) return false;
        if (pos + len > capacity) return false;
        memcpy(buf + pos, data, len);
        pos += len;
        return true;
    }

    bool writeString(uint32_t field, const char *str) {
        return writeBytes(field, (const uint8_t *)str, strlen(str));
    }

    bool writeVarintField(uint32_t field, uint64_t val) {
        if (!writeTag(field, 0)) return false;
        return writeVarint(val);
    }

    bool writeFixed32Field(uint32_t field, uint32_t val) {
        if (!writeTag(field, 5)) return false;
        return writeFixed32(val);
    }
};

static inline PbWriter pbWriter(uint8_t *buf, size_t capacity) {
    return { buf, 0, capacity };
}

/**
 * Encode a meshtastic.Data protobuf.
 * Returns encoded length, or 0 on failure.
 */
static inline size_t meshEncodeData(uint8_t *buf, size_t capacity,
                                     MeshPortNum portnum,
                                     const uint8_t *payload, size_t payload_len,
                                     bool want_response = false)
{
    PbWriter w = pbWriter(buf, capacity);
    if (!w.writeVarintField(1, (uint64_t)portnum)) return 0;
    if (payload_len > 0)
        if (!w.writeBytes(2, payload, payload_len)) return 0;
    if (want_response)
        if (!w.writeVarintField(3, 1)) return 0;
    return w.written();
}
