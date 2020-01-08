#include "base/command_line.h"
#include "curl/curl.h"

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

int curl_get_file_size( std::string url, curl_off_t *filetime) {
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
			curl_easy_getinfo(curl, CURLINFO_FILETIME_T, filetime);
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
		stat(path, &st);
		return st.st_mtime * 1000;
/*		long        value;

		value = st.st_mtim
		value *= 1000;
                value += statinfo.st_mtim.tv_nsec / 1000000;
		return st.st_mtim.tv_nsec * 10000; */
	#endif
	return 0;
}

bool shouldDownloadFile(const char *path, curl_off_t filetime, int size) {
	bool mod_test = true;
	bool size_test = true;
	
	if (getFileModTime(path) < filetime * 1000) {
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
	void setECEmuPath() {
		
	}

#endif