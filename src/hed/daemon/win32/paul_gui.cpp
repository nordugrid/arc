#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#define NOGDI
#include <windows.h>
#include <arc/win32.h>
#include <stdio.h>

#include <arc/ArcConfig.h>
#include "paul_gui.h"

const char g_szClassName[] = "PaulGUI";
HMENU popup_menu;
bool running = false;
Arc::Config *cfg = NULL;
PROCESS_INFORMATION processinfo;

static void start_arched(void)
{
    if (running) {
        return;
    }
    cfg->parse("paul_gui.xml");
    std::string p = (std::string)(*cfg)["ArcPluginPath"];
    SetEnvironmentVariable("ARC_PLUGIN_PATH", p.c_str());
    std::string path = p + ";%PATH%";
    SetEnvironmentVariable("PATH", path.c_str()); 
    std::string cmd = (std::string)(*cfg)["ArcHEDCmd"];
    std::string cfg_name = (std::string)(*cfg)["ArcHEDConfig"];
    // XXX read ArcHEDConfig and determine status URL
    cmd += (" " + cfg_name); // + " >log1 2>log2";
    printf ("cmd: %s\n", cmd.c_str());
    fflush(stdout);
    if (cmd.empty()) {
        return;
    }
    STARTUPINFO startupinfo;
    memset(&startupinfo, 0, sizeof(startupinfo));
    int result = CreateProcess(NULL, 
                             (LPSTR)cmd.c_str(), 
                               NULL, 
                               NULL, 
                               FALSE, 
                               CREATE_NEW_PROCESS_GROUP|CREATE_NO_WINDOW,
                               NULL, 
                               (LPSTR)".",
                               &startupinfo, 
                               &processinfo);
     printf("ret: %d\n", result);
     printf("error %s\n", GetOsErrorMessage().c_str());
     running = true;
     fflush(stdout);
}

static void stop_arched(void)
{
    if (!running) {
        return;
    }
    TerminateProcess(processinfo.hProcess, 256);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
        case WM_COMMAND:
        {   
            switch(LOWORD(wParam)) {
                case ID_SETUP: 
                {
                    printf("Setup\n");
                    fflush(stdout);
                    ShellExecute(NULL, "open", "http://localhost:60000/paul", NULL, NULL, SW_SHOWNORMAL); 
                    return 0;
                }
                break;
                case ID_CLOSE:
                {
                    printf ("close\n");
                    fflush(stdout);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                }
                break;
                case ID_RUN:
                {
                    start_arched();
                    return 0;
                }
                break;
                case ID_END:
                {
                    stop_arched();
                    return 0;
                }
                break;
            }
        }
        break;
        case WM_TRAYNOTIFY:
        {
            // catch event on try
            UINT uID = (UINT) wParam;
            UINT uMouseMsg = (UINT) lParam;
            if (uMouseMsg == WM_RBUTTONUP) {
                if (uID == IDI_PAULGUI) {
                    HMENU submenu = GetSubMenu(popup_menu, 0);
                    if (submenu == NULL) {
                        printf("null\n");
                    }   
                    SetMenuDefaultItem(submenu, 0, TRUE);
                    POINT mouse;
                    GetCursorPos(&mouse);
                    TrackPopupMenu(submenu, 0, mouse.x, mouse.y, 0, hwnd, NULL);                    fflush(stdout);
                    return 0;
                }
            }
        }
        break;
		case WM_CREATE:
		{
            // hide the created window
            ShowWindow(hwnd, SW_HIDE);
		}
		break;
		break;
		case WM_CLOSE:
        {
            stop_arched();
			DestroyWindow(hwnd);
        }
		break;
		case WM_DESTROY:
        {
            // Unregister Tray icon
            NOTIFYICONDATA tnd;
            tnd.cbSize = sizeof(NOTIFYICONDATA);
            tnd.hWnd = hwnd;
            tnd.uID = IDI_PAULGUI;
            Shell_NotifyIcon(NIM_DELETE, &tnd);
			PostQuitMessage(0);
        }
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
    cfg = new Arc::Config();
	
    WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		g_szClassName,
		"Arc - PAUL Service GUI",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 480, 320,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
        
    // Load menu
    popup_menu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
    if (popup_menu == NULL) {
        printf("Error load menu!\n");
        fflush(stdout);
    }
    // System tray
    NOTIFYICONDATA tnd;
    tnd.cbSize = sizeof(NOTIFYICONDATA);
    tnd.hWnd = hwnd;
    tnd.uID = IDI_PAULGUI;
    tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tnd.uCallbackMessage = WM_TRAYNOTIFY;
    tnd.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAULGUI)); 
    strcpy(tnd.szTip, "ARC - Paul Configurator");
    Shell_NotifyIcon(NIM_ADD, &tnd);
    EnableMenuItem(popup_menu, ID_CLOSE, MF_ENABLED);
    
    // start arched
    start_arched();
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
