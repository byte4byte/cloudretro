// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/pepper_plugin_list.h"

#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/pepper_plugin_info.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

#if _WIN32

#include <windows.h>
#include <Shlobj.h>

static const wchar_t *g_ecemuURL = L"https://cloudretro.com/native/windows/win64/ecemu.dll";

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <Shlobj.h>

bool DownloadFile(const wchar_t * szUrl, const wchar_t* szPath) {
	HINTERNET hOpen = NULL;
	HINTERNET hFile = NULL;
	HANDLE hOut = NULL;
	char* lpBuffer = NULL;
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;

	hOpen = InternetOpen(L"CLOUDRETROAGENT", NULL, NULL, NULL, NULL);
	if(!hOpen) return false;

	hFile = InternetOpenUrl(hOpen, szUrl, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, NULL);
	if(!hFile) {
		InternetCloseHandle(hOpen);
		return false;
	}

	hOut = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		InternetCloseHandle(hFile);
		InternetCloseHandle(hOpen);
		return false;
	}

	do {
		lpBuffer = new char[2000];
		ZeroMemory(lpBuffer, 2000);
		InternetReadFile(hFile, (LPVOID)lpBuffer, 2000, &dwBytesRead);
		WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL);
		delete[] lpBuffer;
		lpBuffer = NULL;
	} while (dwBytesRead);

	CloseHandle(hOut);
	InternetCloseHandle(hFile);
	InternetCloseHandle(hOpen);
	return true;
}
#endif

