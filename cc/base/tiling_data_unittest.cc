// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/tiling_data.h"

#include <vector>

#include "cc/test/geometry_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

int NumTiles(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  int num_tiles = tiling.num_tiles_x() * tiling.num_tiles_y();

  // Assert no overflow.
  EXPECT_GE(num_tiles, 0);
  if (num_tiles > 0)
    EXPECT_EQ(num_tiles / tiling.num_tiles_x(), tiling.num_tiles_y());

  return num_tiles;
}

int XIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int x_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TileXIndexFromSrcCoord(x_coord);
}

int YIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int y_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TileYIndexFromSrcCoord(y_coord);
}

int MinBorderXIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int x_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.FirstBorderTileXIndexFromSrcCoord(x_coord);
}

int MinBorderYIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int y_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.FirstBorderTileYIndexFromSrcCoord(y_coord);
}

int MaxBorderXIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int x_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.LastBorderTileXIndexFromSrcCoord(x_coord);
}

int MaxBorderYIndex(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int y_coord) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.LastBorderTileYIndexFromSrcCoord(y_coord);
}

int PosX(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int x_index) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TilePositionX(x_index);
}

int PosY(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int y_index) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TilePositionY(y_index);
}

int SizeX(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int x_index) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TileSizeX(x_index);
}

int SizeY(
    gfx::Size max_texture_size,
    gfx::Size total_size,
    bool has_border_texels,
    int y_index) {
  TilingData tiling(max_texture_size, total_size, has_border_texels);
  return tiling.TileSizeY(y_index);
}

TEST(TilingDataTest, NumTiles_NoTiling) {
  EXPECT_EQ(1, NumTiles(gfx::Size(16, 16), gfx::Size(16, 16), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(16, 16), gfx::Size(15, 15), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(16, 16), gfx::Size(16, 16), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(16, 16), gfx::Size(1, 16), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(15, 15), gfx::Size(15, 15), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(32, 16), gfx::Size(32, 16), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(32, 16), gfx::Size(32, 16), true));
}

TEST(TilingDataTest, NumTiles_TilingNoBorders) {
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(0, 0), false));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(4, 0), false));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(0, 4), false));
  EXPECT_EQ(0, NumTiles(gfx::Size(4, 4), gfx::Size(4, 0), false));
  EXPECT_EQ(0, NumTiles(gfx::Size(4, 4), gfx::Size(0, 4), false));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(1, 1), false));

  EXPECT_EQ(1, NumTiles(gfx::Size(1, 1), gfx::Size(1, 1), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(1, 1), gfx::Size(1, 2), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(1, 1), gfx::Size(2, 1), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(1, 1), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(1, 2), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(2, 1), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(2, 2), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(3, 3), gfx::Size(3, 3), false));

  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(1, 4), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(2, 4), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(3, 4), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(4, 4), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(5, 4), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(6, 4), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(7, 4), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(8, 4), false));
  EXPECT_EQ(3, NumTiles(gfx::Size(4, 4), gfx::Size(9, 4), false));
  EXPECT_EQ(3, NumTiles(gfx::Size(4, 4), gfx::Size(10, 4), false));
  EXPECT_EQ(3, NumTiles(gfx::Size(4, 4), gfx::Size(11, 4), false));

  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(1, 5), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(2, 5), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(3, 5), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(4, 5), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(5, 5), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(6, 5), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(7, 5), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(8, 5), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(9, 5), false));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(10, 5), false));
  EXPECT_EQ(3, NumTiles(gfx::Size(5, 5), gfx::Size(11, 5), false));

  EXPECT_EQ(1, NumTiles(gfx::Size(16, 16), gfx::Size(16, 16), false));
  EXPECT_EQ(1, NumTiles(gfx::Size(17, 17), gfx::Size(16, 16), false));
  EXPECT_EQ(4, NumTiles(gfx::Size(15, 15), gfx::Size(16, 16), false));
  EXPECT_EQ(4, NumTiles(gfx::Size(8, 8), gfx::Size(16, 16), false));
  EXPECT_EQ(6, NumTiles(gfx::Size(8, 8), gfx::Size(17, 16), false));

  EXPECT_EQ(8, NumTiles(gfx::Size(5, 8), gfx::Size(17, 16), false));
}

