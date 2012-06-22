// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_WINDOW_STATE_H_
#define CHROME_BROWSER_UI_BROWSER_WINDOW_STATE_H_
#pragma once

#include <string>

#include "ui/base/ui_base_types.h"

class Browser;

namespace gfx {
class Rect;
}

namespace chrome {

std::string GetWindowPlacementKey(const Browser* browser);

bool ShouldSaveWindowPlacement(const Browser* browser);

void SaveWindowPlacement(const Browser* browser,
                         const gfx::Rect& bounds,
                         ui::WindowShowState show_state);

gfx::Rect GetSavedWindowBounds(const Browser* browser);

ui::WindowShowState GetSavedWindowShowState(const Browser* browser);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_BROWSER_WINDOW_STATE_H_
