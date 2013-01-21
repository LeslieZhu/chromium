# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'cc_source_files': [
      'animation.cc',
      'animation.h',
      'animation_curve.cc',
      'animation_curve.h',
      'animation_events.h',
      'animation_registrar.cc',
      'animation_registrar.h',
      'append_quads_data.h',
      'bitmap_content_layer_updater.cc',
      'bitmap_content_layer_updater.h',
      'bitmap_skpicture_content_layer_updater.cc',
      'bitmap_skpicture_content_layer_updater.h',
      'caching_bitmap_content_layer_updater.cc',
      'caching_bitmap_content_layer_updater.h',
      'checkerboard_draw_quad.cc',
      'checkerboard_draw_quad.h',
      'completion_event.h',
      'compositor_frame.cc',
      'compositor_frame.h',
      'compositor_frame_ack.cc',
      'compositor_frame_ack.h',
      'compositor_frame_metadata.cc',
      'compositor_frame_metadata.h',
      'content_layer.cc',
      'content_layer.h',
      'content_layer_client.h',
      'content_layer_updater.cc',
      'content_layer_updater.h',
      'contents_scaling_layer.cc',
      'contents_scaling_layer.h',
      'damage_tracker.cc',
      'damage_tracker.h',
      'debug_border_draw_quad.cc',
      'debug_border_draw_quad.h',
      'debug_colors.cc',
      'debug_colors.h',
      'debug_rect_history.cc',
      'debug_rect_history.h',
      'delay_based_time_source.cc',
      'delay_based_time_source.h',
      'delegated_frame_data.h',
      'delegated_frame_data.cc',
      'delegated_renderer_layer.cc',
      'delegated_renderer_layer.h',
      'delegated_renderer_layer_impl.cc',
      'delegated_renderer_layer_impl.h',
      'delegating_renderer.cc',
      'delegating_renderer.h',
      'direct_renderer.cc',
      'direct_renderer.h',
      'draw_properties.h',
      'draw_quad.cc',
      'draw_quad.h',
      'font_atlas.cc',
      'font_atlas.h',
      'frame_rate_controller.cc',
      'frame_rate_controller.h',
      'frame_rate_counter.cc',
      'frame_rate_counter.h',
      'geometry_binding.cc',
      'geometry_binding.h',
      'gl_frame_data.h',
      'gl_frame_data.cc',
      'gl_renderer.cc',
      'gl_renderer.h',
      'gl_renderer_draw_cache.cc',
      'gl_renderer_draw_cache.h',
      'hash_pair.h',
      'heads_up_display_layer.cc',
      'heads_up_display_layer.h',
      'heads_up_display_layer_impl.cc',
      'heads_up_display_layer_impl.h',
      'image_layer_updater.cc',
      'image_layer_updater.h',
      'image_layer.cc',
      'image_layer.h',
      'input_handler.h',
      'io_surface_draw_quad.cc',
      'io_surface_draw_quad.h',
      'io_surface_layer.cc',
      'io_surface_layer.h',
      'io_surface_layer_impl.cc',
      'io_surface_layer_impl.h',
      'keyframed_animation_curve.cc',
      'keyframed_animation_curve.h',
      'layer.cc',
      'layer.h',
      'layer_animation_controller.cc',
      'layer_animation_controller.h',
      'layer_animation_event_observer.h',
      'layer_animation_value_observer.h',
      'layer_impl.cc',
      'layer_impl.h',
      'layer_iterator.cc',
      'layer_iterator.h',
      'layer_painter.h',
      'layer_quad.cc',
      'layer_quad.h',
      'layer_sorter.cc',
      'layer_sorter.h',
      'layer_tiling_data.cc',
      'layer_tiling_data.h',
      'layer_tree_debug_state.cc',
      'layer_tree_debug_state.h',
      'layer_tree_host.cc',
      'layer_tree_host.h',
      'layer_tree_host_client.h',
      'layer_tree_host_common.cc',
      'layer_tree_host_common.h',
      'layer_tree_host_impl.cc',
      'layer_tree_host_impl.h',
      'layer_tree_impl.cc',
      'layer_tree_impl.h',
      'layer_tree_settings.cc',
      'layer_tree_settings.h',
      'layer_updater.cc',
      'layer_updater.h',
      'managed_memory_policy.cc',
      'managed_memory_policy.h',
      'math_util.cc',
      'math_util.h',
      'nine_patch_layer.cc',
      'nine_patch_layer.h',
      'nine_patch_layer_impl.cc',
      'nine_patch_layer_impl.h',
      'occlusion_tracker.cc',
      'occlusion_tracker.h',
      'output_surface.h',
      'output_surface_client.h',
      'overdraw_metrics.cc',
      'overdraw_metrics.h',
      'page_scale_animation.cc',
      'page_scale_animation.h',
      'paint_time_counter.cc',
      'paint_time_counter.h',
      'picture.cc',
      'picture.h',
      'picture_image_layer.cc',
      'picture_image_layer.h',
      'picture_layer.cc',
      'picture_layer.h',
      'picture_layer_impl.cc',
      'picture_layer_impl.h',
      'picture_layer_tiling.cc',
      'picture_layer_tiling.h',
      'picture_layer_tiling_set.cc',
      'picture_layer_tiling_set.h',
      'picture_pile.cc',
      'picture_pile.h',
      'picture_pile_base.cc',
      'picture_pile_base.h',
      'picture_pile_impl.cc',
      'picture_pile_impl.h',
      'pinch_zoom_viewport.cc',
      'pinch_zoom_viewport.h',
      'platform_color.h',
      'prioritized_resource.cc',
      'prioritized_resource.h',
      'prioritized_resource_manager.cc',
      'prioritized_resource_manager.h',
      'priority_calculator.cc',
      'priority_calculator.h',
      'program_binding.cc',
      'program_binding.h',
      'proxy.cc',
      'proxy.h',
      'quad_culler.cc',
      'quad_culler.h',
      'quad_sink.h',
      'raster_worker_pool.cc',
      'raster_worker_pool.h',
      'rate_limiter.cc',
      'rate_limiter.h',
      'region.cc',
      'region.h',
      'render_pass.cc',
      'render_pass.h',
      'render_pass_draw_quad.cc',
      'render_pass_draw_quad.h',
      'render_pass_sink.h',
      'render_surface.cc',
      'render_surface.h',
      'render_surface_filters.cc',
      'render_surface_filters.h',
      'render_surface_impl.cc',
      'render_surface_impl.h',
      'renderer.cc',
      'renderer.h',
      'rendering_stats.cc',
      'rendering_stats.h',
      'resource.cc',
      'resource.h',
      'resource_pool.cc',
      'resource_pool.h',
      'resource_provider.cc',
      'resource_provider.h',
      'resource_update.cc',
      'resource_update.h',
      'resource_update_controller.cc',
      'resource_update_controller.h',
      'resource_update_queue.cc',
      'resource_update_queue.h',
      'ring_buffer.h',
      'scheduler.cc',
      'scheduler.h',
      'scheduler_settings.cc',
      'scheduler_settings.h',
      'scheduler_state_machine.cc',
      'scheduler_state_machine.h',
      'scoped_ptr_algorithm.h',
      'scoped_ptr_deque.h',
      'scoped_ptr_hash_map.h',
      'scoped_ptr_vector.h',
      'scoped_resource.cc',
      'scoped_resource.h',
      'scrollbar_animation_controller.h',
      'scrollbar_animation_controller_linear_fade.cc',
      'scrollbar_animation_controller_linear_fade.h',
      'scrollbar_geometry_fixed_thumb.cc',
      'scrollbar_geometry_fixed_thumb.h',
      'scrollbar_geometry_stub.cc',
      'scrollbar_geometry_stub.h',
      'scrollbar_layer.cc',
      'scrollbar_layer.h',
      'scrollbar_layer_impl.cc',
      'scrollbar_layer_impl.h',
      'scrollbar_layer_impl_base.h',
      'shader.cc',
      'shader.h',
      'shared_quad_state.cc',
      'shared_quad_state.h',
      'single_thread_proxy.cc',
      'single_thread_proxy.h',
      'skpicture_content_layer_updater.cc',
      'skpicture_content_layer_updater.h',
      'software_output_device.h',
      'software_renderer.cc',
      'software_renderer.h',
      'solid_color_draw_quad.cc',
      'solid_color_draw_quad.h',
      'solid_color_layer.cc',
      'solid_color_layer.h',
      'solid_color_layer_impl.cc',
      'solid_color_layer_impl.h',
      'stream_video_draw_quad.cc',
      'stream_video_draw_quad.h',
      'switches.cc',
      'switches.h',
      'texture_copier.cc',
      'texture_copier.h',
      'texture_draw_quad.cc',
      'texture_draw_quad.h',
      'texture_layer.cc',
      'texture_layer.h',
      'texture_layer_client.h',
      'texture_layer_impl.cc',
      'texture_layer_impl.h',
      'texture_mailbox.cc',
      'texture_mailbox.h',
      'texture_uploader.cc',
      'texture_uploader.h',
      'thread.h',
      'thread_impl.cc',
      'thread_impl.h',
      'thread_proxy.cc',
      'thread_proxy.h',
      'tile.cc',
      'tile.h',
      'tile_draw_quad.cc',
      'tile_draw_quad.h',
      'tile_manager.cc',
      'tile_manager.h',
      'tile_priority.cc',
      'tile_priority.h',
      'tiled_layer.cc',
      'tiled_layer.h',
      'tiled_layer_impl.cc',
      'tiled_layer_impl.h',
      'tiling_data.cc',
      'tiling_data.h',
      'time_source.h',
      'timing_function.cc',
      'timing_function.h',
      'top_controls_manager.cc',
      'top_controls_manager.h',
      'top_controls_manager_client.h',
      'transferable_resource.cc',
      'transferable_resource.h',
      'transform_operation.cc',
      'transform_operation.h',
      'transform_operations.cc',
      'transform_operations.h',
      'tree_synchronizer.cc',
      'tree_synchronizer.h',
      'util.h',
      'video_frame_provider.h',
      'video_layer.cc',
      'video_layer.h',
      'video_layer_impl.cc',
      'video_layer_impl.h',
      'vsync_time_source.cc',
      'vsync_time_source.h',
      'yuv_video_draw_quad.cc',
      'yuv_video_draw_quad.h',
    ],
    'conditions': [
      ['inside_chromium_build==1', {
        'webkit_src_dir': '<(DEPTH)/third_party/WebKit',
      }, {
        'webkit_src_dir': '<(DEPTH)/../../..',
      }],
    ],
  },
  'conditions': [
    ['inside_chromium_build==0', {
      'defines': [
        'INSIDE_WEBKIT_BUILD=1',
      ],
    }],
  ],
  'targets': [
    {
      'target_name': 'cc',
      'type': '<(component)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
        '<(DEPTH)/ui/ui.gyp:ui',
        '<(webkit_src_dir)/Source/WebKit/chromium/WebKit.gyp:webkit',
      ],
      'defines': [
        'CC_IMPLEMENTATION=1',
      ],
      'sources': [
        '<@(cc_source_files)',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          # TODO(jamesr): Remove once https://webkit.org/b/105259 lands + rolls
          '<(webkit_src_dir)/Source/Platform/chromium'
        ]
      }
    },
  ],
}