TEST(TilingDataTest, NumTiles_TilingWithBorders) {
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(0, 0), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(4, 0), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(0, 4), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(4, 4), gfx::Size(4, 0), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(4, 4), gfx::Size(0, 4), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(0, 0), gfx::Size(1, 1), true));

  EXPECT_EQ(1, NumTiles(gfx::Size(1, 1), gfx::Size(1, 1), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(1, 1), gfx::Size(1, 2), true));
  EXPECT_EQ(0, NumTiles(gfx::Size(1, 1), gfx::Size(2, 1), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(1, 1), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(1, 2), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(2, 1), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(2, 2), gfx::Size(2, 2), true));

  EXPECT_EQ(1, NumTiles(gfx::Size(3, 3), gfx::Size(1, 3), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(3, 3), gfx::Size(2, 3), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(3, 3), gfx::Size(3, 3), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(3, 3), gfx::Size(4, 3), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(3, 3), gfx::Size(5, 3), true));
  EXPECT_EQ(4, NumTiles(gfx::Size(3, 3), gfx::Size(6, 3), true));
  EXPECT_EQ(5, NumTiles(gfx::Size(3, 3), gfx::Size(7, 3), true));

  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(1, 4), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(2, 4), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(3, 4), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(4, 4), gfx::Size(4, 4), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(5, 4), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(4, 4), gfx::Size(6, 4), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(4, 4), gfx::Size(7, 4), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(4, 4), gfx::Size(8, 4), true));
  EXPECT_EQ(4, NumTiles(gfx::Size(4, 4), gfx::Size(9, 4), true));
  EXPECT_EQ(4, NumTiles(gfx::Size(4, 4), gfx::Size(10, 4), true));
  EXPECT_EQ(5, NumTiles(gfx::Size(4, 4), gfx::Size(11, 4), true));

  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(1, 5), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(2, 5), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(3, 5), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(4, 5), true));
  EXPECT_EQ(1, NumTiles(gfx::Size(5, 5), gfx::Size(5, 5), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(6, 5), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(7, 5), true));
  EXPECT_EQ(2, NumTiles(gfx::Size(5, 5), gfx::Size(8, 5), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(5, 5), gfx::Size(9, 5), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(5, 5), gfx::Size(10, 5), true));
  EXPECT_EQ(3, NumTiles(gfx::Size(5, 5), gfx::Size(11, 5), true));

  EXPECT_EQ(30, NumTiles(gfx::Size(8, 5), gfx::Size(16, 32), true));
}

TEST(TilingDataTest, TileXIndexFromSrcCoord) {
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(2, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(3, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(4, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(5, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(6, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(7, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, XIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, XIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 2));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 3));

  EXPECT_EQ(0, XIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 0));
  EXPECT_EQ(0, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 1));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 2));
  EXPECT_EQ(1, XIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 3));
}

TEST(TilingDataTest, FirstBorderTileXIndexFromSrcCoord) {
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(2, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(3, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(4, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(5, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(6, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 2));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 3));

  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 0));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 1));
  EXPECT_EQ(0, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 2));
  EXPECT_EQ(1, MinBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 3));
}

TEST(TilingDataTest, LastBorderTileXIndexFromSrcCoord) {
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(2, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(3, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(4, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(5, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(6, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(7, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(7, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 2));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), false, 3));

  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 0));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 1));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 2));
  EXPECT_EQ(1, MaxBorderXIndex(gfx::Size(3, 3), gfx::Size(4, 3), true, 3));
}

TEST(TilingDataTest, TileYIndexFromSrcCoord) {
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(2, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(3, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(4, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(5, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(6, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(7, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, YIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, YIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 2));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 3));

  EXPECT_EQ(0, YIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 0));
  EXPECT_EQ(0, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 1));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 2));
  EXPECT_EQ(1, YIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 3));
}

TEST(TilingDataTest, FirstBorderTileYIndexFromSrcCoord) {
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(2, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(3, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(4, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(5, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(6, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 2));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 3));

  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 0));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 1));
  EXPECT_EQ(0, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 2));
  EXPECT_EQ(1, MinBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 3));
}

TEST(TilingDataTest, LastBorderTileYIndexFromSrcCoord) {
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 2));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 3));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 4));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 5));
  EXPECT_EQ(2, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 6));
  EXPECT_EQ(2, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 7));
  EXPECT_EQ(2, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 8));
  EXPECT_EQ(3, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 9));
  EXPECT_EQ(3, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 10));
  EXPECT_EQ(3, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), false, 11));

  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(2, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 2));
  EXPECT_EQ(3, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 3));
  EXPECT_EQ(4, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 4));
  EXPECT_EQ(5, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 5));
  EXPECT_EQ(6, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 6));
  EXPECT_EQ(7, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 7));
  EXPECT_EQ(7, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 8));
  EXPECT_EQ(7, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 9));
  EXPECT_EQ(7, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 10));
  EXPECT_EQ(7, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(10, 10), true, 11));

  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(1, 1), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), false, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), false, 2));

  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 2));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), false, 3));

  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(1, 1), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(2, 2), gfx::Size(2, 2), true, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 0));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 1));
  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 3), true, 2));

  EXPECT_EQ(0, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 0));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 1));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 2));
  EXPECT_EQ(1, MaxBorderYIndex(gfx::Size(3, 3), gfx::Size(3, 4), true, 3));
}

