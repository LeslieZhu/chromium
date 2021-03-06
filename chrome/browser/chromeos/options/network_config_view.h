// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/string16.h"
#include "chrome/browser/chromeos/cros/network_library.h"
#include "ui/gfx/native_widget_types.h"  // gfx::NativeWindow
#include "ui/views/controls/button/button.h"  // views::ButtonListener
#include "ui/views/window/dialog_delegate.h"

namespace gfx {
class ImageSkia;
}

namespace views {
class ImageView;
class LabelButton;
}

namespace chromeos {

class ChildNetworkConfigView;
class NetworkPropertyUIData;

// A dialog box for showing a password textfield.
class NetworkConfigView : public views::DialogDelegateView,
                          public views::ButtonListener {
 public:
  class Delegate {
   public:
    // Called when dialog "OK" button is pressed.
    virtual void OnDialogAccepted() = 0;

    // Called when dialog "Cancel" button is pressed.
    virtual void OnDialogCancelled() = 0;

   protected:
     virtual ~Delegate() {}
  };

  // Shows a network connection dialog if none is currently visible.
  // Returns false if a dialog is already visible.
  static bool Show(Network* network, gfx::NativeWindow parent);
  static bool ShowForType(ConnectionType type, gfx::NativeWindow parent);

  // Returns corresponding native window.
  gfx::NativeWindow GetNativeWindow() const;

  // views::DialogDelegate methods.
  virtual string16 GetDialogButtonLabel(ui::DialogButton button) const OVERRIDE;
  virtual bool IsDialogButtonEnabled(ui::DialogButton button) const OVERRIDE;
  virtual bool Cancel() OVERRIDE;
  virtual bool Accept() OVERRIDE;
  virtual views::View* CreateExtraView() OVERRIDE;
  virtual views::View* GetInitiallyFocusedView() OVERRIDE;

  // views::WidgetDelegate methods.
  virtual ui::ModalType GetModalType() const OVERRIDE;

  // views::View overrides.
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

  // views::ButtonListener overrides.
  virtual void ButtonPressed(
      views::Button* sender, const ui::Event& event) OVERRIDE;

  void set_delegate(Delegate* delegate) {
    delegate_ = delegate;
  }

 protected:
  // views::View overrides:
  virtual void Layout() OVERRIDE;
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE;

 private:
  // Login dialog for known networks.
  explicit NetworkConfigView(Network* network);
  // Login dialog for new/hidden networks.
  explicit NetworkConfigView(ConnectionType type);
  virtual ~NetworkConfigView();

  // Creates and shows a dialog containing this view.
  void ShowDialog(gfx::NativeWindow parent);

  // Resets the underlying view to show advanced options.
  void ShowAdvancedView();

  // There's always only one child view, which will get deleted when
  // NetworkConfigView gets cleaned up.
  ChildNetworkConfigView* child_config_view_;

  Delegate* delegate_;

  // Button in lower-left corner, may be null or hidden.
  views::LabelButton* advanced_button_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConfigView);
};

// Children of NetworkConfigView must subclass this and implement the virtual
// methods, which are called by NetworkConfigView.
class ChildNetworkConfigView : public views::View {
 public:
  ChildNetworkConfigView(NetworkConfigView* parent, Network* network)
      : service_path_(network->service_path()),
        parent_(parent) {}
  explicit ChildNetworkConfigView(NetworkConfigView* parent)
      : parent_(parent) {}
  virtual ~ChildNetworkConfigView() {}

  // Returns view that should be focused on dialog activation.
  virtual views::View* GetInitiallyFocusedView() = 0;

  // Called to determine if "Connect" button should be enabled.
  virtual bool CanLogin() = 0;

  // Called when "Connect" button is clicked.
  // Should return false if dialog should remain open.
  virtual bool Login() = 0;

  // Called when "Cancel" button is clicked.
  virtual void Cancel() = 0;

  // Called to set focus when view is recreated with the same dialog
  // being active. For example, clicking on "Advanced" button.
  virtual void InitFocus() = 0;

  // Minimum with of input fields / combo boxes.
  static const int kInputFieldMinWidth;

 protected:
  std::string service_path_;
  NetworkConfigView* parent_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChildNetworkConfigView);
};

// Shows an icon with tooltip indicating whether a setting is under policy
// control.
class ControlledSettingIndicatorView : public views::View {
 public:
  ControlledSettingIndicatorView();
  explicit ControlledSettingIndicatorView(const NetworkPropertyUIData& ui_data);
  virtual ~ControlledSettingIndicatorView();

  // Updates the view based on |ui_data|.
  void Update(const NetworkPropertyUIData& ui_data);

 protected:
  // views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual void OnMouseEntered(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseExited(const ui::MouseEvent& event) OVERRIDE;

 private:
  // Initializes the view.
  void Init();

  bool managed_;
  views::ImageView* image_view_;
  const gfx::ImageSkia* gray_image_;
  const gfx::ImageSkia* color_image_;

  DISALLOW_COPY_AND_ASSIGN(ControlledSettingIndicatorView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_
