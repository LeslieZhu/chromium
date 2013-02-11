// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/dev_tools_controller.h"

#include <algorithm>

#include <Cocoa/Cocoa.h>

#import "base/mac/foundation_util.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

@interface GraySplitView : NSSplitView {
  CGFloat topContentOffset_;
}

@property(assign, nonatomic) CGFloat topContentOffset;

- (NSColor*)dividerColor;

@end


@implementation GraySplitView

@synthesize topContentOffset = topContentOffset_;

- (NSColor*)dividerColor {
  return [NSColor darkGrayColor];
}

- (void)drawDividerInRect:(NSRect)aRect {
  NSRect dividerRect = aRect;
  if ([self isVertical]) {
    dividerRect.size.height -= topContentOffset_;
    dividerRect.origin.y += topContentOffset_;
  }
  [super drawDividerInRect:dividerRect];
}

- (NSView*)hitTest:(NSPoint)point {
  NSPoint viewPoint = [self convertPoint:point fromView:[self superview]];
  if (viewPoint.y < topContentOffset_)
    return nil;
  return [super hitTest:point];
}

@end

// Superview for the dev tools contents view. This class ensures that dev tools
// view doesn't overlap the toolbar when split vertically.
@interface DevToolsContainerView : NSView
@end

@implementation DevToolsContainerView

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  NSRect subviewFrame = [self bounds];
  GraySplitView* splitView =
      base::mac::ObjCCastStrict<GraySplitView>([self superview]);
  if ([splitView isVertical])
    subviewFrame.size.height -= [splitView topContentOffset];

  DCHECK_EQ(1u, [[self subviews] count]);
  [[[self subviews] lastObject] setFrame:subviewFrame];
}

@end


@interface DevToolsController (Private)
- (void)showDevToolsContainer;
- (void)hideDevToolsContainer;
- (void)updateDevToolsSplitPosition;
@end


@implementation DevToolsController