TEST(TilingDataTest, TileSizeX) {
  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(5, 5), false, 0));
  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(5, 5), true, 0));

  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(6, 6), false, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(5, 5), gfx::Size(6, 6), false, 1));
  EXPECT_EQ(4, SizeX(gfx::Size(5, 5), gfx::Size(6, 6), true, 0));
  EXPECT_EQ(2, SizeX(gfx::Size(5, 5), gfx::Size(6, 6), true, 1));

  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(8, 8), false, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(5, 5), gfx::Size(8, 8), false, 1));
  EXPECT_EQ(4, SizeX(gfx::Size(5, 5), gfx::Size(8, 8), true, 0));
  EXPECT_EQ(4, SizeX(gfx::Size(5, 5), gfx::Size(8, 8), true, 1));

  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(5, SizeX(gfx::Size(5, 5), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(4, SizeX(gfx::Size(5, 5), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(5, 5), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(3, SizeX(gfx::Size(5, 5), gfx::Size(10, 10), true, 2));

  EXPECT_EQ(4, SizeX(gfx::Size(5, 5), gfx::Size(11, 11), true, 2));
  EXPECT_EQ(3, SizeX(gfx::Size(5, 5), gfx::Size(12, 12), true, 2));

  EXPECT_EQ(3, SizeX(gfx::Size(5, 9), gfx::Size(12, 17), true, 2));
}

TEST(TilingDataTest, TileSizeY) {
  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(5, 5), false, 0));
  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(5, 5), true, 0));

  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(6, 6), false, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(5, 5), gfx::Size(6, 6), false, 1));
  EXPECT_EQ(4, SizeY(gfx::Size(5, 5), gfx::Size(6, 6), true, 0));
  EXPECT_EQ(2, SizeY(gfx::Size(5, 5), gfx::Size(6, 6), true, 1));

  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(8, 8), false, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(5, 5), gfx::Size(8, 8), false, 1));
  EXPECT_EQ(4, SizeY(gfx::Size(5, 5), gfx::Size(8, 8), true, 0));
  EXPECT_EQ(4, SizeY(gfx::Size(5, 5), gfx::Size(8, 8), true, 1));

  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(10, 10), false, 0));
  EXPECT_EQ(5, SizeY(gfx::Size(5, 5), gfx::Size(10, 10), false, 1));
  EXPECT_EQ(4, SizeY(gfx::Size(5, 5), gfx::Size(10, 10), true, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(5, 5), gfx::Size(10, 10), true, 1));
  EXPECT_EQ(3, SizeY(gfx::Size(5, 5), gfx::Size(10, 10), true, 2));

  EXPECT_EQ(4, SizeY(gfx::Size(5, 5), gfx::Size(11, 11), true, 2));
  EXPECT_EQ(3, SizeY(gfx::Size(5, 5), gfx::Size(12, 12), true, 2));

  EXPECT_EQ(3, SizeY(gfx::Size(9, 5), gfx::Size(17, 12), true, 2));
}

TEST(TilingDataTest, TileSizeX_and_TilePositionX) {
  // Single tile cases:
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(1, 100), false, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(1, 100), false, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(3, 1), false, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(3, 1), false, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(3, 100), false, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(3, 100), false, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(1, 100), true, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(1, 100), true, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(3, 1), true, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(3, 1), true, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(3, 100), true, 0));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(3, 100), true, 0));

  // Multiple tiles:
  // no border
  // positions 0, 3
  EXPECT_EQ(2, NumTiles(gfx::Size(3, 3), gfx::Size(6, 1), false));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), false, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), false, 1));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(6, 1), false, 0));
  EXPECT_EQ(3, PosX(gfx::Size(3, 3), gfx::Size(6, 1), false, 1));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(6, 100), false, 0));
  EXPECT_EQ(3, SizeX(gfx::Size(3, 3), gfx::Size(6, 100), false, 1));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(6, 100), false, 0));
  EXPECT_EQ(3, PosX(gfx::Size(3, 3), gfx::Size(6, 100), false, 1));

  // Multiple tiles:
  // with border
  // positions 0, 2, 3, 4
  EXPECT_EQ(4, NumTiles(gfx::Size(3, 3), gfx::Size(6, 1), true));
  EXPECT_EQ(2, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), true, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), true, 1));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), true, 2));
  EXPECT_EQ(2, SizeX(gfx::Size(3, 3), gfx::Size(6, 1), true, 3));
  EXPECT_EQ(0, PosX(gfx::Size(3, 3), gfx::Size(6, 1), true, 0));
  EXPECT_EQ(2, PosX(gfx::Size(3, 3), gfx::Size(6, 1), true, 1));
  EXPECT_EQ(3, PosX(gfx::Size(3, 3), gfx::Size(6, 1), true, 2));
  EXPECT_EQ(4, PosX(gfx::Size(3, 3), gfx::Size(6, 1), true, 3));
  EXPECT_EQ(2, SizeX(gfx::Size(3, 7), gfx::Size(6, 100), true, 0));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 7), gfx::Size(6, 100), true, 1));
  EXPECT_EQ(1, SizeX(gfx::Size(3, 7), gfx::Size(6, 100), true, 2));
  EXPECT_EQ(2, SizeX(gfx::Size(3, 7), gfx::Size(6, 100), true, 3));
  EXPECT_EQ(0, PosX(gfx::Size(3, 7), gfx::Size(6, 100), true, 0));
  EXPECT_EQ(2, PosX(gfx::Size(3, 7), gfx::Size(6, 100), true, 1));
  EXPECT_EQ(3, PosX(gfx::Size(3, 7), gfx::Size(6, 100), true, 2));
  EXPECT_EQ(4, PosX(gfx::Size(3, 7), gfx::Size(6, 100), true, 3));
}

