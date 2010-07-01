// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/registry.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension.h"
#include "rlz/win/lib/rlz_lib.h"

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, Rlz) {
  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableExperimentalExtensionApis);

  // Before running the tests, clear the state of the RLZ products used.
  rlz_lib::AccessPoint access_points[] = {
    rlz_lib::GD_WEB_SERVER,
    rlz_lib::GD_OUTLOOK,
    rlz_lib::NO_ACCESS_POINT,
  };
  rlz_lib::ClearProductState(rlz_lib::PINYIN_IME, access_points);
  rlz_lib::ClearProductState(rlz_lib::DESKTOP, access_points);

  // Check that the state has really been cleared.
  RegKey key(HKEY_CURRENT_USER, L"Software\\Google\\Common\\Rlz\\Events\\N");
  ASSERT_FALSE(key.Valid());

  // Set the access point that the test code is expecting.
  ASSERT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::GD_DESKBAND, "rlz_apitest"));

  // Now run all the tests.
  ASSERT_TRUE(RunExtensionTest("rlz")) << message_;

  // Now make sure we recorded what was expected.  If the code in test.js
  // changes, need to make appropriate changes here.
  key.Open(HKEY_CURRENT_USER, L"Software\\Google\\Common\\Rlz\\Events\\N");
  ASSERT_TRUE(key.Valid());

  DWORD value;
  ASSERT_TRUE(key.ReadValueDW(L"D3I", &value));
  ASSERT_EQ(1, value);
  ASSERT_TRUE(key.ReadValueDW(L"D3S", &value));
  ASSERT_EQ(1, value);
  ASSERT_TRUE(key.ReadValueDW(L"D3F", &value));
  ASSERT_EQ(1, value);

  ASSERT_TRUE(key.ReadValueDW(L"D4I", &value));
  ASSERT_EQ(1, value);

  key.Open(HKEY_CURRENT_USER, L"Software\\Google\\Common\\Rlz\\Events\\D");
  ASSERT_FALSE(key.Valid());

  // Cleanup.
  rlz_lib::ClearProductState(rlz_lib::PINYIN_IME, access_points);
}