#ifdef _WIN32
	LRESULT CALLBACK LaunchWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static HICON ico;
		static int leftoffset;
		static int anim = 0;
		static int linewidth = 0;
		static HWND urledit;
		static HWND launchbtn, offlinebtn;
		static HRGN hRgn;
		static HDC backbuffDC;
        static HBITMAP backbuffer;
		switch (uMsg) {
			case WM_CREATE: {
				RECT rc;
				GetClientRect(hWnd, &rc);
				ico = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(128));
				hRgn = CreateRoundRectRgn(0, 0, rc.right, rc.bottom, 24, 24);
				SetWindowRgn(hWnd, hRgn, TRUE);
				leftoffset = rc.right/2-200/2;
				SetTimer(hWnd, 1, 3, NULL);
				
				HDC hDC = GetDC(hWnd);
				backbuffDC = CreateCompatibleDC(hDC);
                backbuffer = CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
				ReleaseDC(hWnd, hDC);
				
				ico = (HICON)LoadImage(NULL, TEXT("logo.ico"), IMAGE_ICON, 200, 180, LR_LOADFROMFILE);
				return 0;
			}
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC hDC = BeginPaint(hWnd, &ps);
				
				SelectObject(backbuffDC, backbuffer);
				
				RECT rc;
				GetClientRect(hWnd, &rc);
				
				FillRect(backbuffDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
				
				hRgn = CreateRoundRectRgn(2, 2, rc.right-2, rc.bottom-2, 24, 24);
				HBRUSH hbr = (HBRUSH)CreateSolidBrush(RGB(188,188,188));
				FrameRgn(backbuffDC, hRgn, hbr, 1, 1);
				DeleteObject(hbr);
				DeleteObject(hRgn);
				
				
				
				{
					RECT rect;
					HFONT hFont = CreateFont(55,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					SelectObject(backbuffDC, hFont);
					
					//Sets the coordinates for the rectangle in which the text is to be formatted.
					SetRect(&rect, 260,100,700,200);
					SetBkMode(backbuffDC, TRANSPARENT);
					SetTextColor(backbuffDC, RGB(255,255,255));
					DrawText(backbuffDC, TEXT("Cloud Retro"), -1,&rect, DT_NOCLIP);
					
					DeleteObject(hFont);
					
					RECT rcFill;
					SetRect(&rcFill, 6,100,leftoffset+200,200);
					FillRect(backbuffDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH));
				}
				DrawIconEx( backbuffDC, leftoffset, 30, ico, 200, 180, 0, NULL, DI_NORMAL); // animate to 30 x
				if (leftoffset <= 30) {
					if (anim == 0) {
						anim = 1;
						SetTimer(hWnd, 2, 3, NULL);
					}
				}
				if (linewidth > 0) {
					HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
					HPEN hOldPen = (HPEN)SelectObject(backbuffDC, hPen); 
					MoveToEx(backbuffDC, 10, 240, NULL);
					LineTo(backbuffDC, linewidth+10, 240);
					SelectObject(backbuffDC, hOldPen);
					DeleteObject(hPen);
				}
				
				if (anim >= 2) {
					RECT rect;
					HFONT hFont = CreateFont(48,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					SelectObject(backbuffDC, hFont);
					
					//Sets the coordinates for the rectangle in which the text is to be formatted.
					SetRect(&rect, rc.right-40,10,60,60);
					SetBkMode(backbuffDC, TRANSPARENT);
					SetTextColor(backbuffDC, RGB(255,120,120));
					DrawText(backbuffDC, TEXT("X"), -1,&rect, DT_NOCLIP);
					
					DeleteObject(hFont); 
					hFont = CreateFont(28,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					SelectObject(backbuffDC, hFont);
					SetRect(&rect, 35,290,20,60);
					SetBkMode(backbuffDC, TRANSPARENT);
					SetTextColor(backbuffDC, RGB(255,255,255));
					DrawText(backbuffDC, TEXT("Instance URL:"), -1,&rect, DT_NOCLIP);
					
					DeleteObject(hFont); 
				}
				
				if (anim == 2) {
					anim = 3;
					HFONT hFont = CreateFont(38,0,0,0,FW_THIN,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));

					urledit = CreateWindowEx(0, L"EDIT", L"https://cloudretro.com", WS_CHILD | WS_VISIBLE, 30, 330, rc.right-60, 40, hWnd, NULL, GetModuleHandle(NULL), NULL);
					SendMessage(urledit, WM_SETFONT, (WPARAM)hFont, TRUE);
					
					hFont = CreateFont(22,0,0,0,FW_THIN,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					
					
					launchbtn = CreateWindowEx(0, L"BUTTON", L"Launch", WS_CHILD | WS_VISIBLE, rc.right/2-100/2, 400, 100, 40, hWnd, (HMENU)10000, GetModuleHandle(NULL), NULL);
					SendMessage(launchbtn, WM_SETFONT, (WPARAM)hFont, TRUE);
					offlinebtn = CreateWindowEx(0, L"BUTTON", L"Run in Offline Mode", WS_CHILD | WS_VISIBLE, rc.right/2-200/2, 510, 200, 40, hWnd, NULL, GetModuleHandle(NULL), NULL);
					SendMessage(offlinebtn, WM_SETFONT, (WPARAM)hFont, TRUE);
				}
				
				BitBlt(hDC, 0, 0, rc.right, rc.bottom, backbuffDC, 0, 0, SRCCOPY);
				
				EndPaint(hWnd, &ps);
				return 0;
			}
			case WM_MOUSEMOVE: {
				RECT rc;
				GetClientRect(hWnd, &rc);
				if (GET_X_LPARAM(lParam) >= (rc.right-40) && GET_Y_LPARAM(lParam) <= 60) {
					SetCursor(LoadCursor(NULL, IDC_HAND));
				}
				else {
					SetCursor(LoadCursor(NULL, IDC_ARROW));
				}
				return 0;
			}
			case WM_SETCURSOR:
				return 0;
			case WM_LBUTTONUP:
			{
				RECT rc;
				GetClientRect(hWnd, &rc);
				if (GET_X_LPARAM(lParam) >= (rc.right-40) && GET_Y_LPARAM(lParam) <= 60) {
					DestroyWindow(hWnd);
					exit(0);
				}
				return 0;
			}
			case WM_COMMAND:
				if(LOWORD(wParam)==10000)
				{
					DestroyWindow(hWnd);
				}
				return 0;
			case WM_TIMER:
				if (wParam == 1) {
					if (leftoffset > 30) {
						leftoffset -= 6;
						InvalidateRect(hWnd, NULL, TRUE);
					}
					else {
						KillTimer(hWnd, 1);
						InvalidateRect(hWnd, NULL, TRUE);
					}
					
				}
				else if (wParam == 2) {
					RECT rc;
					GetClientRect(hWnd, &rc);
					linewidth += 20;
					if (linewidth >= (rc.right-20)) {
						KillTimer(hWnd, 2);
						linewidth = rc.right-20;
						anim = 2;
					}
					InvalidateRect(hWnd, NULL, TRUE);
				}
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return (DefWindowProc(hWnd, uMsg, wParam, lParam));
		}
		return FALSE;
	}
