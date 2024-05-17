//
// Created by helgesander on 17.04.2024.
//

#pragma once
#ifndef ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H
#define ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <tchar.h>
#include "exceptions.h"
#include <Windows.h>
#include <WTSApi32.h>
#include <sddl.h>
#include <thread>
#include <format>
#include <vector>
#include <type_traits>
#include <locale>
#include <codecvt>

import Scanner;
import Logger;

static SERVICE_STATUS        g_ServiceStatus	= { 0 };
static SERVICE_STATUS_HANDLE g_StatusHandle		= NULL;
static HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
static std::vector<PROCESS_INFORMATION> processSessions;
static Logger GlobalLogger("C:\\Users\\helgesander\\antivirus.log");
static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent);
void StartProcessInSession(DWORD sessionId);
SECURITY_ATTRIBUTES GetSecurityAttributes(const std::wstring& sddl);
std::wstring GetUserSid(HANDLE userToken);
void PrintSignaturesToFile(std::ofstream&, ScanFileResult const& name);
void TerminateAllUiProcesses();

#define SERVICE_NAME  _T("antivirus")
//#define GUI_PATH_L L"antivirus_gui_3.exe"
#define BINARY_BASE "C:\\Users\\helgesander\\source\\repos\\antivirus\\antivirus_service\\base.bin"


#endif //ANTIVIRUS_SERVICE_V3_ANTIVIRUS_SERVICE_H
