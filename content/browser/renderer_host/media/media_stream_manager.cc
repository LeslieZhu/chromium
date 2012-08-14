// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/media_stream_manager.h"

#include <list>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/win/scoped_com_initializer.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/browser/renderer_host/media/media_stream_device_settings.h"
#include "content/browser/renderer_host/media/media_stream_requester.h"
#include "content/browser/renderer_host/media/video_capture_manager.h"
#include "content/common/media/media_stream_options.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/media_observer.h"
#include "googleurl/src/gurl.h"

using content::BrowserThread;

namespace media_stream {

// Creates a random label used to identify requests.
static std::string RandomLabel() {
  // An earlier PeerConnection spec,
  // http://dev.w3.org/2011/webrtc/editor/webrtc.html, specified the
  // MediaStream::label alphabet as containing 36 characters from
  // range: U+0021, U+0023 to U+0027, U+002A to U+002B, U+002D to U+002E,
  // U+0030 to U+0039, U+0041 to U+005A, U+005E to U+007E.
  // Here we use a safe subset.
  static const char kAlphabet[] = "0123456789"
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  std::string label(36, ' ');
  for (size_t i = 0; i < label.size(); ++i) {
    int random_char = base::RandGenerator(sizeof(kAlphabet) - 1);
    label[i] = kAlphabet[random_char];
  }
  return label;
}

// Helper to verify if a media stream type is part of options or not.
static bool Requested(const StreamOptions& options,
                      MediaStreamType stream_type) {
  return (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE &&
          options.video) ||
         (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE &&
          options.audio);
}

DeviceThread::DeviceThread(const char* name)
    : base::Thread(name) {
}

DeviceThread::~DeviceThread() {
  Stop();
}

void DeviceThread::Init() {
  using base::win::ScopedCOMInitializer;
  // Enter the multi-threaded apartment.
  com_initializer_.reset(new ScopedCOMInitializer(ScopedCOMInitializer::kMTA));
}

void DeviceThread::CleanUp() {
  com_initializer_.reset();
}

// TODO(xians): Merge DeviceRequest with MediaStreamRequest.
struct MediaStreamManager::DeviceRequest {
  enum RequestState {
    STATE_NOT_REQUESTED = 0,
    STATE_REQUESTED,
    STATE_PENDING_APPROVAL,
    STATE_OPENING,
    STATE_DONE,
    STATE_ERROR
  };

  enum RequestType {
    GENERATE_STREAM = 0,
    ENUMERATE_DEVICES,
    OPEN_DEVICE
  };

  DeviceRequest()
      : requester(NULL),
        state(content::NUM_MEDIA_STREAM_DEVICE_TYPES, STATE_NOT_REQUESTED),
        type(GENERATE_STREAM),
        render_process_id(-1),
        render_view_id(-1) {
    options.audio = false;
    options.video = false;
  }

  DeviceRequest(MediaStreamRequester* requester,
                const StreamOptions& request_options,
                int render_process_id,
                int render_view_id,
                const GURL& request_security_origin)
      : requester(requester),
        options(request_options),
        state(content::NUM_MEDIA_STREAM_DEVICE_TYPES, STATE_NOT_REQUESTED),
        type(GENERATE_STREAM),
        render_process_id(render_process_id),
        render_view_id(render_view_id),
        security_origin(request_security_origin) {
    DCHECK(requester);
  }

  ~DeviceRequest() {}

