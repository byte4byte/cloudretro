#ifndef __ANDROID__
#include "base/command_line.h"
#include "curl/curl.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif

const char kRegisterPepperPlugins[]         = "register-pepper-plugins";
//const char kEnablePepperTesting[] = "enable-pepper-testing";
//const char kFilesDir[] 						= "files-dir";
//const char kEcLibDir[] 						= "ec-lib-dir";
//const char kEcWwwDir[] 						= "ec-www-dir";

const char kProcessType[]                   = "type";

const char kEcUrl[]                 = "ec-url";

size_t dl_filesize(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	return size*nmemb;
}

int curl_get_file_size( std::string url, time_t *filetime) {
  // Assume failure.
  int result = -1;

  CURL *curl = curl_easy_init();
  
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	//MessageBoxA(NULL, url.c_str(), "" ,MB_OK);
	curl_easy_setopt(curl, CURLOPT_NOBODY, true);
	curl_easy_setopt(curl, CURLOPT_HEADER, true);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true );
	//ec_curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, true );
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_filesize);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	if (filetime) {
		curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
	}
	
	res = curl_easy_perform(curl);
	  if(res != CURLE_OK) {
		//mytoast(ec_curl_easy_strerror(res));
	  }
	  else {
		double cl = -1;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,   &cl);
		result = (int)cl;
		if (filetime) {
			long mtime;
			curl_easy_getinfo(curl, CURLINFO_FILETIME, &mtime);
			*filetime = (time_t)mtime;
		}
		  
		curl_easy_cleanup(curl);
	  }

  return result;
}

#ifdef _WIN32
double FileTime_to_POSIX(FILETIME ft)
{
// takes the last modified date
LARGE_INTEGER date, adjust;
date.HighPart = ft.dwHighDateTime;
date.LowPart = ft.dwLowDateTime;
// 100-nanoseconds = milliseconds * 10000
adjust.QuadPart = 11644473600000 * 10000;
// removes the diff between 1970 and 1601
date.QuadPart -= adjust.QuadPart;
// converts back from 100-nanoseconds to seconds
return (double)(date.QuadPart / 10000000 * 1000);
}
#endif

FILE *g_dep_fp;
size_t writedep(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	if (g_dep_fp) {
		fwrite(ptr, size, nmemb, g_dep_fp);
	}
	return size*nmemb;
}

int emu_dl_progress_callback(void *clientp,   double dltotal,   double dlnow,   double ultotal,   double ulnow) {	
	//TODO: timeout logic here
	
	/*dl_wrote = (unsigned int)dlnow;

	long long now = current_timestamp();
	if (g_knownSizes && now - last_emu_dl_timestamp >= MIN_DL_UPDATE_TIME) {	
		pp::VarDictionary json;
		json.Set("action", "emu_dl_progress");
		json.Set("dl", pp::Var((int)(dl_wrote + total_emu_dl_installed)));
		json.Set("total", pp::Var((int)(total_emu_dl_size)));
		send_json(json, false);
		last_emu_dl_timestamp = now;
	}*/
	
	return 0;
}

time_t getFileModTime(const char *path) {
	#ifdef _WIN32
	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME  create, write, access;
		GetFileTime(hFile, &create, &access, &write);
		CloseHandle(hFile);
		
		return FileTime_to_POSIX(write);
	}
	#else
		// linux file time here
		struct stat st;
		if (stat(path, &st) == 0) {
			puts("stat success");
			return st.st_mtime;
		}
		puts("stat failed");
/*		long        value;

		value = st.st_mtim
		value *= 1000;
                value += statinfo.st_mtim.tv_nsec / 1000000;
		return st.st_mtim.tv_nsec * 10000; */
	#endif
	return 0;
}

