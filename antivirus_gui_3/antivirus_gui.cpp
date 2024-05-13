#include "antivirus_gui.h"
#include "messages.h"
#include <sddl.h>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
	hInst = hInstance;
	int nArgs;
	LPWSTR* arglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (NULL == arglist) 
		return -1;
	if (nArgs > 1) {
		if (arglist[1] == std::wstring(L"--secure-desktop")) {
			HDESK hCurrentDesktop = OpenInputDesktop(
				0,
				TRUE,
				DESKTOP_SWITCHDESKTOP
			);
			auto sid = GetCurrentUserSid();
			auto ssdl = std::format(L"O:{0}G:{0}D:", sid);
			auto security = GetSecurityAttributes(ssdl);

			HDESK hDesk = CreateDesktopW(
				L"AntivirusConfirmationDesktop",
				NULL,
				NULL,
				0,
				DESKTOP_CREATEWINDOW | DESKTOP_SWITCHDESKTOP,
				&security
			);

			SetThreadDesktop(hDesk);
			if (!SwitchDesktop(hDesk)) {
				return -1;
			}

			bool result = (IDYES == MessageBox(
				NULL,
				L"Are you shure?",
				L"Warning",
				MB_ICONWARNING | MB_YESNO
			));

			SwitchDesktop(hCurrentDesktop);
			SetThreadDesktop(hCurrentDesktop);

			CloseDesktop(hDesk);
			CloseDesktop(hCurrentDesktop);
			return result ? 1 : 0;
		}
	}
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
		case SCAN_FILE:
			DWORD bytesRead = 0;
			Write(pipe, reinterpret_cast<uint8_t*>(SCAN_FILE), sizeof(SCAN_FILE));
			Write(pipe, reinterpret_cast<uint8_t*>(selectedScanFilePath), sizeof(selectedScanFilePath));
			Write(pipe, reinterpret_cast<uint8_t*>(selectedFilePathToWrite), sizeof(selectedFilePathToWrite));
			int signal;
			Read(pipe, reinterpret_cast<uint8_t*>(signal), sizeof(signal), bytesRead);
			break;
		case SCAN_FOLDER:
			Write(pipe, reinterpret_cast<uint8_t*>(SCAN_FILE), sizeof(SCAN_FILE));
			Write(pipe, reinterpret_cast<uint8_t*>(g_szFolderPath), sizeof(g_szFolderPath));
			Write(pipe, reinterpret_cast<uint8_t*>(selectedFilePathToWrite), sizeof(selectedFilePathToWrite));
			int signal;
			Read(pipe, reinterpret_cast<uint8_t*>(signal), sizeof(signal), bytesRead);
			break;
		case CANCEL:
			GetMainMenu();
			SetMainMenuWindowPos(hWnd);
			break;
		case CHOOSE_FILE:
			SetOpenFileParams(hWnd, TEXT("All Files\0*.*\0Executable Files\0*.exe\0"), selectedScanFilePath);
			if (GetOpenFileName(&ofn) == TRUE) 
				SetWindowText(gui.hSelectedFolderOrFile, selectedScanFilePath);
			break;
		case CHOOSE_FOLDER:
			SelectFolderDialog(hWnd);
			if (GetOpenFileName(&ofn) == TRUE)
				SetWindowText(gui.hSelectedFolderOrFile, g_szFolderPath);
			break;
		case CHOOSE_WRITE_FILE:
			SetOpenFileParams(hWnd, TEXT("Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0"), selectedFilePathToWrite);
			if (GetOpenFileName(&ofn) == TRUE)
				SetWindowText(gui.hSelectedFileToWrite, selectedFilePathToWrite);
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
		InitializeConnection(hWnd);
		break;
	case WM_DESTROY:
		Write(pipe, reinterpret_cast<uint8_t*>(EXIT), sizeof(EXIT));
		CloseHandle(pipe);
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

SECURITY_ATTRIBUTES GetSecurityAttributes(const std::wstring& sddl) {
	SECURITY_ATTRIBUTES securityAttributes{};
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.bInheritHandle = TRUE;

	PSECURITY_DESCRIPTOR psd = nullptr;

	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &psd, nullptr))
		securityAttributes.lpSecurityDescriptor = psd;
	return securityAttributes;
}

