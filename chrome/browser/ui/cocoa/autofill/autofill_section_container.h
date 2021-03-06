// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_SECTION_CONTAINER_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_SECTION_CONTAINER_H_

#import <Cocoa/Cocoa.h>

#include "base/memory/scoped_nsobject.h"
#include "chrome/browser/ui/autofill/autofill_dialog_types.h"

namespace autofill {
  class AutofillDialogController;
}

@class LayoutView;
@class MenuButton;
@class MenuController;

// View controller for a section of the payment details. Contains a label
// describing the section as well as associated inputs and controls. Built
// dynamically based on data retrieved from AutofillDialogController.
@interface AutofillSectionContainer : NSViewController {
 @private
  scoped_nsobject<LayoutView> inputs_;
  scoped_nsobject<MenuButton> suggestButton_;
  scoped_nsobject<MenuController> menuController_;
  autofill::DialogSection section_;
  autofill::AutofillDialogController* controller_;  // Not owned.
}

@property(readonly, nonatomic) autofill::DialogSection section;

// Designated initializer. Queries |controller| for the list of desired input
// fields for |section|.
- (id)initWithController:(autofill::AutofillDialogController*)controller
              forSection:(autofill::DialogSection)section;

// Populates |output| with mappings from field identification to input value.
- (void)getInputs:(autofill::DetailOutputMap*)output;

// Called when the controller-maintained suggestions model has changed.
- (void)modelChanged;

@end

@interface AutofillSectionContainer (ForTesting)
- (NSControl*)getField:(autofill::AutofillFieldType)type;
@end

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_AUTOFILL_SECTION_CONTAINER_H_
