// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Simple implementation of about: protocol handler that treats everything as
// about:blank.  No other about: features should be available to web content,
// so they're not implemented here.

#include "net/url_request/url_request_about_job.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"

namespace net {

URLRequestAboutJob::URLRequestAboutJob(URLRequest* request,
                                       NetworkDelegate* network_delegate)
    : URLRequestJob(request, network_delegate),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {
}

void URLRequestAboutJob::Start() {
  // Start reading asynchronously so that all error reporting and data
  // callbacks happen as they would for network requests.
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&URLRequestAboutJob::StartAsync, weak_factory_.GetWeakPtr()));
}

bool URLRequestAboutJob::GetMimeType(std::string* mime_type) const {
  *mime_type = "text/html";
  return true;
}

URLRequestAboutJob::~URLRequestAboutJob() {
}

void URLRequestAboutJob::StartAsync() {
  NotifyHeadersComplete();
}

}  // namespace net
