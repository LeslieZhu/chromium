// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/display/output_configurator.h"

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/Xrandr.h>

// Xlib defines Status as int which causes our include of dbus/bus.h to fail
// when it tries to name an enum Status.  Thus, we need to undefine it (note
// that this will cause a problem if code needs to use the Status type).
// RootWindow causes similar problems in that there is a Chromium type with that
// name.
#undef Status
#undef RootWindow

#include "base/bind.h"
#include "base/chromeos/chromeos_version.h"
#include "base/logging.h"
#include "base/message_pump_aurax11.h"
#include "base/metrics/histogram.h"
#include "base/perftimer.h"
#include "base/time.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

struct OutputSnapshot {
  RROutput output;
  RRCrtc crtc;
  RRMode current_mode;
  int height;
  int y;
  RRMode native_mode;
  RRMode mirror_mode;
  bool is_internal;
};

namespace {
// DPI measurements.
const float kMmInInch = 25.4;
const float kDpi96 = 96.0;
const float kPixelsToMmScale = kMmInInch / kDpi96;

// The DPI threshold to detech high density screen.
// Higher DPI than this will use device_scale_factor=2
// Should be kept in sync with display_change_observer_x11.cc
const unsigned int kHighDensityDIPThreshold = 160;

// Prefixes for the built-in displays.
const char kInternal_LVDS[] = "LVDS";
const char kInternal_eDP[] = "eDP";

// The delay to wait NotifyOnDisplayChanged().  See the comment in Dispatch().
const int kNotificationTimerDelayMs = 500;

// Gap between screens so cursor at bottom of active display doesn't partially
// appear on top of inactive display. Higher numbers guard against larger
// cursors, but also waste more memory.
// For simplicity, this is hard-coded to 60 to avoid the complexity of always
// determining the DPI of the screen and rationalizing which screen we need to
// use for the DPI calculation.
// See crbug.com/130188 for initial discussion.
const int kVerticalGap = 60;

// TODO: Determine if we need to organize modes in a way which provides better
// than O(n) lookup time.  In many call sites, for example, the "next" mode is
// typically what we are looking for so using this helper might be too
// expensive.
static XRRModeInfo* ModeInfoForID(XRRScreenResources* screen, RRMode modeID) {
  XRRModeInfo* result = NULL;
  for (int i = 0; (i < screen->nmode) && (result == NULL); i++)
    if (modeID == screen->modes[i].id)
      result = &screen->modes[i];

  return result;
}

// A helper to call XRRSetCrtcConfig with the given options but some of our
// default output count and rotation arguments.
static void ConfigureCrtc(Display* display,
                          XRRScreenResources* screen,
                          RRCrtc crtc,
                          int x,
                          int y,
                          RRMode mode,
                          RROutput output) {
  VLOG(1) << "ConfigureCrtc crtc: " << crtc
          << ", mode " << mode
          << ", output " << output
          << ", x " << x
          << ", y " << y;
  const Rotation kRotate = RR_Rotate_0;
  RROutput* outputs = NULL;
  int num_outputs = 0;

  // Check the output and mode argument - if either are None, we should disable.
  if ((output != None) && (mode != None)) {
    outputs = &output;
    num_outputs = 1;
  }

  XRRSetCrtcConfig(display,
                   screen,
                   crtc,
                   CurrentTime,
                   x,
                   y,
                   mode,
                   kRotate,
                   outputs,
                   num_outputs);
}

// Called to set the frame buffer (underling XRR "screen") size.  Has a
// side-effect of disabling all CRTCs.
static void CreateFrameBuffer(Display* display,
                              XRRScreenResources* screen,
                              Window window,
                              int width,
                              int height) {
  VLOG(1) << "CreateFrameBuffer " << width << " by " << height;

  // Don't do anything if the framebuffer is already the right size.
  // This speeds up modesetting and suspend/resume.
  if (width == DisplayWidth(display, DefaultScreen (display)) &&
      height == DisplayHeight(display, DefaultScreen (display)))
    return;

  // Note that setting the screen size fails if any CRTCs are currently
  // pointing into it so disable them all.
  for (int i = 0; i < screen->ncrtc; ++i) {
    const int x = 0;
    const int y = 0;
    const RRMode kMode = None;
    const RROutput kOutput = None;

    // We don't worry about the cached state of the outputs here since we are
    // not interested in the state we are setting - it is just to get the CRTCs
    // off the screen so we can rebuild the frame buffer.
    ConfigureCrtc(display,
                  screen,
                  screen->crtcs[i],
                  x,
                  y,
                  kMode,
                  kOutput);
  }
  int mm_width = width * kPixelsToMmScale;
  int mm_height = height * kPixelsToMmScale;
  XRRSetScreenSize(display, window, width, height, mm_width, mm_height);
}

static OutputState InferCurrentState(Display* display,
                                     XRRScreenResources* screen,
                                     const OutputSnapshot* outputs,
                                     int output_count) {
  OutputState state = STATE_INVALID;
  switch (output_count) {
    case 0:
      state = STATE_HEADLESS;
      break;
    case 1:
      state = STATE_SINGLE;
      break;
    case 2: {
      RRMode primary_mode = outputs[0].current_mode;
      RRMode secondary_mode = outputs[1].current_mode;

      if ((0 == outputs[0].y) && (0 == outputs[1].y)) {
        // Displays in the same spot so this is either mirror or unknown.
        // Note that we should handle no configured CRTC as a "wildcard" since
        // that allows us to preserve mirror mode state while power is switched
        // off on one display.
        bool primary_mirror = (outputs[0].mirror_mode == primary_mode) ||
            (None == primary_mode);
        bool secondary_mirror = (outputs[1].mirror_mode == secondary_mode) ||
            (None == secondary_mode);
        if (primary_mirror && secondary_mirror) {
          state = STATE_DUAL_MIRROR;
        } else {
          // We should never normally get into this state but it can help us
          // make sense of situations where the configuration may have been
          // changed for testing, etc.
          state = STATE_DUAL_UNKNOWN;
        }
      } else {
        // At this point, we expect both displays to be in native mode and tiled
        // such that one is primary and another is correctly positioned as
        // secondary.  If any of these assumptions are false, this is an unknown
        // configuration.
        bool primary_native = (outputs[0].native_mode == primary_mode) ||
            (None == primary_mode);
        bool secondary_native = (outputs[1].native_mode == secondary_mode) ||
            (None == secondary_mode);
        if (primary_native && secondary_native) {
          // Just check the relative locations.
          int secondary_offset = outputs[0].height + kVerticalGap;
          int primary_offset = outputs[1].height + kVerticalGap;
          if ((0 == outputs[0].y) && (secondary_offset == outputs[1].y)) {
            state = STATE_DUAL_PRIMARY_ONLY;
          } else if ((0 == outputs[1].y) && (primary_offset == outputs[0].y)) {
            state = STATE_DUAL_SECONDARY_ONLY;
          } else {
            // Unexpected locations.
            state = STATE_DUAL_UNKNOWN;
          }
        } else {
          // Mode assumptions don't hold.
          state = STATE_DUAL_UNKNOWN;
        }
      }
      break;
    }
    default:
      CHECK(false);
  }
  return state;
}

static OutputState GetNextState(Display* display,
                                XRRScreenResources* screen,
                                OutputState current_state,
                                const OutputSnapshot* outputs,
                                int output_count) {
  OutputState state = STATE_INVALID;

  switch (output_count) {
    case 0:
      state = STATE_HEADLESS;
      break;
    case 1:
      state = STATE_SINGLE;
      break;
    case 2: {
      bool mirror_supported = (0 != outputs[0].mirror_mode) &&
          (0 != outputs[1].mirror_mode);
      switch (current_state) {
        case STATE_DUAL_PRIMARY_ONLY:
          state =
              mirror_supported ? STATE_DUAL_MIRROR : STATE_DUAL_PRIMARY_ONLY;
          break;
        case STATE_DUAL_MIRROR:
          state = STATE_DUAL_PRIMARY_ONLY;
          break;
        default:
          // Default to primary only.
          state = STATE_DUAL_PRIMARY_ONLY;
      }
      break;
    }
    default:
      CHECK(false);
  }
  return state;
}

static RRCrtc GetNextCrtcAfter(Display* display,
                               XRRScreenResources* screen,
                               RROutput output,
                               RRCrtc previous) {
  RRCrtc crtc = None;
  XRROutputInfo* output_info = XRRGetOutputInfo(display, screen, output);

  for (int i = 0; (i < output_info->ncrtc) && (None == crtc); ++i) {
    RRCrtc this_crtc = output_info->crtcs[i];

    if (previous != this_crtc)
      crtc = this_crtc;
  }
  XRRFreeOutputInfo(output_info);
  return crtc;
}

static bool EnterState(Display* display,
                       XRRScreenResources* screen,
                       Window window,
                       OutputState new_state,
                       const OutputSnapshot* outputs,
                       int output_count) {
  switch (output_count) {
    case 0:
      // Do nothing as no 0-display states are supported.
      break;
    case 1: {
      // Re-allocate the framebuffer to fit.
      XRRModeInfo* mode_info = ModeInfoForID(screen, outputs[0].native_mode);
      if (mode_info == NULL) {
        UMA_HISTOGRAM_COUNTS("Display.EnterState.single_failures", 1);
        return false;
      }

      int width = mode_info->width;
      int height = mode_info->height;
      CreateFrameBuffer(display, screen, window, width, height);

      // Re-attach native mode for the CRTC.
      const int x = 0;
      const int y = 0;
      RRCrtc crtc = GetNextCrtcAfter(display, screen, outputs[0].output, None);
      ConfigureCrtc(display,
                    screen,
                    crtc,
                    x,
                    y,
                    outputs[0].native_mode,
                    outputs[0].output);
      break;
    }
    case 2: {
      RRCrtc primary_crtc =
          GetNextCrtcAfter(display, screen, outputs[0].output, None);
      RRCrtc secondary_crtc =
          GetNextCrtcAfter(display, screen, outputs[1].output, primary_crtc);

      if (STATE_DUAL_MIRROR == new_state) {
        XRRModeInfo* mode_info = ModeInfoForID(screen, outputs[0].mirror_mode);
        if (mode_info == NULL) {
          UMA_HISTOGRAM_COUNTS("Display.EnterState.mirror_failures", 1);
          return false;
        }

        int width = mode_info->width;
        int height = mode_info->height;
        CreateFrameBuffer(display, screen, window, width, height);

        const int x = 0;
        const int y = 0;
        ConfigureCrtc(display,
                      screen,
                      primary_crtc,
                      x,
                      y,
                      outputs[0].mirror_mode,
                      outputs[0].output);
        ConfigureCrtc(display,
                      screen,
                      secondary_crtc,
                      x,
                      y,
                      outputs[1].mirror_mode,
                      outputs[1].output);
      } else {
        XRRModeInfo* primary_mode_info =
            ModeInfoForID(screen, outputs[0].native_mode);
        XRRModeInfo* secondary_mode_info =
            ModeInfoForID(screen, outputs[1].native_mode);
        if (primary_mode_info == NULL || secondary_mode_info == NULL) {
          UMA_HISTOGRAM_COUNTS("Display.EnterState.dual_failures", 1);
          return false;
        }

        int width =
            std::max<int>(primary_mode_info->width, secondary_mode_info->width);
        int primary_height = primary_mode_info->height;
        int secondary_height = secondary_mode_info->height;
        int height = primary_height + secondary_height + kVerticalGap;
        CreateFrameBuffer(display, screen, window, width, height);

        const int x = 0;
        if (STATE_DUAL_PRIMARY_ONLY == new_state) {
          int primary_y = 0;
          ConfigureCrtc(display,
                        screen,
                        primary_crtc,
                        x,
                        primary_y,
                        outputs[0].native_mode,
                        outputs[0].output);
          int secondary_y = primary_height + kVerticalGap;
          ConfigureCrtc(display,
                        screen,
                        secondary_crtc,
                        x,
                        secondary_y,
                        outputs[1].native_mode,
                        outputs[1].output);
        } else {
          int primary_y = secondary_height + kVerticalGap;
          ConfigureCrtc(display,
                        screen,
                        primary_crtc,
                        x,
                        primary_y,
                        outputs[0].native_mode,
                        outputs[0].output);
          int secondary_y = 0;
          ConfigureCrtc(display,
                        screen,
                        secondary_crtc,
                        x,
                        secondary_y,
                        outputs[1].native_mode,
                        outputs[1].output);
        }
      }
      break;
    }
    default:
      CHECK(false);
  }

  return true;
}

static XRRScreenResources* GetScreenResourcesAndRecordUMA(Display* display,
                                                          Window window) {
  // This call to XRRGetScreenResources is implicated in a hang bug so
  // instrument it to see its typical running time (crbug.com/134449).
  // TODO(disher): Remove these UMA calls once crbug.com/134449 is resolved.
  UMA_HISTOGRAM_BOOLEAN("Display.XRRGetScreenResources_completed", false);
  PerfTimer histogram_timer;
  XRRScreenResources* screen = XRRGetScreenResources(display, window);
  base::TimeDelta duration = histogram_timer.Elapsed();
  UMA_HISTOGRAM_BOOLEAN("Display.XRRGetScreenResources_completed", true);
  UMA_HISTOGRAM_LONG_TIMES("Display.XRRGetScreenResources_duration", duration);
  return screen;
}

// Determine if there is an "internal" output and how many outputs are
// connected.
static bool IsProjecting(const OutputSnapshot* outputs, int output_count) {
  bool has_internal_output = false;
  int connected_output_count = output_count;
  for (int i = 0; i < output_count; ++i)
    has_internal_output |= outputs[i].is_internal;

  // "Projecting" is defined as having more than 1 output connected while at
  // least one of them is an internal output.
  return has_internal_output && (connected_output_count > 1);
}

}  // namespace

