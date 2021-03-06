# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../appcache/webkit_appcache.gypi',
    '../blob/webkit_blob.gypi',
    '../database/webkit_database.gypi',
    '../dom_storage/webkit_dom_storage.gypi',
    '../fileapi/webkit_fileapi.gypi',
    '../quota/webkit_quota.gypi',

    # TODO(kinuko): Deprecate this when we have a new target for
    # webkit_browser.  crbug.com/239710
    '../browser/webkit_browser.gypi',
  ],
  'targets': [
    {
      'target_name': 'webkit_storage',
      'type': '<(component)',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '<(DEPTH)/third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '<(DEPTH)/third_party/sqlite/sqlite.gyp:sqlite',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_base',
      ],
      'defines': ['WEBKIT_STORAGE_IMPLEMENTATION'],
      'sources': [
        '../storage/webkit_storage_export.h',
        '<@(webkit_appcache_sources)',
        '<@(webkit_blob_sources)',
        '<@(webkit_database_sources)',
        '<@(webkit_dom_storage_sources)',
        '<@(webkit_fileapi_sources)',
        '<@(webkit_quota_sources)',
        # TODO(kinuko): Deprecate this when we have a new target for
        # webkit_browser.  crbug.com/239710
        '<@(webkit_browser_storage_sources)',
      ],
      'conditions': [
        ['chromeos==1', {
          'sources': [
            '<@(webkit_fileapi_chromeos_sources)',
          ],
        }],
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ],
}
