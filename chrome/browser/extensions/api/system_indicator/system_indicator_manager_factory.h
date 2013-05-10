// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_FACTORY_H__
#define CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_FACTORY_H__

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace extensions {
class SystemIndicatorManager;

// ProfileKeyedServiceFactory for each SystemIndicatorManager.
class SystemIndicatorManagerFactory : public ProfileKeyedServiceFactory {
 public:
  static SystemIndicatorManager* GetForProfile(Profile* profile);

  static SystemIndicatorManagerFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<SystemIndicatorManagerFactory>;

  SystemIndicatorManagerFactory();
  virtual ~SystemIndicatorManagerFactory();

  // ProfileKeyedBaseFactory implementation.
  virtual ProfileKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const OVERRIDE;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_MANAGER_FACTORY_H__