OutputConfigurator::OutputConfigurator()
    : is_running_on_chrome_os_(base::chromeos::IsRunningOnChromeOS()),
      is_panel_fitting_enabled_(false),
      connected_output_count_(0),
      xrandr_event_base_(0),
      output_state_(STATE_INVALID) {
}

void OutputConfigurator::Init(bool is_panel_fitting_enabled) {
  if (!is_running_on_chrome_os_)
    return;

  is_panel_fitting_enabled_ = is_panel_fitting_enabled;

  // Cache the initial output state.
  Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
  CHECK(display != NULL);
  XGrabServer(display);
  Window window = DefaultRootWindow(display);
  XRRScreenResources* screen = GetScreenResourcesAndRecordUMA(display, window);
  CHECK(screen != NULL);

  // Detect our initial state.
  OutputSnapshot outputs[2] = { {0}, {0} };
  connected_output_count_ =
      GetDualOutputs(display, screen, &outputs[0], &outputs[1]);
  output_state_ =
      InferCurrentState(display, screen, outputs, connected_output_count_);
  // Ensure that we are in a supported state with all connected displays powered
  // on.
  OutputState starting_state = GetNextState(display,
                                            screen,
                                            STATE_INVALID,
                                            outputs,
                                            connected_output_count_);
  if (output_state_ != starting_state &&
      EnterState(display,
                 screen,
                 window,
                 starting_state,
                 outputs,
                 connected_output_count_)) {
    output_state_ = starting_state;
  }
  bool is_projecting = IsProjecting(outputs, connected_output_count_);

  // Find xrandr_event_base_ since we need it to interpret events, later.
  int error_base_ignored = 0;
  XRRQueryExtension(display, &xrandr_event_base_, &error_base_ignored);

  // Force the DPMS on chrome startup as the driver doesn't always detect
  // that all displays are on when signing out.
  CHECK(DPMSEnable(display));
  CHECK(DPMSForceLevel(display, DPMSModeOn));

  // Relinquish X resources.
  XRRFreeScreenResources(screen);
  XUngrabServer(display);

  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->
      SetIsProjecting(is_projecting);
}

