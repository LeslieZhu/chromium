// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_UTIL_LOGGING_H_
#define SYNC_UTIL_LOGGING_H_
#pragma once

#include "base/logging.h"

// TODO(akalin): This probably belongs in base/ somewhere.

namespace tracked_objects {
class Location;
}  // namespace tracked_objects

namespace csync {

bool VlogIsOnForLocation(const tracked_objects::Location& from_here,
                         int verbose_level);

}  // namespace csync

#define VLOG_LOC_STREAM(from_here, verbose_level)                       \
  logging::LogMessage(from_here.file_name(), from_here.line_number(),   \
                      -verbose_level).stream()

#define DVLOG_LOC(from_here, verbose_level)                             \
  LAZY_STREAM(                                                          \
      VLOG_LOC_STREAM(from_here, verbose_level),                        \
      ::logging::DEBUG_MODE &&                                          \
      (VLOG_IS_ON(verbose_level) ||                                     \
       ::csync::VlogIsOnForLocation(from_here, verbose_level)))  \

#endif  // SYNC_UTIL_LOGGING_H_