bool shouldDownloadFile(const char *path, time_t filetime, int size) {
	bool mod_test = true;
	bool size_test = true;
	
	time_t path_time = getFileModTime(path);
	time_t remote_time = filetime;
	printf("mod failed - %s | %s\n", ctime(&path_time), ctime(&remote_time));
	if (path_time < remote_time) {
		
		mod_test = false;
	}

	#ifdef _WIN32
	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (GetFileSize(hFile, NULL) != (DWORD)size) {
			size_test = false;
		}
		CloseHandle(hFile);
	}
	#else
		struct stat st;
		stat(path, &st);
		if (size != st.st_size) {
			size_test = false;
			puts("size failed");
		}
	#endif

	return mod_test == false || size_test == false;
}

#if _WIN32

#include <windows.h>
#include <Shlobj.h>

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

typedef struct {
	HWND hWnd;
	HWND hStatus;
	char *url;
} dlinfo;

#endif

#ifdef _WIN32

#define WM_LAUNCHREADY  WM_USER+1

DWORD WINAPI DownloadThread(dlinfo *dl) {
		const char *ecemupath = "/native/windows/win64/ecemu.dll";
		char *ecemu = (char *)malloc(lstrlenA(dl->url) + lstrlenA(ecemupath) + 4);
		char *text = (char *)malloc(lstrlenA(dl->url) + lstrlenA(ecemupath) + 200);
		wsprintfA(ecemu, "%s%s", dl->url, ecemupath);
		wsprintfA(text, "Status: Checking - %s", ecemu);
		SetWindowTextA(dl->hStatus, text);
		
		curl_off_t filetime;
		DWORD size = curl_get_file_size(ecemu, &filetime);
		
		char path[MAX_PATH];
		SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
		lstrcatA(path, "\\CloudRetro");
		CreateDirectoryA(path, NULL);
		lstrcatA(path, "\\ecemu.dll");
		
		if (shouldDownloadFile(path, filetime, size)) {
			free(text);
			text = (char *)malloc(lstrlenA(dl->url) + lstrlenA(ecemu) + 200);
			wsprintfA(text, "Status: Downloading - %s", ecemu);
			SetWindowTextA(dl->hStatus, text);
			
			g_dep_fp = fopen(path, "wb");

			CURL *curl = curl_easy_init();
			if(curl) {
				CURLcode res;
				curl_easy_setopt(curl, CURLOPT_URL, ecemu);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writedep);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
				curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, emu_dl_progress_callback);
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
				res = curl_easy_perform(curl);
				if(res != CURLE_OK) {
					//mytoast(ec_curl_easy_strerror(res));
				}
				else {
					curl_easy_cleanup(curl);
				}
			  
				//total_emu_dl_installed += dl_wrote;
				//dl_wrote = 0;
			}
			
			if (g_dep_fp) fclose(g_dep_fp);
			
		}
		
		free(ecemu);
		free(text);

		free(dl->url);
		PostMessage(dl->hWnd, WM_LAUNCHREADY, 0, 0L);
		
		free(dl);
		return 0;
}

LPSTR g_szCRURL = NULL;

LPSTR get_val(const char *szKey) {
	HKEY pKey;
	DWORD dwType, dwBufSz = 0;
	LONG lResu = RegCreateKeyA( HKEY_CURRENT_USER , "Software\\Byte4Byte\\CloudRetro" , &pKey);
	if (lResu != ERROR_SUCCESS) return NULL;
	lResu = RegQueryValueExA(pKey, szKey, 0, &dwType, NULL, &dwBufSz);
	if (lResu != ERROR_SUCCESS) {
		RegCloseKey( pKey );
		return NULL;
	}
	if (dwType != REG_SZ) {
		RegCloseKey( pKey );
		return NULL;
	}
	if (dwBufSz == 0) {
		RegCloseKey( pKey );
		return NULL;
	}
	LPSTR ret = (LPSTR)malloc(dwBufSz+1);
	lResu = RegQueryValueExA(pKey, szKey, 0, &dwType, (LPBYTE)ret, &dwBufSz);
	if (lResu != ERROR_SUCCESS) {
		free(ret);
		RegCloseKey( pKey );
		return NULL;
	}
	RegCloseKey( pKey );
	return ret;
}

