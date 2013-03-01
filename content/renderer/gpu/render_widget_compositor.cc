// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/render_widget_compositor.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "base/time.h"
#include "cc/context_provider.h"
#include "cc/layer.h"
#include "cc/layer_tree_debug_state.h"
#include "cc/layer_tree_host.h"
#include "cc/switches.h"
#include "cc/thread_impl.h"
#include "content/renderer/gpu/compositor_thread.h"
#include "content/renderer/render_thread_impl.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebLayerTreeViewClient.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebSharedGraphicsContext3D.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebSize.h"
#include "webkit/compositor_bindings/web_layer_impl.h"
#include "webkit/compositor_bindings/web_to_ccinput_handler_adapter.h"

namespace cc {
class Layer;
}

using WebKit::WebFloatPoint;
using WebKit::WebSize;
using WebKit::WebRect;

namespace content {
namespace {

bool GetSwitchValueAsInt(
    const CommandLine& command_line,
    const std::string& switch_string,
    int min_value,
    int max_value,
    int* result) {
  std::string string_value = command_line.GetSwitchValueASCII(switch_string);
  int int_value;
  if (base::StringToInt(string_value, &int_value) &&
      int_value >= min_value && int_value <= max_value) {
    *result = int_value;
    return true;
  } else {
    LOG(WARNING) << "Failed to parse switch " << switch_string  << ": " <<
        string_value;
    return false;
  }
}

bool GetSwitchValueAsFloat(
    const CommandLine& command_line,
    const std::string& switch_string,
    float min_value,
    float max_value,
    float* result) {
  std::string string_value = command_line.GetSwitchValueASCII(switch_string);
  double double_value;
  if (base::StringToDouble(string_value, &double_value) &&
      double_value >= min_value && double_value <= max_value) {
    *result = static_cast<float>(double_value);
    return true;
  } else {
    LOG(WARNING) << "Failed to parse switch " << switch_string  << ": " <<
        string_value;
    return false;
  }
}


}  // namespace

// static
scoped_ptr<RenderWidgetCompositor> RenderWidgetCompositor::Create(
      RenderWidget* widget,
      WebKit::WebLayerTreeViewClient* client,
      WebKit::WebLayerTreeView::Settings web_settings) {
  scoped_ptr<RenderWidgetCompositor> comp(
      new RenderWidgetCompositor(widget, client));

  CommandLine* cmd = CommandLine::ForCurrentProcess();

  cc::LayerTreeSettings settings;
  settings.acceleratePainting = web_settings.acceleratePainting;
  settings.renderVSyncEnabled = web_settings.renderVSyncEnabled;
  settings.perTilePaintingEnabled = web_settings.perTilePaintingEnabled;
  settings.acceleratedAnimationEnabled =
      web_settings.acceleratedAnimationEnabled;
  settings.refreshRate = web_settings.refreshRate;
  settings.defaultTileSize = web_settings.defaultTileSize;
  settings.maxUntiledLayerSize = web_settings.maxUntiledLayerSize;

  settings.rightAlignedSchedulingEnabled =
      cmd->HasSwitch(cc::switches::kEnableRightAlignedScheduling);
  settings.implSidePainting = cc::switches::IsImplSidePaintingEnabled();
  settings.useCheapnessEstimator =
      cmd->HasSwitch(cc::switches::kUseCheapnessEstimator);

  settings.calculateTopControlsPosition =
      cmd->HasSwitch(cc::switches::kEnableTopControlsPositionCalculation);
  if (cmd->HasSwitch(cc::switches::kTopControlsHeight)) {
    std::string controls_height_str =
        cmd->GetSwitchValueASCII(cc::switches::kTopControlsHeight);
    double controls_height;
    if (base::StringToDouble(controls_height_str, &controls_height) &&
        controls_height > 0)
      settings.topControlsHeight = controls_height;
  }

  settings.compositorFrameMessage =
      cmd->HasSwitch(cc::switches::kEnableCompositorFrameMessage);

  if (settings.calculateTopControlsPosition &&
      (settings.topControlsHeight <= 0 || !settings.compositorFrameMessage)) {
    DCHECK(false) << "Top controls repositioning enabled without valid height "
                     "or compositorFrameMessage set.";
    settings.calculateTopControlsPosition = false;
  }

  if (cmd->HasSwitch(cc::switches::kTopControlsShowThreshold)) {
      std::string top_threshold_str =
          cmd->GetSwitchValueASCII(cc::switches::kTopControlsShowThreshold);
      double show_threshold;
      if (base::StringToDouble(top_threshold_str, &show_threshold) &&
          show_threshold >= 0.f && show_threshold <= 1.f)
        settings.topControlsShowThreshold = show_threshold;
  }

  if (cmd->HasSwitch(cc::switches::kTopControlsHideThreshold)) {
      std::string top_threshold_str =
          cmd->GetSwitchValueASCII(cc::switches::kTopControlsHideThreshold);
      double hide_threshold;
      if (base::StringToDouble(top_threshold_str, &hide_threshold) &&
          hide_threshold >= 0.f && hide_threshold <= 1.f)
        settings.topControlsHideThreshold = hide_threshold;
  }

  settings.partialSwapEnabled =
      cmd->HasSwitch(cc::switches::kEnablePartialSwap);
  settings.backgroundColorInsteadOfCheckerboard =
      cmd->HasSwitch(cc::switches::kBackgroundColorInsteadOfCheckerboard);
  settings.showOverdrawInTracing =
      cmd->HasSwitch(cc::switches::kTraceOverdraw);

  settings.initialDebugState.showFPSCounter = web_settings.showFPSCounter;
  settings.initialDebugState.showPaintRects = web_settings.showPaintRects;
  settings.initialDebugState.showPlatformLayerTree =
      web_settings.showPlatformLayerTree;
  settings.initialDebugState.showDebugBorders = web_settings.showDebugBorders;
  settings.initialDebugState.showPropertyChangedRects =
      cmd->HasSwitch(cc::switches::kShowPropertyChangedRects);
  settings.initialDebugState.showSurfaceDamageRects =
      cmd->HasSwitch(cc::switches::kShowSurfaceDamageRects);
  settings.initialDebugState.showScreenSpaceRects =
      cmd->HasSwitch(cc::switches::kShowScreenSpaceRects);
  settings.initialDebugState.showReplicaScreenSpaceRects =
      cmd->HasSwitch(cc::switches::kShowReplicaScreenSpaceRects);
  settings.initialDebugState.showOccludingRects =
      cmd->HasSwitch(cc::switches::kShowOccludingRects);
  settings.initialDebugState.showNonOccludingRects =
      cmd->HasSwitch(cc::switches::kShowNonOccludingRects);
  settings.initialDebugState.setRecordRenderingStats(
      web_settings.recordRenderingStats);
  settings.initialDebugState.traceAllRenderedFrames =
      cmd->HasSwitch(cc::switches::kTraceAllRenderedFrames);

  if (cmd->HasSwitch(cc::switches::kSlowDownRasterScaleFactor)) {
    const int kMinSlowDownScaleFactor = 0;
    const int kMaxSlowDownScaleFactor = INT_MAX;
    GetSwitchValueAsInt(*cmd, cc::switches::kSlowDownRasterScaleFactor,
                        kMinSlowDownScaleFactor, kMaxSlowDownScaleFactor,
                        &settings.initialDebugState.slowDownRasterScaleFactor);
  }

  if (cmd->HasSwitch(cc::switches::kNumRasterThreads)) {
    const int kMinRasterThreads = 1;
    const int kMaxRasterThreads = 64;
    int num_raster_threads;
    if (GetSwitchValueAsInt(*cmd, cc::switches::kNumRasterThreads,
                            kMinRasterThreads, kMaxRasterThreads,
                            &num_raster_threads))
      settings.numRasterThreads = num_raster_threads;
  }

  if (cmd->HasSwitch(cc::switches::kLowResolutionContentsScaleFactor)) {
    const int kMinScaleFactor = settings.minimumContentsScale;
    const int kMaxScaleFactor = 1;
    GetSwitchValueAsFloat(*cmd,
                          cc::switches::kLowResolutionContentsScaleFactor,
                          kMinScaleFactor, kMaxScaleFactor,
                          &settings.lowResContentsScaleFactor);
  }

#if defined(OS_ANDROID)
  // TODO(danakj): Move these to the android code.
  settings.canUseLCDText = false;
  settings.maxPartialTextureUpdates = 0;
  settings.useLinearFadeScrollbarAnimator = true;
  settings.solidColorScrollbars = true;
  settings.solidColorScrollbarColor = SkColorSetARGB(128, 128, 128, 128);
  settings.solidColorScrollbarThicknessDIP = 3;
#endif

  if (!comp->initialize(settings))
    return scoped_ptr<RenderWidgetCompositor>();

  return comp.Pass();
}

RenderWidgetCompositor::RenderWidgetCompositor(
    RenderWidget* widget, WebKit::WebLayerTreeViewClient* client)
  : suppress_schedule_composite_(false),
    widget_(widget),
    client_(client) {
}

RenderWidgetCompositor::~RenderWidgetCompositor() {}

void RenderWidgetCompositor::SetSuppressScheduleComposite(bool suppress) {
  if (suppress_schedule_composite_ == suppress)
    return;

  if (suppress)
    TRACE_EVENT_ASYNC_BEGIN0("gpu",
        "RenderWidgetCompositor::SetSuppressScheduleComposite", this);
  else
    TRACE_EVENT_ASYNC_END0("gpu",
        "RenderWidgetCompositor::SetSuppressScheduleComposite", this);
  suppress_schedule_composite_ = suppress;
}

bool RenderWidgetCompositor::initialize(cc::LayerTreeSettings settings) {
  scoped_ptr<cc::Thread> impl_thread;
  CompositorThread* compositor_thread =
      RenderThreadImpl::current()->compositor_thread();
  threaded_ = !!compositor_thread;
  if (compositor_thread)
    impl_thread = cc::ThreadImpl::createForDifferentThread(
        compositor_thread->message_loop()->message_loop_proxy());
  layer_tree_host_ = cc::LayerTreeHost::create(this,
                                               settings,
                                               impl_thread.Pass());
  return layer_tree_host_;
}

void RenderWidgetCompositor::setSurfaceReady() {
  layer_tree_host_->setSurfaceReady();
}

void RenderWidgetCompositor::setRootLayer(const WebKit::WebLayer& layer) {
  layer_tree_host_->setRootLayer(
      static_cast<const WebKit::WebLayerImpl*>(&layer)->layer());
}

void RenderWidgetCompositor::clearRootLayer() {
  layer_tree_host_->setRootLayer(scoped_refptr<cc::Layer>());
}

void RenderWidgetCompositor::setViewportSize(
    const WebSize& layout_viewport_size,
    const WebSize& device_viewport_size) {
  layer_tree_host_->setViewportSize(layout_viewport_size, device_viewport_size);
}

WebSize RenderWidgetCompositor::layoutViewportSize() const {
  return layer_tree_host_->layoutViewportSize();
}

WebSize RenderWidgetCompositor::deviceViewportSize() const {
  return layer_tree_host_->deviceViewportSize();
}

WebFloatPoint RenderWidgetCompositor::adjustEventPointForPinchZoom(
    const WebFloatPoint& point) const {
  return point;
}

void RenderWidgetCompositor::setDeviceScaleFactor(float device_scale) {
  layer_tree_host_->setDeviceScaleFactor(device_scale);
}

float RenderWidgetCompositor::deviceScaleFactor() const {
  return layer_tree_host_->deviceScaleFactor();
}

void RenderWidgetCompositor::setBackgroundColor(WebKit::WebColor color) {
  layer_tree_host_->setBackgroundColor(color);
}

void RenderWidgetCompositor::setHasTransparentBackground(bool transparent) {
  layer_tree_host_->setHasTransparentBackground(transparent);
}

void RenderWidgetCompositor::setVisible(bool visible) {
  layer_tree_host_->setVisible(visible);
}

void RenderWidgetCompositor::setPageScaleFactorAndLimits(
    float page_scale_factor, float minimum, float maximum) {
  layer_tree_host_->setPageScaleFactorAndLimits(
      page_scale_factor, minimum, maximum);
}

void RenderWidgetCompositor::startPageScaleAnimation(
    const WebKit::WebPoint& destination,
    bool use_anchor,
    float new_page_scale,
    double duration_sec) {
  base::TimeDelta duration = base::TimeDelta::FromMicroseconds(
      duration_sec * base::Time::kMicrosecondsPerSecond);
  layer_tree_host_->startPageScaleAnimation(
      gfx::Vector2d(destination.x, destination.y),
      use_anchor,
      new_page_scale,
      duration);
}

void RenderWidgetCompositor::setNeedsAnimate() {
  layer_tree_host_->setNeedsAnimate();
}

void RenderWidgetCompositor::setNeedsRedraw() {
  if (threaded_)
    layer_tree_host_->setNeedsAnimate();
  else
    widget_->scheduleAnimation();
}

bool RenderWidgetCompositor::commitRequested() const {
  return layer_tree_host_->commitRequested();
}

void RenderWidgetCompositor::composite() {
  layer_tree_host_->composite();
}

void RenderWidgetCompositor::updateAnimations(double frame_begin_time_sec) {
  base::TimeTicks frame_begin_time =
    base::TimeTicks::FromInternalValue(frame_begin_time_sec *
                                       base::Time::kMicrosecondsPerSecond);
  layer_tree_host_->updateAnimations(frame_begin_time);
}

void RenderWidgetCompositor::didStopFlinging() {
  layer_tree_host_->didStopFlinging();
}

bool RenderWidgetCompositor::compositeAndReadback(void *pixels,
                                                  const WebRect& rect) {
  return layer_tree_host_->compositeAndReadback(pixels, rect);
}

void RenderWidgetCompositor::finishAllRendering() {
  layer_tree_host_->finishAllRendering();
}

void RenderWidgetCompositor::setDeferCommits(bool defer_commits) {
  layer_tree_host_->setDeferCommits(defer_commits);
}

void RenderWidgetCompositor::setShowFPSCounter(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->debugState();
  debug_state.showFPSCounter = show;
  layer_tree_host_->setDebugState(debug_state);
}

void RenderWidgetCompositor::setShowPaintRects(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->debugState();
  debug_state.showPaintRects = show;
  layer_tree_host_->setDebugState(debug_state);
}

void RenderWidgetCompositor::setShowDebugBorders(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->debugState();
  debug_state.showDebugBorders = show;
  layer_tree_host_->setDebugState(debug_state);
}

void RenderWidgetCompositor::setContinuousPaintingEnabled(bool enabled) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->debugState();
  debug_state.continuousPainting = enabled;
  layer_tree_host_->setDebugState(debug_state);
}

