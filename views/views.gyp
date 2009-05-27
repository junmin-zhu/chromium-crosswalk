# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common.gypi',
  ],
  'target_defaults': {
    'sources/': [
      ['exclude', '/(cocoa|gtk|win)/'],
      ['exclude', '_(cocoa|gtk|linux|mac|posix|skia|win|x)\\.(cc|mm?)$'],
      ['exclude', '/(gtk|win|x11)_[^/]*\\.cc$'],
    ],
    'conditions': [
      ['OS=="linux"', {'sources/': [
        ['include', '/gtk/'],
        ['include', '_(gtk|linux|posix|skia|x)\\.cc$'],
        ['include', '/(gtk|x11)_[^/]*\\.cc$'],
      ]}],
      ['OS=="mac"', {'sources/': [
        ['include', '/cocoa/'],
        ['include', '_(cocoa|mac|posix)\\.(cc|mm?)$'],
      ]}, { # else: OS != "mac"
        'sources/': [
          ['exclude', '\\.mm?$'],
        ],
      }],
      ['OS=="win"', {'sources/': [
        ['include', '_(win)\\.cc$'],
        ['include', '/win/'],
        ['include', '/win_[^/]*\\.cc$'],
      ]}],
    ],
  },
  'targets': [
    {
      'target_name': 'views',
      'type': '<(library)',
      'msvs_guid': '6F9258E5-294F-47B2-919D-17FFE7A8B751',
      'dependencies': [
        '../app/app.gyp:app_base',      
        '../app/app.gyp:app_strings',      
        '../app/app.gyp:app_resources',      
        '../skia/skia.gyp:skia',
        '../third_party/icu38/icu38.gyp:icui18n',
        '../third_party/icu38/icu38.gyp:icuuc',
      ],
      'sources': [
        # All .cc, .h under views, except unittests
        'accelerator.cc',
        'accelerator.h',
        'accessibility/accessibility_types.h',
        'accessibility/view_accessibility.cc',
        'accessibility/view_accessibility.h',
        'accessibility/view_accessibility_wrapper.cc',
        'accessibility/view_accessibility_wrapper.h',
        'background.cc',
        'background.h',
        'border.cc',
        'border.h',
        'controls/button/button.cc',
        'controls/button/button.h',
        'controls/button/button_dropdown.cc',
        'controls/button/button_dropdown.h',
        'controls/button/checkbox.cc',
        'controls/button/checkbox.h',
        'controls/button/custom_button.cc',
        'controls/button/custom_button.h',
        'controls/button/image_button.cc',
        'controls/button/image_button.h',
        'controls/button/menu_button.cc',
        'controls/button/menu_button.h',
        'controls/button/native_button.cc',
        'controls/button/native_button.h',
        'controls/button/native_button_gtk.cc',
        'controls/button/native_button_gtk.h',
        'controls/button/native_button_win.cc',
        'controls/button/native_button_win.h',
        'controls/button/native_button_wrapper.h',
        'controls/button/radio_button.cc',
        'controls/button/radio_button.h',
        'controls/button/text_button.cc',
        'controls/button/text_button.h',
        'controls/combo_box.cc',
        'controls/combo_box.h',
        'controls/hwnd_view.cc',
        'controls/hwnd_view.h',
        'controls/image_view.cc',
        'controls/image_view.h',
        'controls/label.cc',
        'controls/label.h',
        'controls/link.cc',
        'controls/link.h',
        'controls/menu/chrome_menu.cc',
        'controls/menu/chrome_menu.h',
        'controls/menu/controller.h',
        'controls/menu/menu.cc',
        'controls/menu/menu.h',
        'controls/menu/menu_gtk.cc',
        'controls/menu/menu_gtk.h',
        'controls/menu/menu_win.cc',
        'controls/menu/menu_win.h',
        'controls/menu/view_menu_delegate.h',
        'controls/message_box_view.cc',
        'controls/message_box_view.h',
        'controls/native_control.cc',
        'controls/native_control.h',
        'controls/native_control_gtk.cc',
        'controls/native_control_gtk.h',
        'controls/native_control_win.cc',
        'controls/native_control_win.h',
        'controls/native_view_host.cc',
        'controls/native_view_host.h',
        'controls/native_view_host_gtk.cc',
        'controls/native_view_host_gtk.h',
        'controls/scroll_view.cc',
        'controls/scroll_view.h',
        'controls/scrollbar/bitmap_scroll_bar.cc',
        'controls/scrollbar/bitmap_scroll_bar.h',
        'controls/scrollbar/native_scroll_bar.cc',
        'controls/scrollbar/native_scroll_bar.h',
        'controls/scrollbar/scroll_bar.cc',
        'controls/scrollbar/scroll_bar.h',
        'controls/separator.cc',
        'controls/separator.h',
        'controls/single_split_view.cc',
        'controls/single_split_view.h',
        'controls/tabbed_pane.cc',
        'controls/tabbed_pane.h',
        'controls/table/group_table_view.cc',
        'controls/table/group_table_view.h',
        'controls/table/table_view.cc',
        'controls/table/table_view.h',
        'controls/textfield/textfield.cc',
        'controls/textfield/textfield.h',
        'controls/textfield/native_textfield_gtk.cc',
        'controls/textfield/native_textfield_gtk.h',
        'controls/textfield/native_textfield_win.cc',
        'controls/textfield/native_textfield_win.h',
        'controls/textfield/native_textfield_wrapper.h',
        'controls/throbber.cc',
        'controls/throbber.h',
        'controls/tree/tree_view.cc',
        'controls/tree/tree_view.h',
        'drag_utils.cc',
        'drag_utils.h',
        'event.cc',
        'event.h',
        'event_gtk.cc',
        'event_win.cc',
        'fill_layout.cc',
        'fill_layout.h',
        'focus/external_focus_tracker.cc',
        'focus/external_focus_tracker.h',
        'focus/focus_manager.cc',
        'focus/focus_manager.h',
        'focus/focus_util_win.cc',
        'focus/focus_util_win.h',
        'focus/view_storage.cc',
        'focus/view_storage.h',
        'grid_layout.cc',
        'grid_layout.h',
        'layout_manager.cc',
        'layout_manager.h',
        'painter.cc',
        'painter.h',
        'repeat_controller.cc',
        'repeat_controller.h',
        'standard_layout.h',
        'view.cc',
        'view.h',
        'view_constants.cc',
        'view_constants.h',
        'view_gtk.cc',
        'view_win.cc',
        'widget/accelerator_handler.cc',
        'widget/accelerator_handler.h',
        'widget/aero_tooltip_manager.cc',
        'widget/aero_tooltip_manager.h',
        'widget/default_theme_provider.cc',
        'widget/default_theme_provider.h',
        'widget/root_view.cc',
        'widget/root_view.h',
        'widget/root_view_drop_target.cc',
        'widget/root_view_drop_target.h',
        'widget/root_view_gtk.cc',
        'widget/root_view_win.cc',
        'widget/tooltip_manager.cc',
        'widget/tooltip_manager.h',
        'widget/widget.h',
        'widget/widget_gtk.cc',
        'widget/widget_gtk.h',
        'widget/widget_win.cc',
        'widget/widget_win.h',
        'window/client_view.cc',
        'window/client_view.h',
        'window/custom_frame_view.cc',
        'window/custom_frame_view.h',
        'window/dialog_client_view.cc',
        'window/dialog_client_view.h',
        'window/dialog_delegate.cc',
        'window/dialog_delegate.h',
        'window/native_frame_view.cc',
        'window/native_frame_view.h',
        'window/non_client_view.cc',
        'window/non_client_view.h',
        'window/window.h',
        'window/window_delegate.h',
        'window/window_delegate.cc',
        'window/window_resources.h',
        'window/window_gtk.cc',
        'window/window_gtk.h',
        'window/window_win.cc',
        'window/window_win.h',
      ],
      'include_dirs': [
        '../chrome/third_party/wtl/include',
      ],
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
          'sources!': [
            'accelerator.cc',
            'accessibility/accessible_wrapper.cc',
            'accessibility/view_accessibility.cc',
            'accessibility/view_accessibility_wrapper.cc',
            'controls/scrollbar/bitmap_scroll_bar.cc',
            'controls/button/button_dropdown.cc',
            'controls/button/radio_button.cc',
            'controls/combo_box.cc',
            'controls/hwnd_view.cc',
            'controls/menu/chrome_menu.cc',
            'controls/message_box_view.cc',
            'controls/scroll_view.cc',
            'controls/table/group_table_view.cc',
            'controls/native_control.cc',
            'controls/scrollbar/native_scroll_bar.cc',
            'controls/button/radio_button.cc',
            'controls/separator.cc',
            'controls/single_split_view.cc',
            'controls/tabbed_pane.cc',
            'controls/table/table_view.cc',
            'controls/table/group_table_view.cc',
            'controls/text_field.cc',
            'controls/tree/tree_view.cc',
            'drag_utils.cc',
            'drag_utils.h',
            'event_win.cc',
            'resize_corner.cc',
            'widget/accelerator_handler.cc',
            'widget/aero_tooltip_manager.cc',
            'widget/root_view_drop_target.cc',
            'widget/tooltip_manager.cc',
            'window/dialog_delegate.cc',
            'window/dialog_client_view.cc',
            'window/hit_test.cc',
            'window/native_frame_view.cc',
            'widget/widget_win.cc',
          ],
        }],
        ['OS=="win"', {
          'defines': [
            '__STD_C',
            '_CRT_SECURE_NO_DEPRECATE',
            '_SCL_SECURE_NO_DEPRECATE',
          ],
          'include_dirs': [
            # TODO(beng): move wtl to src/third_party
            '../chrome/third_party/wtl/include',
          ],
        }],
      ],
    },
  ],
}