TEST(TilingDataTest, TileSizeY_and_TilePositionY) {
  // Single tile cases:
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 1), false, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(100, 1), false, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(100, 1), false, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(1, 3), false, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 3), false, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(100, 3), false, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(100, 3), false, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 1), true, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(100, 1), true, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(100, 1), true, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(1, 3), true, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 3), true, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(100, 3), true, 0));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(100, 3), true, 0));

  // Multiple tiles:
  // no border
  // positions 0, 3
  EXPECT_EQ(2, NumTiles(gfx::Size(3, 3), gfx::Size(1, 6), false));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), false, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), false, 1));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 6), false, 0));
  EXPECT_EQ(3, PosY(gfx::Size(3, 3), gfx::Size(1, 6), false, 1));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(100, 6), false, 0));
  EXPECT_EQ(3, SizeY(gfx::Size(3, 3), gfx::Size(100, 6), false, 1));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(100, 6), false, 0));
  EXPECT_EQ(3, PosY(gfx::Size(3, 3), gfx::Size(100, 6), false, 1));

  // Multiple tiles:
  // with border
  // positions 0, 2, 3, 4
  EXPECT_EQ(4, NumTiles(gfx::Size(3, 3), gfx::Size(1, 6), true));
  EXPECT_EQ(2, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), true, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), true, 1));
  EXPECT_EQ(1, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), true, 2));
  EXPECT_EQ(2, SizeY(gfx::Size(3, 3), gfx::Size(1, 6), true, 3));
  EXPECT_EQ(0, PosY(gfx::Size(3, 3), gfx::Size(1, 6), true, 0));
  EXPECT_EQ(2, PosY(gfx::Size(3, 3), gfx::Size(1, 6), true, 1));
  EXPECT_EQ(3, PosY(gfx::Size(3, 3), gfx::Size(1, 6), true, 2));
  EXPECT_EQ(4, PosY(gfx::Size(3, 3), gfx::Size(1, 6), true, 3));
  EXPECT_EQ(2, SizeY(gfx::Size(7, 3), gfx::Size(100, 6), true, 0));
  EXPECT_EQ(1, SizeY(gfx::Size(7, 3), gfx::Size(100, 6), true, 1));
  EXPECT_EQ(1, SizeY(gfx::Size(7, 3), gfx::Size(100, 6), true, 2));
  EXPECT_EQ(2, SizeY(gfx::Size(7, 3), gfx::Size(100, 6), true, 3));
  EXPECT_EQ(0, PosY(gfx::Size(7, 3), gfx::Size(100, 6), true, 0));
  EXPECT_EQ(2, PosY(gfx::Size(7, 3), gfx::Size(100, 6), true, 1));
  EXPECT_EQ(3, PosY(gfx::Size(7, 3), gfx::Size(100, 6), true, 2));
  EXPECT_EQ(4, PosY(gfx::Size(7, 3), gfx::Size(100, 6), true, 3));
}

TEST(TilingDataTest, SetTotalSize) {
  TilingData data(gfx::Size(5, 5), gfx::Size(5, 5), false);
  EXPECT_EQ(5, data.total_size().width());
  EXPECT_EQ(5, data.total_size().height());
  EXPECT_EQ(1, data.num_tiles_x());
  EXPECT_EQ(5, data.TileSizeX(0));
  EXPECT_EQ(1, data.num_tiles_y());
  EXPECT_EQ(5, data.TileSizeY(0));

  data.SetTotalSize(gfx::Size(6, 5));
  EXPECT_EQ(6, data.total_size().width());
  EXPECT_EQ(5, data.total_size().height());
  EXPECT_EQ(2, data.num_tiles_x());
  EXPECT_EQ(5, data.TileSizeX(0));
  EXPECT_EQ(1, data.TileSizeX(1));
  EXPECT_EQ(1, data.num_tiles_y());
  EXPECT_EQ(5, data.TileSizeY(0));

  data.SetTotalSize(gfx::Size(5, 12));
  EXPECT_EQ(5, data.total_size().width());
  EXPECT_EQ(12, data.total_size().height());
  EXPECT_EQ(1, data.num_tiles_x());
  EXPECT_EQ(5, data.TileSizeX(0));
  EXPECT_EQ(3, data.num_tiles_y());
  EXPECT_EQ(5, data.TileSizeY(0));
  EXPECT_EQ(5, data.TileSizeY(1));
  EXPECT_EQ(2, data.TileSizeY(2));
}

TEST(TilingDataTest, SetMaxTextureSizeNoBorders) {
  TilingData data(gfx::Size(8, 8), gfx::Size(16, 32), false);
  EXPECT_EQ(2, data.num_tiles_x());
  EXPECT_EQ(4, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(32, 32));
  EXPECT_EQ(gfx::Size(32, 32), data.max_texture_size());
  EXPECT_EQ(1, data.num_tiles_x());
  EXPECT_EQ(1, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(2, 2));
  EXPECT_EQ(gfx::Size(2, 2), data.max_texture_size());
  EXPECT_EQ(8, data.num_tiles_x());
  EXPECT_EQ(16, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(5, 5));
  EXPECT_EQ(gfx::Size(5, 5), data.max_texture_size());
  EXPECT_EQ(4, data.num_tiles_x());
  EXPECT_EQ(7, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(8, 5));
  EXPECT_EQ(gfx::Size(8, 5), data.max_texture_size());
  EXPECT_EQ(2, data.num_tiles_x());
  EXPECT_EQ(7, data.num_tiles_y());
}

