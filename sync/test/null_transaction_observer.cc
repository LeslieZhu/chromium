// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/test/null_transaction_observer.h"

#include "base/memory/weak_ptr.h"

namespace syncable {

csync::WeakHandle<TransactionObserver> NullTransactionObserver() {
  return csync::MakeWeakHandle(base::WeakPtr<TransactionObserver>());
}

}  // namespace syncable