  MediaStreamRequester* requester;
  StreamOptions options;
  std::vector<RequestState> state;
  RequestType type;
  int render_process_id;
  int render_view_id;
  GURL security_origin;
  std::string requested_device_id;
  StreamDeviceInfoArray audio_devices;
  StreamDeviceInfoArray video_devices;
};

MediaStreamManager::EnumerationCache::EnumerationCache()
    : valid(false) {
}

MediaStreamManager::EnumerationCache::~EnumerationCache() {
}

MediaStreamManager::MediaStreamManager(
    AudioInputDeviceManager* audio_input_device_manager,
    VideoCaptureManager* video_capture_manager)
    : ALLOW_THIS_IN_INITIALIZER_LIST(
          device_settings_(new MediaStreamDeviceSettings(this))),
      audio_input_device_manager_(audio_input_device_manager),
      video_capture_manager_(video_capture_manager),
      monitoring_started_(false),
      io_loop_(NULL) {
  memset(active_enumeration_ref_count_, 0,
         sizeof(active_enumeration_ref_count_));
}

MediaStreamManager::~MediaStreamManager() {
  DCHECK(requests_.empty());
  DCHECK(!device_thread_.get());
  DCHECK(!io_loop_);
}

VideoCaptureManager* MediaStreamManager::video_capture_manager() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(video_capture_manager_);
  EnsureDeviceThreadAndListener();
  return video_capture_manager_;
}

AudioInputDeviceManager* MediaStreamManager::audio_input_device_manager() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(audio_input_device_manager_);
  EnsureDeviceThreadAndListener();
  return audio_input_device_manager_;
}

void MediaStreamManager::GenerateStream(MediaStreamRequester* requester,
                                        int render_process_id,
                                        int render_view_id,
                                        const StreamOptions& options,
                                        const GURL& security_origin,
                                        std::string* label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Create a new request based on options.
  DeviceRequest new_request(requester, options,
                            render_process_id,
                            render_view_id,
                            security_origin);
  StartEnumeration(&new_request, label);

  // Get user confirmation to use capture devices.
  device_settings_->RequestCaptureDeviceUsage(*label,
                                              render_process_id,
                                              render_view_id,
                                              options,
                                              security_origin);
}

void MediaStreamManager::CancelGenerateStream(const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  DeviceRequests::iterator it = requests_.find(label);
  if (it != requests_.end()) {
    // The request isn't complete.
    if (!RequestDone(it->second)) {
      DeviceRequest* request = &(it->second);
      if (request->state[content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE] ==
          DeviceRequest::STATE_OPENING) {
        for (StreamDeviceInfoArray::iterator it =
             request->audio_devices.begin(); it != request->audio_devices.end();
             ++it) {
          audio_input_device_manager()->Close(it->session_id);
        }
      }
      if (request->state[content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE] ==
          DeviceRequest::STATE_OPENING) {
        for (StreamDeviceInfoArray::iterator it =
             request->video_devices.begin(); it != request->video_devices.end();
             ++it) {
          video_capture_manager()->Close(it->session_id);
        }
      }
      requests_.erase(it);
    } else {
      StopGeneratedStream(label);
    }
    device_settings_->RemovePendingCaptureRequest(label);
  }
}

void MediaStreamManager::StopGeneratedStream(const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Find the request and close all open devices for the request.
  DeviceRequests::iterator it = requests_.find(label);
  if (it != requests_.end()) {
    if (it->second.type == DeviceRequest::ENUMERATE_DEVICES) {
      StopEnumerateDevices(label);
      return;
    }
    for (StreamDeviceInfoArray::iterator audio_it =
         it->second.audio_devices.begin();
         audio_it != it->second.audio_devices.end(); ++audio_it) {
      audio_input_device_manager()->Close(audio_it->session_id);
    }
    for (StreamDeviceInfoArray::iterator video_it =
         it->second.video_devices.begin();
         video_it != it->second.video_devices.end(); ++video_it) {
      video_capture_manager()->Close(video_it->session_id);
    }
    if (it->second.type == DeviceRequest::GENERATE_STREAM &&
        RequestDone(it->second)) {
      NotifyObserverDevicesClosed(&(it->second));
    }
    requests_.erase(it);
  }
}

