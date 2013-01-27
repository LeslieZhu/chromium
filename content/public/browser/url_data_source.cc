// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/url_data_source.h"

#include "content/browser/webui/url_data_manager.h"
#include "content/public/browser/browser_thread.h"

namespace content {

void URLDataSource::Add(BrowserContext* browser_context,
                        URLDataSource* source) {
  ChromeURLDataManager::AddDataSource(browser_context, source);
}

MessageLoop* URLDataSource::MessageLoopForRequestPath(
    const std::string& path) const {
  return BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::UI);
}

bool URLDataSource::ShouldReplaceExistingSource() const {
  return true;
}

bool URLDataSource::AllowCaching() const {
  return true;
}

bool URLDataSource::ShouldAddContentSecurityPolicy() const {
  return true;
}

std::string URLDataSource::GetContentSecurityPolicyObjectSrc() const {
  return "object-src 'none';";
}

std::string URLDataSource::GetContentSecurityPolicyFrameSrc() const {
  return "frame-src 'none';";
}

bool URLDataSource::ShouldDenyXFrameOptions() const {
  return true;
}

}  // namespace content
