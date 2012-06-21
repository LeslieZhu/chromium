// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_UTIL_NIGORI_H_
#define SYNC_UTIL_NIGORI_H_
#pragma once

#include <string>

#include "base/memory/scoped_ptr.h"

namespace crypto {
class SymmetricKey;
}  // namespace crypto

namespace csync {

// A (partial) implementation of Nigori, a protocol to securely store secrets in
// the cloud. This implementation does not support server authentication or
// assisted key derivation.
//
// To store secrets securely, use the |Permute| method to derive a lookup name
// for your secret (basically a map key), and |Encrypt| and |Decrypt| to store
// and retrieve the secret.
//
// TODO: Link to doc.
class Nigori {
 public:
  enum Type {
    Password = 1,
  };

  Nigori();
  virtual ~Nigori();

  // Initialize the client with the given |hostname|, |username| and |password|.
  bool InitByDerivation(const std::string& hostname,
                        const std::string& username,
                        const std::string& password);

  // Initialize the client by importing the given keys instead of deriving new
  // ones.
  bool InitByImport(const std::string& user_key,
                    const std::string& encryption_key,
                    const std::string& mac_key);

  // Derives a secure lookup name from |type| and |name|. If |hostname|,
  // |username| and |password| are kept constant, a given |type| and |name| pair
  // always yields the same |permuted| value. Note that |permuted| will be
  // Base64 encoded.
  bool Permute(Type type, const std::string& name, std::string* permuted) const;

  // Encrypts |value|. Note that on success, |encrypted| will be Base64
  // encoded.
  bool Encrypt(const std::string& value, std::string* encrypted) const;

  // Decrypts |value| into |decrypted|. It is assumed that |value| is Base64
  // encoded.
  bool Decrypt(const std::string& value, std::string* decrypted) const;

  // Exports the raw derived keys.
  bool ExportKeys(std::string* user_key,
                  std::string* encryption_key,
                  std::string* mac_key) const;

  static const char kSaltSalt[];  // The salt used to derive the user salt.
  static const size_t kSaltKeySizeInBits = 128;
  static const size_t kDerivedKeySizeInBits = 128;
  static const size_t kIvSize = 16;
  static const size_t kHashSize = 32;

  static const size_t kSaltIterations = 1001;
  static const size_t kUserIterations = 1002;
  static const size_t kEncryptionIterations = 1003;
  static const size_t kSigningIterations = 1004;

 private:
  scoped_ptr<crypto::SymmetricKey> user_key_;
  scoped_ptr<crypto::SymmetricKey> encryption_key_;
  scoped_ptr<crypto::SymmetricKey> mac_key_;
};

}  // namespace csync

#endif  // SYNC_UTIL_NIGORI_H_