- (id)init {
  if ((self = [super init])) {
    splitView_.reset([[GraySplitView alloc] initWithFrame:NSZeroRect]);
    [splitView_ setDividerStyle:NSSplitViewDividerStyleThin];
    [splitView_ setVertical:NO];
    [splitView_ setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [splitView_ setDelegate:self];

    dockSide_ = DEVTOOLS_DOCK_SIDE_BOTTOM;
  }
  return self;
}

- (void)dealloc {
  [splitView_ setDelegate:nil];
  [super dealloc];
}

- (NSView*)view {
  return splitView_.get();
}

- (NSSplitView*)splitView {
  return splitView_.get();
}

- (void)updateDevToolsForWebContents:(WebContents*)contents
                         withProfile:(Profile*)profile {
  DevToolsWindow* newDevToolsWindow = contents ?
      DevToolsWindow::GetDockedInstanceForInspectedTab(contents) : NULL;

  // Fast return in case of the same window having same orientation.
  if (devToolsWindow_ == newDevToolsWindow) {
    if (!newDevToolsWindow ||
        (newDevToolsWindow->dock_side() == dockSide_)) {
      if (newDevToolsWindow)
        [self updateDevToolsSplitPosition];
      return;
    }
  }

  // Store last used position.
  if (devToolsWindow_) {
    NSArray* subviews = [splitView_ subviews];
    DCHECK_EQ([subviews count], 2u);
    NSView* devToolsView = [subviews objectAtIndex:1];
    if (dockSide_ == DEVTOOLS_DOCK_SIDE_RIGHT)
      devToolsWindow_->SetWidth(NSWidth([devToolsView frame]));
    else
      devToolsWindow_->SetHeight(NSHeight([devToolsView frame]));
  }

  if (devToolsWindow_)
    [self hideDevToolsContainer];

  devToolsWindow_ = newDevToolsWindow;

  if (devToolsWindow_) {
    dockSide_ = devToolsWindow_->dock_side();
    [self showDevToolsContainer];
  }
}

- (void)setTopContentOffset:(CGFloat)offset {
  [splitView_ setTopContentOffset:offset];
}

- (void)showDevToolsContainer {
  NSArray* subviews = [splitView_ subviews];
  DCHECK_EQ([subviews count], 1u);
  WebContents* devToolsContents = devToolsWindow_->web_contents();

  // |devToolsView| is a TabContentsViewCocoa object, whose ViewID was
  // set to VIEW_ID_TAB_CONTAINER initially, so we need to change it to
  // VIEW_ID_DEV_TOOLS_DOCKED here.
  NSView* devToolsView = devToolsContents->GetNativeView();
  view_id_util::SetID(devToolsView, VIEW_ID_DEV_TOOLS_DOCKED);

  scoped_nsobject<DevToolsContainerView> devToolsContainerView(
      [[DevToolsContainerView alloc] initWithFrame:[devToolsView bounds]]);
  [devToolsContainerView addSubview:devToolsView];
  [splitView_ addSubview:devToolsContainerView];

  BOOL isVertical = devToolsWindow_->dock_side() == DEVTOOLS_DOCK_SIDE_RIGHT;
  [splitView_ setVertical:isVertical];
  [self updateDevToolsSplitPosition];
}

- (void)hideDevToolsContainer {
  NSArray* subviews = [splitView_ subviews];
  DCHECK_EQ([subviews count], 2u);
  NSView* oldDevToolsContentsView = [subviews objectAtIndex:1];
  [oldDevToolsContentsView removeFromSuperview];
  [splitView_ adjustSubviews];
}

- (void)updateDevToolsSplitPosition {
  NSArray* subviews = [splitView_ subviews];

  // It seems as if |-setPosition:ofDividerAtIndex:| should do what's needed,
  // but I can't figure out how to use it. Manually resize web and devtools.
  // TODO(alekseys): either make setPosition:ofDividerAtIndex: work or to add a
  // category on NSSplitView to handle manual resizing.
  NSView* webView = [subviews objectAtIndex:0];
  NSRect webFrame = [webView frame];
  NSView* devToolsView = [subviews objectAtIndex:1];
  NSRect devToolsFrame = [devToolsView frame];

  if (devToolsWindow_->dock_side() == DEVTOOLS_DOCK_SIDE_RIGHT) {
    CGFloat size = devToolsWindow_->GetWidth(NSWidth([splitView_ frame]));
    devToolsFrame.size.width = size;
    webFrame.size.width =
        NSWidth([splitView_ frame]) - ([splitView_ dividerThickness] + size);
  } else {
    CGFloat size = devToolsWindow_->GetHeight(NSHeight([splitView_ frame]));
    devToolsFrame.size.height = size;
    webFrame.size.height =
        NSHeight([splitView_ frame]) - ([splitView_ dividerThickness] + size);
  }

  [[splitView_ window] disableScreenUpdatesUntilFlush];
  [webView setFrame:webFrame];
  [devToolsView setFrame:devToolsFrame];

  [splitView_ adjustSubviews];
}

// NSSplitViewDelegate protocol.
- (BOOL)splitView:(NSSplitView *)splitView
    shouldAdjustSizeOfSubview:(NSView *)subview {
  // Return NO for the devTools view to indicate that it should not be resized
  // automatically. It preserves the height set by the user and also keeps
  // view height the same while changing tabs when one of the tabs shows infobar
  // and others are not.
  if ([[splitView_ subviews] indexOfObject:subview] == 1)
    return NO;
  return YES;
}

- (CGFloat)splitView:(NSSplitView*)splitView
    constrainSplitPosition:(CGFloat)proposedPosition
               ofSubviewAt:(NSInteger)dividerIndex {
  if (![splitView_ isVertical] &&
      proposedPosition < [splitView_ topContentOffset]) {
    return [splitView_ topContentOffset];
  }
  return proposedPosition;
}

-(void)splitViewWillResizeSubviews:(NSNotification *)notification {
  [[splitView_ window] disableScreenUpdatesUntilFlush];
}

@end
