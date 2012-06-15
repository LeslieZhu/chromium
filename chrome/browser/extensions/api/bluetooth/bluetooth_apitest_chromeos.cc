// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "chrome/browser/chromeos/bluetooth/bluetooth_adapter.h"
#include "chrome/browser/chromeos/bluetooth/test/mock_bluetooth_adapter.h"
#include "chrome/browser/chromeos/bluetooth/test/mock_bluetooth_device.h"
#include "chrome/browser/chromeos/extensions/bluetooth_event_router.h"
#include "chrome/browser/extensions/api/bluetooth/bluetooth_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/ui/browser.h"
#include "chromeos/dbus/bluetooth_out_of_band_client.h"
#include "testing/gmock/include/gmock/gmock.h"

using extensions::Extension;

namespace utils = extension_function_test_utils;
namespace api = extensions::api;

namespace chromeos {

class BluetoothAdapater;

}  // namespace chromeos


namespace {

class BluetoothApiTest : public PlatformAppApiTest {
 public:
  BluetoothApiTest() : empty_extension_(utils::CreateEmptyExtension()) {}

  virtual void SetUpOnMainThread() OVERRIDE {
    // The browser will clean this up when it is torn down
    mock_adapter_ = new testing::StrictMock<chromeos::MockBluetoothAdapter>;
    event_router()->SetAdapterForTest(mock_adapter_);
  }

  void expectBooleanResult(bool expected,
                           UIThreadExtensionFunction* function,
                           const std::string& args) {
    scoped_ptr<base::Value> result(
        utils::RunFunctionAndReturnResult(function, args, browser()));
    ASSERT_TRUE(result.get() != NULL);
    ASSERT_EQ(base::Value::TYPE_BOOLEAN, result->GetType());
    bool boolean_value;
    result->GetAsBoolean(&boolean_value);
    EXPECT_EQ(expected, boolean_value);
  }

  template <class T>
  T* setupFunction(T* function) {
    function->set_extension(empty_extension_.get());
    function->set_has_callback(true);
    return function;
  }

 protected:
  testing::StrictMock<chromeos::MockBluetoothAdapter>* mock_adapter_;

 private:
  chromeos::ExtensionBluetoothEventRouter* event_router() {
    return browser()->profile()->GetExtensionService()->
        bluetooth_event_router();
  }

