// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_
#define MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_

#include <jni.h>
#include <map>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "base/timer.h"
#include "googleurl/src/gurl.h"
#include "media/base/android/media_player_android.h"
#include "media/base/android/media_player_listener.h"

namespace media {

class MediaPlayerManager;

// This class serves as a bridge for native code to call java functions inside
// android mediaplayer class. For more information on android mediaplayer, check
// http://developer.android.com/reference/android/media/MediaPlayer.html
// The actual android mediaplayer instance is created lazily when Start(),
// Pause(), SeekTo() gets called. As a result, media information may not
// be available until one of those operations is performed. After that, we
// will cache those information in case the mediaplayer gets released.
class MEDIA_EXPORT MediaPlayerBridge : public MediaPlayerAndroid {
 public:
  static bool RegisterMediaPlayerBridge(JNIEnv* env);

  // Construct a MediaPlayerBridge object with all the needed media player
  // callbacks. This object needs to call |manager|'s RequestMediaResources()
  // before decoding the media stream. This allows |manager| to track
  // unused resources and free them when needed. On the other hand, it needs
  // to call ReleaseMediaResources() when it is done with decoding.
  MediaPlayerBridge(int player_id,
                    const GURL& url,
                    const GURL& first_party_for_cookies,
                    bool hide_url_log,
                    MediaPlayerManager* manager);
  virtual ~MediaPlayerBridge();

  // MediaPlayerAndroid implementation.
  virtual void SetVideoSurface(jobject surface) OVERRIDE;
  virtual void Start() OVERRIDE;
  virtual void Pause() OVERRIDE;
  virtual void SeekTo(base::TimeDelta time) OVERRIDE;
  virtual void Release() OVERRIDE;
  virtual void SetVolume(float leftVolume, float rightVolume) OVERRIDE;
  virtual int GetVideoWidth() OVERRIDE;
  virtual int GetVideoHeight() OVERRIDE;
  virtual base::TimeDelta GetCurrentTime() OVERRIDE;
  virtual base::TimeDelta GetDuration() OVERRIDE;
  virtual bool IsPlaying() OVERRIDE;
  virtual bool CanPause() OVERRIDE;
  virtual bool CanSeekForward() OVERRIDE;
  virtual bool CanSeekBackward() OVERRIDE;
  virtual bool IsPlayerReady() OVERRIDE;

 protected:
  void SetMediaPlayer(jobject j_media_player);
  void SetMediaPlayerListener();

  // MediaPlayerAndroid implementation.
  virtual void OnVideoSizeChanged(int width, int height) OVERRIDE;
  virtual void OnPlaybackComplete() OVERRIDE;
  virtual void OnMediaInterrupted() OVERRIDE;

  virtual void PendingSeekInternal(base::TimeDelta time);

  // Prepare the player for playback, asynchronously. When succeeds,
  // OnMediaPrepared() will be called. Otherwise, OnMediaError() will
  // be called with an error type.
  virtual void Prepare();
  void OnMediaPrepared();

 private:
  // Initialize this object and extract the metadata from the media.
  void Initialize();

  // Create the actual android media player.
  void CreateMediaPlayer();

  // Set the data source for the media player.
  void SetDataSource(const std::string& url);

  // Functions that implements media player control.
  void StartInternal();
  void PauseInternal();
  void SeekInternal(base::TimeDelta time);

  // Get allowed operations from the player.
  void GetAllowedOperations();

  // Callback function passed to |resource_getter_|. Called when the cookies
  // are retrieved.
  void OnCookiesRetrieved(const std::string& cookies);

  // Extract the media metadata from a url, asynchronously.
  // OnMediaMetadataExtracted() will be called when this call finishes.
  void ExtractMediaMetadata(const std::string& url);
  void OnMediaMetadataExtracted(base::TimeDelta duration, int width, int height,
                                bool success);

  // Whether the player is prepared for playback.
  bool prepared_;

  // Pending play event while player is preparing.
  bool pending_play_;

  // Pending seek time while player is preparing.
  base::TimeDelta pending_seek_;

  // Url for playback.
  GURL url_;

  // First party url for cookies.
  GURL first_party_for_cookies_;

  // Hide url log from media player.
  bool hide_url_log_;

  // Stats about the media.
  base::TimeDelta duration_;
  int width_;
  int height_;

  // Meta data about actions can be taken.
  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;

  // Cookies for |url_|.
  std::string cookies_;

  // Java MediaPlayer instance.
  base::android::ScopedJavaGlobalRef<jobject> j_media_player_;

  base::RepeatingTimer<MediaPlayerBridge> time_update_timer_;

  // Weak pointer passed to |listener_| for callbacks.
  base::WeakPtrFactory<MediaPlayerBridge> weak_this_;

  // Listener object that listens to all the media player events.
  MediaPlayerListener listener_;

  friend class MediaPlayerListener;
  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridge);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_