OutputConfigurator::~OutputConfigurator() {
}

bool OutputConfigurator::CycleDisplayMode() {
  VLOG(1) << "CycleDisplayMode";
  if (!is_running_on_chrome_os_)
    return false;

  bool did_change = false;
  Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
  CHECK(display != NULL);
  XGrabServer(display);
  Window window = DefaultRootWindow(display);
  XRRScreenResources* screen = GetScreenResourcesAndRecordUMA(display, window);
  CHECK(screen != NULL);

  OutputSnapshot outputs[2] = { {0}, {0} };
  connected_output_count_ =
      GetDualOutputs(display, screen, &outputs[0], &outputs[1]);
  OutputState original =
      InferCurrentState(display, screen, outputs, connected_output_count_);
  OutputState next_state =
      GetNextState(display, screen, original, outputs, connected_output_count_);
  if (original != next_state &&
      EnterState(display,
                 screen,
                 window,
                 next_state,
                 outputs,
                 connected_output_count_)) {
    did_change = true;
  }
  // We have seen cases where the XRandR data can get out of sync with our own
  // cache so over-write it with whatever we detected, even if we didn't think
  // anything changed.
  output_state_ = next_state;

  XRRFreeScreenResources(screen);
  XUngrabServer(display);

  if (!did_change)
    FOR_EACH_OBSERVER(Observer, observers_, OnDisplayModeChangeFailed());
  return did_change;
}

