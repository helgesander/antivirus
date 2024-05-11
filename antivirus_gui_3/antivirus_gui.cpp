#include "antivirus_gui.h"
#include "messages.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
	hInst = hInstance;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	WNDCLASS SoftwareMainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_ARROW), hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)), L"MainWndClass", WndProc);

	if (!RegisterClassW(&SoftwareMainClass)) { return -1; }
	MSG SoftwareMainMessage = { 0 };

	hMainWindow = CreateWindow(L"MainWndClass", L"Kaspersky v2.0", WS_SYSMENU | WS_MINIMIZEBOX, 300, 300, 400, 220, NULL, NULL, NULL, NULL);
	HDC hdc = GetDC(hMainWindow);
	SelectObject(hdc, hFont);
	ReleaseDC(hMainWindow, hdc);
	while (GetMessage(&SoftwareMainMessage, NULL, NULL, NULL)) {
		TranslateMessage(&SoftwareMainMessage);
		DispatchMessageW(&SoftwareMainMessage);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_COMMAND:
		switch (wp) {
		case EXIT: 
			PostQuitMessage(0);
			break;
		case SCAN_FILE_MENU:
			PrepareFileScanMenu(hWnd);
			break;
		case SCAN_FOLDER_MENU:
			PrepareFolderScanMenu(hWnd);
			break;
		case CANCEL:
			GetMainMenu();
			SetMainMenuWindowPos(hWnd);
			break;
		case CHOOSE_FILE:
			break;
		case CHOOSE_FOLDER:
			SelectFolderDialog(hWnd);
			break;
		case ID_KASPERSKYV2_40001:
			ShowWindow(hWnd, SW_SHOW);
			break;
		case ID_KASPERSKYV2_40002:
			PostQuitMessage(0);
			break;
		}
		break;
	case WMAPP_NOTIFYCALLBACK:
		switch (LOWORD(lp))
		{
		case NIN_SELECT:
			ShowWindow(hWnd, SW_SHOW);
			break;
		case WM_CONTEXTMENU:
			POINT const pt = { LOWORD(wp), HIWORD(wp) };
			ShowContextMenu(hWnd, pt);
			break;
		}
		break;
	case WM_CREATE: 
		InitWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE: 
		ShowWindow(hWnd, SW_HIDE);
		UpdateWindow(hWnd);
		break;
	default: return DefWindowProc(hWnd, msg, wp, lp);
	}
}

WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure) {
	WNDCLASS NWC = { 0 };

	NWC.hIcon = Icon;
	NWC.hCursor = Cursor;
	NWC.hInstance = hInst;
	NWC.lpszClassName = Name;
	NWC.hbrBackground = BGColor;
	NWC.lpfnWndProc = Procedure;

	return NWC;
}

void InitWindow(HWND hwnd) {
	AddNotificationIcon(hwnd);
	gui.hScanFileMenu = CreateWindowW(L"BUTTON", L"Сканировать файл", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 45, 50, 140, 40, hwnd, (HMENU)SCAN_FILE_MENU, hInst, nullptr);
	gui.hScanFolderMenu = CreateWindowW(L"BUTTON", L"Сканировать папку", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 205, 50, 140, 40, hwnd, (HMENU)SCAN_FOLDER_MENU, hInst, nullptr);
	gui.hScanFileBtn = CreateWindowW(L"BUTTON", L"Сканировать", WS_CHILD | WS_TABSTOP | ES_CENTER, 50, 170, 120, 40, hwnd, (HMENU)SCAN_FILE, hInst, nullptr);
	gui.hScanFolderBtn = CreateWindowW(L"BUTTON", L"Сканировать", WS_CHILD | WS_TABSTOP | ES_CENTER, 50, 170, 120, 40, hwnd, (HMENU)SCAN_FOLDER, hInst, nullptr);
	gui.hScanFileLabel = CreateWindowW(L"STATIC", L"Выберите файл для сканирования: ", WS_CHILD | ES_CENTER, 45, 50, 140, 40, hwnd, NULL, hInst, nullptr);
	gui.hScanFolderLabel = CreateWindowW(L"STATIC", L"Выберите папку для сканирования: ", WS_CHILD | ES_CENTER, 45, 50, 140, 40, hwnd, NULL, hInst, nullptr);
	gui.hChooseFileToWriteLabel = CreateWindowW(L"STATIC", L"Выберите файл для сохранения результатов", WS_CHILD | ES_CENTER, 45, 110, 140, 40, hwnd, NULL, hInst, nullptr);
	gui.hChooseFileToWriteBtn = CreateWindowW(L"BUTTON", L"...", WS_CHILD | ES_CENTER, 150, 110, 30, 30, hwnd, (HMENU)CHOOSE_FILE, hInst, nullptr);
	gui.hCancelButton = CreateWindowW(L"BUTTON", L"Назад", WS_CHILD, 50, 220, 50, 30, hwnd, (HMENU)CANCEL, hInst, nullptr);
	gui.hChooseFileToScan = CreateWindowW(L"BUTTON", L"...", WS_CHILD, 200, 50, 30, 30, hwnd, (HMENU)CHOOSE_FILE, hInst, nullptr);
	gui.hChooseFolderToScan = CreateWindowW(L"BUTTON", L"...", WS_CHILD, 200, 50, 30, 30, hwnd, (HMENU)CHOOSE_FOLDER, hInst, nullptr);
}