#endif

void setECEmuPath() {
	
	#ifdef _WIN32
	{	
	
	
	const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  bool is_browser_process =
      command_line.GetSwitchValueASCII(switches::kProcessType).empty();
	if (is_browser_process) {
	
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)LaunchWindow;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	wcex.lpszClassName = L"CLOUDRETRO_LAUNCHER";
	RegisterClassEx(&wcex);
	
	int w = 800;
	int h = 600;
	
	CreateWindowEx(WS_EX_TOPMOST, L"CLOUDRETRO_LAUNCHER", L"CloudRetro Launcher", WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN, GetSystemMetrics(SM_CXSCREEN)/2-(w/2), GetSystemMetrics(SM_CYSCREEN)/2-(h/2), w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	
	}
 wchar_t path[MAX_PATH];
  SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path);
  lstrcat(path, L"\\CloudRetro");
  CreateDirectory(path, NULL);
  lstrcat(path, L"\\ecemu.dll");
  
 wchar_t bridgepath[MAX_PATH];
  SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, bridgepath);
  lstrcat(bridgepath, L"\\CloudRetro");
  CreateDirectory(bridgepath, NULL);
  lstrcat(bridgepath, L"\\ecbridge.dll");  
  
  if (is_browser_process) {
  
  //MessageBoxA(NULL, "b4", "", MB_OK);
  base::CommandLine* wcommand_line = base::CommandLine::ForCurrentProcess();
	if (wcommand_line->HasSwitch(switches::kEcUrl)) {
		const std::string theurl = wcommand_line->GetSwitchValueASCII(
					  switches::kEcUrl);
		std::string myurl = theurl;
		std::string ecemupath = "/native/windows/win64/ecemu.dll";
		myurl.append(ecemupath);
		
		wchar_t *wszDest = (wchar_t *)malloc(myurl.length() * 4 + 4);
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, myurl.c_str(), -1, wszDest, myurl.length() * 4);
		DownloadFile(wszDest, path);	
		
		free(wszDest);
	}
	else { 
		DownloadFile(g_ecemuURL, path);
	}
	
	CopyFile(path,bridgepath, FALSE);
	
  }
	
  //wchar_t w_ecemustr[MAX_PATH];
 // lstrcpy(w_ecemustr, path);
  //mbstowcs(w_ecemustr, path, sizeof(path));	
	}
#endif
				
	
	
	
	{
	char path[MAX_PATH*2+200];
	char homepath[MAX_PATH];
  SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, homepath);
  
  strcpy(path, homepath);
  strcat(path, "\\CloudRetro");
  strcat(path, "\\ecemu.dll");
  strcat(path, ";application/x-ppapi-ecemu");
  
  strcat(path, ",");
  
  strcat(path, homepath);
  strcat(path, "\\CloudRetro");
  strcat(path, "\\ecbridge.dll");
  strcat(path, ";application/x-ppapi-ecfs");
  
  wchar_t w_ecemustr[MAX_PATH*2+200];
  mbstowcs(w_ecemustr, path, sizeof(path));
  
   base::CommandLine::ForCurrentProcess()->AppendSwitchNative(
      switches::kRegisterPepperPlugins, w_ecemustr);
	}
}

#elif defined(OS_MACOSX)
	void setECEmuPath() {
		
	}

#endif

namespace content {
namespace {

// The maximum number of plugins allowed to be registered from command line.
const size_t kMaxPluginsToRegisterFromCommandLine = 64;

// Appends any plugins from the command line to the given vector.
void ComputePluginsFromCommandLine(std::vector<PepperPluginInfo>* plugins) {
  // On Linux, once we're sandboxed, we can't know if a plugin is available or
  // not. But (on Linux) this function is always called once before we're
  // sandboxed. So when this function is called for the first time we set a
  // flag if the plugin file is available. Then we can skip the check on file
  // existence in subsequent calls if the flag is set.
  // NOTE: In theory we could have unlimited number of plugins registered in
  // command line. But in practice, 64 plugins should be more than enough.
  static uint64_t skip_file_check_flags = 0;
  static_assert(
      kMaxPluginsToRegisterFromCommandLine <= sizeof(skip_file_check_flags) * 8,
      "max plugins to register from command line exceeds limit");

  bool out_of_process = true;
  //if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //        switches::kPpapiInProcess))
  //  out_of_process = false;
  
#if ! defined(OS_ANDROID)
	setECEmuPath();
#endif  

  const std::string value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kRegisterPepperPlugins);
  if (value.empty())
    return;

