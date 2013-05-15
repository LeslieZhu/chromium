// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/perf/perf_ui_test_suite.h"

#include <stdio.h>

#include "base/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "base/threading/platform_thread.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/themes/browser_theme_pack.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/test/perf/generate_profile.h"
#include "content/public/test/test_browser_thread.h"

#if defined(OS_WIN)
#include <atlbase.h>
#include "ui/base/resource/resource_bundle_win.h"
#endif

using content::BrowserThread;
using extensions::Extension;

namespace {

const int kNumURLs = 20000;

const char kThemeExtension[] = "mblmlcbknbnfebdfjnolmcapmdofhmme";

base::LazyInstance<base::FilePath> g_default_profile_dir =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<base::FilePath> g_complex_profile_dir =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

PerfUITestSuite::PerfUITestSuite(int argc, char** argv)
    : UITestSuite(argc, argv) {
  fprintf(stderr, "In the constructor of the PerfUITestSuite.\n");
  base::PlatformThread::SetName("Tests_Main");
}

PerfUITestSuite::~PerfUITestSuite() {
}

base::FilePath PerfUITestSuite::GetPathForProfileType(
    ProfileType profile_type) {
  switch (profile_type) {
    case DEFAULT_THEME:
      return g_default_profile_dir.Get();
    case COMPLEX_THEME:
      return g_complex_profile_dir.Get();
    default:
      NOTREACHED();
      return base::FilePath();
  }
}

void PerfUITestSuite::Initialize() {
#if defined(OS_WIN)
  // On Windows, make sure we've loaded the main chrome dll for its resource
  // before we let our parent class initialize the shared resource bundle
  // infrastructure. We have to do this manually because the extension system
  // uses several resources which, on Windows, are only compiled into the
  // browser DLL, but the base chrome testing stuff is used outside of browser.
  //
  // TODO(darin): Kill this once http://crbug.com/52609 is fixed.
  base::FilePath dll;
  fprintf(stderr, "About to get dir module.\n");
  PathService::Get(base::DIR_MODULE, &dll);
  dll = dll.Append(chrome::kBrowserResourcesDll);
  fprintf(stderr, "About to LoadLibraryExW.\n");
  HMODULE res_mod = ::LoadLibraryExW(dll.value().c_str(),
      NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
  DCHECK(res_mod);
  fprintf(stderr, "About to _AtlBaseModule.SetResourceInstance().\n");
  _AtlBaseModule.SetResourceInstance(res_mod);

  fprintf(stderr, "About to ui::SetResourcesDataDLL.\n");
  ui::SetResourcesDataDLL(_AtlBaseModule.GetResourceInstance());
#endif

  fprintf(stderr, "About to go through UITestSuite::Initialize().\n");
  UITestSuite::Initialize();

  fprintf(stderr, "About to default_profile_dir_.CreateUniqueTempDir().\n");
  if (!default_profile_dir_.CreateUniqueTempDir()) {
    LOG(FATAL) << "Failed to create default profile directory...";
  }

  // Build a profile in default profile dir.
  fprintf(stderr, "About to call GenerateProfile().\n");
  base::FilePath default_path =
      default_profile_dir_.path().AppendASCII("Default");
  if (!GenerateProfile(TOP_SITES, kNumURLs, default_path)) {
    LOG(FATAL) << "Failed to generate default profile for tests...";
  }

  g_default_profile_dir.Get() = default_profile_dir_.path();

  fprintf(stderr, "About to build second temp dir().\n");
  if (!complex_profile_dir_.CreateUniqueTempDir()) {
    LOG(FATAL) << "Failed to create complex profile directory...";
  }

  fprintf(stderr, "About to copy profile directory.\n");
  if (!file_util::CopyDirectory(default_path,
                                complex_profile_dir_.path(),
                                true)) {
    LOG(FATAL) << "Failed to copy data to complex profile directory...";
  }

  // Copy the Extensions directory from the template into the
  // complex_profile_dir dir.
  base::FilePath base_data_dir;
  if (!PathService::Get(chrome::DIR_TEST_DATA, &base_data_dir))
    LOG(FATAL) << "Failed to fetch test data dir";

  base_data_dir = base_data_dir.AppendASCII("profiles");
  base_data_dir = base_data_dir.AppendASCII("profile_with_complex_theme");
  base_data_dir = base_data_dir.AppendASCII("Default");

  fprintf(stderr, "About to copy extensions directory.\n");
  if (!file_util::CopyDirectory(base_data_dir,
                                complex_profile_dir_.path(),
                                true)) {
    LOG(FATAL) << "Failed to copy default to complex profile";
  }

  fprintf(stderr, "About to build theme pack.\n");
  // Parse the manifest and make a temporary extension object because the
  // theme system takes extensions as input.
  base::FilePath extension_base =
      complex_profile_dir_.path()
      .AppendASCII("Default")
      .AppendASCII("Extensions")
      .AppendASCII(kThemeExtension)
      .AppendASCII("1.1");
  BuildCachedThemePakIn(extension_base);

  g_complex_profile_dir.Get() = complex_profile_dir_.path();
  fprintf(stderr, "All done with init.\n");
}

void PerfUITestSuite::BuildCachedThemePakIn(
    const base::FilePath& extension_base) {
  int error_code = 0;
  std::string error;
  JSONFileValueSerializer serializer(
      extension_base.AppendASCII("manifest.json"));
  scoped_ptr<DictionaryValue> valid_value(static_cast<DictionaryValue*>(
      serializer.Deserialize(&error_code, &error)));
  if (error_code != 0 || !valid_value)
    LOG(FATAL) << "Error parsing theme manifest: " << error;

  scoped_refptr<Extension> extension =
      Extension::Create(extension_base,
                        extensions::Manifest::INVALID_LOCATION,
                        *valid_value,
                        Extension::NO_FLAGS,
                        &error);
  if (!extension)
    LOG(FATAL) << "Error loading theme extension: " << error;

  // Build the "Cached Theme.pak" file in the template. (It's not committed
  // both because committing large binary files is bad in a git world and
  // because people don't remember to update it anyway, meaning we usually
  // have the theme rebuild penalty in our data.)
  MessageLoopForUI message_loop_;
  content::TestBrowserThread ui_thread_(BrowserThread::UI, &message_loop_);
  content::TestBrowserThread file_thread_(BrowserThread::FILE,
                                          &message_loop_);

  scoped_refptr<BrowserThemePack> theme(
      BrowserThemePack::BuildFromExtension(extension));
  if (!theme)
    LOG(FATAL) << "Failed to load theme from extension";

  theme->WriteToDisk(extension_base.AppendASCII("Cached Theme.pak"));
}
