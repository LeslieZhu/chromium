// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_utils.h"

#include <ctype.h>

#include "base/logging.h"
#include "base/port.h"
#include "base/strings/string_number_conversions.h"

using std::string;

namespace net {

// static
uint64 QuicUtils::FNV1a_64_Hash(const char* data, int len) {
  static const uint64 kOffset = GG_UINT64_C(14695981039346656037);
  static const uint64 kPrime = GG_UINT64_C(1099511628211);

  const uint8* octets = reinterpret_cast<const uint8*>(data);

  uint64 hash = kOffset;

  for (int i = 0; i < len; ++i) {
    hash = hash ^ octets[i];
    hash = hash * kPrime;
  }

  return hash;
}

// static
uint128 QuicUtils::FNV1a_128_Hash(const char* data, int len) {
  // The following two constants are defined as part of the hash algorithm.
  // see http://www.isthe.com/chongo/tech/comp/fnv/
  // 309485009821345068724781371
  const uint128 kPrime(16777216, 315);
  // 144066263297769815596495629667062367629
  const uint128 kOffset(GG_UINT64_C(7809847782465536322),
                        GG_UINT64_C(7113472399480571277));

  const uint8* octets = reinterpret_cast<const uint8*>(data);

  uint128 hash = kOffset;

  for (int i = 0; i < len; ++i) {
    hash  = hash ^ uint128(0, octets[i]);
    hash = hash * kPrime;
  }

  return hash;
}

// static
bool QuicUtils::FindMutualTag(const QuicTagVector& our_tags_vector,
                              const QuicTag* their_tags,
                              size_t num_their_tags,
                              Priority priority,
                              QuicTag* out_result,
                              size_t* out_index) {
  if (our_tags_vector.empty()) {
    return false;
  }
  const size_t num_our_tags = our_tags_vector.size();
  const QuicTag* our_tags = &our_tags_vector[0];

  size_t num_priority_tags, num_inferior_tags;
  const QuicTag* priority_tags;
  const QuicTag* inferior_tags;
  if (priority == LOCAL_PRIORITY) {
    num_priority_tags = num_our_tags;
    priority_tags = our_tags;
    num_inferior_tags = num_their_tags;
    inferior_tags = their_tags;
  } else {
    num_priority_tags = num_their_tags;
    priority_tags = their_tags;
    num_inferior_tags = num_our_tags;
    inferior_tags = our_tags;
  }

  for (size_t i = 0; i < num_priority_tags; i++) {
    for (size_t j = 0; j < num_inferior_tags; j++) {
      if (priority_tags[i] == inferior_tags[j]) {
        *out_result = priority_tags[i];
        if (out_index) {
          if (priority == LOCAL_PRIORITY) {
            *out_index = j;
          } else {
            *out_index = i;
          }
        }
        return true;
      }
    }
  }

  return false;
}

// static
void QuicUtils::SerializeUint128(uint128 v, uint8* out) {
  const uint64 lo = Uint128Low64(v);
  const uint64 hi = Uint128High64(v);
  // This assumes that the system is little-endian.
  memcpy(out, &lo, sizeof(lo));
  memcpy(out + sizeof(lo), &hi, sizeof(hi));
}

// static
uint128 QuicUtils::ParseUint128(const uint8* in) {
  uint64 lo, hi;
  memcpy(&lo, in, sizeof(lo));
  memcpy(&hi, in + sizeof(lo), sizeof(hi));
  return uint128(hi, lo);
}

#define RETURN_STRING_LITERAL(x) \
case x: \
return #x;

// static
const char* QuicUtils::StreamErrorToString(QuicRstStreamErrorCode error) {
  switch (error) {
    RETURN_STRING_LITERAL(QUIC_STREAM_NO_ERROR);
    RETURN_STRING_LITERAL(QUIC_STREAM_CONNECTION_ERROR);
    RETURN_STRING_LITERAL(QUIC_SERVER_ERROR_PROCESSING_STREAM);
    RETURN_STRING_LITERAL(QUIC_MULTIPLE_TERMINATION_OFFSETS);
    RETURN_STRING_LITERAL(QUIC_BAD_APPLICATION_PAYLOAD);
    RETURN_STRING_LITERAL(QUIC_STREAM_PEER_GOING_AWAY);
    RETURN_STRING_LITERAL(QUIC_STREAM_LAST_ERROR);
  }
  // Return a default value so that we return this when |error| doesn't match
  // any of the QuicRstStreamErrorCodes. This can happen when the RstStream
  // frame sent by the peer (attacker) has invalid error code.
  return "INVALID_RST_STREAM_ERROR_CODE";
}

// static
const char* QuicUtils::ErrorToString(QuicErrorCode error) {
  switch (error) {
    RETURN_STRING_LITERAL(QUIC_NO_ERROR);
    RETURN_STRING_LITERAL(QUIC_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_STREAM_DATA_AFTER_TERMINATION);
    RETURN_STRING_LITERAL(QUIC_INVALID_PACKET_HEADER);
    RETURN_STRING_LITERAL(QUIC_INVALID_FRAME_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_FEC_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_RST_STREAM_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_CONNECTION_CLOSE_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_GOAWAY_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_ACK_DATA);
    RETURN_STRING_LITERAL(QUIC_INVALID_VERSION_NEGOTIATION_PACKET);
    RETURN_STRING_LITERAL(QUIC_INVALID_PUBLIC_RST_PACKET);
    RETURN_STRING_LITERAL(QUIC_DECRYPTION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_ENCRYPTION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_PACKET_TOO_LARGE);
    RETURN_STRING_LITERAL(QUIC_PACKET_FOR_NONEXISTENT_STREAM);
    RETURN_STRING_LITERAL(QUIC_PEER_GOING_AWAY);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TAGS_OUT_OF_ORDER);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TOO_MANY_ENTRIES);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_TOO_MANY_REJECTS);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_INVALID_VALUE_LENGTH)
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_INTERNAL_ERROR);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_VERSION_NOT_SUPPORTED);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_NO_SUPPORT);
    RETURN_STRING_LITERAL(QUIC_INVALID_CRYPTO_MESSAGE_TYPE);
    RETURN_STRING_LITERAL(QUIC_INVALID_CRYPTO_MESSAGE_PARAMETER);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_PARAMETER_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_PARAMETER_NO_OVERLAP);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_MESSAGE_INDEX_NOT_FOUND);
    RETURN_STRING_LITERAL(QUIC_INVALID_STREAM_ID);
    RETURN_STRING_LITERAL(QUIC_TOO_MANY_OPEN_STREAMS);
    RETURN_STRING_LITERAL(QUIC_PUBLIC_RESET);
    RETURN_STRING_LITERAL(QUIC_INVALID_VERSION);
    RETURN_STRING_LITERAL(QUIC_STREAM_RST_BEFORE_HEADERS_DECOMPRESSED);
    RETURN_STRING_LITERAL(QUIC_INVALID_HEADER_ID);
    RETURN_STRING_LITERAL(QUIC_INVALID_NEGOTIATED_VALUE);
    RETURN_STRING_LITERAL(QUIC_DECOMPRESSION_FAILURE);
    RETURN_STRING_LITERAL(QUIC_CONNECTION_TIMED_OUT);
    RETURN_STRING_LITERAL(QUIC_ERROR_MIGRATING_ADDRESS);
    RETURN_STRING_LITERAL(QUIC_PACKET_WRITE_ERROR);
    RETURN_STRING_LITERAL(QUIC_PROOF_INVALID);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_DUPLICATE_TAG);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_ENCRYPTION_LEVEL_INCORRECT);
    RETURN_STRING_LITERAL(QUIC_CRYPTO_SERVER_CONFIG_EXPIRED);
    RETURN_STRING_LITERAL(QUIC_LAST_ERROR);
    // Intentionally have no default case, so we'll break the build
    // if we add errors and don't put them here.
  }
  // Return a default value so that we return this when |error| doesn't match
  // any of the QuicErrorCodes. This can happen when the ConnectionClose
  // frame sent by the peer (attacker) has invalid error code.
  return "INVALID_ERROR_CODE";
}

// static
string QuicUtils::TagToString(QuicTag tag) {
  char chars[4];
  bool ascii = true;
  const QuicTag orig_tag = tag;

  for (size_t i = 0; i < sizeof(chars); i++) {
    chars[i] = tag;
    if (chars[i] == 0 && i == 3) {
      chars[i] = ' ';
    }
    if (!isprint(static_cast<unsigned char>(chars[i]))) {
      ascii = false;
      break;
    }
    tag >>= 8;
  }

  if (ascii) {
    return string(chars, sizeof(chars));
  }

  return base::UintToString(orig_tag);
}

}  // namespace net