  // FORMAT:
  // command-line = <plugin-entry> + *( LWS + "," + LWS + <plugin-entry> )
  // plugin-entry =
  //    <file-path> +
  //    ["#" + <name> + ["#" + <description> + ["#" + <version>]]] +
  //    *1( LWS + ";" + LWS + <mime-type-data> )
  // mime-type-data = <mime-type> + [ LWS + "#" + LWS + <extension> ]
  std::vector<std::string> modules = base::SplitString(
      value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  size_t plugins_to_register = modules.size();
  if (plugins_to_register > kMaxPluginsToRegisterFromCommandLine) {
    DVLOG(1) << plugins_to_register << " pepper plugins registered from"
             << " command line which exceeds the limit (maximum "
             << kMaxPluginsToRegisterFromCommandLine << " plugins allowed)";
    plugins_to_register = kMaxPluginsToRegisterFromCommandLine;
  }

  for (size_t i = 0; i < plugins_to_register; ++i) {
    std::vector<std::string> parts = base::SplitString(
        modules[i], ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (parts.size() < 2) {
      DVLOG(1) << "Required mime-type not found";
      continue;
    }

    std::vector<std::string> name_parts = base::SplitString(
        parts[0], "#", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    PepperPluginInfo plugin;
    plugin.is_out_of_process = out_of_process;
#if defined(OS_WIN)
    // This means we can't provide plugins from non-ASCII paths, but
    // since this switch is only for development I don't think that's
    // too awful.
    plugin.path = base::FilePath(base::ASCIIToUTF16(name_parts[0]));
#else
    plugin.path = base::FilePath(name_parts[0]);
#endif

    uint64_t index_mask = 1ULL << i;
    if (!(skip_file_check_flags & index_mask)) {
      if (base::PathExists(plugin.path)) {
        skip_file_check_flags |= index_mask;
      } else {
        DVLOG(1) << "Plugin doesn't exist: " << plugin.path.MaybeAsASCII();
        continue;
      }
    }

    if (name_parts.size() > 1)
      plugin.name = name_parts[1];
    if (name_parts.size() > 2)
      plugin.description = name_parts[2];
    if (name_parts.size() > 3)
      plugin.version = name_parts[3];
    for (size_t j = 1; j < parts.size(); ++j) {
      std::vector<std::string> mime_parts = base::SplitString(
          parts[j], "#", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
      DCHECK_GE(mime_parts.size(), 1u);
      std::string mime_extension;
      if (mime_parts.size() > 1)
        mime_extension = mime_parts[1];
      WebPluginMimeType mime_type(mime_parts[0],
                                  mime_extension,
                                  plugin.description);
      plugin.mime_types.push_back(mime_type);
    }

    // If the plugin name is empty, use the filename.
    if (plugin.name.empty()) {
      plugin.name =
          base::UTF16ToUTF8(plugin.path.BaseName().LossyDisplayName());
    }

    // Command-line plugins get full permissions.
    plugin.permissions = ppapi::PERMISSION_ALL_BITS;

    plugins->push_back(plugin);
  }
}

}  // namespace

bool MakePepperPluginInfo(const WebPluginInfo& webplugin_info,
                          PepperPluginInfo* pepper_info) {
  if (!webplugin_info.is_pepper_plugin())
    return false;

  pepper_info->is_out_of_process =
      webplugin_info.type == WebPluginInfo::PLUGIN_TYPE_PEPPER_OUT_OF_PROCESS;

  pepper_info->path = base::FilePath(webplugin_info.path);
  pepper_info->name = base::UTF16ToASCII(webplugin_info.name);
  pepper_info->description = base::UTF16ToASCII(webplugin_info.desc);
  pepper_info->version = base::UTF16ToASCII(webplugin_info.version);
  pepper_info->mime_types = webplugin_info.mime_types;
  pepper_info->permissions = webplugin_info.pepper_permissions;

  return true;
}

void ComputePepperPluginList(std::vector<PepperPluginInfo>* plugins) {
  GetContentClient()->AddPepperPlugins(plugins);
  ComputePluginsFromCommandLine(plugins);
}

}  // namespace content
