// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/chromeos_switches.h"

namespace chromeos {
namespace switches {

// Path for app's OEM manifest file.
const char kAppOemManifestFile[]            = "app-mode-oem-manifest";

// When wallpaper boot animation is not disabled this switch
// is used to override OOBE/sign in WebUI init type.
// Possible values: parallel|postpone. Default: parallel.
const char kAshWebUIInit[]                  = "ash-webui-init";

// Enables overriding the path for the default authentication extension.
const char kAuthExtensionPath[]             = "auth-ext-path";

// Enables overriding the Chrome OS board type when running on Linux.
const char kChromeOSReleaseBoard[] = "chromeos-release-board";

// Forces the stub implementation of dbus clients.
const char kDbusStub[] = "dbus-stub";

// Disables Kiosk app mode for ChromeOS.
const char kDisableAppMode[]                = "disable-app-mode";

// Disables wallpaper boot animation (except of OOBE case).
const char kDisableBootAnimation[]          = "disable-boot-animation";

// Disables Chrome Captive Portal detector, which initiates Captive
// Portal detection for new active networks.
const char kDisableChromeCaptivePortalDetector[] =
    "disable-chrome-captive-portal-detector";

// Disables Google Drive integration.
const char kDisableDrive[]                  = "disable-drive";

// Disable policy-configured local accounts.
const char kDisableLocalAccounts[]          = "disable-local-accounts";

// Avoid doing expensive animations upon login.
const char kDisableLoginAnimations[]        = "disable-login-animations";

// Disable Quickoffice component app thus handlers won't be registered so
// it will be possible to install another version as normal app for testing.
const char kDisableQuickofficeComponentApp[] =
    "disable-quickoffice-component-app";

// Disables fetching online CrOS EULA page, only static version is shown.
const char kDisableOnlineEULA[] = "disable-cros-online-eula";

// Avoid doing animations upon oobe.
const char kDisableOobeAnimation[]          = "disable-oobe-animation";

// Disables portal detection and network error handling before auto
// update.
const char kDisableOOBEBlockingUpdate[] =
    "disable-oobe-blocking-update";

// Disables fake ethernet network in the stub implementations.
const char kDisableStubEthernet[] = "disable-stub-ethernet";

// Enables overriding the path for the default echo component extension.
// Useful for testing.
const char kEchoExtensionPath[]             = "echo-ext-path";

// Enables component extension that initializes background pages of
// certain hosted applications.
const char kEnableBackgroundLoader[]        = "enable-background-loader";

// Enables switching between different cellular carriers from the UI.
const char kEnableCarrierSwitching[]        = "enable-carrier-switching";

// Enable switching between audio devices in Chrome instead of cras.
const char kEnableChromeAudioSwitching[] = "enable-chrome-audio-switching";

// Enables Chrome Captive Portal detector, which initiates Captive
// Portal detection for new active networks.
const char kEnableChromeCaptivePortalDetector[] =
    "enable-chrome-captive-portal-detector";

// Enable experimental Bluetooth features.
const char kEnableExperimentalBluetooth[] = "enable-experimental-bluetooth";

// Disables the new NetworkChangeNotifier which uses NetworkStateHandler.
const char kDisableNewNetworkChangeNotifier[] =
    "disable-new-network-change-notifier";

// Enables screensaver extensions.
const char kEnableScreensaverExtensions[] = "enable-screensaver-extensions";

// Enable "interactive" mode for stub implemenations (e.g. NetworkStateHandler)
const char kEnableStubInteractive[] = "enable-stub-interactive";

// Enables touchpad three-finger-click as middle button.
const char kEnableTouchpadThreeFingerClick[]
    = "enable-touchpad-three-finger-click";

// Enables touchpad three-finger swipe.
const char kEnableTouchpadThreeFingerSwipe[]
    = "enable-touchpad-three-finger-swipe";

// Enable Kiosk mode for ChromeOS.
const char kEnableKioskMode[]               = "enable-kiosk-mode";

// Enables request of tablet site (via user agent override).
const char kEnableRequestTabletSite[]       = "enable-request-tablet-site";

// Enables static ip configuration. This flag should be removed when it's on by
// default.
const char kEnableStaticIPConfig[]          = "enable-static-ip-config";

// Power of the power-of-2 initial modulus that will be used by the
// auto-enrollment client. E.g. "4" means the modulus will be 2^4 = 16.
const char kEnterpriseEnrollmentInitialModulus[] =
    "enterprise-enrollment-initial-modulus";

// Power of the power-of-2 maximum modulus that will be used by the
// auto-enrollment client.
const char kEnterpriseEnrollmentModulusLimit[] =
    "enterprise-enrollment-modulus-limit";

// Loads the File Manager as an extension instead of a platform app.
// This flag is obsolete. Remove it, once Files.app v2 is stable.
const char kFileManagerLegacy[]             = "file-manager-legacy";

// Loads the File Manager with the legacy UI.
const char kFileManagerLegacyUI[]           = "file-manager-legacy-ui";

// Passed to Chrome on first boot. Not passed on restart after sign out.
const char kFirstBoot[]                     = "first-boot";

// Usually in browser tests the usual login manager bringup is skipped so that
// tests can change how it's brought up. This flag disables that.
const char kForceLoginManagerInTests[]      = "force-login-manager-in-tests";

// Indicates that the browser is in "browse without sign-in" (Guest session)
// mode. Should completely disable extensions, sync and bookmarks.
const char kGuestSession[]                  = "bwsi";

// If true, the Chromebook has a Chrome OS keyboard. Don't use the flag for
// Chromeboxes.
const char kHasChromeOSKeyboard[]           = "has-chromeos-keyboard";

// If true, the Chromebook has a keyboard with a diamond key.
const char kHasChromeOSDiamondKey[]         = "has-chromeos-diamond-key";

// Path for the screensaver used in Kiosk mode
const char kKioskModeScreensaverPath[]      = "kiosk-mode-screensaver-path";

// Enables Chrome-as-a-login-manager behavior.
const char kLoginManager[]                  = "login-manager";

// Specifies a password to be used to login (along with login-user).
const char kLoginPassword[]                 = "login-password";

// Specifies the profile to use once a chromeos user is logged in.
const char kLoginProfile[]                  = "login-profile";

// Allows to override the first login screen. The value should be the name of
// the first login screen to show (see
// chrome/browser/chromeos/login/login_wizard_view.cc for actual names).
// Ignored if kLoginManager is not specified. TODO(avayvod): Remove when the
// switch is no longer needed for testing.
const char kLoginScreen[]                   = "login-screen";

// Controls the initial login screen size. Pass width,height.
const char kLoginScreenSize[]               = "login-screen-size";

// Specifies the user which is already logged in.
const char kLoginUser[]                     = "login-user";

// Enables natural scroll by default.
const char kNaturalScrollDefault[]          = "enable-natural-scroll-default";

// Disables tab discard in low memory conditions, a feature which silently
// closes inactive tabs to free memory and to attempt to avoid the kernel's
// out-of-memory process killer.
const char kNoDiscardTabs[]                 = "no-discard-tabs";

#ifndef NDEBUG
// Skips all other OOBE pages after user login.
const char kOobeSkipPostLogin[]             = "oobe-skip-postlogin";
#endif  // NDEBUG

// Sends test messages on first call to RequestUpdate (stub only).
const char kSmsTestMessages[] = "sms-test-messages";

// Indicates that a stub implementation of CrosSettings that stores settings in
// memory without signing should be used, treating current user as the owner.
// This option is for testing the chromeos build of chrome on the desktop only.
const char kStubCrosSettings[]              = "stub-cros-settings";

// Enables usage of the new ManagedNetworkConfigurationHandler and
// NetworkConfigurationHandler singletons.
const char kUseNewNetworkConfigurationHandlers[] =
    "use-new-network-configuration-handlers";

} // namespace switches
}  // namespace chromeos
