// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/process_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/extensions/crashed_extension_infobar.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_process_manager.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/test/ui_test_utils.h"

class ExtensionCrashRecoveryTest : public ExtensionBrowserTest {
 protected:
  ExtensionsService* GetExtensionsService() {
    return browser()->profile()->GetExtensionsService();
  }

  ExtensionProcessManager* GetExtensionProcessManager() {
    return browser()->profile()->GetExtensionProcessManager();
  }

  CrashedExtensionInfoBarDelegate* GetCrashedExtensionInfoBarDelegate(
      int index) {
    TabContents* current_tab = browser()->GetSelectedTabContents();
    EXPECT_LT(index, current_tab->infobar_delegate_count());
    InfoBarDelegate* delegate = current_tab->GetInfoBarDelegateAt(index);
    return delegate->AsCrashedExtensionInfoBarDelegate();
  }

  void AcceptCrashedExtensionInfobar(int index) {
    CrashedExtensionInfoBarDelegate* infobar =
        GetCrashedExtensionInfoBarDelegate(index);
    ASSERT_TRUE(infobar);
    infobar->Accept();
    WaitForExtensionLoad();
  }

  void CancelCrashedExtensionInfobar(int index) {
    CrashedExtensionInfoBarDelegate* infobar =
        GetCrashedExtensionInfoBarDelegate(index);
    ASSERT_TRUE(infobar);
    infobar->Cancel();
  }

  void CrashExtension(size_t index) {
    ASSERT_LT(index, GetExtensionsService()->extensions()->size());
    Extension* extension = GetExtensionsService()->extensions()->at(index);
    ASSERT_TRUE(extension);
    std::string extension_id(extension->id());
    ExtensionHost* extension_host =
        GetExtensionProcessManager()->GetBackgroundHostForExtension(extension);
    ASSERT_TRUE(extension_host);

    RenderProcessHost* extension_rph =
        extension_host->render_view_host()->process();
    base::KillProcess(extension_rph->GetHandle(),
                      base::PROCESS_END_KILLED_BY_USER, false);
    ASSERT_TRUE(WaitForExtensionCrash(extension_id));
    ASSERT_FALSE(
        GetExtensionProcessManager()->GetBackgroundHostForExtension(extension));
  }

  void CheckExtensionConsistency(size_t index) {
    ASSERT_LT(index, GetExtensionsService()->extensions()->size());
    Extension* extension = GetExtensionsService()->extensions()->at(index);
    ASSERT_TRUE(extension);
    ExtensionHost* extension_host =
        GetExtensionProcessManager()->GetBackgroundHostForExtension(extension);
    ASSERT_TRUE(extension_host);
    ASSERT_TRUE(GetExtensionProcessManager()->HasExtensionHost(extension_host));
    ASSERT_TRUE(extension_host->IsRenderViewLive());
    ASSERT_EQ(extension_host->render_view_host()->process(),
        GetExtensionProcessManager()->GetExtensionProcess(extension->id()));
  }

  void LoadTestExtension() {
    ExtensionBrowserTest::SetUpInProcessBrowserTestFixture();
    ASSERT_TRUE(LoadExtension(
        test_data_dir_.AppendASCII("common").AppendASCII("background_page")));
    Extension* extension = GetExtensionsService()->extensions()->at(0);
    ASSERT_TRUE(extension);
    first_extension_id_ = extension->id();
    CheckExtensionConsistency(0);
  }

  void LoadSecondExtension() {
    int offset = GetExtensionsService()->extensions()->size();
    ASSERT_TRUE(LoadExtension(
        test_data_dir_.AppendASCII("install").AppendASCII("install")));
    Extension* extension = GetExtensionsService()->extensions()->at(offset);
    ASSERT_TRUE(extension);
    second_extension_id_ = extension->id();
    CheckExtensionConsistency(offset);
  }

