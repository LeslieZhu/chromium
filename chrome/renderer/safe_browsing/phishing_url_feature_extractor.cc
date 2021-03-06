// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/phishing_url_feature_extractor.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/perftimer.h"
#include "base/string_util.h"
#include "base/strings/string_split.h"
#include "chrome/renderer/safe_browsing/features.h"
#include "googleurl/src/gurl.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace safe_browsing {

PhishingUrlFeatureExtractor::PhishingUrlFeatureExtractor() {}

PhishingUrlFeatureExtractor::~PhishingUrlFeatureExtractor() {}

bool PhishingUrlFeatureExtractor::ExtractFeatures(const GURL& url,
                                                  FeatureMap* features) {
  PerfTimer timer;
  if (url.HostIsIPAddress()) {
    if (!features->AddBooleanFeature(features::kUrlHostIsIpAddress))
      return false;
  } else {
    std::string host;
    TrimString(url.host(), ".", &host);  // Remove any leading/trailing dots.

    // TODO(bryner): Ensure that the url encoding is consistent with
    // the features in the model.

    // Disallow unknown registries so that we don't classify
    // partial hostnames (e.g. "www.subdomain").
    size_t registry_length =
        net::registry_controlled_domains::GetRegistryLength(
            host,
            net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
            net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);

    if (registry_length == 0 || registry_length == std::string::npos) {
      DVLOG(1) << "Could not find TLD for host: " << host;
      return false;
    }
    DCHECK_LT(registry_length, host.size()) << "Non-zero registry length, but "
        "host is only a TLD: " << host;
    size_t tld_start = host.size() - registry_length;
    if (!features->AddBooleanFeature(features::kUrlTldToken +
                                     host.substr(tld_start)))
      return false;

    // Pull off the TLD and the preceeding dot.
    host.erase(tld_start - 1);
    std::vector<std::string> host_tokens;
    base::SplitStringDontTrim(host, '.', &host_tokens);
    // Get rid of any empty components.
    std::vector<std::string>::iterator new_end =
        std::remove(host_tokens.begin(), host_tokens.end(), "");
    host_tokens.erase(new_end, host_tokens.end());
    if (host_tokens.empty()) {
      DVLOG(1) << "Could not find domain for host: " << host;
      return false;
    }
    if (!features->AddBooleanFeature(features::kUrlDomainToken +
                                     host_tokens.back()))
      return false;
    host_tokens.pop_back();

    // Now we're just left with the "other" host tokens.
    for (std::vector<std::string>::iterator it = host_tokens.begin();
         it != host_tokens.end(); ++it) {
      if (!features->AddBooleanFeature(features::kUrlOtherHostToken + *it))
        return false;
    }

    if (host_tokens.size() > 1) {
      if (!features->AddBooleanFeature(features::kUrlNumOtherHostTokensGTOne))
        return false;
      if (host_tokens.size() > 3) {
        if (!features->AddBooleanFeature(
                features::kUrlNumOtherHostTokensGTThree))
          return false;
      }
    }
  }

  std::vector<std::string> long_tokens;
  SplitStringIntoLongAlphanumTokens(url.path(), &long_tokens);
  for (std::vector<std::string>::iterator it = long_tokens.begin();
       it != long_tokens.end(); ++it) {
    if (!features->AddBooleanFeature(features::kUrlPathToken + *it))
      return false;
  }

  UMA_HISTOGRAM_TIMES("SBClientPhishing.URLFeatureTime", timer.Elapsed());
  return true;
}

// static
void PhishingUrlFeatureExtractor::SplitStringIntoLongAlphanumTokens(
    const std::string& full,
    std::vector<std::string>* tokens) {
  // Split on common non-alphanumerics.
  // TODO(bryner): Split on all(?) non-alphanumerics and handle %XX properly.
  static const char kTokenSeparators[] = ".,\\/_-|=%:!&";
  std::vector<std::string> raw_splits;
  Tokenize(full, kTokenSeparators, &raw_splits);

  // Copy over only the splits that are 3 or more chars long.
  // TODO(bryner): Determine a meaningful min size.
  for (std::vector<std::string>::iterator it = raw_splits.begin();
       it != raw_splits.end(); ++it) {
    if (it->length() >= kMinPathComponentLength)
      tokens->push_back(*it);
  }
}

}  // namespace safe_browsing
