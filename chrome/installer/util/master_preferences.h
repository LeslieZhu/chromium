// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains functions processing master preference file used by
// setup and first run.

#ifndef CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_H_
#define CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_H_

#include "base/file_path.h"
#include "base/values.h"

namespace installer_util {

namespace master_preferences {
// All the preferences below are expected to be inside the JSON "distribution"
// block.

// Boolean pref that triggers skipping the first run dialogs.
extern const wchar_t kDistroSkipFirstRunPref[];
// Boolean pref that triggers loading the welcome page.
extern const wchar_t kDistroShowWelcomePage[];
// Boolean pref that triggers silent import of the default search engine.
extern const wchar_t kDistroImportSearchPref[];
// Boolean pref that triggers silent import of the default browser history.
extern const wchar_t kDistroImportHistoryPref[];
// Boolean pref that triggers silent import of the default browser bookmarks.
extern const wchar_t kDistroImportBookmarksPref[];
// RLZ ping delay in seconds
extern const wchar_t kDistroPingDelay[];
// Register Chrome as default browser for the current user.
extern const wchar_t kMakeChromeDefaultForUser[];
// The following boolean prefs have the same semantics as the corresponding
// setup command line switches. See chrome/installer/util/util_constants.cc
// for more info.
// Create Desktop and QuickLaunch shortcuts.
extern const wchar_t kCreateAllShortcuts[];
// Prevent installer from launching Chrome after a successful first install.
extern const wchar_t kDoNotLaunchChrome[];
// Register Chrome as default browser on the system.
extern const wchar_t kMakeChromeDefault[];
// Install Chrome to system wise location.
extern const wchar_t kSystemLevel[];
// Run installer in verbose mode.
extern const wchar_t kVerboseLogging[];
// Show EULA dialog and install only if accepted.
extern const wchar_t kRequireEula[];
// Use alternate shortcut text for the main shortcut.
extern const wchar_t kAltShortcutText[];
// Use alternate smaller first run info bubble.
extern const wchar_t kAltFirstRunBubble[];
// Boolean pref that triggers silent import of the default browser homepage.
extern const wchar_t kDistroImportHomePagePref[];

extern const wchar_t kMasterPreferencesValid[];
}

// This is the default name for the master preferences file used to pre-set
// values in the user profile at first run.
const wchar_t kDefaultMasterPrefs[] = L"master_preferences";

bool GetBooleanPreference(const DictionaryValue* prefs,
                          const std::wstring& name);

// This function gets ping delay (ping_delay in the sample above) from master
// preferences.
bool GetDistributionPingDelay(const DictionaryValue* prefs,
                              int* ping_delay);

// The master preferences is a JSON file with the same entries as the
// 'Default\Preferences' file. This function parses the distribution
// section of the preferences file.
//
// A prototypical 'master_preferences' file looks like this:
//
// {
//   "distribution": {
//      "skip_first_run_ui": true,
//      "show_welcome_page": true,
//      "import_search_engine": true,
//      "import_history": false,
//      "import_bookmarks": false,
//      "import_home_page": false,
//      "create_all_shortcuts": true,
//      "do_not_launch_chrome": false,
//      "make_chrome_default": false,
//      "make_chrome_default_for_user": true,
//      "system_level": false,
//      "verbose_logging": true,
//      "require_eula": true,
//      "alternate_shortcut_text": false,
//      "ping_delay": 40
//   },
//   "browser": {
//      "show_home_button": true
//   },
//   "bookmark_bar": {
//      "show_on_all_tabs": true
//   },
//   "first_run_tabs": [
//      "http://gmail.com",
//      "https://igoogle.com"
//   ],
//   "homepage": "http://example.org",
//   "homepage_is_newtabpage": false
// }
//
// A reserved "distribution" entry in the file is used to group related
// installation properties. This entry will be ignored at other times.
// This function parses the 'distribution' entry and returns a combination
// of MasterPrefResult.
DictionaryValue* ParseDistributionPreferences(
    const FilePath& master_prefs_path);

// As part of the master preferences an optional section indicates the tabs
// to open during first run. An example is the following:
//
//  {
//    "first_run_tabs": [
//       "http://google.com/f1",
//       "https://google.com/f2"
//    ]
//  }
//
// Note that the entries are usually urls but they don't have to.
//
// This function retuns the list as a vector of strings. If the master
// preferences file does not contain such list the vector is empty.
std::vector<std::wstring> ParseFirstRunTabs(const DictionaryValue* prefs);
}

#endif  // CHROME_INSTALLER_UTIL_MASTER_PREFERENCES_H_
