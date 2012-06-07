// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"

using content::OpenURLParams;
using content::Referrer;

// Basic sanity check of ViewID use on the mac.
class ViewIDTest : public InProcessBrowserTest {
 public:
  ViewIDTest() : root_window_(nil) {
    CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableExperimentalExtensionApis);
  }

  void CheckViewID(ViewID view_id, bool should_have) {
    if (!root_window_)
      root_window_ = browser()->window()->GetNativeWindow();

    ASSERT_TRUE(root_window_);
    NSView* view = view_id_util::GetView(root_window_, view_id);
    EXPECT_EQ(should_have, !!view) << " Failed id=" << view_id;
  }

  void DoTest() {
    // Make sure FindBar is created to test
    // VIEW_ID_FIND_IN_PAGE_TEXT_FIELD and VIEW_ID_FIND_IN_PAGE.
    browser()->ShowFindBar();

    // Make sure docked devtools is created to test VIEW_ID_DEV_TOOLS_DOCKED
    browser()->profile()->GetPrefs()->SetBoolean(prefs::kDevToolsOpenDocked,
                                                 true);
    browser()->ToggleDevToolsWindow(DEVTOOLS_TOGGLE_ACTION_INSPECT);

    // Make sure download shelf is created to test VIEW_ID_DOWNLOAD_SHELF
    browser()->window()->GetDownloadShelf()->Show();

    // Create a bookmark to test VIEW_ID_BOOKMARK_BAR_ELEMENT
    BookmarkModel* bookmark_model = browser()->profile()->GetBookmarkModel();
    if (bookmark_model) {
      if (!bookmark_model->IsLoaded())
        ui_test_utils::WaitForBookmarkModelToLoad(bookmark_model);

      bookmark_utils::AddIfNotBookmarked(
          bookmark_model, GURL(chrome::kAboutBlankURL), ASCIIToUTF16("about"));
    }

    for (int i = VIEW_ID_TOOLBAR; i < VIEW_ID_PREDEFINED_COUNT; ++i) {
      // Mac implementation does not support following ids yet.
      if (i == VIEW_ID_STAR_BUTTON ||
          i == VIEW_ID_AUTOCOMPLETE ||
          i == VIEW_ID_CONTENTS_SPLIT ||
          i == VIEW_ID_FEEDBACK_BUTTON ||
          i == VIEW_ID_OMNIBOX ||
          i == VIEW_ID_CHROME_TO_MOBILE_BUTTON) {
        continue;
      }

      CheckViewID(static_cast<ViewID>(i), true);
    }

    CheckViewID(VIEW_ID_TAB, true);
    CheckViewID(VIEW_ID_TAB_STRIP, true);
    CheckViewID(VIEW_ID_PREDEFINED_COUNT, false);
  }

 private:
  NSWindow* root_window_;
};

IN_PROC_BROWSER_TEST_F(ViewIDTest, Basic) {
  ASSERT_NO_FATAL_FAILURE(DoTest());
}

// Flaky on Mac: http://crbug.com/90557.
IN_PROC_BROWSER_TEST_F(ViewIDTest, DISABLED_Fullscreen) {
  browser()->window()->EnterFullscreen(
      GURL(), FEB_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION);
  ASSERT_NO_FATAL_FAILURE(DoTest());
}

IN_PROC_BROWSER_TEST_F(ViewIDTest, Tab) {
  CheckViewID(VIEW_ID_TAB_0, true);
  CheckViewID(VIEW_ID_TAB_LAST, true);

  // Open 9 new tabs.
  for (int i = 1; i <= 9; ++i) {
    CheckViewID(static_cast<ViewID>(VIEW_ID_TAB_0 + i), false);
    browser()->OpenURL(OpenURLParams(
        GURL(chrome::kAboutBlankURL), Referrer(), NEW_BACKGROUND_TAB,
         content::PAGE_TRANSITION_TYPED, false));
    CheckViewID(static_cast<ViewID>(VIEW_ID_TAB_0 + i), true);
    // VIEW_ID_TAB_LAST should always be available.
    CheckViewID(VIEW_ID_TAB_LAST, true);
  }

  // Open the 11th tab.
  browser()->OpenURL(OpenURLParams(
      GURL(chrome::kAboutBlankURL), Referrer(), NEW_BACKGROUND_TAB,
      content::PAGE_TRANSITION_TYPED, false));
  CheckViewID(VIEW_ID_TAB_LAST, true);
}