void MediaStreamManager::EnumerateDevices(
    MediaStreamRequester* requester,
    int render_process_id,
    int render_view_id,
    MediaStreamType type,
    const GURL& security_origin,
    std::string* label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE ||
         type == content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE);

  // Create a new request.
  StreamOptions options;
  EnumerationCache* cache = NULL;
  if (type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
    options.audio = true;
    cache = &audio_enumeration_cache_;
  } else {
    options.video = true;
    cache = &video_enumeration_cache_;
  }

  DeviceRequest new_request(requester, options,
                            render_process_id,
                            render_view_id,
                            security_origin);
  new_request.type = DeviceRequest::ENUMERATE_DEVICES;

  if (cache->valid) {
    // Cached device list of this type exists. Just send it out.
    new_request.state[type] = DeviceRequest::STATE_REQUESTED;
    AddRequest(&new_request, label);
    // Need to post a task since the requester won't have label till
    // this function returns.
    BrowserThread::PostTask(BrowserThread::IO,
        FROM_HERE,
        base::Bind(&MediaStreamManager::SendCachedDeviceList,
                   base::Unretained(this), cache, *label));
  } else {
    StartEnumeration(&new_request, label);
    StartMonitoring();
  }
}

void MediaStreamManager::StopEnumerateDevices(const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  DeviceRequests::iterator it = requests_.find(label);
  if (it != requests_.end()) {
    DCHECK_EQ(it->second.type, DeviceRequest::ENUMERATE_DEVICES);
    requests_.erase(it);
    if (!HasEnumerationRequest()) {
      StopMonitoring();
    }
  }
}

void MediaStreamManager::OpenDevice(
    MediaStreamRequester* requester,
    int render_process_id,
    int render_view_id,
    const std::string& device_id,
    MediaStreamType type,
    const GURL& security_origin,
    std::string* label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Create a new request.
  StreamOptions options;
  if (type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE)
    options.audio = true;
  else
    options.video = true;

  DeviceRequest new_request(requester, options,
                            render_process_id,
                            render_view_id,
                            security_origin);
  new_request.type = DeviceRequest::OPEN_DEVICE;
  new_request.requested_device_id = device_id;

  StartEnumeration(&new_request, label);
}

void MediaStreamManager::SendCachedDeviceList(
    EnumerationCache* cache,
    const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (cache->valid) {
    DeviceRequests::iterator it = requests_.find(label);
    if (it != requests_.end()) {
      it->second.requester->DevicesEnumerated(label, cache->devices);
    }
  }
}

void MediaStreamManager::StartMonitoring() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!monitoring_started_) {
    monitoring_started_ = true;
    base::SystemMonitor::Get()->AddDevicesChangedObserver(this);
  }
}

void MediaStreamManager::StopMonitoring() {
  DCHECK_EQ(MessageLoop::current(), io_loop_);
  if (monitoring_started_ && !HasEnumerationRequest()) {
    base::SystemMonitor::Get()->RemoveDevicesChangedObserver(this);
    monitoring_started_ = false;
    ClearEnumerationCache(&audio_enumeration_cache_);
    ClearEnumerationCache(&video_enumeration_cache_);
  }
}

void MediaStreamManager::ClearEnumerationCache(EnumerationCache* cache) {
  DCHECK_EQ(MessageLoop::current(), io_loop_);
  cache->valid = false;
}

void MediaStreamManager::StartEnumeration(
    DeviceRequest* new_request,
    std::string* label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  MediaStreamType stream_type = content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE;
  if (Requested(new_request->options, stream_type)) {
    new_request->state[stream_type] = DeviceRequest::STATE_REQUESTED;
    DCHECK_GE(active_enumeration_ref_count_[stream_type], 0);
    if (!active_enumeration_ref_count_[stream_type]) {
      ++active_enumeration_ref_count_[stream_type];
      GetDeviceManager(stream_type)->EnumerateDevices();
    }
  }
  stream_type = content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE;
  if (Requested(new_request->options, stream_type)) {
    new_request->state[stream_type] = DeviceRequest::STATE_REQUESTED;
    DCHECK_GE(active_enumeration_ref_count_[stream_type], 0);
    if (!active_enumeration_ref_count_[stream_type]) {
      ++active_enumeration_ref_count_[stream_type];
      GetDeviceManager(stream_type)->EnumerateDevices();
    }
  }

  AddRequest(new_request, label);
}