std::wstring GetUserSid(HANDLE userToken) {
	std::wstring userSid;
	DWORD err = 0;
	LPVOID pvInfo = NULL;
	DWORD cbSize = 0;
	if (!GetTokenInformation(userToken, TokenUser, NULL, 0, &cbSize)) {
		err = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == err) {
			err = 0;
			pvInfo = LocalAlloc(LPTR, cbSize);
			if (!pvInfo)
				err = ERROR_OUTOFMEMORY;
			else if (!GetTokenInformation(userToken, TokenUser, pvInfo, cbSize, &cbSize))
				err = GetLastError();
			else {
				err = 0;
				const TOKEN_USER* pUser = (const TOKEN_USER*)pvInfo;
				LPWSTR userSidBuf;
				ConvertSidToStringSidW(pUser->User.Sid, &userSidBuf);
				userSid.assign(userSidBuf);
				LocalFree(userSidBuf);
			}
		}
	}
	return userSid;
}

std::wstring GetCurrentUserSid() {
	HANDLE token;
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &token)) {
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
			return L"";
	}
	return GetUserSid(token);
}

void InitWindow(HWND hwnd) {
	AddNotificationIcon(hwnd);
	gui.hScanFileMenu = CreateWindowW(L"BUTTON", L"Сканировать файл", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 45, 50, 140, 40, hwnd, (HMENU)SCAN_FILE_MENU, hInst, nullptr);
	gui.hScanFolderMenu = CreateWindowW(L"BUTTON", L"Сканировать папку", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 205, 50, 140, 40, hwnd, (HMENU)SCAN_FOLDER_MENU, hInst, nullptr);
	gui.hScanFileBtn = CreateWindowW(L"BUTTON", L"Сканировать", WS_CHILD | WS_TABSTOP | ES_CENTER, 70, 170, 120, 40, hwnd, (HMENU)SCAN_FILE, hInst, nullptr);
	gui.hScanFolderBtn = CreateWindowW(L"BUTTON", L"Сканировать", WS_CHILD | WS_TABSTOP | ES_CENTER, 70, 170, 120, 40, hwnd, (HMENU)SCAN_FOLDER, hInst, nullptr);
	gui.hScanFileLabel = CreateWindowW(L"STATIC", L"Выберите файл для сканирования: ", WS_CHILD | ES_CENTER, 45, 50, 200, 40, hwnd, NULL, hInst, nullptr);
	gui.hScanFolderLabel = CreateWindowW(L"STATIC", L"Выберите папку для сканирования: ", WS_CHILD | ES_CENTER, 45, 50, 200, 40, hwnd, NULL, hInst, nullptr);
	gui.hChooseFileToWriteLabel = CreateWindowW(L"STATIC", L"Выберите файл для сохранения результатов", WS_CHILD | ES_CENTER, 45, 110, 200, 40, hwnd, NULL, hInst, nullptr);
	gui.hChooseFileToWriteBtn = CreateWindowW(L"BUTTON", L"...", WS_CHILD | ES_CENTER, 250, 110, 30, 30, hwnd, (HMENU)CHOOSE_FILE, hInst, nullptr);
	gui.hCancelButton = CreateWindowW(L"BUTTON", L"Назад", WS_CHILD, 50, 220, 50, 30, hwnd, (HMENU)CANCEL, hInst, nullptr);
	gui.hChooseFileToScan = CreateWindowW(L"BUTTON", L"...", WS_CHILD, 250, 50, 30, 30, hwnd, (HMENU)CHOOSE_FILE, hInst, nullptr);
	gui.hChooseFolderToScan = CreateWindowW(L"BUTTON", L"...", WS_CHILD, 250, 50, 30, 30, hwnd, (HMENU)CHOOSE_FOLDER, hInst, nullptr);
	gui.hSelectedFolderOrFile = CreateWindow(L"STATIC", L"", WS_CHILD, 250, 50, 100, 40, hwnd, NULL, hInst, nullptr);
	gui.hSelectedFileToWrite = CreateWindow(L"STATIC", L"", WS_CHILD, 300, 50, 100, 40, hwnd, NULL, hInst, nullptr);
}

