// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_RESOURCE_UPDATE_H_
#define CC_RESOURCES_RESOURCE_UPDATE_H_

#include "base/memory/ref_counted.h"
#include "cc/base/cc_export.h"
#include "skia/ext/refptr.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/vector2d.h"

class SkBitmap;
class SkPicture;
class SkCanvas;

namespace cc {

class PrioritizedResource;

struct CC_EXPORT ResourceUpdate {
  static ResourceUpdate Create(PrioritizedResource* resource,
                               const SkBitmap* bitmap,
                               gfx::Rect content_rect,
                               gfx::Rect source_rect,
                               gfx::Vector2d dest_offset);
  static ResourceUpdate CreateFromCanvas(PrioritizedResource* resource,
                                         const skia::RefPtr<SkCanvas>& canvas,
                                         gfx::Rect content_rect,
                                         gfx::Rect source_rect,
                                         gfx::Vector2d dest_offset);
  static ResourceUpdate CreateFromPicture(PrioritizedResource* resource,
                                          SkPicture* bitmap,
                                          gfx::Rect content_rect,
                                          gfx::Rect source_rect,
                                          gfx::Vector2d dest_offset);

  ResourceUpdate();
  virtual ~ResourceUpdate();

  PrioritizedResource* texture;
  const SkBitmap* bitmap;
  SkPicture* picture;
  skia::RefPtr<SkCanvas> canvas;
  gfx::Rect content_rect;
  gfx::Rect source_rect;
  gfx::Vector2d dest_offset;
};

}  // namespace cc

#endif  // CC_RESOURCES_RESOURCE_UPDATE_H_