void RenderWidgetCompositor::willBeginFrame() {
  widget_->InstrumentWillBeginFrame();
  widget_->willBeginCompositorFrame();
}

void RenderWidgetCompositor::didBeginFrame() {
  widget_->InstrumentDidBeginFrame();
}

void RenderWidgetCompositor::animate(double monotonic_frame_begin_time) {
  widget_->webwidget()->animate(monotonic_frame_begin_time);
}

// Can delete from WebLayerTreeViewClient
void RenderWidgetCompositor::layout() {
  widget_->webwidget()->layout();
}

// TODO(jamesr): This should go through WebWidget
void RenderWidgetCompositor::applyScrollAndScale(gfx::Vector2d scroll_delta,
                                                 float page_scale) {
  client_->applyScrollAndScale(scroll_delta, page_scale);
}

scoped_ptr<cc::OutputSurface> RenderWidgetCompositor::createOutputSurface() {
  return widget_->CreateOutputSurface();
}

// TODO(jamesr): This should go through WebWidget
void RenderWidgetCompositor::didRecreateOutputSurface(bool success) {
  client_->didRecreateOutputSurface(success);
}

// TODO(jamesr): This should go through WebWidget
scoped_ptr<cc::InputHandler> RenderWidgetCompositor::createInputHandler() {
  scoped_ptr<cc::InputHandler> ret;
  scoped_ptr<WebKit::WebInputHandler> web_handler(
      client_->createInputHandler());
  if (web_handler)
     ret = WebKit::WebToCCInputHandlerAdapter::create(web_handler.Pass());
  return ret.Pass();
}

