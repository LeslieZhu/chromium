// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugin_status_pref_setter.h"

#include "base/bind.h"
#include "chrome/browser/pepper_flash_settings_manager.h"
#include "chrome/browser/plugin_data_remover_helper.h"
#include "chrome/browser/plugin_prefs.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/plugin_service.h"
#include "webkit/plugins/webplugininfo.h"

using content::BrowserThread;
using content::PluginService;

PluginStatusPrefSetter::PluginStatusPrefSetter()
    : profile_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(factory_(this)) {}

PluginStatusPrefSetter::~PluginStatusPrefSetter() {
}

void PluginStatusPrefSetter::Init(Profile* profile,
                                  content::NotificationObserver* observer) {
  clear_plugin_lso_data_enabled_.Init(prefs::kClearPluginLSODataEnabled,
                                      profile->GetPrefs(), observer);
  pepper_flash_settings_enabled_.Init(prefs::kPepperFlashSettingsEnabled,
                                      profile->GetPrefs(), observer);
  profile_ = profile;
  registrar_.Add(this, chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED,
                 content::Source<Profile>(profile));
  StartUpdate();
}

void PluginStatusPrefSetter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED) {
    StartUpdate();
  } else {
    NOTREACHED();
  }
}

void PluginStatusPrefSetter::StartUpdate() {
  PluginService::GetInstance()->GetPlugins(
      base::Bind(&PluginStatusPrefSetter::GotPlugins, factory_.GetWeakPtr(),
                 PluginPrefs::GetForProfile(profile_)));
}

void PluginStatusPrefSetter::GotPlugins(
    scoped_refptr<PluginPrefs> plugin_prefs,
    const std::vector<webkit::WebPluginInfo>& /* plugins */) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Set the values on the PrefService instead of through the PrefMembers to
  // notify observers if they changed.
  profile_->GetPrefs()->SetBoolean(
      clear_plugin_lso_data_enabled_.GetPrefName().c_str(),
      PluginDataRemoverHelper::IsSupported(plugin_prefs));
  profile_->GetPrefs()->SetBoolean(
      pepper_flash_settings_enabled_.GetPrefName().c_str(),
      PepperFlashSettingsManager::IsPepperFlashInUse(plugin_prefs, NULL));
}