void MediaStreamManager::AddRequest(
    DeviceRequest* new_request,
    std::string* label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Create a label for this request and verify it is unique.
  std::string request_label;
  do {
    request_label = RandomLabel();
  } while (requests_.find(request_label) != requests_.end());

  requests_.insert(std::make_pair(request_label, *new_request));

  (*label) = request_label;
}

void MediaStreamManager::EnsureDeviceThreadAndListener() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (device_thread_.get())
    return;

  device_thread_.reset(new DeviceThread("MediaStreamDeviceThread"));
  CHECK(device_thread_->Start());

  audio_input_device_manager_->Register(this,
                                        device_thread_->message_loop_proxy());
  video_capture_manager_->Register(this, device_thread_->message_loop_proxy());

  // We want to be notified of IO message loop destruction to delete the thread
  // and the device managers.
  io_loop_ = MessageLoop::current();
  io_loop_->AddDestructionObserver(this);
}

void MediaStreamManager::Opened(MediaStreamType stream_type,
                                int capture_session_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Find the request containing this device and mark it as used.
  DeviceRequest* request = NULL;
  StreamDeviceInfoArray* devices = NULL;
  std::string label;
  for (DeviceRequests::iterator request_it = requests_.begin();
       request_it != requests_.end() && request == NULL; ++request_it) {
    if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
      devices = &(request_it->second.audio_devices);
    } else if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE) {
      devices = &(request_it->second.video_devices);
    } else {
      NOTREACHED();
    }

    for (StreamDeviceInfoArray::iterator device_it = devices->begin();
         device_it != devices->end(); ++device_it) {
      if (device_it->session_id == capture_session_id) {
        // We've found the request.
        device_it->in_use = true;
        label = request_it->first;
        request = &(request_it->second);
        break;
      }
    }
  }
  if (request == NULL) {
    // The request doesn't exist.
    return;
  }

  DCHECK_NE(request->state[stream_type], DeviceRequest::STATE_REQUESTED);

  // Check if all devices for this stream type are opened. Update the state if
  // they are.
  for (StreamDeviceInfoArray::iterator device_it = devices->begin();
       device_it != devices->end(); ++device_it) {
    if (device_it->in_use == false) {
      // Wait for more devices to be opened before we're done.
      return;
    }
  }
  request->state[stream_type] = DeviceRequest::STATE_DONE;

  if (!RequestDone(*request)) {
    // This stream_type is done, but not the other type.
    return;
  }

  switch (request->type) {
    case DeviceRequest::OPEN_DEVICE:
      request->requester->DeviceOpened(label, (*devices)[0]);
      break;
    case DeviceRequest::GENERATE_STREAM:
      request->requester->StreamGenerated(label, request->audio_devices,
                                          request->video_devices);
      NotifyObserverDevicesOpened(request);
      break;
    default:
      NOTREACHED();
  }
}

void MediaStreamManager::Closed(MediaStreamType stream_type,
                                int capture_session_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
}