void RenderWidgetCompositor::willCommit() {
  widget_->InstrumentWillComposite();
}

void RenderWidgetCompositor::didCommit() {
  widget_->DidCommitCompositorFrame();
  widget_->didBecomeReadyForAdditionalInput();
}

void RenderWidgetCompositor::didCommitAndDrawFrame() {
  widget_->didCommitAndDrawCompositorFrame();
}

void RenderWidgetCompositor::didCompleteSwapBuffers() {
  widget_->didCompleteSwapBuffers();
}

void RenderWidgetCompositor::scheduleComposite() {
  if (!suppress_schedule_composite_)
    widget_->scheduleComposite();
}

class RenderWidgetCompositor::MainThreadContextProvider
    : public cc::ContextProvider {
 public:
  virtual bool InitializeOnMainThread() OVERRIDE { return true; }
  virtual bool BindToCurrentThread() OVERRIDE { return true; }

  virtual WebKit::WebGraphicsContext3D* Context3d() OVERRIDE {
    return WebKit::WebSharedGraphicsContext3D::mainThreadContext();
  }
  virtual class GrContext* GrContext() OVERRIDE {
    return WebKit::WebSharedGraphicsContext3D::mainThreadGrContext();
  }

  virtual void VerifyContexts() OVERRIDE {}

 protected:
  virtual ~MainThreadContextProvider() {}
};

