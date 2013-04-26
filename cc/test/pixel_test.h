// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util.h"
#include "cc/quads/render_pass.h"
#include "cc/test/pixel_comparator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/size.h"

#ifndef CC_TEST_PIXEL_TEST_H_
#define CC_TEST_PIXEL_TEST_H_

namespace cc {
class GLRenderer;
class OutputSurface;
class ResourceProvider;

class PixelTest : public testing::Test {
 protected:
  PixelTest();
  virtual ~PixelTest();

  virtual void SetUp() OVERRIDE;

  bool RunPixelTest(RenderPassList* pass_list,
                    const base::FilePath& ref_file,
                    const PixelComparator& comparator);

  void ReadbackResult(scoped_ptr<SkBitmap> bitmap);

  bool PixelsMatchReference(const base::FilePath& ref_file,
                            const PixelComparator& comparator);

  gfx::Size device_viewport_size_;
  scoped_ptr<OutputSurface> output_surface_;
  scoped_ptr<ResourceProvider> resource_provider_;
  class PixelTestRendererClient;
  scoped_ptr<PixelTestRendererClient> fake_client_;
  scoped_ptr<GLRenderer> renderer_;
  scoped_ptr<SkBitmap> result_bitmap_;
};

}  // namespace cc

#endif  // CC_TEST_PIXEL_TEST_H_