BOOL set_val(const char * szKey, const char * szVal) {
	HKEY pKey;
	LONG lResu = RegCreateKeyA( HKEY_CURRENT_USER , "Software\\Byte4Byte\\CloudRetro" , &pKey);
	if (lResu != ERROR_SUCCESS) return FALSE;
	lResu = RegSetValueExA(pKey , szKey , 0 , REG_SZ , (BYTE *)szVal , lstrlenA(szVal)+1);
	if (lResu != ERROR_SUCCESS) {
		RegCloseKey( pKey );
		return FALSE;
	}
	RegCloseKey( pKey );
	return TRUE;
}

	void LaunchDownload(HWND hWnd, HWND hStatus, char *url) {
		set_val("cloudretro_url", url);
		g_szCRURL = (LPSTR)malloc(lstrlenA(url)+1);
		lstrcpyA(g_szCRURL, url);
		dlinfo *dl = (dlinfo *)malloc(sizeof(dlinfo));
		dl->hWnd = hWnd;
		dl->hStatus = hStatus;
		dl->url = url;
		DWORD threadId;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DownloadThread, (void *)dl, 0, &threadId);
	}


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
		static bool launching = false;
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
					SetRect(&rect, rc.right-40,5,60,60);
					SetBkMode(backbuffDC, TRANSPARENT);
					SetTextColor(backbuffDC, RGB(255,120,120));
					DrawText(backbuffDC, TEXT("x"), -1,&rect, DT_NOCLIP);
					
					DeleteObject(hFont); 
					hFont = CreateFont(28,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					SelectObject(backbuffDC, hFont);
					SetRect(&rect, 35,290,20,60);
					SetBkMode(backbuffDC, TRANSPARENT);
					SetTextColor(backbuffDC, RGB(255,255,255));
					if (! launching) DrawText(backbuffDC, TEXT("Instance URL:"), -1,&rect, DT_NOCLIP);
					
					DeleteObject(hFont); 
				}
				
				if (anim == 2) {
					anim = 3;
					HFONT hFont = CreateFont(38,0,0,0,FW_THIN,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));

					urledit = CreateWindowEx(0, L"EDIT", L"https://cloudretro.com", WS_CHILD | WS_VISIBLE, 30, 330, rc.right-60, 40, hWnd, NULL, GetModuleHandle(NULL), NULL);
					SendMessage(urledit, WM_SETFONT, (WPARAM)hFont, TRUE);
					LPSTR url = get_val("cloudretro_url");
					if (url) {
						SetWindowTextA(urledit, url);
						free(url);
					}
					
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
			case WM_CTLCOLORSTATIC:
			{
				HDC hdcStatic = (HDC) wParam;
				SetTextColor(hdcStatic, RGB(255,255,255));
				SetBkColor(hdcStatic, RGB(0,0,0));
				return (INT_PTR)GetStockObject(BLACK_BRUSH);
			}
			case WM_COMMAND:
				if(LOWORD(wParam)==10000) // launch
				{
					//kEcUrl
					int len = GetWindowTextLengthA(urledit);
					char *url = (char *)malloc(len+2);
					GetWindowTextA(urledit, url, len+1);
					ShowWindow(launchbtn, SW_HIDE);
					ShowWindow(offlinebtn, SW_HIDE);
					ShowWindow(urledit, SW_HIDE);
					HFONT hFont = CreateFont(22,0,0,0,FW_THIN,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
						CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
					
					RECT rc;
					GetClientRect(hWnd, &rc);
					HWND hStatus = CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 30, 290, rc.right-60, 40, hWnd, NULL, GetModuleHandle(NULL), NULL);
					SendMessage(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
					SetWindowTextA(hStatus, "Status:");
					
					launching = false;
					InvalidateRect(hWnd, NULL, FALSE);
					
					LaunchDownload(hWnd, hStatus, url);
					//DestroyWindow(hWnd);
				}
				return 0;
			case WM_LAUNCHREADY:
				DestroyWindow(hWnd);
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
#ifdef _WIN32
typedef std::string (*t_GetSwitchValueASCII)(const char *str);
typedef bool (*t_HasSwitch)(const char *str);
typedef void (*t_AppendSwitchNative)(const char *str, wchar_t *val);
__declspec(dllexport) void setECEmuPath(t_GetSwitchValueASCII GetSwitchValueASCII, t_HasSwitch HasSwitch, t_AppendSwitchNative AppendSwitchNative) {	
	
  bool is_browser_process =
      GetSwitchValueASCII(kProcessType).empty();
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
	
	curl_easy_init();
	
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
 
	
	CopyFile(path,bridgepath, FALSE);
	
 // }
	
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
  
   AppendSwitchNative(
     kRegisterPepperPlugins, w_ecemustr);
	
	if (g_szCRURL) {
	wchar_t *w_url = (wchar_t *)malloc((lstrlenA(g_szCRURL)+2)*sizeof(wchar_t));
	mbstowcs(w_url, g_szCRURL, (lstrlenA(g_szCRURL)+2)*sizeof(wchar_t));
AppendSwitchNative(
     kEcUrl, w_url);
	free(w_url);
	free(g_szCRURL);
	}
	}
}


