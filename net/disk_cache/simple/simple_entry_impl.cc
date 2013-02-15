// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_entry_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/message_loop_proxy.h"
#include "base/threading/worker_pool.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/simple/simple_synchronous_entry.h"

namespace {

typedef disk_cache::Entry::CompletionCallback CompletionCallback;
typedef disk_cache::SimpleSynchronousEntry::SynchronousCreationCallback
    SynchronousCreationCallback;
typedef disk_cache::SimpleSynchronousEntry::SynchronousOperationCallback
    SynchronousOperationCallback;

}  // namespace

namespace disk_cache {

using base::FilePath;
using base::MessageLoopProxy;
using base::Time;
using base::WeakPtr;
using base::WorkerPool;

// static
int SimpleEntryImpl::OpenEntry(const FilePath& path,
                               const std::string& key,
                               Entry** entry,
                               const CompletionCallback& callback) {
  SynchronousCreationCallback sync_creation_callback =
      base::Bind(&SimpleEntryImpl::CreationOperationComplete, callback, entry);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::OpenEntry, path, key,
                                  MessageLoopProxy::current(),
                                  sync_creation_callback),
                       true);
  return net::ERR_IO_PENDING;
}

// static
int SimpleEntryImpl::CreateEntry(const FilePath& path,
                                 const std::string& key,
                                 Entry** entry,
                                 const CompletionCallback& callback) {
  SynchronousCreationCallback sync_creation_callback =
      base::Bind(&SimpleEntryImpl::CreationOperationComplete, callback, entry);
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::CreateEntry, path,
                                  key, MessageLoopProxy::current(),
                                  sync_creation_callback),
                       true);
  return net::ERR_IO_PENDING;
}

// static
int SimpleEntryImpl::DoomEntry(const FilePath& path,
                               const std::string& key,
                               const CompletionCallback& callback) {
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::DoomEntry, path, key,
                                  MessageLoopProxy::current(), callback),
                       true);
  return net::ERR_IO_PENDING;
}

void SimpleEntryImpl::Doom() {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    return;
  }
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::DoomAndClose,
                                  base::Unretained(synchronous_entry_)),
                       true);
  synchronous_entry_ = NULL;
  has_been_doomed_ = true;
}

void SimpleEntryImpl::Close() {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    delete this;
    return;
  }
  DCHECK(synchronous_entry_ || has_been_doomed_);
  if (!has_been_doomed_) {
    WorkerPool::PostTask(FROM_HERE,
                         base::Bind(&SimpleSynchronousEntry::Close,
                                    base::Unretained(synchronous_entry_)),
                         true);
    synchronous_entry_ = NULL;
  }
  // Entry::Close() is expected to release this entry. See disk_cache.h for
  // details.
  delete this;
}

std::string SimpleEntryImpl::GetKey() const {
  return key_;
}

Time SimpleEntryImpl::GetLastUsed() const {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  return synchronous_entry_->last_used();
}

Time SimpleEntryImpl::GetLastModified() const {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  return synchronous_entry_->last_modified();
}

int32 SimpleEntryImpl::GetDataSize(int index) const {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  return synchronous_entry_->data_size(index);
}

int SimpleEntryImpl::ReadData(int index,
                              int offset,
                              net::IOBuffer* buf,
                              int buf_len,
                              const CompletionCallback& callback) {
  // TODO(gavinp): Add support for overlapping reads. The net::HttpCache does
  // make overlapping read requests when multiple transactions access the same
  // entry as read only.
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  synchronous_entry_in_use_by_worker_ = true;
  SynchronousOperationCallback sync_operation_callback =
      base::Bind(&SimpleEntryImpl::EntryOperationComplete,
                 callback, weak_ptr_factory_.GetWeakPtr());
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::ReadData,
                                  base::Unretained(synchronous_entry_),
                                  index, offset, make_scoped_refptr(buf),
                                  buf_len, sync_operation_callback),
                       true);
  return net::ERR_IO_PENDING;
}

int SimpleEntryImpl::WriteData(int index,
                               int offset,
                               net::IOBuffer* buf,
                               int buf_len,
                               const CompletionCallback& callback,
                               bool truncate) {
  if (synchronous_entry_in_use_by_worker_) {
    NOTIMPLEMENTED();
    CHECK(false);
  }
  synchronous_entry_in_use_by_worker_ = true;
  SynchronousOperationCallback sync_operation_callback =
      base::Bind(&SimpleEntryImpl::EntryOperationComplete,
                 callback, weak_ptr_factory_.GetWeakPtr());
  WorkerPool::PostTask(FROM_HERE,
                       base::Bind(&SimpleSynchronousEntry::WriteData,
                                  base::Unretained(synchronous_entry_),
                                  index, offset, make_scoped_refptr(buf),
                                  buf_len, sync_operation_callback, truncate),
                       true);
  return net::ERR_IO_PENDING;
}

int SimpleEntryImpl::ReadSparseData(int64 offset,
                                    net::IOBuffer* buf,
                                    int buf_len,
                                    const CompletionCallback& callback) {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

int SimpleEntryImpl::WriteSparseData(int64 offset,
                                     net::IOBuffer* buf,
                                     int buf_len,
                                     const CompletionCallback& callback) {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

int SimpleEntryImpl::GetAvailableRange(int64 offset,
                                       int len,
                                       int64* start,
                                       const CompletionCallback& callback) {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

bool SimpleEntryImpl::CouldBeSparse() const {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  return false;
}

void SimpleEntryImpl::CancelSparseIO() {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
}

int SimpleEntryImpl::ReadyForSparseIO(const CompletionCallback& callback) {
  // TODO(gavinp): Determine if the simple backend should support sparse data.
  NOTIMPLEMENTED();
  return net::ERR_FAILED;
}

SimpleEntryImpl::SimpleEntryImpl(
    SimpleSynchronousEntry* synchronous_entry)
    : ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)),
      key_(synchronous_entry->key()),
      synchronous_entry_(synchronous_entry),
      synchronous_entry_in_use_by_worker_(false),
      has_been_doomed_(false) {
  DCHECK(synchronous_entry);
}

SimpleEntryImpl::~SimpleEntryImpl() {
  DCHECK(!synchronous_entry_);
}

// static
void SimpleEntryImpl::CreationOperationComplete(
    const CompletionCallback& completion_callback,
    Entry** out_entry,
    SimpleSynchronousEntry* sync_entry) {
  if (!sync_entry) {
    completion_callback.Run(net::ERR_FAILED);
    return;
  }
  *out_entry = new SimpleEntryImpl(sync_entry);
  completion_callback.Run(net::OK);
}

// static
void SimpleEntryImpl::EntryOperationComplete(
    const CompletionCallback& completion_callback,
    base::WeakPtr<SimpleEntryImpl> entry,
    int result) {
  if (entry) {
    DCHECK(entry->synchronous_entry_in_use_by_worker_);
    entry->synchronous_entry_in_use_by_worker_ = false;
  }
  completion_callback.Run(result);
}

}  // namespace disk_cache