  chromeos::BluetoothAdapter* original_adapter_;
  scoped_refptr<Extension> empty_extension_;
};

static const char kOutOfBandPairingDataHash[] = "0123456789ABCDEh";
static const char kOutOfBandPairingDataRandomizer[] = "0123456789ABCDEr";

static chromeos::BluetoothOutOfBandPairingData GetOutOfBandPairingData() {
  chromeos::BluetoothOutOfBandPairingData data;
  memcpy(&(data.hash), kOutOfBandPairingDataHash,
      chromeos::kBluetoothOutOfBandPairingDataSize);
  memcpy(&(data.randomizer), kOutOfBandPairingDataRandomizer,
      chromeos::kBluetoothOutOfBandPairingDataSize);
  return data;
}

static bool CallClosure(const base::Closure& callback) {
  callback.Run();
  return true;
}

static bool CallOutOfBandPairingDataCallback(
      const chromeos::BluetoothAdapter::BluetoothOutOfBandPairingDataCallback&
          callback) {
  callback.Run(GetOutOfBandPairingData());
  return true;
}

}  // namespace

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, IsAvailable) {
  EXPECT_CALL(*mock_adapter_, IsPresent())
      .WillOnce(testing::Return(false));

  scoped_refptr<api::BluetoothIsAvailableFunction> is_available;

  is_available = setupFunction(new api::BluetoothIsAvailableFunction);
  expectBooleanResult(false, is_available, "[]");

  testing::Mock::VerifyAndClearExpectations(mock_adapter_);
  EXPECT_CALL(*mock_adapter_, IsPresent())
      .WillOnce(testing::Return(true));

  is_available = setupFunction(new api::BluetoothIsAvailableFunction);
  expectBooleanResult(true, is_available, "[]");
}

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, IsPowered) {
  EXPECT_CALL(*mock_adapter_, IsPowered())
      .WillOnce(testing::Return(false));

  scoped_refptr<api::BluetoothIsPoweredFunction> is_powered;

  is_powered = setupFunction(new api::BluetoothIsPoweredFunction);
  expectBooleanResult(false, is_powered, "[]");

  testing::Mock::VerifyAndClearExpectations(mock_adapter_);
  EXPECT_CALL(*mock_adapter_, IsPowered())
      .WillOnce(testing::Return(true));

  is_powered = setupFunction(new api::BluetoothIsPoweredFunction);
  expectBooleanResult(true, is_powered, "[]");
}

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, GetDevicesWithServiceUUID) {
  testing::NiceMock<chromeos::MockBluetoothDevice> device1(
      mock_adapter_, "d1", "11:12:13:14:15:16",
      true /* paired */, false /* bonded */, true /* connected */);
  testing::NiceMock<chromeos::MockBluetoothDevice> device2(
      mock_adapter_, "d2", "21:22:23:24:25:26",
      false /* paired */, true /* bonded */, false /* connected */);
  chromeos::BluetoothAdapter::ConstDeviceList devices;
  devices.push_back(&device1);
  devices.push_back(&device2);

  EXPECT_CALL(device1, ProvidesServiceWithUUID("foo"))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(device2, ProvidesServiceWithUUID("foo"))
      .WillOnce(testing::Return(true));

  EXPECT_CALL(*mock_adapter_, GetDevices())
      .WillOnce(testing::Return(devices));

  scoped_refptr<api::BluetoothGetDevicesWithServiceUUIDFunction> get_devices;

  get_devices =
      setupFunction(new api::BluetoothGetDevicesWithServiceUUIDFunction);
  scoped_ptr<base::Value> result(
      utils::RunFunctionAndReturnResult(get_devices, "[\"foo\"]", browser()));

  ASSERT_EQ(base::Value::TYPE_LIST, result->GetType());
  base::ListValue* list;
  ASSERT_TRUE(result->GetAsList(&list));

  EXPECT_EQ(1u, list->GetSize());
  base::Value* device_value;
  EXPECT_TRUE(list->Get(0, &device_value));
  EXPECT_EQ(base::Value::TYPE_DICTIONARY, device_value->GetType());
  base::DictionaryValue* device;
  ASSERT_TRUE(device_value->GetAsDictionary(&device));

  std::string name;
  ASSERT_TRUE(device->GetString("name", &name));
  EXPECT_EQ("d2", name);
  std::string address;
  ASSERT_TRUE(device->GetString("address", &address));
  EXPECT_EQ("21:22:23:24:25:26", address);
  bool paired;
  ASSERT_TRUE(device->GetBoolean("paired", &paired));
  EXPECT_EQ(false, paired);
  bool bonded;
  ASSERT_TRUE(device->GetBoolean("bonded", &bonded));
  EXPECT_EQ(true, bonded);
  bool connected;
  ASSERT_TRUE(device->GetBoolean("connected", &connected));
  EXPECT_EQ(false, connected);
}

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, GetLocalOutOfBandPairingData) {
  EXPECT_CALL(*mock_adapter_,
              ReadLocalOutOfBandPairingData(
                  testing::Truly(CallOutOfBandPairingDataCallback),
                  testing::_));

  scoped_refptr<api::BluetoothGetLocalOutOfBandPairingDataFunction>
      get_oob_function(setupFunction(
            new api::BluetoothGetLocalOutOfBandPairingDataFunction));

  scoped_ptr<base::Value> result(
      utils::RunFunctionAndReturnResult(get_oob_function, "[]", browser()));

  base::DictionaryValue* dict;
  EXPECT_TRUE(result->GetAsDictionary(&dict));

  base::BinaryValue* binary_value;
  EXPECT_TRUE(dict->GetBinary("hash", &binary_value));
  EXPECT_STREQ(kOutOfBandPairingDataHash,
      std::string(binary_value->GetBuffer(), binary_value->GetSize()).c_str());
  EXPECT_TRUE(dict->GetBinary("randomizer", &binary_value));
  EXPECT_STREQ(kOutOfBandPairingDataRandomizer,
      std::string(binary_value->GetBuffer(), binary_value->GetSize()).c_str());

  // Try again with an error
  testing::Mock::VerifyAndClearExpectations(mock_adapter_);
  EXPECT_CALL(*mock_adapter_,
              ReadLocalOutOfBandPairingData(
                  testing::_,
                  testing::Truly(CallClosure)));

  get_oob_function =
      setupFunction(new api::BluetoothGetLocalOutOfBandPairingDataFunction);

  std::string error(
      utils::RunFunctionAndReturnError(get_oob_function, "[]", browser()));
  EXPECT_FALSE(error.empty());
}

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, DISABLED_SetOutOfBandPairingData) {
  // TODO(bryeung): Fill in this test once it is possible to include an
  // ArrayBuffer in the arguments to the RunFunctionAnd* methods.
  // crbug.com/132796
}

IN_PROC_BROWSER_TEST_F(BluetoothApiTest, ClearOutOfBandPairingData) {
  std::string device_address("11:12:13:14:15:16");
  testing::NiceMock<chromeos::MockBluetoothDevice> device(
      mock_adapter_, "d1", device_address);
  EXPECT_CALL(*mock_adapter_, GetDevice(device_address))
      .WillOnce(testing::Return(&device));
  EXPECT_CALL(device,
              ClearOutOfBandPairingData(
                  testing::Truly(CallClosure),
                  testing::_));

  char buf[32];
  snprintf(buf, sizeof(buf), "[\"%s\"]", device_address.c_str());
  std::string params(buf);

  scoped_refptr<api::BluetoothClearOutOfBandPairingDataFunction>
      clear_oob_function;
  clear_oob_function = setupFunction(
      new api::BluetoothClearOutOfBandPairingDataFunction);
  // There isn't actually a result.
  (void)utils::RunFunctionAndReturnResult(
      clear_oob_function, params, browser());

  // Try again with an error
  testing::Mock::VerifyAndClearExpectations(mock_adapter_);
  testing::Mock::VerifyAndClearExpectations(&device);
  EXPECT_CALL(*mock_adapter_, GetDevice(device_address))
      .WillOnce(testing::Return(&device));
  EXPECT_CALL(device,
              ClearOutOfBandPairingData(
                  testing::_,
                  testing::Truly(CallClosure)));

  clear_oob_function = setupFunction(
      new api::BluetoothClearOutOfBandPairingDataFunction);
  std::string error(
      utils::RunFunctionAndReturnError(clear_oob_function, params, browser()));
  EXPECT_FALSE(error.empty());
}
