/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/frame/navigator_id.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "third_party/blink/public/common/features.h"

#if !defined(OS_MACOSX) && !defined(OS_WIN)
#include <sys/utsname.h>
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"
#endif

extern std::vector<std::string> g_wiimotes_read_data[4];
extern std::vector<std::string> g_wiimotes_real_read_data[4];


typedef struct {
  std::string axisName;
  float value;
} axis_data;

extern std::vector<int> g_nav_keyup[16];
extern std::vector<int> g_nav_keydown[16];
extern std::vector<int> g_native_keyup[16];
extern std::vector<int> g_native_keydown[16];
extern std::vector<axis_data> g_native_axis[16];

extern std::vector<int> g_nav_keyup_real[16];
extern std::vector<int> g_nav_keydown_real[16];
extern std::vector<int> g_native_keyup_real[16];
extern std::vector<int> g_native_keydown_real[16];
extern  std::vector<axis_data> g_native_axis_real[16];

namespace blink {

String NavigatorID::appCodeName() {
  return "Mozilla";
}

String NavigatorID::appName() {
  return "Netscape";
}

String NavigatorID::appVersion() {
  // Version is everything in the user agent string past the "Mozilla/" prefix.
  const String& agent = userAgent();
  return agent.Substring(agent.find('/') + 1);
}

String NavigatorID::platform() const {
  // If the User-Agent string is frozen, platform should be a value
  // matching the frozen string per https://github.com/WICG/ua-client-hints. See
  // content::frozen_user_agent_strings.
  if (base::FeatureList::IsEnabled(features::kFreezeUserAgent)) {
#if defined(OS_ANDROID)
    // Matches the frozen mobile User-Agent string (arbitrary Android device).
    return "Linux armv8l";
#else
    // Matches the frozen desktop User-Agent string (Windows).
    return "Win32";
#endif
  }

#if defined(OS_MACOSX)
  // Match Safari and Mozilla on Mac x86.
  return "MacIntel";
#elif defined(OS_WIN)
  // Match Safari and Mozilla on Windows.
  return "Win32";
#else  // Unix-like systems
  struct utsname osname;
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<String>, platform_name, ());
  if (platform_name->IsNull()) {
    *platform_name =
        String(uname(&osname) >= 0 ? String(osname.sysname) + String(" ") +
                                         String(osname.machine)
                                   : g_empty_string);
  }
  return *platform_name;
#endif
}

String NavigatorID::product() {
  return "Gecko";
}

void NavigatorID::saveWiimotePayloads(unsigned long index) {
	while (! g_wiimotes_read_data[(int)index].empty()) {
		g_wiimotes_real_read_data[(int)index].push_back(g_wiimotes_read_data[(int)index][0]);

		g_wiimotes_read_data[(int)index].erase(g_wiimotes_read_data[(int)index].begin());
	}
}

String NavigatorID::readWiimotePayload(unsigned long index) {
	if (! g_wiimotes_real_read_data[(int)index].size()) return g_empty_string;

	std::string str = g_wiimotes_real_read_data[(int)index][0];


	g_wiimotes_real_read_data[(int)index].erase(g_wiimotes_real_read_data[(int)index].begin());

	return str.c_str();
}

  void NavigatorID::saveNativeAxis( long index){
    index++;
    while (! g_native_axis[(int)index].empty()) {
      g_native_axis_real[(int)index].push_back(g_native_axis[(int)index][0]);

      g_native_axis[(int)index].erase(g_native_axis[(int)index].begin());
    } 
  }
  void NavigatorID::saveNativeKeys( long index){
    index++;
      while (! g_native_keydown[(int)index].empty()) {
      g_native_keydown_real[(int)index].push_back(g_native_keydown[(int)index][0]);

      g_native_keydown[(int)index].erase(g_native_keydown[(int)index].begin());
    } 
     while (! g_native_keyup[(int)index].empty()) {
      g_native_keyup_real[(int)index].push_back(g_native_keyup[(int)index][0]);

      g_native_keyup[(int)index].erase(g_native_keyup[(int)index].begin());
    } 
  }
  void NavigatorID::saveNavKeys( long index){
    index++;

    while (! g_nav_keydown[(int)index].empty()) {
      g_nav_keydown_real[(int)index].push_back(g_nav_keydown[(int)index][0]);

      g_nav_keydown[(int)index].erase(g_nav_keydown[(int)index].begin());
    } 
     while (! g_nav_keyup[(int)index].empty()) {
      g_nav_keyup_real[(int)index].push_back(g_nav_keyup[(int)index][0]);

      g_nav_keyup[(int)index].erase(g_nav_keyup[(int)index].begin());
    } 
  }

    NavKey *NavigatorID::readNavKey( long index) { 
         index++;

        if (g_nav_keydown_real[(int)index].size()) {
        int keycode = g_nav_keydown_real[(int)index][0];
        g_nav_keydown_real[(int)index].erase(g_nav_keydown_real[(int)index].begin());
          NavKey* ret = NavKey::Create();
          ret->setPressed(true);
          ret->setKey(keycode);
          return ret;
      }
      if (g_nav_keyup_real[(int)index].size()) {
        int keycode = g_nav_keyup_real[(int)index][0];
        g_nav_keyup_real[(int)index].erase(g_nav_keyup_real[(int)index].begin());
        NavKey* ret = NavKey::Create();
          ret->setPressed(false);
          ret->setKey(keycode);
          return ret;
      }

        return nullptr;
    }

    NativeKey *NavigatorID::readNativeKey( long index)  { 
      index++;

    	if (g_native_keydown_real[(int)index].size()) {
        int keycode = g_native_keydown_real[(int)index][0];
        g_native_keydown_real[(int)index].erase(g_native_keydown_real[(int)index].begin());
        NativeKey* ret = NativeKey::Create();
          ret->setPressed(true);
          ret->setKey(keycode);
          return ret;
      }
      if (g_native_keyup_real[(int)index].size()) {
        int keycode = g_native_keyup_real[(int)index][0];
        g_native_keyup_real[(int)index].erase(g_native_keyup_real[(int)index].begin());
        NativeKey* ret = NativeKey::Create();
          ret->setPressed(false);
          ret->setKey(keycode);
          return ret;
      }

        return nullptr;
      }
    NativeAxis *NavigatorID::readNativeAxis( long index)  { 
      index++;
        if (g_native_axis_real[(int)index].size()) {
        //int keycode = g_nav_keyup_real[0];
        axis_data ax = g_native_axis_real[(int)index][0];
        g_native_axis_real[(int)index].erase(g_native_axis_real[(int)index].begin());
        NativeAxis* ret = NativeAxis::Create();
          ret->setValue(ax.value);
          ret->setAxisName(ax.axisName.c_str());
          return ret;
      }
        return nullptr;
    }

}  // namespace blink
