// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_BOOKMARK_HANDLER_AURA_H_
#define CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_BOOKMARK_HANDLER_AURA_H_
#pragma once

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_drag_dest_delegate.h"
#include "chrome/browser/bookmarks/bookmark_node_data.h"

class TabContents;
typedef TabContents TabContentsWrapper;

// Chrome needs to intercept content drag events so it can dispatch them to the
// bookmarks and extensions system.
// Note that unlike the other platforms, Aura doesn't use all of the
// WebDragDest infrastructure, just the WebDragDestDelegate.
class WebDragBookmarkHandlerAura : public content::WebDragDestDelegate {
 public:
  WebDragBookmarkHandlerAura();
  virtual ~WebDragBookmarkHandlerAura();

  // Overridden from content::WebDragDestDelegate:
  virtual void DragInitialize(content::WebContents* contents) OVERRIDE;
  virtual void OnDragOver() OVERRIDE;
  virtual void OnDragEnter() OVERRIDE;
  virtual void OnDrop() OVERRIDE;
  virtual void OnDragLeave() OVERRIDE;

  virtual void OnReceiveDragData(const ui::OSExchangeData& data) OVERRIDE;

 private:
  // The TabContentsWrapper for the drag.
  // Weak reference; may be NULL if the contents aren't contained in a wrapper
  // (e.g. WebUI dialogs).
  TabContentsWrapper* tab_;

  // The bookmark data for the active drag.  Empty when there is no active drag.
  BookmarkNodeData bookmark_drag_data_;

  DISALLOW_COPY_AND_ASSIGN(WebDragBookmarkHandlerAura);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_BOOKMARK_HANDLER_AURA_H_
