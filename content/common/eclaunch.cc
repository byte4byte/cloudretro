#include "base/command_line.h"

const char kRegisterPepperPlugins[]         = "register-pepper-plugins";
//const char kEnablePepperTesting[] = "enable-pepper-testing";
//const char kFilesDir[] 						= "files-dir";
//const char kEcLibDir[] 						= "ec-lib-dir";
//const char kEcWwwDir[] 						= "ec-www-dir";
const char kProcessType[]                   = "type";
const char kEcUrl[]                 = "ec-url";

#if _WIN32

#include <windows.h>
#include <Shlobj.h>

static const wchar_t *g_ecemuURL = L"https://cloudretro.com/native/windows/win64/ecemu.dll";

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
#include <Shlobj.h>

#include "curl/curl.h"

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
  
  //MessageBoxA(NULL, "b4", "", MB_OK);
//  base::CommandLine* wcommand_line = base::CommandLine::ForCurrentProcess();
	if (HasSwitch(kEcUrl)) {
		const std::string theurl = GetSwitchValueASCII(
					  kEcUrl);
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
	}
}


#elif defined(OS_MACOSX)
	void setECEmuPath() {
		
	}

#endif