#elif defined(OS_MACOSX)

#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc.h>
#include <objc/objc-runtime.h>

static bool launching = false;
static NSWindow *window;
static NSTextField *textField;

typedef struct {
	char *url;
} dlinfo;

#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <sys/sendfile.h>
#endif

int copy_file(const char* source, const char* destination)
{    
    int input, output;    
    if ((input = open(source, O_RDONLY)) == -1)
    {
        return -1;
    }    
    if ((output = creat(destination, 0660)) == -1)
    {
        close(input);
        return -1;
    }

    //Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
    //fcopyfile works on FreeBSD and OS X 10.5+ 
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    //sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
}

void *DownloadThread(void *vdl) {
	dlinfo *dl = (dlinfo *)vdl;
	
	NSString *home = NSHomeDirectory();
	NSMutableString *hpath = [home mutableCopy];
	[hpath appendString:@"/CloudRetro"];
	mkdir([hpath UTF8String], 0777);
	
	NSMutableString *fsbridge = [hpath mutableCopy];
	[fsbridge appendString:@"/ecbridge.so"];
	
	[hpath appendString:@"/ecemu.so"];
	
	const char * path = [hpath UTF8String];
	const char *fspath = [fsbridge UTF8String];
	
	const char *ecemupath = "/native/osx/x64/ecemu";
	char *ecemu = (char *)malloc(strlen(dl->url) + strlen(ecemupath) + 4);
	char *text = (char *)malloc(strlen(dl->url) + strlen(ecemupath) + 200);
	sprintf(ecemu, "%s%s", dl->url, ecemupath);
	sprintf(text, "Status: Checking - %s", ecemu);
	//SetWindowTextA(dl->hStatus, text);
	
	curl_off_t filetime;
	int size = curl_get_file_size(ecemu, &filetime);
	
	//char path[1024];
	//SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	//strcpy(path, "/Users/patrick/byte4byte/cloudretro/ecemu.dll");
	//strcat(path, "\\CloudRetro");
	//CreateDirectoryA(path, NULL);
	//strcat(path, "\\ecemu.dll");
	
	if (shouldDownloadFile(path, filetime, size)) {
		free(text);
		text = (char *)malloc(strlen(dl->url) + strlen(ecemu) + 200);
		sprintf(text, "Status: Downloading - %s", ecemu);
		//SetWindowTextA(dl->hStatus, text);
		
		g_dep_fp = fopen(path, "wb");

		CURL *curl = curl_easy_init();
		if(curl) {
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL, ecemu);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writedep);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, emu_dl_progress_callback);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				//mytoast(ec_curl_easy_strerror(res));
			}
			else {
				curl_easy_cleanup(curl);
			}
		  
			//total_emu_dl_installed += dl_wrote;
			//dl_wrote = 0;
		}
		
		if (g_dep_fp) fclose(g_dep_fp);
		
		
		copy_file(path, fspath);
	}
	
	free(ecemu);
	free(text);

	//free((void *)dl->url);
	//PostMessage(dl->hWnd, WM_LAUNCHREADY, 0, 0L);
	
	free(dl);
	
	[[NSApplication sharedApplication] stopModal];
	[window close];
	
	
	return NULL;
}

