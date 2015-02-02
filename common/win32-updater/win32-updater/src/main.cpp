#include "stdafx.h"

#include <windows.h>
#include "../resource.h"
#include "updater.h"

volatile bool allowRun;

DWORD ThreadProc (LPVOID lpdwThreadParam)
{
	updater upd;
	upd.run();
	allowRun = false;
	ExitThread(0);
	return 0;
}

int CALLBACK DialogProc(HWND hwndDlg,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hwndDlg, TRUE);
            PostQuitMessage(0);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, FALSE);
            PostQuitMessage(0);
            return TRUE;

        default:
            return FALSE;
        }
        break;
    }
    return FALSE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	allowRun = true;
    BOOL bRet;
    MSG msg;
    HWND hwndDlg = NULL;

    hwndDlg =
        CreateDialog(
        hInstance,
        MAKEINTRESOURCE(IDD_DIALOG),
        NULL,
        DialogProc);
    bRet = ShowWindow(hwndDlg, SW_SHOW);
    bRet = UpdateWindow(hwndDlg);

	DWORD dwThreadId;


	HANDLE hThreadWorker = CreateThread(
		NULL, //Choose default security
		0, //Default stack size
		(LPTHREAD_START_ROUTINE)&ThreadProc,
		//Routine to execute
		NULL, //Thread parameter
		0, //Immediately run the thread
		&dwThreadId //Thread Id
	);

	SetDlgItemText(hwndDlg, IDC_TXT, MAKEINTRESOURCE(IDS_STRING_UPD));
   while ( allowRun && (bRet = GetMessage(&msg, NULL, 0, 0)) != 0 ) 
    { 
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
    }
    return 0;
}