bool OutputConfigurator::ScreenPowerSet(bool power_on, bool all_displays) {
  VLOG(1) << "OutputConfigurator::SetScreensOn " << power_on
          << " all displays " << all_displays;
  if (!is_running_on_chrome_os_)
    return false;

  bool success = false;
  Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
  CHECK(display != NULL);
  XGrabServer(display);
  Window window = DefaultRootWindow(display);
  XRRScreenResources* screen = GetScreenResourcesAndRecordUMA(display, window);
  CHECK(screen != NULL);

  OutputSnapshot outputs[2] = { {0}, {0} };
  connected_output_count_ =
      GetDualOutputs(display, screen, &outputs[0], &outputs[1]);

  if (all_displays && power_on) {
    // Resume all displays using the current state.
    if (EnterState(display,
                   screen,
                   window,
                   output_state_,
                   outputs,
                   connected_output_count_)) {
      // Force the DPMS on since the driver doesn't always detect that it should
      // turn on. This is needed when coming back from idle suspend.
      CHECK(DPMSEnable(display));
      CHECK(DPMSForceLevel(display, DPMSModeOn));

      XRRFreeScreenResources(screen);
      XUngrabServer(display);
      return true;
    }
  }

  RRCrtc crtc = None;
  // Set the CRTCs based on whether we want to turn the power on or off and
  // select the outputs to operate on by name or all_displays.
  for (int i = 0; i < connected_output_count_; ++i) {
    if (all_displays || outputs[i].is_internal || power_on) {
      const int x = 0;
      const int y = outputs[i].y;
      RROutput output = outputs[i].output;
      RRMode mode = None;
      if (power_on) {
        mode = (STATE_DUAL_MIRROR == output_state_) ?
            outputs[i].mirror_mode : outputs[i].native_mode;
      } else if (connected_output_count_ > 1 && !all_displays &&
                 outputs[i].is_internal) {
        // Workaround for crbug.com/148365: leave internal display in native
        // mode so user can move cursor (and hence windows) onto internal
        // display even when dimmed
        mode = outputs[i].native_mode;
      }
      crtc = GetNextCrtcAfter(display, screen, output, crtc);

      ConfigureCrtc(display,
                    screen,
                    crtc,
                    x,
                    y,
                    mode,
                    output);
      success = true;
    }
  }

  // Force the DPMS on since the driver doesn't always detect that it should
  // turn on. This is needed when coming back from idle suspend.
  if (power_on) {
    CHECK(DPMSEnable(display));
    CHECK(DPMSForceLevel(display, DPMSModeOn));
  }

  XRRFreeScreenResources(screen);
  XUngrabServer(display);

  return success;
}