static char *g_szCRURL = NULL;

NSString *get_val(NSString *szKey) {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	return [prefs stringForKey:szKey];
}

bool set_val(NSString *szKey, NSString *szVal) {
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults]; 
	[prefs setObject:szVal forKey:szKey];
	[prefs synchronize];
	return true;
}

void LaunchDownload(const char *url) {
	set_val(@"cloudretro_url", [NSString stringWithUTF8String:url]);
	g_szCRURL = (char *)malloc(strlen(url)+1);
	strcpy(g_szCRURL, url);
	dlinfo *dl = (dlinfo *)malloc(sizeof(dlinfo));
	dl->url = (char *)url;
	pthread_t dl_thread=0;
	pthread_create(&dl_thread, NULL, DownloadThread, (void *)dl);
}

@interface MyDialog : NSWindowController
- (instancetype)initWithFrame:(NSRect)frame;
- (void)runModal;
@end

@implementation MyDialog

- (instancetype)initWithFrame:(NSRect)frame {
    NSWindowStyleMask windowMask = NSWindowStyleMaskClosable | NSWindowStyleMaskTitled;
    window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:windowMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
	[window setBackgroundColor:[NSColor blackColor]];
	//[window setLevel: NSPopUpMenuWindowLevel];
	window.movable = NO;
	window.titlebarAppearsTransparent = true;
	
	NSView *superview = [window contentView]; 
	NSRect btnframe = NSMakeRect(800/2-120/2, 600/2-40/2-50, 120, 40); 
	NSButton *button = [[NSButton alloc] initWithFrame:btnframe]; 
	[button setTitle:@"Launch"]; 
	[superview addSubview:button]; 
	[button setTarget: self];
	[button setAction: @selector(myButtonWasHit:)];
	
	NSRect btntxt = NSMakeRect(40, 600/2-30/2-5, 800-40-40, 30); 
	textField = [[NSTextField alloc] initWithFrame:btntxt];
	[superview addSubview:textField];
	NSString *defurl = get_val(@"cloudretro_url");
	if (defurl) {
		[textField setStringValue:defurl];
	}
	else {
		[textField setStringValue:@"https://cloudretro.com"];
	}
	[textField setFont:[NSFont systemFontOfSize:20]];
	//[textField release];
	//rect.origin.y -= (padding + control_Height);
	//[button release]; 

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(windowWillClose:)
                                               name:NSWindowWillCloseNotification
                                             object:nil];
    return [super initWithWindow:window];
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
	[super dealloc];
}

- (void)runModal {
    [[NSApplication sharedApplication] runModalForWindow:self.window];
}

- (void)myButtonWasHit:(NSNotification *)notification {
	launching = true;
	NSString *url = [textField stringValue];
	
	/*NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:url];
	[alert addButtonWithTitle:@"OK"];
	[alert setAlertStyle:NSWarningAlertStyle];
	[alert runModal];*/
	LaunchDownload([url UTF8String]);
}

- (void)windowWillClose:(NSNotification *)notification {
    if (! launching) {
		[[NSApplication sharedApplication] stopModal];
		exit(0);
	}
}

@end

typedef std::string (*t_GetSwitchValueASCII)(const char *str);
typedef bool (*t_HasSwitch)(const char *str);
typedef void (*t_AppendSwitchNative)(const char *str, const char *val);

#define EXPORT __attribute__((visibility("default")))