TEST(TilingDataTest, SetMaxTextureSizeBorders) {
  TilingData data(gfx::Size(8, 8), gfx::Size(16, 32), true);
  EXPECT_EQ(3, data.num_tiles_x());
  EXPECT_EQ(5, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(32, 32));
  EXPECT_EQ(gfx::Size(32, 32), data.max_texture_size());
  EXPECT_EQ(1, data.num_tiles_x());
  EXPECT_EQ(1, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(2, 2));
  EXPECT_EQ(gfx::Size(2, 2), data.max_texture_size());
  EXPECT_EQ(0, data.num_tiles_x());
  EXPECT_EQ(0, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(5, 5));
  EXPECT_EQ(gfx::Size(5, 5), data.max_texture_size());
  EXPECT_EQ(5, data.num_tiles_x());
  EXPECT_EQ(10, data.num_tiles_y());

  data.SetMaxTextureSize(gfx::Size(8, 5));
  EXPECT_EQ(gfx::Size(8, 5), data.max_texture_size());
  EXPECT_EQ(3, data.num_tiles_x());
  EXPECT_EQ(10, data.num_tiles_y());
}

TEST(TilingDataTest, Assignment) {
  {
    TilingData source(gfx::Size(8, 8), gfx::Size(16, 32), true);
    TilingData dest = source;
    EXPECT_EQ(source.border_texels(), dest.border_texels());
    EXPECT_EQ(source.max_texture_size(), dest.max_texture_size());
    EXPECT_EQ(source.num_tiles_x(), dest.num_tiles_x());
    EXPECT_EQ(source.num_tiles_y(), dest.num_tiles_y());
    EXPECT_EQ(source.total_size().width(), dest.total_size().width());
    EXPECT_EQ(source.total_size().height(), dest.total_size().height());
  }
  {
    TilingData source(gfx::Size(7, 3), gfx::Size(6, 100), false);
    TilingData dest(source);
    EXPECT_EQ(source.border_texels(), dest.border_texels());
    EXPECT_EQ(source.max_texture_size(), dest.max_texture_size());
    EXPECT_EQ(source.num_tiles_x(), dest.num_tiles_x());
    EXPECT_EQ(source.num_tiles_y(), dest.num_tiles_y());
    EXPECT_EQ(source.total_size().width(), dest.total_size().width());
    EXPECT_EQ(source.total_size().height(), dest.total_size().height());
  }
}

TEST(TilingDataTest, SetBorderTexels) {
  TilingData data(gfx::Size(8, 8), gfx::Size(16, 32), false);
  EXPECT_EQ(2, data.num_tiles_x());
  EXPECT_EQ(4, data.num_tiles_y());

  data.SetHasBorderTexels(true);
  EXPECT_EQ(3, data.num_tiles_x());
  EXPECT_EQ(5, data.num_tiles_y());

  data.SetHasBorderTexels(true);
  EXPECT_EQ(3, data.num_tiles_x());
  EXPECT_EQ(5, data.num_tiles_y());

  data.SetHasBorderTexels(false);
  EXPECT_EQ(2, data.num_tiles_x());
  EXPECT_EQ(4, data.num_tiles_y());
}

