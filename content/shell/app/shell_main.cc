// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "content/public/app/content_main.h"
#include "content/shell/app/shell_main_delegate.h"

#if defined(OS_WIN)
#include "base/win/win_util.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif

#if defined(OS_WIN)

#if !defined(WIN_CONSOLE_APP)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
#else
int main() {
  HINSTANCE instance = GetModuleHandle(NULL);
#endif
  // Load and pin user32.dll to avoid having to load it once tests start while
  // on the main thread loop where blocking calls are disallowed.
  base::win::PinUser32();
  sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
  content::InitializeSandboxInfo(&sandbox_info);
  content::ShellMainDelegate delegate;

  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  return content::ContentMain(params);
}

#else

int main(int old_argc, char** old_argv) {
	int new_argc = old_argc + 1;
	char **new_argv = (char **)malloc(sizeof(char *) * (new_argc));
	const char *nosandbox = "--no-sandbox";
	for (int i = 0; i < old_argc; i++) {
		new_argv[i] = old_argv[i];
	}
	new_argv[old_argc] = (char *)nosandbox;
	
  content::ShellMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = new_argc;
  params.argv = (const char **)new_argv;
  return content::ContentMain(params);
}

#endif  // OS_POSIX
