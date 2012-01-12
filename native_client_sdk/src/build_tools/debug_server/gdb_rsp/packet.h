/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// This module provides interfaces for writing to and reading from a
// GDB RSP packet.  A packet represents a single data transfer from one
// RSP endpoint to another excluding all additional encapsulating data
// generated by the session layer. See:
// http://ftp.gnu.org/old-gnu/Manuals/gdb-5.1.1/html_node/gdb_129.html#SEC134
//
// The packet object stores the data in a character vector in a cooked format,
// tracking both a read and write position within the stream.  An empty
// packet still contains a valid pointer to NULL, so it is always safe to
// get the Payload and use it as a string. In addition packets may be
// sequenced by setting an 8 bit sequence number which helps both sides
// detect when packets have been lost.  By default the sequence number is not
// set which is represented as a -1.
//
// This API is not expected to throw unless the underlying vector attempts
// to resize when the system is out of memory, in which case it will throw
// a std::bad_alloc.
#ifndef NATIVE_CLIENT_GDB_RSP_PACKET_H_
#define NATIVE_CLIENT_GDB_RSP_PACKET_H_ 1

#include <string>
#include <vector>

#include "native_client/src/debug_server/port/std_types.h"

namespace gdb_rsp {

class Packet {
  typedef void (*StrFunc_t)(void *ctx, const char *str);

 public:
  Packet();

  // Empty the vector and reset the read/write pointers.
  void Clear();

  // Reset the read pointer, allowing the packet to be re-read.
  void Rewind();

  // Return true of the read pointer has reached the write pointer.
  bool EndOfPacket() const;

  // Store a single raw 8 bit value
  void AddRawChar(char ch);

  // Store a block of data as hex pairs per byte
  void AddBlock(const void *ptr, uint32_t len);

  // Store an 8, 16, 32, or 64 bit word as a block without removing preceeding
  // zeros.  This is used for fixed sized fields.
  void AddWord8(uint8_t val);
  void AddWord16(uint16_t val);
  void AddWord32(uint32_t val);
  void AddWord64(uint64_t val);

  // Store a number up to 64 bits, with preceeding zeros removed.  Since
  // zeros can be removed, the width of this number is unknown, and always
  // followed by NUL or a seperator (non hex digit).
  void AddNumberSep(uint64_t val, char sep);

  // Add a raw string.  This is dangerous since the other side may incorrectly
  // interpret certain special characters such as: ":,#$"
  void AddString(const char *str);

  // Add a string stored as a stream of ASCII hex digit pairs.  It is safe
  // to use any character in this stream.  If this does not terminate the
  // packet, there should be a sperator (non hex digit) immediately following.
  void AddHexString(const char *str);

  // Retrieve a single character if available
  bool GetRawChar(char *ch);

  // Retreive "len" ASCII character pairs.
  bool GetBlock(void *ptr, uint32_t len);

  // Retreive a 8, 16, 32, or 64 bit word as pairs of hex digits.  These
  // functions will always consume bits/4 characters from the stream.
  bool GetWord8(uint8_t *val);
  bool GetWord16(uint16_t *val);
  bool GetWord32(uint32_t *val);
  bool GetWord64(uint64_t *val);

  // Retreive a number and the seperator.  If SEP is null, the seperator is
  // consumed but thrown away.
  bool GetNumberSep(uint64_t *val, char *sep);

  // Get a string from the stream
  bool GetString(std::string *str);
  bool GetHexString(std::string *str);

  // Callback with the passed in context, and a NUL terminated string.
  // These methods provide a means to avoid an extra memcpy.
  bool GetStringCB(void *ctx, StrFunc_t cb);
  bool GetHexStringCB(void *ctx, StrFunc_t cb);

  // Return a pointer to the entire packet payload
  const char *GetPayload() const;

  // Returns true and the sequence number, or false if it is unset.
  bool GetSequence(int32_t *seq) const;

  // Set the sequence number.
  void SetSequence(int32_t seq);

 private:
  int32_t seq_;
  std::vector<char> data_;
  size_t read_index_;
  size_t write_index_;
};

}  // namespace gdb_rsp

#endif  // NATIVE_CLIENT_GDB_RSP_PACKET_H_