bool OutputConfigurator::SetDisplayMode(OutputState new_state) {
  if (output_state_ == STATE_INVALID ||
      output_state_ == STATE_HEADLESS ||
      output_state_ == STATE_SINGLE)
    return false;

  if (output_state_ == new_state)
    return true;

  Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
  CHECK(display != NULL);
  XGrabServer(display);
  Window window = DefaultRootWindow(display);
  XRRScreenResources* screen = GetScreenResourcesAndRecordUMA(display, window);
  CHECK(screen != NULL);

  OutputSnapshot outputs[2] = { {0}, {0} };
  connected_output_count_ =
      GetDualOutputs(display, screen, &outputs[0], &outputs[1]);
  if (EnterState(display,
                 screen,
                 window,
                 new_state,
                 outputs,
                 connected_output_count_)) {
    output_state_ = new_state;
  }

  XRRFreeScreenResources(screen);
  XUngrabServer(display);

  if (output_state_ != new_state)
    FOR_EACH_OBSERVER(Observer, observers_, OnDisplayModeChangeFailed());
  return true;
}

bool OutputConfigurator::Dispatch(const base::NativeEvent& event) {
  // Ignore this event if the Xrandr extension isn't supported.
  if (!is_running_on_chrome_os_ ||
      (event->type - xrandr_event_base_ != RRNotify)) {
    return true;
  }
  XEvent* xevent = static_cast<XEvent*>(event);
  XRRNotifyEvent* notify_event =
      reinterpret_cast<XRRNotifyEvent*>(xevent);
  if (notify_event->subtype == RRNotify_OutputChange) {
    XRROutputChangeNotifyEvent* output_change_event =
        reinterpret_cast<XRROutputChangeNotifyEvent*>(xevent);
    if ((output_change_event->connection == RR_Connected) ||
        (output_change_event->connection == RR_Disconnected)) {
      Display* display = base::MessagePumpAuraX11::GetDefaultXDisplay();
      CHECK(display != NULL);
      XGrabServer(display);
      Window window = DefaultRootWindow(display);
      XRRScreenResources* screen =
          GetScreenResourcesAndRecordUMA(display, window);
      CHECK(screen != NULL);

      OutputSnapshot outputs[2] = { {0}, {0} };
      int new_output_count =
          GetDualOutputs(display, screen, &outputs[0], &outputs[1]);
      if (new_output_count != connected_output_count_) {
        connected_output_count_ = new_output_count;
        OutputState new_state = GetNextState(display,
                                             screen,
                                             STATE_INVALID,
                                             outputs,
                                             connected_output_count_);
        if (EnterState(display,
                       screen,
                       window,
                       new_state,
                       outputs,
                       connected_output_count_)) {
          output_state_ = new_state;
        }
      }

      bool is_projecting = IsProjecting(outputs, connected_output_count_);
      XRRFreeScreenResources(screen);
      XUngrabServer(display);

      chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->
          SetIsProjecting(is_projecting);
    }
    // Ignore the case of RR_UnkownConnection.
  }

  // Sets the timer for NotifyOnDisplayChanged().  When an output state change
  // is issued, several notifications chould arrive and NotifyOnDisplayChanged()
  // should be called once for the last one.  The timer could lead at most a few
  // handreds milliseconds of delay for the notification, but it would be
  // unrecognizable for users.
  if (notification_timer_.get()) {
    notification_timer_->Reset();
  } else {
    notification_timer_.reset(new base::OneShotTimer<OutputConfigurator>());
    notification_timer_->Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kNotificationTimerDelayMs),
        this,
        &OutputConfigurator::NotifyOnDisplayChanged);
  }
  return true;
}