scoped_refptr<cc::ContextProvider>
RenderWidgetCompositor::OffscreenContextProviderForMainThread() {
  if (!contexts_main_thread_)
    contexts_main_thread_ = new MainThreadContextProvider;
  return contexts_main_thread_;
}

class RenderWidgetCompositor::CompositorThreadContextProvider
    : public cc::ContextProvider {
 public:
  CompositorThreadContextProvider() : initialized_(false), destroyed_(false) {}

  virtual bool InitializeOnMainThread() OVERRIDE {
    if (!initialized_) {
      initialized_ =
          WebKit::WebSharedGraphicsContext3D::createCompositorThreadContext();
    }
    return initialized_;
  }
  virtual bool BindToCurrentThread() OVERRIDE {
    return Context3d()->makeContextCurrent();
  }

  virtual WebKit::WebGraphicsContext3D* Context3d() OVERRIDE {
    return WebKit::WebSharedGraphicsContext3D::compositorThreadContext();
  }
  virtual class GrContext* GrContext() OVERRIDE {
    return WebKit::WebSharedGraphicsContext3D::compositorThreadGrContext();
  }

  virtual void VerifyContexts() OVERRIDE {
    if (Context3d() && !Context3d()->isContextLost())
      return;
    base::AutoLock lock(destroyed_lock_);
    destroyed_ = true;
  }
  bool DestroyedOnMainThread() {
    base::AutoLock lock(destroyed_lock_);
    return destroyed_;
  }

 protected:
  virtual ~CompositorThreadContextProvider() {}

 private:
  bool initialized_;

  base::Lock destroyed_lock_;
  bool destroyed_;
};

scoped_refptr<cc::ContextProvider>
RenderWidgetCompositor::OffscreenContextProviderForCompositorThread() {
  if (!contexts_compositor_thread_ ||
      contexts_compositor_thread_->DestroyedOnMainThread())
    contexts_compositor_thread_ = new CompositorThreadContextProvider;
  return contexts_compositor_thread_;
}

}  // namespace content
