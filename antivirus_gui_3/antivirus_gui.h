#pragma once

#include <Windows.h>
#include "resource.h"
#include <shlobj.h>
#include <format>
#include <thread>

#define BUFSIZE 256

struct GUI {
	HINSTANCE hInst;
	HWND hScanFileMenu;
	HWND hScanFolderMenu;
	HWND hScanFileBtn;
	HWND hScanFolderBtn;
	HWND hScanFileLabel;
	HWND hScanFolderLabel;
	HWND hChooseFileToWriteBtn;
	HWND hChooseFileToWriteLabel;
	HWND hChooseFileToScan;
	HWND hChooseFolderToScan;
	HWND hCancelButton; 
	HWND hSelectedFolderOrFile;
	HWND hSelectedFileToWrite;
};

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

HINSTANCE hInst;
HWND hMainWindow;
GUI gui;
OPENFILENAMEW ofn;
TCHAR g_szFolderPath[MAX_PATH];
HANDLE pipe;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
WNDCLASS NewWindowClass(HBRUSH, HCURSOR, HINSTANCE, HICON, LPCWSTR, WNDPROC);
void InitWindow(HWND);
void PrepareFileScanMenu(HWND hWnd);
void PrepareFolderScanMenu(HWND hWnd);
void GetMainMenu();
void SetOpenFileParams(HWND, LPCTSTR, wchar_t*);
BOOL SelectFolderDialog(HWND);
BOOL AddNotificationIcon(HWND);
void SetMainMenuWindowPos(HWND hWnd);
void ShowContextMenu(HWND, POINT);
void InitializeConnection(HWND);