EXPORT void setECEmuPath(t_GetSwitchValueASCII GetSwitchValueASCII, t_HasSwitch HasSwitch, t_AppendSwitchNative AppendSwitchNative) {	
	bool is_browser_process = GetSwitchValueASCII(kProcessType).empty();
	if (is_browser_process) {
		curl_easy_init();
		
		NSRect frame = NSMakeRect(0, 0, 800, 600);
		MyDialog *dialog = [[MyDialog alloc] initWithFrame:frame];
		[dialog runModal];
	}
	NSString *home = NSHomeDirectory();
	NSMutableString *hpath = [home mutableCopy];
	[hpath appendString:@"/CloudRetro"];
	[hpath appendString:@"/ecemu.so"];
	[hpath appendString:@";application/x-ppapi-ecemu"];
	
	[hpath appendString:@","];
	[hpath appendString:home];
	[hpath appendString:@"/CloudRetro"];
	[hpath appendString:@"/ecbridge.so"];
	[hpath appendString:@";application/x-ppapi-ecfs"];
  
	AppendSwitchNative(kRegisterPepperPlugins, [hpath UTF8String]);
	
	if (g_szCRURL) {
		char *w_url = (char *)malloc((strlen(g_szCRURL)+2)*sizeof(char));
		strcpy(w_url, g_szCRURL);
		AppendSwitchNative(kEcUrl, w_url);
		free(w_url);
		free(g_szCRURL);
	}
}

#endif
#endif

#ifdef __linux__

static void (* g_ready_func)(int success) = NULL;
static void (* g_set_status_text)(const char *txt)  = NULL;

static bool launching = false;

typedef struct {
	char *url;
} dnlinfo;

#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <sys/sendfile.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <dlfcn.h>

int copy_file(const char* source, const char* destination)
{    
    int input, output;    
    if ((input = open(source, O_RDONLY)) == -1)
    {
        return -1;
    }    
    if ((output = creat(destination, 0660)) == -1)
    {
        close(input);
        return -1;
    }

    //Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
    //fcopyfile works on FreeBSD and OS X 10.5+ 
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    //sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
}

void *DownloadThread(void *vdl) {
	dnlinfo *dl = (dnlinfo *)vdl;
	
	struct passwd *pw = getpwuid(getuid());
	const char *home = pw->pw_dir;
	//puts(homedir);
	
	char *hpath = (char *)malloc(strlen(home) + 100);
	strcpy(hpath, home);
	strcat(hpath, "/CloudRetro");
	
	puts(hpath);
	mkdir(hpath, 0777);
	
	char *fsbridge = (char *)malloc(strlen(hpath) + 100);
	strcpy(fsbridge, hpath);
	strcat(fsbridge, "/libecbridge.so");
	
	strcat(hpath, "/libecemu.so");
	
	const char *path = hpath;
	const char *fspath = fsbridge;
	
	//const char * path = [hpath UTF8String];
	//const char *fspath = [fsbridge UTF8String];
	
	const char *ecemupath = "/native/linux/arm/libecemu.so";
	char *ecemu = (char *)malloc(strlen(dl->url) + strlen(ecemupath) + 4);
	char *text = (char *)malloc(strlen(dl->url) + strlen(ecemupath) + 200);
	sprintf(ecemu, "%s%s", dl->url, ecemupath);
	sprintf(text, "Status: Checking - %s", ecemu);
	puts(text);
	//SetWindowTextA(dl->hStatus, text);
	
	time_t filetime;
	int size = curl_get_file_size(ecemu, &filetime);
	
	//char path[1024];
	//SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	//strcpy(path, "/Users/patrick/byte4byte/cloudretro/ecemu.dll");
	//strcat(path, "\\CloudRetro");
	//CreateDirectoryA(path, NULL);
	//strcat(path, "\\ecemu.dll");
	
	if (shouldDownloadFile(path, filetime, size)) {
		free(text);
		text = (char *)malloc(strlen(dl->url) + strlen(ecemu) + 200);
		sprintf(text, "Status: Downloading - %s", ecemu);
		puts(text);
		//SetWindowTextA(dl->hStatus, text);
		
		g_dep_fp = fopen(path, "wb");

		CURL *curl = curl_easy_init();
		if(curl) {
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL, ecemu);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writedep);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, emu_dl_progress_callback);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				//mytoast(ec_curl_easy_strerror(res));
			}
			else {
				curl_easy_cleanup(curl);
			}
		  
			//total_emu_dl_installed += dl_wrote;
			//dl_wrote = 0;
		}
		
		if (g_dep_fp) fclose(g_dep_fp);
		
		
		copy_file(path, fspath);
	}
	
	free(ecemu);
	free(text);

	//free((void *)dl->url);
	//PostMessage(dl->hWnd, WM_LAUNCHREADY, 0, 0L);
	
	free(dl);
	
	launching = false;
	
	free(hpath);
	free(fsbridge);
	
	puts("done");
	
	if (g_ready_func) g_ready_func(1);
	
	
	return NULL;
}