void PrepareFileScanMenu(HWND hWnd) {
	if (IsWindowVisible(gui.hScanFolderBtn)) ShowWindow(gui.hScanFolderBtn, SW_HIDE);
	if (IsWindowVisible(gui.hScanFolderLabel)) ShowWindow(gui.hScanFolderLabel, SW_HIDE);
	ShowWindow(gui.hScanFileBtn, SW_SHOW);
	ShowWindow(gui.hScanFileLabel, SW_SHOW);
	ShowWindow(gui.hCancelButton, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteBtn, SW_SHOW);
	ShowWindow(gui.hChooseFileToScan, SW_SHOW);
	ShowWindow(gui.hScanFileMenu, SW_HIDE);
	ShowWindow(gui.hScanFolderMenu, SW_HIDE);
	SetWindowPos(hWnd, NULL, 300, 300, 400, 400, NULL);
}

void PrepareFolderScanMenu(HWND hWnd) {
	if (IsWindowVisible(gui.hScanFileBtn)) ShowWindow(gui.hScanFileBtn, SW_HIDE);
	if (IsWindowVisible(gui.hScanFileLabel)) ShowWindow(gui.hScanFileLabel, SW_HIDE);
	ShowWindow(gui.hScanFolderBtn, SW_SHOW);
	ShowWindow(gui.hScanFolderLabel, SW_SHOW);
	ShowWindow(gui.hCancelButton, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteBtn, SW_SHOW);
	ShowWindow(gui.hChooseFolderToScan, SW_SHOW);
	ShowWindow(gui.hScanFileMenu, SW_HIDE);
	ShowWindow(gui.hScanFolderMenu, SW_HIDE);
	SetWindowPos(hWnd, NULL, 300, 300, 400, 400, NULL);
}

void GetMainMenu() {
	ShowWindow(gui.hScanFileMenu, SW_SHOW);
	ShowWindow(gui.hScanFolderMenu, SW_SHOW);
	IsWindowVisible(gui.hScanFileBtn) ? ShowWindow(gui.hScanFileBtn, SW_HIDE)
		: ShowWindow(gui.hScanFolderBtn, SW_HIDE);
	IsWindowVisible(gui.hScanFileLabel) ? ShowWindow(gui.hScanFileLabel, SW_HIDE)
		: ShowWindow(gui.hScanFolderLabel, SW_HIDE);
	ShowWindow(gui.hCancelButton, SW_HIDE);
	ShowWindow(gui.hChooseFileToWriteBtn, SW_HIDE);
	ShowWindow(gui.hChooseFileToWriteLabel, SW_HIDE);
	IsWindowVisible(gui.hChooseFileToScan) ? ShowWindow(gui.hChooseFileToScan, SW_HIDE)
		: ShowWindow(gui.hChooseFolderToScan, SW_HIDE);
}

void SetMainMenuWindowPos(HWND hWnd) {
	SetWindowPos(hWnd, NULL, 300, 300, 400, 220, NULL);
}

void SetOpenFileParams() {
	
}

BOOL SelectFolderDialog(HWND hWnd)
{
	BROWSEINFO browse_info = { 0 };
	TCHAR folder_path[MAX_PATH] = { 0 };

	browse_info.hwndOwner = hWnd;
	browse_info.pidlRoot = NULL;
	browse_info.pszDisplayName = folder_path;
	browse_info.lpszTitle = TEXT("Выберите папку для сканирования");
	browse_info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST item_list = SHBrowseForFolder(&browse_info);
	if (item_list != NULL)
	{
		SHGetPathFromIDList(item_list, g_szFolderPath);
		CoTaskMemFree(item_list);
		return TRUE;
	}
	return FALSE;
}



BOOL AddNotificationIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hwnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	// LoadString(hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
} 

void ShowContextMenu(HWND hwnd, POINT pt)
{
	HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_SYSTRAYMENU));
	if (hMenu) {
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			SetForegroundWindow(hwnd);
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
				uFlags |= TPM_RIGHTALIGN;
			else
				uFlags |= TPM_LEFTALIGN;

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
		}
		DestroyMenu(hMenu);
	}
} 