void MediaStreamManager::DevicesEnumerated(
    MediaStreamType stream_type, const StreamDeviceInfoArray& devices) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Only cache device list when there is EnumerateDevices request, since
  // other requests don't turn on device monitoring.
  bool need_update_clients = false;
  EnumerationCache* cache =
      (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE ?
       &audio_enumeration_cache_ : &video_enumeration_cache_);
  if (HasEnumerationRequest(stream_type) &&
      (!cache->valid ||
       devices.size() != cache->devices.size() ||
       !std::equal(devices.begin(), devices.end(), cache->devices.begin(),
                   media_stream::StreamDeviceInfo::IsEqual))) {
    cache->valid = true;
    cache->devices = devices;
    need_update_clients = true;
  }

  // Publish the result for all requests waiting for device list(s).
  // Find the requests waiting for this device list, store their labels and
  // release the iterator before calling device settings. We might get a call
  // back from device_settings that will need to iterate through devices.
  std::list<std::string> label_list;
  for (DeviceRequests::iterator it = requests_.begin(); it != requests_.end();
       ++it) {
    if (it->second.state[stream_type] == DeviceRequest::STATE_REQUESTED &&
        Requested(it->second.options, stream_type)) {
      if (it->second.type != DeviceRequest::ENUMERATE_DEVICES)
        it->second.state[stream_type] = DeviceRequest::STATE_PENDING_APPROVAL;
      label_list.push_back(it->first);
    }
  }
  for (std::list<std::string>::iterator it = label_list.begin();
       it != label_list.end(); ++it) {
    DeviceRequest& request = requests_[*it];
    switch (request.type) {
      case DeviceRequest::ENUMERATE_DEVICES:
        if (need_update_clients)
          request.requester->DevicesEnumerated(*it, devices);
        break;
      case DeviceRequest::OPEN_DEVICE:
        for (StreamDeviceInfoArray::const_iterator device_it = devices.begin();
             device_it != devices.end(); device_it++) {
          if (request.requested_device_id == device_it->device_id) {
            StreamDeviceInfo device = *device_it;
            device.in_use = false;
            device.session_id =
                GetDeviceManager(device_it->stream_type)->Open(device);
            request.state[device_it->stream_type] =
                DeviceRequest::STATE_OPENING;
            if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE)
              request.audio_devices.push_back(device);
            else
              request.video_devices.push_back(device);
            break;
          }
        }
        break;
      default:
        device_settings_->AvailableDevices(*it, stream_type, devices);
    }
  }
  label_list.clear();
  --active_enumeration_ref_count_[stream_type];
  DCHECK_GE(active_enumeration_ref_count_[stream_type], 0);
}

void MediaStreamManager::Error(MediaStreamType stream_type,
                               int capture_session_id,
                               MediaStreamProviderError error) {
  // Find the device for the error call.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  for (DeviceRequests::iterator it = requests_.begin(); it != requests_.end();
       ++it) {
    StreamDeviceInfoArray* devices = NULL;
    if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
      devices = &(it->second.audio_devices);
    } else if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE) {
      devices = &(it->second.video_devices);
    } else {
      NOTREACHED();
    }

    int device_idx = 0;
    for (StreamDeviceInfoArray::iterator device_it = devices->begin();
         device_it != devices->end(); ++device_it, ++device_idx) {
      if (device_it->session_id == capture_session_id) {
        // We've found the failing device. Find the error case:
        if (it->second.state[stream_type] == DeviceRequest::STATE_DONE) {
          // 1. Already opened -> signal device failure and close device.
          //    Use device_idx to signal which of the devices encountered an
          //    error.
          if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
            it->second.requester->AudioDeviceFailed(it->first, device_idx);
          } else if (stream_type ==
                     content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE) {
            it->second.requester->VideoDeviceFailed(it->first, device_idx);
          }
          GetDeviceManager(stream_type)->Close(capture_session_id);
          // We don't erase the devices here so that we can update the UI
          // properly in StopGeneratedStream().
          it->second.state[stream_type] = DeviceRequest::STATE_ERROR;
        } else {
          // Request is not done, devices are not opened in this case.
          if ((it->second.audio_devices.size() +
               it->second.video_devices.size()) <= 1) {
            // 2. Device not opened and no other devices for this request ->
            //    signal stream error and remove the request.
            it->second.requester->StreamGenerationFailed(it->first);
            requests_.erase(it);
          } else {
            // 3. Not opened but other devices exists for this request -> remove
            //    device from list, but don't signal an error.
            devices->erase(device_it);
          }
        }
        return;
      }
    }
  }
}

