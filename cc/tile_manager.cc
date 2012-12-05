// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tile_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/threading/sequenced_worker_pool.h"
#include "cc/platform_color.h"
#include "cc/rendering_stats.h"
#include "cc/resource_pool.h"
#include "cc/switches.h"
#include "cc/tile.h"
#include "third_party/skia/include/core/SkDevice.h"

namespace {

void RasterizeTile(cc::PicturePileImpl* picture_pile,
                   uint8_t* mapped_buffer,
                   const gfx::Rect& rect,
                   float contents_scale,
                   cc::RenderingStats* stats) {
  TRACE_EVENT0("cc", "RasterizeTile");
  DCHECK(mapped_buffer);
  DCHECK(picture_pile);
  SkBitmap bitmap;
  bitmap.setConfig(SkBitmap::kARGB_8888_Config, rect.width(), rect.height());
  bitmap.setPixels(mapped_buffer);
  SkDevice device(bitmap);
  SkCanvas canvas(&device);
  picture_pile->Raster(
      &canvas,
      rect,
      contents_scale,
      stats);
}

const char* kRasterThreadNamePrefix = "CompositorRaster";

// Allow two pending raster tasks per thread. This keeps resource usage
// low while making sure raster threads aren't unnecessarily idle.
const int kNumPendingRasterTasksPerThread = 2;

}  // namespace

