//
// Created by helgesander on 17.04.2024.
//

#ifndef ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H
#define ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <tchar.h>
#include "exceptions.h"
#include <Windows.h>


SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
void StartProcessInSession(DWORD sessionId);

#define SERVICE_NAME  _T("antivirus")
#define LOGFILE "C:\\Users\\helgesander\\source\\repos\\antivirus\\antivirus.log"
#define GUI_PATH "C:\\Users\\helgesander\\source\\repos\\antivirus\\antivirus_gui\\build\\Debug\\antivirus_gui.exe"
#define GUI_PATH_L L"C:\\Users\\helgesander\\source\\repos\\antivirus\\antivirus_gui\\build\\Debug\\antivirus_gui.exe"


#endif //ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H