void OutputConfigurator::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void OutputConfigurator::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
bool OutputConfigurator::IsInternalOutputName(const std::string& name) {
  return name.find(kInternal_LVDS) == 0 || name.find(kInternal_eDP) == 0;
}

void OutputConfigurator::NotifyOnDisplayChanged() {
  notification_timer_.reset();
  FOR_EACH_OBSERVER(Observer, observers_, OnDisplayModeChanged());
}

int OutputConfigurator::GetDualOutputs(Display* display,
                                       XRRScreenResources* screen,
                                       OutputSnapshot* one,
                                       OutputSnapshot* two) {
  int found_count = 0;
  XRROutputInfo* one_info = NULL;
  XRROutputInfo* two_info = NULL;

  for (int i = 0; (i < screen->noutput) && (found_count < 2); ++i) {
    RROutput this_id = screen->outputs[i];
    XRROutputInfo* output_info = XRRGetOutputInfo(display, screen, this_id);
    bool is_connected = (RR_Connected == output_info->connection);

    if (is_connected) {
      OutputSnapshot *to_populate = NULL;

      if (0 == found_count) {
        to_populate = one;
        one_info = output_info;
      } else {
        to_populate = two;
        two_info = output_info;
      }

      to_populate->output = this_id;
      // Now, look up the corresponding CRTC and any related info.
      to_populate->crtc = output_info->crtc;
      if (None != to_populate->crtc) {
        XRRCrtcInfo* crtc_info =
            XRRGetCrtcInfo(display, screen, to_populate->crtc);
        to_populate->current_mode = crtc_info->mode;
        to_populate->height = crtc_info->height;
        to_populate->y = crtc_info->y;
        XRRFreeCrtcInfo(crtc_info);
      } else {
        to_populate->current_mode = 0;
        to_populate->height = 0;
        to_populate->y = 0;
      }
      // Find the native_mode and leave the mirror_mode for the pass after the
      // loop.
      to_populate->native_mode = GetOutputNativeMode(output_info);
      to_populate->mirror_mode = 0;

      // See if this output refers to an internal display.
      to_populate->is_internal = IsInternalOutput(output_info);

      VLOG(1) << "Found display #" << found_count
              << " with output " << (int)to_populate->output
              << " crtc " << (int)to_populate->crtc
              << " current mode " << (int)to_populate->current_mode;
      ++found_count;
    } else {
      XRRFreeOutputInfo(output_info);
    }
  }

  if (2 == found_count) {
    bool one_is_internal = IsInternalOutput(one_info);
    bool two_is_internal = IsInternalOutput(two_info);
    int internal_outputs = (one_is_internal ? 1 : 0) +
      (two_is_internal ? 1 : 0);

    DCHECK(internal_outputs < 2);
    LOG_IF(WARNING, internal_outputs == 2) << "Two internal outputs detected.";

    bool can_mirror = false;

    for (int attempt = 0; attempt < 2 && !can_mirror; attempt++) {
      // Try preserving external output's aspect ratio on the first attempt
      // If that fails, fall back to the highest matching resolution
      bool preserve_aspect = attempt == 0;

      if (internal_outputs == 1) {
        if (one_is_internal) {
          can_mirror = FindOrCreateMirrorMode(display,
                                              screen,
                                              one_info,
                                              two_info,
                                              one->output,
                                              is_panel_fitting_enabled_,
                                              preserve_aspect,
                                              &one->mirror_mode,
                                              &two->mirror_mode);
        } else {  // if (two_is_internal)
          can_mirror = FindOrCreateMirrorMode(display,
                                              screen,
                                              two_info,
                                              one_info,
                                              two->output,
                                              is_panel_fitting_enabled_,
                                              preserve_aspect,
                                              &two->mirror_mode,
                                              &one->mirror_mode);
        }
      } else {  // if (internal_outputs == 0)
        // No panel fitting for external outputs, so fall back to exact match
        can_mirror = FindOrCreateMirrorMode(display,
                                            screen,
                                            one_info,
                                            two_info,
                                            one->output,
                                            false,
                                            preserve_aspect,
                                            &one->mirror_mode,
                                            &two->mirror_mode);
        if (!can_mirror && preserve_aspect) {
          // FindOrCreateMirrorMode will try to preserve aspect ratio of
          // what it thinks is external display, so if it didn't succeed
          // with one, maybe it will succeed with the other.
          // This way we will have correct aspect ratio on at least one of them.
          can_mirror = FindOrCreateMirrorMode(display,
                                              screen,
                                              two_info,
                                              one_info,
                                              two->output,
                                              false,
                                              preserve_aspect,
                                              &two->mirror_mode,
                                              &one->mirror_mode);
        }
      }
    }

    if (!can_mirror) {
      // We can't mirror so set mirror_mode to None.
      one->mirror_mode = None;
      two->mirror_mode = None;
    }
  }

  XRRFreeOutputInfo(one_info);
  XRRFreeOutputInfo(two_info);
  return found_count;
}