static char *g_szCRURL = NULL;

const char *get_val(const char *szKey) {
	static const char *ret = "https://cloudretro.com/";
	return ret;
}

bool set_val(const char *szKey, const char *szVal) {
	return true;
}

pthread_t dl_thread=0;

void LaunchDownload(const char *url) {
	set_val("cloudretro_url", url);
	g_szCRURL = (char *)malloc(strlen(url)+1);
	strcpy(g_szCRURL, url);
	dnlinfo *dl = (dnlinfo *)malloc(sizeof(dnlinfo));
	dl->url = (char *)url;
	
	pthread_create(&dl_thread, NULL, DownloadThread, (void *)dl);
}


static void g_launch_click_callback(const char *url) {
	if (launching) return;
	launching = true;
	
	LaunchDownload(url);
	//g_ready_func(1);
}


extern "C" void launch_window(const char *url,
				void (* launch_click_callback)(const char *url),
				   void (** ready)(int success),
				   void (** set_status_text)(const char *txt));


typedef std::string (*t_GetSwitchValueASCII)(const char *str);
typedef bool (*t_HasSwitch)(const char *str);
typedef void (*t_AppendSwitchNative)(const char *str, const char *val);
#define EXPORT __attribute__((visibility("default")))
EXPORT void setECEmuPath(t_GetSwitchValueASCII GetSwitchValueASCII, t_HasSwitch HasSwitch, t_AppendSwitchNative AppendSwitchNative) {
    bool is_browser_process = GetSwitchValueASCII(kProcessType).empty();
    if (is_browser_process) {		
		if (! HasSwitch("no-launcher")) {
			launch_window(get_val("url"), g_launch_click_callback, &g_ready_func, &g_set_status_text);
			AppendSwitchNative(kEcUrl, g_szCRURL);
		}
		else {
			LaunchDownload(get_val("url"));
			void *unused;
			pthread_join(dl_thread, &unused);
			AppendSwitchNative(get_val("url"), g_szCRURL);
		}
		
	
	struct passwd *pw = getpwuid(getuid());
	const char *home = pw->pw_dir;
	
	//const char *home = "/home/pi";
	
	//puts(homedir);
	
	char *path = (char *)malloc(strlen(home) * 2 + 512);
	strcpy(path, "");
	strcpy(path, home);
	strcat(path, "/CloudRetro/");
	
  strcat(path, "libecemu.so");
  strcat(path, ";application/x-ppapi-ecemu");
  
  strcat(path, ",");
  
  strcat(path, home);
  strcat(path, "/CloudRetro/");
  strcat(path, "libecbridge.so");
  strcat(path, ";application/x-ppapi-ecfs");
 /* 
  if (! dlopen("/home/pi/CloudRetro/libecbridge.so", RTLD_LAZY | RTLD_GLOBAL)) {
	  puts(dlerror());
	  //exit(0);
  }
  else {
	  puts("opened");
	  //exit(0);
  }*/
  
   AppendSwitchNative(
    kRegisterPepperPlugins, path);
	
	puts(path);
	 
	 free(path);
	 
	}
	 
	 //putenv((char *)"LD_LIBRARY_PATH=/home/pi/CloudRetro");

 if (kRegisterPepperPlugins[0]){}
}

#endif