namespace cc {

ManagedTileState::ManagedTileState()
    : can_use_gpu_memory(false),
      can_be_freed(true),
      resource_is_being_initialized(false),
      contents_swizzled(false) {
}

ManagedTileState::~ManagedTileState() {
  DCHECK(!resource);
  DCHECK(!resource_is_being_initialized);
}

TileManager::TileManager(
    TileManagerClient* client,
    ResourceProvider* resource_provider,
    size_t num_raster_threads)
    : client_(client),
      resource_pool_(ResourcePool::Create(resource_provider,
                                          Renderer::ImplPool)),
      manage_tiles_pending_(false),
      pending_raster_tasks_(0),
      num_raster_threads_(num_raster_threads),
      worker_pool_(new base::SequencedWorkerPool(num_raster_threads,
                                                 kRasterThreadNamePrefix)) {
}

TileManager::~TileManager() {
  // Reset global state and manage. This should cause
  // our memory usage to drop to zero.
  global_state_ = GlobalStateThatImpactsTilePriority();
  ManageTiles();
  DCHECK(tiles_.size() == 0);
  // This should finish all pending raster tasks and release any
  // uninitialized resources.
  worker_pool_->Shutdown();
  DCHECK(pending_raster_tasks_ == 0);
}

void TileManager::SetGlobalState(const GlobalStateThatImpactsTilePriority& global_state) {
  global_state_ = global_state;
  resource_pool_->SetMaxMemoryUsageBytes(global_state_.memory_limit_in_bytes);
  ScheduleManageTiles();
}

void TileManager::RegisterTile(Tile* tile) {
  tiles_.push_back(tile);
  ScheduleManageTiles();
}

void TileManager::UnregisterTile(Tile* tile) {
  for (TileVector::iterator it = tiles_.begin(); it != tiles_.end(); it++) {
    if (*it == tile) {
      FreeResourcesForTile(tile);
      tiles_.erase(it);
      return;
    }
  }
  DCHECK(false) << "Could not find tile version.";
}

void TileManager::WillModifyTilePriority(Tile*, WhichTree tree, const TilePriority& new_priority) {
  // TODO(nduca): Do something smarter if reprioritization turns out to be
  // costly.
  ScheduleManageTiles();
}

void TileManager::ScheduleManageTiles() {
  if (manage_tiles_pending_)
    return;
  client_->ScheduleManageTiles();
  manage_tiles_pending_ = true;
}

class BinComparator {
public:
  bool operator() (const Tile* a, const Tile* b) const {
    const ManagedTileState& ams = a->managed_state();
    const ManagedTileState& bms = b->managed_state();
    if (ams.bin != bms.bin)
      return ams.bin < bms.bin;

    if (ams.resolution != bms.resolution)
      return ams.resolution < ams.resolution;

    return
      ams.time_to_needed_in_seconds <
      bms.time_to_needed_in_seconds;
  }
};

void TileManager::ManageTiles() {
  TRACE_EVENT0("cc", "TileManager::ManageTiles");
  manage_tiles_pending_ = false;

  // The amount of time for which we want to have prepainting coverage.
  const double prepainting_window_time_seconds = 1.0;
  const double backfling_guard_distance_pixels = 314.0;

  const bool smoothness_takes_priority = global_state_.smoothness_takes_priority;

  // Bin into three categories of tiles: things we need now, things we need soon, and eventually
  for (TileVector::iterator it = tiles_.begin(); it != tiles_.end(); ++it) {
    Tile* tile = *it;
    ManagedTileState& mts = tile->managed_state();
    TilePriority prio;
    if (smoothness_takes_priority)
      prio = tile->priority(ACTIVE_TREE);
    else
      prio = tile->combined_priority();

    mts.resolution = prio.resolution;
    mts.time_to_needed_in_seconds = prio.time_to_needed_in_seconds();

    if (mts.time_to_needed_in_seconds ==
        std::numeric_limits<float>::max()) {
      mts.bin = NEVER_BIN;
      continue;
    }

    if (mts.resolution == NON_IDEAL_RESOLUTION) {
      mts.bin = EVENTUALLY_BIN;
      continue;
    }

    if (mts.time_to_needed_in_seconds == 0 ||
        prio.distance_to_visible_in_pixels < backfling_guard_distance_pixels) {
      mts.bin = NOW_BIN;
      continue;
    }

    if (prio.time_to_needed_in_seconds() < prepainting_window_time_seconds) {
      mts.bin = SOON_BIN;
      continue;
    }

    mts.bin = EVENTUALLY_BIN;
  }

  // Memory limit policy works by mapping some bin states to the NEVER bin.
  TileManagerBin bin_map[NUM_BINS];
  if (global_state_.memory_limit_policy == ALLOW_NOTHING) {
    bin_map[NOW_BIN] = NEVER_BIN;
    bin_map[SOON_BIN] = NEVER_BIN;
    bin_map[EVENTUALLY_BIN] = NEVER_BIN;
    bin_map[NEVER_BIN] = NEVER_BIN;
  } else if (global_state_.memory_limit_policy == ALLOW_ABSOLUTE_MINIMUM) {
    bin_map[NOW_BIN] = NOW_BIN;
    bin_map[SOON_BIN] = NEVER_BIN;
    bin_map[EVENTUALLY_BIN] = NEVER_BIN;
    bin_map[NEVER_BIN] = NEVER_BIN;
  } else if (global_state_.memory_limit_policy == ALLOW_PREPAINT_ONLY) {
    bin_map[NOW_BIN] = NOW_BIN;
    bin_map[SOON_BIN] = SOON_BIN;
    bin_map[EVENTUALLY_BIN] = NEVER_BIN;
    bin_map[NEVER_BIN] = NEVER_BIN;
  } else {
    bin_map[NOW_BIN] = NOW_BIN;
    bin_map[SOON_BIN] = SOON_BIN;
    bin_map[EVENTUALLY_BIN] = EVENTUALLY_BIN;
    bin_map[NEVER_BIN] = NEVER_BIN;
  }
  for (TileVector::iterator it = tiles_.begin(); it != tiles_.end(); ++it) {
    Tile* tile = *it;
    TileManagerBin bin = bin_map[tile->managed_state().bin];
    tile->managed_state().bin = bin;
  }

  // Sort by bin.
  std::sort(tiles_.begin(), tiles_.end(), BinComparator());

  // Assign gpu memory and determine what tiles need to be rasterized.
  AssignGpuMemoryToTiles();

  // Finally, kick the rasterizer.
  DispatchMoreRasterTasks();
}

void TileManager::renderingStats(RenderingStats* stats) {
  stats->totalRasterizeTimeInSeconds =
    rendering_stats_.totalRasterizeTimeInSeconds;
  stats->totalPixelsRasterized = rendering_stats_.totalPixelsRasterized;
}

void TileManager::AssignGpuMemoryToTiles() {
  TRACE_EVENT0("cc", "TileManager::AssignGpuMemoryToTiles");
  // Some memory cannot be released. Figure out which.
  size_t unreleasable_bytes = 0;
  for (TileVector::iterator it = tiles_.begin(); it != tiles_.end(); ++it) {
    Tile* tile = *it;
    if (!tile->managed_state().can_be_freed)
      unreleasable_bytes += tile->bytes_consumed_if_allocated();
  }

  // Now give memory out to the tiles until we're out, and build
  // the needs-to-be-rasterized queue.
  tiles_that_need_to_be_rasterized_.erase(
      tiles_that_need_to_be_rasterized_.begin(),
      tiles_that_need_to_be_rasterized_.end());

  size_t bytes_left = global_state_.memory_limit_in_bytes - unreleasable_bytes;
  for (TileVector::iterator it = tiles_.begin(); it != tiles_.end(); ++it) {
    Tile* tile = *it;
    size_t tile_bytes = tile->bytes_consumed_if_allocated();
    ManagedTileState& managed_tile_state = tile->managed_state();
    if (!managed_tile_state.can_be_freed)
      continue;
    if (managed_tile_state.bin == NEVER_BIN) {
      managed_tile_state.can_use_gpu_memory = false;
      FreeResourcesForTile(tile);
      continue;
    }
    if (tile_bytes > bytes_left) {
      managed_tile_state.can_use_gpu_memory = false;
      FreeResourcesForTile(tile);
      continue;
    }
    bytes_left -= tile_bytes;
    managed_tile_state.can_use_gpu_memory = true;
    if (!managed_tile_state.resource &&
        !managed_tile_state.resource_is_being_initialized)
      tiles_that_need_to_be_rasterized_.push_back(tile);
  }

  // Reverse two tiles_that_need_* vectors such that pop_back gets
  // the highest priority tile.
  std::reverse(
      tiles_that_need_to_be_rasterized_.begin(),
      tiles_that_need_to_be_rasterized_.end());
}

void TileManager::FreeResourcesForTile(Tile* tile) {
  ManagedTileState& managed_tile_state = tile->managed_state();
  DCHECK(managed_tile_state.can_be_freed);
  if (managed_tile_state.resource)
    resource_pool_->ReleaseResource(managed_tile_state.resource.Pass());
}

void TileManager::DispatchMoreRasterTasks() {
  while (!tiles_that_need_to_be_rasterized_.empty()) {
    int max_pending_tasks = kNumPendingRasterTasksPerThread *
        num_raster_threads_;

    // Stop dispatching raster tasks when too many are pending.
    if (pending_raster_tasks_ >= max_pending_tasks)
      break;

    DispatchOneRasterTask(tiles_that_need_to_be_rasterized_.back());
    tiles_that_need_to_be_rasterized_.pop_back();
  }
}

void TileManager::DispatchOneRasterTask(scoped_refptr<Tile> tile) {
  TRACE_EVENT0("cc", "TileManager::DispatchOneRasterTask");
  scoped_refptr<PicturePileImpl> cloned_picture_pile =
      tile->picture_pile()->CloneForDrawing();

  ManagedTileState& managed_tile_state = tile->managed_state();
  DCHECK(managed_tile_state.can_use_gpu_memory);
  scoped_ptr<ResourcePool::Resource> resource =
      resource_pool_->AcquireResource(tile->tile_size_.size(), tile->format_);
  resource_pool_->resource_provider()->acquirePixelBuffer(resource->id());

  managed_tile_state.resource_is_being_initialized = true;
  managed_tile_state.can_be_freed = false;

  ResourceProvider::ResourceId resource_id = resource->id();
  RenderingStats* stats = new RenderingStats();

  ++pending_raster_tasks_;
  worker_pool_->GetTaskRunnerWithShutdownBehavior(
      base::SequencedWorkerPool::SKIP_ON_SHUTDOWN)->PostTaskAndReply(
          FROM_HERE,
          base::Bind(&RasterizeTile,
                     base::Unretained(cloned_picture_pile.get()),
                     resource_pool_->resource_provider()->mapPixelBuffer(
                         resource_id),
                     tile->rect_inside_picture_,
                     tile->contents_scale(),
                     stats),
          base::Bind(&TileManager::OnRasterTaskCompleted,
                     base::Unretained(this),
                     tile,
                     base::Passed(&resource),
                     cloned_picture_pile,
                     stats));
}

void TileManager::OnRasterTaskCompleted(
    scoped_refptr<Tile> tile,
    scoped_ptr<ResourcePool::Resource> resource,
    scoped_refptr<PicturePileImpl> cloned_picture_pile,
    RenderingStats* stats) {
  TRACE_EVENT0("cc", "TileManager::OnRasterTaskCompleted");
  --pending_raster_tasks_;

  rendering_stats_.totalRasterizeTimeInSeconds +=
    stats->totalRasterizeTimeInSeconds;
  rendering_stats_.totalPixelsRasterized += stats->totalPixelsRasterized;
  delete stats;

  // Release raster resources.
  resource_pool_->resource_provider()->unmapPixelBuffer(resource->id());

  ManagedTileState& managed_tile_state = tile->managed_state();
  managed_tile_state.can_be_freed = true;

  // Tile can be freed after the completion of the raster task. Call
  // AssignGpuMemoryToTiles() to re-assign gpu memory to highest priority
  // tiles. The result of this could be that this tile is no longer
  // allowed to use gpu memory and in that case we need to abort
  // initialization and free all associated resources before calling
  // DispatchMoreRasterTasks().
  AssignGpuMemoryToTiles();

  // Finish resource initialization if |can_use_gpu_memory| is true.
  if (managed_tile_state.can_use_gpu_memory) {
    resource_pool_->resource_provider()->setPixelsFromBuffer(resource->id());
    resource_pool_->resource_provider()->releasePixelBuffer(resource->id());

    // The component order may be bgra if we uploaded bgra pixels to rgba
    // texture. Mark contents as swizzled if image component order is
    // different than texture format.
    managed_tile_state.contents_swizzled =
        !PlatformColor::sameComponentOrder(tile->format_);

    DidFinishTileInitialization(tile, resource.Pass());
  } else {
    resource_pool_->resource_provider()->releasePixelBuffer(resource->id());
    resource_pool_->ReleaseResource(resource.Pass());
    managed_tile_state.resource_is_being_initialized = false;
  }

  DispatchMoreRasterTasks();
}

void TileManager::DidFinishTileInitialization(
    Tile* tile, scoped_ptr<ResourcePool::Resource> resource) {
  ManagedTileState& managed_tile_state = tile->managed_state();
  DCHECK(!managed_tile_state.resource);
  managed_tile_state.resource = resource.Pass();
  managed_tile_state.resource_is_being_initialized = false;
  // TODO(qinmin): Make this conditional on managed_tile_state.bin == NOW_BIN.
  client_->ScheduleRedraw();
}

}