void MediaStreamManager::DevicesAccepted(const std::string& label,
                                         const StreamDeviceInfoArray& devices) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DeviceRequests::iterator request_it = requests_.find(label);
  if (request_it != requests_.end()) {
    if (devices.empty()) {
      // No available devices or user didn't accept device usage.
      request_it->second.requester->StreamGenerationFailed(request_it->first);
      requests_.erase(request_it);
      return;
    }

    // Loop through all device types for this request.
    for (StreamDeviceInfoArray::const_iterator device_it = devices.begin();
         device_it != devices.end(); ++device_it) {
      StreamDeviceInfo device_info = *device_it;

      // Set in_use to false to be able to track if this device has been
      // opened. in_use might be true if the device type can be used in more
      // than one session.
      DCHECK_EQ(request_it->second.state[device_it->stream_type],
                DeviceRequest::STATE_PENDING_APPROVAL);
      device_info.in_use = false;
      device_info.session_id =
          GetDeviceManager(device_info.stream_type)->Open(device_info);
      request_it->second.state[device_it->stream_type] =
          DeviceRequest::STATE_OPENING;
      if (device_info.stream_type ==
          content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
        request_it->second.audio_devices.push_back(device_info);
      } else if (device_info.stream_type ==
                 content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE) {
        request_it->second.video_devices.push_back(device_info);
      } else {
        NOTREACHED();
      }
    }
    // Check if we received all stream types requested.
    MediaStreamType stream_type =
        content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE;
    if (Requested(request_it->second.options, stream_type) &&
        request_it->second.audio_devices.size() == 0) {
      request_it->second.state[stream_type] = DeviceRequest::STATE_ERROR;
    }
    stream_type = content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE;
    if (Requested(request_it->second.options, stream_type) &&
        request_it->second.video_devices.size() == 0) {
      request_it->second.state[stream_type] = DeviceRequest::STATE_ERROR;
    }
    return;
  }
}

void MediaStreamManager::SettingsError(const std::string& label) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Erase this request and report an error.
  DeviceRequests::iterator it = requests_.find(label);
  if (it != requests_.end()) {
    DCHECK_EQ(it->second.type, DeviceRequest::GENERATE_STREAM);
    it->second.requester->StreamGenerationFailed(label);
    requests_.erase(it);
    return;
  }
}

void MediaStreamManager::UseFakeDevice() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  video_capture_manager()->UseFakeDevice();
  device_settings_->UseFakeUI();
}

void MediaStreamManager::WillDestroyCurrentMessageLoop() {
  DCHECK_EQ(MessageLoop::current(), io_loop_);
  DCHECK(requests_.empty());
  if (device_thread_.get()) {
    StopMonitoring();

    video_capture_manager_->Unregister();
    audio_input_device_manager_->Unregister();
    device_thread_.reset();
  }

  audio_input_device_manager_ = NULL;
  video_capture_manager_ = NULL;
  io_loop_ = NULL;
  device_settings_.reset();
}

void MediaStreamManager::NotifyObserverDevicesOpened(DeviceRequest* request) {
  content::MediaObserver* media_observer =
      content::GetContentClient()->browser()->GetMediaObserver();
  content::MediaStreamDevices opened_devices;
  DevicesFromRequest(request, &opened_devices);
  DCHECK(!opened_devices.empty());
  media_observer->OnCaptureDevicesOpened(request->render_process_id,
                                         request->render_view_id,
                                         opened_devices);
}

void MediaStreamManager::NotifyObserverDevicesClosed(DeviceRequest* request) {
  content::MediaObserver* media_observer =
      content::GetContentClient()->browser()->GetMediaObserver();
  content::MediaStreamDevices closed_devices;
  DevicesFromRequest(request, &closed_devices);
  if (closed_devices.empty())
    return;
  media_observer->OnCaptureDevicesClosed(request->render_process_id,
                                         request->render_view_id,
                                         closed_devices);
}