  std::string first_extension_id_;
  std::string second_extension_id_;
};

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, Basic) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
  AcceptCrashedExtensionInfobar(0);

  SCOPED_TRACE("after clicking the infobar");
  CheckExtensionConsistency(0);
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, CloseAndReload) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
  CancelCrashedExtensionInfobar(0);
  ReloadExtension(first_extension_id_);

  SCOPED_TRACE("after reloading");
  CheckExtensionConsistency(0);
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, ReloadIndependently) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());

  ReloadExtension(first_extension_id_);

  SCOPED_TRACE("after reloading");
  CheckExtensionConsistency(0);

  TabContents* current_tab = browser()->GetSelectedTabContents();
  ASSERT_TRUE(current_tab);

  // The infobar should automatically hide after the extension is successfully
  // reloaded.
  ASSERT_EQ(0, current_tab->infobar_delegate_count());
}

// Make sure that when we don't do anything about the crashed extension
// and close the browser, it doesn't crash. The browser is closed implicitly
// at the end of each browser test.
IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, ShutdownWhileCrashed) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, TwoExtensionsCrashFirst) {
  LoadTestExtension();
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  AcceptCrashedExtensionInfobar(0);

  SCOPED_TRACE("after clicking the infobar");
  CheckExtensionConsistency(0);
  CheckExtensionConsistency(1);
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, TwoExtensionsCrashSecond) {
  LoadTestExtension();
  LoadSecondExtension();
  CrashExtension(1);
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  AcceptCrashedExtensionInfobar(0);

  SCOPED_TRACE("after clicking the infobar");
  CheckExtensionConsistency(0);
  CheckExtensionConsistency(1);
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest,
                       TwoExtensionsCrashBothAtOnce) {
  LoadTestExtension();
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());

  {
    SCOPED_TRACE("first infobar");
    AcceptCrashedExtensionInfobar(0);
    CheckExtensionConsistency(0);
  }

  {
    SCOPED_TRACE("second infobar");
    AcceptCrashedExtensionInfobar(0);
    CheckExtensionConsistency(0);
    CheckExtensionConsistency(1);
  }
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest, TwoExtensionsOneByOne) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());

  {
    SCOPED_TRACE("first infobar");
    AcceptCrashedExtensionInfobar(0);
    CheckExtensionConsistency(0);
  }

  {
    SCOPED_TRACE("second infobar");
    AcceptCrashedExtensionInfobar(0);
    CheckExtensionConsistency(0);
    CheckExtensionConsistency(1);
  }
}

// Make sure that when we don't do anything about the crashed extensions
// and close the browser, it doesn't crash. The browser is closed implicitly
// at the end of each browser test.
IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest,
                       TwoExtensionsShutdownWhileCrashed) {
  LoadTestExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest,
                       TwoExtensionsIgnoreFirst) {
  LoadTestExtension();
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());

  CancelCrashedExtensionInfobar(0);
  AcceptCrashedExtensionInfobar(1);

  SCOPED_TRACE("infobars done");
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  CheckExtensionConsistency(0);
}

IN_PROC_BROWSER_TEST_F(ExtensionCrashRecoveryTest,
                       TwoExtensionsReloadIndependently) {
  LoadTestExtension();
  LoadSecondExtension();
  CrashExtension(0);
  ASSERT_EQ(1U, GetExtensionsService()->extensions()->size());
  CrashExtension(0);
  ASSERT_TRUE(GetExtensionsService()->extensions()->empty());

  {
    SCOPED_TRACE("first: reload");
    TabContents* current_tab = browser()->GetSelectedTabContents();
    ASSERT_TRUE(current_tab);
    // At the beginning we should have one infobar displayed for each extension.
    ASSERT_EQ(2, current_tab->infobar_delegate_count());
    ReloadExtension(first_extension_id_);
    // One of the infobars should hide after the extension is reloaded.
    ASSERT_EQ(1, current_tab->infobar_delegate_count());
    CheckExtensionConsistency(0);
  }

  {
    SCOPED_TRACE("second: infobar");
    AcceptCrashedExtensionInfobar(0);
    CheckExtensionConsistency(0);
    CheckExtensionConsistency(1);
  }
}