bool OutputConfigurator::FindOrCreateMirrorMode(
    Display* display,
    XRRScreenResources* screen,
    XRROutputInfo* internal_info,
    XRROutputInfo* external_info,
    RROutput internal_output_id,
    bool try_creating,
    bool preserve_aspect,
    RRMode* internal_mirror_mode,
    RRMode* external_mirror_mode) {
  RRMode internal_mode_id = GetOutputNativeMode(internal_info);
  RRMode external_mode_id = GetOutputNativeMode(external_info);

  if (internal_mode_id == None || external_mode_id == None)
    return false;

  XRRModeInfo* internal_native_mode = ModeInfoForID(screen, internal_mode_id);
  XRRModeInfo* external_native_mode = ModeInfoForID(screen, external_mode_id);

  // Check if some external output resolution can be mirrored on internal.
  // Prefer the modes in the order that X sorts them,
  // assuming this is the order in which they look better on the monitor.
  // If X's order is not satisfactory, we can either fix X's sorting,
  // or implement our sorting here.
  for (int i = 0; i < external_info->nmode; i++) {
    external_mode_id = external_info->modes[i];
    XRRModeInfo* external_mode = ModeInfoForID(screen, external_mode_id);
    bool is_native_aspect_ratio =
        external_native_mode->width * external_mode->height ==
        external_native_mode->height * external_mode->width;
    if (preserve_aspect && !is_native_aspect_ratio)
      continue;  // Allow only aspect ratio preserving modes for mirroring

    // Try finding exact match
    for (int j = 0; j < internal_info->nmode; j++) {
      internal_mode_id = internal_info->modes[j];
      XRRModeInfo* internal_mode = ModeInfoForID(screen, internal_mode_id);
      if (internal_mode->width == external_mode->width &&
          internal_mode->height == external_mode->height) {
        *internal_mirror_mode = internal_mode_id;
        *external_mirror_mode = external_mode_id;
        return true;  // Mirror mode found
      }
    }

    // Try to create a matching internal output mode by panel fitting
    if (try_creating) {
      // We can downscale by 1.125, and upscale indefinitely
      // Downscaling looks ugly, so, can fit == can upscale
      bool can_fit =
          internal_native_mode->width >= external_mode->width &&
          internal_native_mode->height >= external_mode->height;
      if (can_fit) {
        XRRAddOutputMode(display, internal_output_id, external_mode_id);
        *internal_mirror_mode = *external_mirror_mode = external_mode_id;
        return true;  // Mirror mode created
      }
    }
  }

  return false;
}

// static
bool OutputConfigurator::IsInternalOutput(const XRROutputInfo* output_info) {
  return IsInternalOutputName(std::string(output_info->name));
}

// static
RRMode OutputConfigurator::GetOutputNativeMode(
    const XRROutputInfo* output_info) {
  if (output_info->nmode <= 0)
    return None;

  return output_info->modes[0];
}

}  // namespace chromeos