TEST(TilingDataTest, LargeBorders) {
  TilingData data(gfx::Size(100, 80), gfx::Size(200, 145), 30);
  EXPECT_EQ(30, data.border_texels());

  EXPECT_EQ(70, data.TileSizeX(0));
  EXPECT_EQ(40, data.TileSizeX(1));
  EXPECT_EQ(40, data.TileSizeX(2));
  EXPECT_EQ(50, data.TileSizeX(3));
  EXPECT_EQ(4, data.num_tiles_x());

  EXPECT_EQ(50, data.TileSizeY(0));
  EXPECT_EQ(20, data.TileSizeY(1));
  EXPECT_EQ(20, data.TileSizeY(2));
  EXPECT_EQ(20, data.TileSizeY(3));
  EXPECT_EQ(35, data.TileSizeY(4));
  EXPECT_EQ(5, data.num_tiles_y());

  EXPECT_RECT_EQ(gfx::Rect(0, 0, 70, 50), data.TileBounds(0, 0));
  EXPECT_RECT_EQ(gfx::Rect(70, 50, 40, 20), data.TileBounds(1, 1));
  EXPECT_RECT_EQ(gfx::Rect(110, 110, 40, 35), data.TileBounds(2, 4));
  EXPECT_RECT_EQ(gfx::Rect(150, 70, 50, 20), data.TileBounds(3, 2));
  EXPECT_RECT_EQ(gfx::Rect(150, 110, 50, 35), data.TileBounds(3, 4));

  EXPECT_RECT_EQ(gfx::Rect(0, 0, 100, 80), data.TileBoundsWithBorder(0, 0));
  EXPECT_RECT_EQ(gfx::Rect(40, 20, 100, 80), data.TileBoundsWithBorder(1, 1));
  EXPECT_RECT_EQ(gfx::Rect(80, 80, 100, 65), data.TileBoundsWithBorder(2, 4));
  EXPECT_RECT_EQ(gfx::Rect(120, 40, 80, 80), data.TileBoundsWithBorder(3, 2));
  EXPECT_RECT_EQ(gfx::Rect(120, 80, 80, 65), data.TileBoundsWithBorder(3, 4));

  EXPECT_EQ(0, data.TileXIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.TileXIndexFromSrcCoord(69));
  EXPECT_EQ(1, data.TileXIndexFromSrcCoord(70));
  EXPECT_EQ(1, data.TileXIndexFromSrcCoord(109));
  EXPECT_EQ(2, data.TileXIndexFromSrcCoord(110));
  EXPECT_EQ(2, data.TileXIndexFromSrcCoord(149));
  EXPECT_EQ(3, data.TileXIndexFromSrcCoord(150));
  EXPECT_EQ(3, data.TileXIndexFromSrcCoord(199));

  EXPECT_EQ(0, data.TileYIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.TileYIndexFromSrcCoord(49));
  EXPECT_EQ(1, data.TileYIndexFromSrcCoord(50));
  EXPECT_EQ(1, data.TileYIndexFromSrcCoord(69));
  EXPECT_EQ(2, data.TileYIndexFromSrcCoord(70));
  EXPECT_EQ(2, data.TileYIndexFromSrcCoord(89));
  EXPECT_EQ(3, data.TileYIndexFromSrcCoord(90));
  EXPECT_EQ(3, data.TileYIndexFromSrcCoord(109));
  EXPECT_EQ(4, data.TileYIndexFromSrcCoord(110));
  EXPECT_EQ(4, data.TileYIndexFromSrcCoord(144));

  EXPECT_EQ(0, data.FirstBorderTileXIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.FirstBorderTileXIndexFromSrcCoord(99));
  EXPECT_EQ(1, data.FirstBorderTileXIndexFromSrcCoord(100));
  EXPECT_EQ(1, data.FirstBorderTileXIndexFromSrcCoord(139));
  EXPECT_EQ(2, data.FirstBorderTileXIndexFromSrcCoord(140));
  EXPECT_EQ(2, data.FirstBorderTileXIndexFromSrcCoord(179));
  EXPECT_EQ(3, data.FirstBorderTileXIndexFromSrcCoord(180));
  EXPECT_EQ(3, data.FirstBorderTileXIndexFromSrcCoord(199));

  EXPECT_EQ(0, data.FirstBorderTileYIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.FirstBorderTileYIndexFromSrcCoord(79));
  EXPECT_EQ(1, data.FirstBorderTileYIndexFromSrcCoord(80));
  EXPECT_EQ(1, data.FirstBorderTileYIndexFromSrcCoord(99));
  EXPECT_EQ(2, data.FirstBorderTileYIndexFromSrcCoord(100));
  EXPECT_EQ(2, data.FirstBorderTileYIndexFromSrcCoord(119));
  EXPECT_EQ(3, data.FirstBorderTileYIndexFromSrcCoord(120));
  EXPECT_EQ(3, data.FirstBorderTileYIndexFromSrcCoord(139));
  EXPECT_EQ(4, data.FirstBorderTileYIndexFromSrcCoord(140));
  EXPECT_EQ(4, data.FirstBorderTileYIndexFromSrcCoord(144));

  EXPECT_EQ(0, data.LastBorderTileXIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.LastBorderTileXIndexFromSrcCoord(39));
  EXPECT_EQ(1, data.LastBorderTileXIndexFromSrcCoord(40));
  EXPECT_EQ(1, data.LastBorderTileXIndexFromSrcCoord(79));
  EXPECT_EQ(2, data.LastBorderTileXIndexFromSrcCoord(80));
  EXPECT_EQ(2, data.LastBorderTileXIndexFromSrcCoord(119));
  EXPECT_EQ(3, data.LastBorderTileXIndexFromSrcCoord(120));
  EXPECT_EQ(3, data.LastBorderTileXIndexFromSrcCoord(199));

  EXPECT_EQ(0, data.LastBorderTileYIndexFromSrcCoord(0));
  EXPECT_EQ(0, data.LastBorderTileYIndexFromSrcCoord(19));
  EXPECT_EQ(1, data.LastBorderTileYIndexFromSrcCoord(20));
  EXPECT_EQ(1, data.LastBorderTileYIndexFromSrcCoord(39));
  EXPECT_EQ(2, data.LastBorderTileYIndexFromSrcCoord(40));
  EXPECT_EQ(2, data.LastBorderTileYIndexFromSrcCoord(59));
  EXPECT_EQ(3, data.LastBorderTileYIndexFromSrcCoord(60));
  EXPECT_EQ(3, data.LastBorderTileYIndexFromSrcCoord(79));
  EXPECT_EQ(4, data.LastBorderTileYIndexFromSrcCoord(80));
  EXPECT_EQ(4, data.LastBorderTileYIndexFromSrcCoord(144));
}

void TestIterate(
    const TilingData& data,
    gfx::Rect rect,
    int expect_left,
    int expect_top,
    int expect_right,
    int expect_bottom) {

  EXPECT_GE(expect_left, 0);
  EXPECT_GE(expect_top, 0);
  EXPECT_LT(expect_right, data.num_tiles_x());
  EXPECT_LT(expect_bottom, data.num_tiles_y());

  std::vector<std::pair<int, int> > original_expected;
  for (int x = 0; x < data.num_tiles_x(); ++x) {
    for (int y = 0; y < data.num_tiles_y(); ++y) {
      gfx::Rect bounds = data.TileBoundsWithBorder(x, y);
      if (x >= expect_left && x <= expect_right &&
          y >= expect_top && y <= expect_bottom) {
        EXPECT_TRUE(bounds.Intersects(rect));
        original_expected.push_back(std::make_pair(x, y));
      } else {
        EXPECT_FALSE(bounds.Intersects(rect));
      }
    }
  }

  // Verify with vanilla iterator.
  {
    std::vector<std::pair<int, int> > expected = original_expected;
    for (TilingData::Iterator iter(&data, rect); iter; ++iter) {
      bool found = false;
      for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] == iter.index()) {
          expected[i] = expected.back();
          expected.pop_back();
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
    EXPECT_EQ(0, expected.size());
  }

  // Make sure this also works with a difference iterator and an empty ignore.
  {
    std::vector<std::pair<int, int> > expected = original_expected;
    for (TilingData::DifferenceIterator iter(&data, rect, gfx::Rect());
         iter; ++iter) {
      bool found = false;
      for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] == iter.index()) {
          expected[i] = expected.back();
          expected.pop_back();
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
    EXPECT_EQ(0, expected.size());
  }
}