void MediaStreamManager::DevicesFromRequest(
    DeviceRequest* request, content::MediaStreamDevices* devices) {
  StreamDeviceInfoArray::const_iterator it = request->audio_devices.begin();
  for (; it != request->audio_devices.end(); ++it) {
    devices->push_back(
        content::MediaStreamDevice(
            content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE,
            it->device_id,
            it->name));
  }
  for (it = request->video_devices.begin(); it != request->video_devices.end();
       ++it) {
    devices->push_back(
        content::MediaStreamDevice(
            content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE,
            it->device_id,
            it->name));
  }
}

bool MediaStreamManager::RequestDone(const DeviceRequest& request) const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Check if all devices are opened.
  MediaStreamType stream_type = content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE;
  if (Requested(request.options, stream_type)) {
    if (request.state[stream_type] != DeviceRequest::STATE_DONE &&
        request.state[stream_type] != DeviceRequest::STATE_ERROR) {
      return false;
    }

    for (StreamDeviceInfoArray::const_iterator it =
         request.audio_devices.begin(); it != request.audio_devices.end();
         ++it) {
      if (it->in_use == false) {
        return false;
      }
    }
  }

  stream_type = content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE;
  if (Requested(request.options, stream_type)) {
    if (request.state[stream_type] != DeviceRequest::STATE_DONE &&
        request.state[stream_type] != DeviceRequest::STATE_ERROR) {
      return false;
    }

    for (StreamDeviceInfoArray::const_iterator it =
         request.video_devices.begin(); it != request.video_devices.end();
         ++it) {
      if (it->in_use == false) {
        return false;
      }
    }
  }

  return true;
}

// Called to get media capture device manager of specified type.
MediaStreamProvider* MediaStreamManager::GetDeviceManager(
    MediaStreamType stream_type) {
  if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE) {
    return video_capture_manager();
  } else if (stream_type == content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE) {
    return audio_input_device_manager();
  }
  NOTREACHED();
  return NULL;
}

void MediaStreamManager::OnDevicesChanged(
    base::SystemMonitor::DeviceType device_type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  MediaStreamType stream_type;
  EnumerationCache* cache;
  if (device_type == base::SystemMonitor::DEVTYPE_AUDIO_CAPTURE) {
    stream_type = content::MEDIA_STREAM_DEVICE_TYPE_AUDIO_CAPTURE;
    cache = &audio_enumeration_cache_;
  } else if (device_type == base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE) {
    stream_type = content::MEDIA_STREAM_DEVICE_TYPE_VIDEO_CAPTURE;
    cache = &video_enumeration_cache_;
  } else {
    return;  // Uninteresting device change.
  }

  if (!HasEnumerationRequest(stream_type)) {
    // There is no request for that type, No need to enumerate devices.
    // Therefore, invalidate the cache of that type.
    ClearEnumerationCache(cache);
    return;
  }

  // Always do enumeration even though some enumeration is in progress,
  // because those enumeration commands could be sent before these devices
  // change.
  ++active_enumeration_ref_count_[stream_type];
  GetDeviceManager(stream_type)->EnumerateDevices();
}

bool MediaStreamManager::HasEnumerationRequest() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  for (DeviceRequests::iterator it = requests_.begin();
       it != requests_.end(); ++it) {
    if (it->second.type == DeviceRequest::ENUMERATE_DEVICES) {
      return true;
    }
  }
  return false;
}

bool MediaStreamManager::HasEnumerationRequest(
    MediaStreamType stream_type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  for (DeviceRequests::iterator it = requests_.begin();
       it != requests_.end(); ++it) {
    if (it->second.type == DeviceRequest::ENUMERATE_DEVICES &&
        Requested(it->second.options, stream_type)) {
      return true;
    }
  }
  return false;
}

}  // namespace media_stream