void PrepareFileScanMenu(HWND hWnd) {
	if (IsWindowVisible(gui.hScanFolderBtn)) ShowWindow(gui.hScanFolderBtn, SW_HIDE);
	if (IsWindowVisible(gui.hScanFolderLabel)) ShowWindow(gui.hScanFolderLabel, SW_HIDE);
	ShowWindow(gui.hScanFileBtn, SW_SHOW);
	ShowWindow(gui.hScanFileLabel, SW_SHOW);
	ShowWindow(gui.hCancelButton, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteBtn, SW_SHOW);
	ShowWindow(gui.hChooseFileToScan, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteLabel, SW_SHOW);
	ShowWindow(gui.hScanFileMenu, SW_HIDE);
	ShowWindow(gui.hScanFolderMenu, SW_HIDE);
	SetWindowPos(hWnd, NULL, 300, 300, 350, 350, NULL);
}

void PrepareFolderScanMenu(HWND hWnd) {
	if (IsWindowVisible(gui.hScanFileBtn)) ShowWindow(gui.hScanFileBtn, SW_HIDE);
	if (IsWindowVisible(gui.hScanFileLabel)) ShowWindow(gui.hScanFileLabel, SW_HIDE);
	ShowWindow(gui.hScanFolderBtn, SW_SHOW);
	ShowWindow(gui.hScanFolderLabel, SW_SHOW);
	ShowWindow(gui.hCancelButton, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteBtn, SW_SHOW);
	ShowWindow(gui.hChooseFolderToScan, SW_SHOW);
	ShowWindow(gui.hChooseFileToWriteLabel, SW_SHOW);
	ShowWindow(gui.hScanFileMenu, SW_HIDE);
	ShowWindow(gui.hScanFolderMenu, SW_HIDE);
	SetWindowPos(hWnd, NULL, 300, 300, 350, 350, NULL);
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

void SetOpenFileParams(HWND hWnd, LPCTSTR filter, wchar_t* szFile) {
	TCHAR szFolderPath[MAX_PATH];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szFolderPath)))
	{
		ofn.lpstrInitialDir = szFolderPath;
	}
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
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

HANDLE ConnectToServerPipe(const std::wstring& name, uint32_t timeout)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	while (true)
	{
		hPipe = CreateFileW(
			reinterpret_cast<LPCWSTR>(name.c_str()),
			GENERIC_READ |
			GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (hPipe != INVALID_HANDLE_VALUE)
		{
			break;
		}
		DWORD error = GetLastError();
		if (error != ERROR_PIPE_BUSY)
		{
			return INVALID_HANDLE_VALUE;
		}
		if (!WaitNamedPipe(reinterpret_cast<LPCWSTR>(name.c_str()), timeout))
		{
			return INVALID_HANDLE_VALUE;
		}
	}
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL fSuccess = SetNamedPipeHandleState(
		hPipe,
		&dwMode,
		NULL,
		NULL);
	if (!fSuccess)
	{
		return INVALID_HANDLE_VALUE;
	}
	return hPipe;
}

void InitializeConnection(HWND hWnd) {
	std::thread clientThread([hWnd]() {
		DWORD sessionId;
		ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
		std::wstring path = std::format(L"\\\\.\\pipe\\antivirus_{}", sessionId);
		pipe = ConnectToServerPipe(path, 0);
		if (pipe == INVALID_HANDLE_VALUE) {
			MessageBox(nullptr, L"Failed to connect to the pipe.", L"Error", MB_OK | MB_ICONERROR);
			return;
		}
	});
	clientThread.detach();
}

bool Read(HANDLE handle, uint8_t* data, uint64_t length, DWORD& bytesRead)
{
	bytesRead = 0;
	BOOL fSuccess = ReadFile(
		handle,
		data,
		length,
		&bytesRead,
		NULL);
	if (!fSuccess || bytesRead == 0)
	{
		return false;
	}
	return true;
}

bool Write(HANDLE handle, uint8_t* data, uint64_t length)
{
	DWORD cbWritten = 0;
	BOOL fSuccess = WriteFile(
		handle,
		data,
		length,
		&cbWritten,
		NULL);
	if (!fSuccess || length != cbWritten)
	{
		return false;
	}
	return true;
}