TEST(TilingDataTest, IteratorNoBorderTexels) {
  TilingData data(gfx::Size(10, 10), gfx::Size(40, 25), false);
  // X border index by src coord: [0-10), [10-20), [20, 30), [30, 40)
  // Y border index by src coord: [0-10), [10-20), [20, 25)
  TestIterate(data, gfx::Rect(0, 0, 40, 25), 0, 0, 3, 2);
  TestIterate(data, gfx::Rect(15, 15, 8, 8), 1, 1, 2, 2);

  // Oversized.
  TestIterate(data, gfx::Rect(-100, -100, 1000, 1000), 0, 0, 3, 2);
  TestIterate(data, gfx::Rect(-100, 20, 1000, 1), 0, 2, 3, 2);
  TestIterate(data, gfx::Rect(29, -100, 31, 1000), 2, 0, 3, 2);
  // Nonintersecting.
  TestIterate(data, gfx::Rect(60, 80, 100, 100), 0, 0, -1, -1);
}

TEST(TilingDataTest, IteratorOneBorderTexel) {
  TilingData data(gfx::Size(10, 20), gfx::Size(25, 45), true);
  // X border index by src coord: [0-10), [8-18), [16-25)
  // Y border index by src coord: [0-20), [18-38), [36-45)
  TestIterate(data, gfx::Rect(0, 0, 25, 45), 0, 0, 2, 2);
  TestIterate(data, gfx::Rect(18, 19, 3, 17), 2, 0, 2, 1);
  TestIterate(data, gfx::Rect(10, 20, 6, 16), 1, 1, 1, 1);
  TestIterate(data, gfx::Rect(9, 19, 8, 18), 0, 0, 2, 2);

  // Oversized.
  TestIterate(data, gfx::Rect(-100, -100, 1000, 1000), 0, 0, 2, 2);
  TestIterate(data, gfx::Rect(-100, 20, 1000, 1), 0, 1, 2, 1);
  TestIterate(data, gfx::Rect(18, -100, 6, 1000), 2, 0, 2, 2);
  // Nonintersecting.
  TestIterate(data, gfx::Rect(60, 80, 100, 100), 0, 0, -1, -1);
}

TEST(TilingDataTest, IteratorManyBorderTexels) {
  TilingData data(gfx::Size(50, 60), gfx::Size(65, 110), 20);
  // X border index by src coord: [0-50), [10-60), [20-65)
  // Y border index by src coord: [0-60), [20-80), [40-100), [60-110)
  TestIterate(data, gfx::Rect(0, 0, 65, 110), 0, 0, 2, 3);
  TestIterate(data, gfx::Rect(50, 60, 15, 65), 1, 1, 2, 3);
  TestIterate(data, gfx::Rect(60, 30, 2, 10), 2, 0, 2, 1);

  // Oversized.
  TestIterate(data, gfx::Rect(-100, -100, 1000, 1000), 0, 0, 2, 3);
  TestIterate(data, gfx::Rect(-100, 10, 1000, 10), 0, 0, 2, 0);
  TestIterate(data, gfx::Rect(10, -100, 10, 1000), 0, 0, 1, 3);
  // Nonintersecting.
  TestIterate(data, gfx::Rect(65, 110, 100, 100), 0, 0, -1, -1);
}

TEST(TilingDataTest, IteratorOneTile) {
  TilingData no_border(gfx::Size(1000, 1000), gfx::Size(30, 40), false);
  TestIterate(no_border, gfx::Rect(0, 0, 30, 40), 0, 0, 0, 0);
  TestIterate(no_border, gfx::Rect(10, 10, 20, 20), 0, 0, 0, 0);
  TestIterate(no_border, gfx::Rect(30, 40, 100, 100), 0, 0, -1, -1);

  TilingData one_border(gfx::Size(1000, 1000), gfx::Size(30, 40), true);
  TestIterate(one_border, gfx::Rect(0, 0, 30, 40), 0, 0, 0, 0);
  TestIterate(one_border, gfx::Rect(10, 10, 20, 20), 0, 0, 0, 0);
  TestIterate(one_border, gfx::Rect(30, 40, 100, 100), 0, 0, -1, -1);

  TilingData big_border(gfx::Size(1000, 1000), gfx::Size(30, 40), 50);
  TestIterate(big_border, gfx::Rect(0, 0, 30, 40), 0, 0, 0, 0);
  TestIterate(big_border, gfx::Rect(10, 10, 20, 20), 0, 0, 0, 0);
  TestIterate(big_border, gfx::Rect(30, 40, 100, 100), 0, 0, -1, -1);
}

TEST(TilingDataTest, IteratorNoTiles) {
  TilingData data(gfx::Size(100, 100), gfx::Size(), false);
  TestIterate(data, gfx::Rect(0, 0, 100, 100), 0, 0, -1, -1);
}

