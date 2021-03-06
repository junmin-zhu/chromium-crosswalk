// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/ui/web_contents_sizer.h"

#import <Cocoa/Cocoa.h>

#include "content/public/browser/web_contents.h"

namespace apps {

void ResizeWebContents(content::WebContents* web_contents,
                       const gfx::Size& new_size) {
  NSView* view = web_contents->GetNativeView();
  NSRect old_wcv_frame = [view frame];
  CGFloat new_x = old_wcv_frame.origin.x;
  CGFloat new_y =
      old_wcv_frame.origin.y + (old_wcv_frame.size.height - new_size.height());
  NSRect new_wcv_frame =
      NSMakeRect(new_x, new_y, new_size.width(), new_size.height());
  [view setFrame:new_wcv_frame];
}

}  // namespace apps
