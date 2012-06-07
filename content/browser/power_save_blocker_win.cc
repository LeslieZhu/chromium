// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/power_save_blocker.h"

#include <windows.h>

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

// Called only from UI thread.
// static
void PowerSaveBlocker::ApplyBlock(PowerSaveBlockerType type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DWORD flags = ES_CONTINUOUS;

  switch (type) {
    case kPowerSaveBlockPreventSystemSleep:
      flags |= ES_SYSTEM_REQUIRED;
      break;
    case kPowerSaveBlockPreventDisplaySleep:
      flags |= ES_DISPLAY_REQUIRED;
      break;
    default:
      break;
  }

  SetThreadExecutionState(flags);
}

// TODO(rvargas): Remove after the old interface goes away.
#define PowerSaveBlocker PowerSaveBlocker2

namespace {

int g_blocker_count[2];

#if _WIN32_WINNT <= _WIN32_WINNT_WIN7
POWER_REQUEST_TYPE PowerRequestExecutionRequired =
    static_cast<POWER_REQUEST_TYPE>(PowerRequestAwayModeRequired + 1);
#endif

HANDLE CreatePowerRequest(POWER_REQUEST_TYPE type, const std::string& reason) {
  typedef HANDLE (WINAPI* PowerCreateRequestPtr)(PREASON_CONTEXT);
  typedef BOOL (WINAPI* PowerSetRequestPtr)(HANDLE, POWER_REQUEST_TYPE);

  if (type == PowerRequestExecutionRequired &&
      base::win::GetVersion() < base::win::VERSION_WIN8) {
    return INVALID_HANDLE_VALUE;
  }

  static PowerCreateRequestPtr PowerCreateRequestFn = NULL;
  static PowerSetRequestPtr PowerSetRequestFn = NULL;

  if (!PowerCreateRequestFn || !PowerSetRequestFn) {
    HMODULE module = GetModuleHandle(L"kernel32.dll");
    PowerCreateRequestFn = reinterpret_cast<PowerCreateRequestPtr>(
        GetProcAddress(module, "PowerCreateRequest"));
    PowerSetRequestFn = reinterpret_cast<PowerSetRequestPtr>(
        GetProcAddress(module, "PowerSetRequest"));

    if (!PowerCreateRequestFn || !PowerSetRequestFn)
      return INVALID_HANDLE_VALUE;
  }
  string16 wide_reason = ASCIIToUTF16(reason);
  REASON_CONTEXT context = {0};
  context.Version = POWER_REQUEST_CONTEXT_VERSION;
  context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
  context.Reason.SimpleReasonString = const_cast<wchar_t*>(wide_reason.c_str());

  base::win::ScopedHandle handle(PowerCreateRequestFn(&context));
  if (!handle.IsValid())
    return INVALID_HANDLE_VALUE;

  if (PowerSetRequestFn(handle, type))
    return handle.Take();

  // Something went wrong.
  return INVALID_HANDLE_VALUE;
}

// Takes ownership of the |handle|.
void DeletePowerRequest(POWER_REQUEST_TYPE type, HANDLE handle) {
  base::win::ScopedHandle request_handle(handle);
  if (!request_handle.IsValid())
    return;

  if (type == PowerRequestExecutionRequired &&
      base::win::GetVersion() < base::win::VERSION_WIN8) {
    return;
  }

  typedef BOOL (WINAPI* PowerClearRequestPtr)(HANDLE, POWER_REQUEST_TYPE);
  HMODULE module = GetModuleHandle(L"kernel32.dll");
  PowerClearRequestPtr PowerClearRequestFn =
      reinterpret_cast<PowerClearRequestPtr>(
          GetProcAddress(module, "PowerClearRequest"));

  if (!PowerClearRequestFn)
    return;

  BOOL success = PowerClearRequestFn(request_handle, type);
  DCHECK(success);
}

void ApplySimpleBlock(content::PowerSaveBlocker::PowerSaveBlockerType type,
                      int delta) {
  g_blocker_count[type] += delta;
  DCHECK_GE(g_blocker_count[type], 0);

  if (g_blocker_count[type] > 1)
    return;

  DWORD this_flag = 0;
  if (type == content::PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension)
    this_flag |= ES_SYSTEM_REQUIRED;
  else
    this_flag |= ES_DISPLAY_REQUIRED;

  DCHECK(this_flag);

  static DWORD flags = ES_CONTINUOUS;
  if (!g_blocker_count[type])
    flags &= ~this_flag;
  else
    flags |= this_flag;

  SetThreadExecutionState(flags);
}

}  // namespace.

namespace content {

class PowerSaveBlocker::Delegate
    : public base::RefCountedThreadSafe<PowerSaveBlocker::Delegate> {
 public:
  Delegate(PowerSaveBlockerType type, const std::string& reason)
      : type_(type), reason_(reason) {}

  // Does the actual work to apply or remove the desired power save block.
  void ApplyBlock();
  void RemoveBlock();

  // Returns the equivalent POWER_REQUEST_TYPE for this request.
  POWER_REQUEST_TYPE RequestType();

 private:
  friend class base::RefCountedThreadSafe<PowerSaveBlocker::Delegate>;
  ~Delegate() {}

  PowerSaveBlockerType type_;
  const std::string reason_;
  base::win::ScopedHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

void PowerSaveBlocker::Delegate::ApplyBlock() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (base::win::GetVersion() < base::win::VERSION_WIN7)
    return ApplySimpleBlock(type_, 1);

  handle_.Set(CreatePowerRequest(RequestType(), reason_));
}

void PowerSaveBlocker::Delegate::RemoveBlock() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (base::win::GetVersion() < base::win::VERSION_WIN7)
    return ApplySimpleBlock(type_, -1);

  DeletePowerRequest(RequestType(), handle_.Take());
}

POWER_REQUEST_TYPE PowerSaveBlocker::Delegate::RequestType() {
  if (type_ == kPowerSaveBlockPreventDisplaySleep)
    return PowerRequestDisplayRequired;

  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return PowerRequestSystemRequired;

  return PowerRequestExecutionRequired;
}

PowerSaveBlocker::PowerSaveBlocker(PowerSaveBlockerType type,
                                   const std::string& reason)
    : delegate_(new Delegate(type, reason)) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Delegate::ApplyBlock, delegate_));
}

PowerSaveBlocker::~PowerSaveBlocker() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Delegate::RemoveBlock, delegate_));
}

}  // namespace content