void TestDiff(
    const TilingData& data,
    gfx::Rect consider,
    gfx::Rect ignore,
    size_t num_tiles) {

  std::vector<std::pair<int, int> > expected;
  for (int y = 0; y < data.num_tiles_y(); ++y) {
    for (int x = 0; x < data.num_tiles_x(); ++x) {
      gfx::Rect bounds = data.TileBoundsWithBorder(x, y);
      if (bounds.Intersects(consider) && !bounds.Intersects(ignore))
        expected.push_back(std::make_pair(x, y));
    }
  }

  // Sanity check the test.
  EXPECT_EQ(num_tiles, expected.size());

  for (TilingData::DifferenceIterator iter(&data, consider, ignore);
       iter; ++iter) {
    bool found = false;
    for (size_t i = 0; i < expected.size(); ++i) {
      if (expected[i] == iter.index()) {
        expected[i] = expected.back();
        expected.pop_back();
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found);
  }
  EXPECT_EQ(0, expected.size());
}

TEST(TilingDataTest, DifferenceIteratorIgnoreGeometry) {
  // This test is checking that the iterator can handle different geometries of
  // ignore rects relative to the consider rect.  The consider rect indices
  // themselves are mostly tested by the non-difference iterator tests, so the
  // full rect is mostly used here for simplicity.

  // X border index by src coord: [0-10), [10-20), [20, 30), [30, 40)
  // Y border index by src coord: [0-10), [10-20), [20, 25)
  TilingData data(gfx::Size(10, 10), gfx::Size(40, 25), false);

  // Fully ignored
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 0, 40, 25), 0);
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(-100, -100, 200, 200), 0);
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(9, 9, 30, 15), 0);
  TestDiff(data, gfx::Rect(15, 15, 8, 8), gfx::Rect(15, 15, 8, 8), 0);

  // Fully un-ignored
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(-30, -20, 8, 8), 12);
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(), 12);

  // Top left, remove 2x2 tiles
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 0, 20, 19), 8);
  // Bottom right, remove 2x2 tiles
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(20, 15, 20, 6), 8);
  // Bottom left, remove 2x2 tiles
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 15, 20, 6), 8);
  // Top right, remove 2x2 tiles
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(20, 0, 20, 19), 8);
  // Center, remove only one tile
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(10, 10, 5, 5), 11);

  // Left column, flush left, removing two columns
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 0, 11, 25), 6);
  // Middle column, removing two columns
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(11, 0, 11, 25), 6);
  // Right column, flush right, removing one column
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(30, 0, 2, 25), 9);

  // Top row, flush top, removing one row
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 5, 40, 5), 8);
  // Middle row, removing one row
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 13, 40, 5), 8);
  // Bottom row, flush bottom, removing two rows
  TestDiff(data, gfx::Rect(0, 0, 40, 25), gfx::Rect(0, 13, 40, 12), 4);

  // Non-intersecting, but still touching two of the same tiles.
  TestDiff(data, gfx::Rect(8, 0, 32, 25), gfx::Rect(0, 12, 5, 12), 10);

  // Intersecting, but neither contains the other. 2x3 with one overlap.
  TestDiff(data, gfx::Rect(5, 2, 20, 10), gfx::Rect(25, 15, 5, 10), 5);
}

TEST(TilingDataTest, DifferenceIteratorManyBorderTexels) {
  // X border index by src coord: [0-50), [10-60), [20-65)
  // Y border index by src coord: [0-60), [20-80), [40-100), [60-110)
  TilingData data(gfx::Size(50, 60), gfx::Size(65, 110), 20);

  // Ignore one column, three rows
  TestDiff(data, gfx::Rect(0, 30, 55, 80), gfx::Rect(5, 30, 5, 15), 9);

  // Knock out three columns, leaving only one.
  TestDiff(data, gfx::Rect(10, 30, 55, 80), gfx::Rect(30, 59, 20, 1), 3);

  // Overlap all tiles with ignore rect.
  TestDiff(data, gfx::Rect(0, 0, 65, 110), gfx::Rect(30, 59, 1, 2), 0);
}

TEST(TilingDataTest, DifferenceIteratorOneTile) {
  TilingData no_border(gfx::Size(1000, 1000), gfx::Size(30, 40), false);
  TestDiff(no_border, gfx::Rect(0, 0, 30, 40), gfx::Rect(), 1);
  TestDiff(no_border, gfx::Rect(5, 5, 100, 100), gfx::Rect(5, 5, 1, 1), 0);

  TilingData one_border(gfx::Size(1000, 1000), gfx::Size(30, 40), true);
  TestDiff(one_border, gfx::Rect(0, 0, 30, 40), gfx::Rect(), 1);
  TestDiff(one_border, gfx::Rect(5, 5, 100, 100), gfx::Rect(5, 5, 1, 1), 0);

  TilingData big_border(gfx::Size(1000, 1000), gfx::Size(30, 40), 50);
  TestDiff(big_border, gfx::Rect(0, 0, 30, 40), gfx::Rect(), 1);
  TestDiff(big_border, gfx::Rect(5, 5, 100, 100), gfx::Rect(5, 5, 1, 1), 0);
}

TEST(TilingDataTest, DifferenceIteratorNoTiles) {
  TilingData data(gfx::Size(100, 100), gfx::Size(), false);
  TestDiff(data, gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 5, 5), 0);
}

}  // namespace
}  // namespace cc
