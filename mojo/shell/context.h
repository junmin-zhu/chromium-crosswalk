// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SHELL_CONTEXT_H_
#define MOJO_SHELL_CONTEXT_H_

#include <string>

#include "mojo/application_manager/application_manager.h"
#include "mojo/shell/keep_alive.h"
#include "mojo/shell/mojo_url_resolver.h"
#include "mojo/shell/task_runners.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_java_ref.h"
#endif  // defined(OS_ANDROID)

namespace mojo {

class Spy;

namespace shell {

class DynamicApplicationLoader;

// The "global" context for the shell's main process.
class Context {
 public:
  Context();
  ~Context();

  void Init();

  TaskRunners* task_runners() { return task_runners_.get(); }
  ApplicationManager* application_manager() { return &application_manager_; }
  KeepAliveCounter* keep_alive_counter() { return &keep_alive_counter_; }
  MojoURLResolver* mojo_url_resolver() { return &mojo_url_resolver_; }

#if defined(OS_ANDROID)
  base::MessageLoop* ui_loop() const { return ui_loop_; }
  void set_ui_loop(base::MessageLoop* ui_loop) { ui_loop_ = ui_loop; }
#endif  // defined(OS_ANDROID)

 private:
  class NativeViewportApplicationLoader;

  scoped_ptr<TaskRunners> task_runners_;
  ApplicationManager application_manager_;
  MojoURLResolver mojo_url_resolver_;
  scoped_ptr<Spy> spy_;
#if defined(OS_ANDROID)
  base::MessageLoop* ui_loop_;
#endif  // defined(OS_ANDROID)

  KeepAliveCounter keep_alive_counter_;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

}  // namespace shell
}  // namespace mojo

#endif  // MOJO_SHELL_CONTEXT_H_
