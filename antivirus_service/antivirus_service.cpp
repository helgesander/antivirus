//
// Created by helgesander on 17.04.2024.
//

#include "antivirus_service.h"

import Channel;
import Logger;


void WriteLog(const std::string& msg) {
    std::ofstream logfile("logfile.log");
    logfile << msg << std::endl;
    logfile.flush();
}

int _tmain(int argc, TCHAR* argv[])
{

    GlobalLogger.write("Main: Entry", INFO);
    WriteLog("Main: Entry");

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
            {const_cast<LPWSTR>(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            {NULL, NULL}
    };
    try {
        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
            throw ServiceException(std::format("Main: StartServiceCtrlDispatcher returned error: {}", GetLastError()));
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
        return GetLastError();
    }

    GlobalLogger.write("Main: Exit", INFO);
    return OK;
}

bool Write(HANDLE handle, uint8_t* data, uint64_t length) {
    DWORD cbWritten = 0;
    BOOL fSuccess = WriteFile(
        handle,
        data,
        length,
        &cbWritten,
        NULL
    );
    if (!fSuccess || length != cbWritten)
        return false;
    return true;
}

bool Read(HANDLE handle, uint8_t* data, uint64_t length, DWORD& bytesRead) {
    bytesRead = 0;
    BOOL fSuccess = ReadFile(
        handle,
        data,
        length,
        &bytesRead,
        NULL
    );
    if (!fSuccess || bytesRead == 0)
        return false;
    return true;
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


SECURITY_ATTRIBUTES GetSecurityAttributes(const std::wstring& sddl) {
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;

    PSECURITY_DESCRIPTOR psd = nullptr;

    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &psd, nullptr))
        securityAttributes.lpSecurityDescriptor = psd;
    else
        GlobalLogger.write("SDDL parse error", ERR);
    return securityAttributes;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    GlobalLogger.write("ServiceMain: Entry", INFO);
    try {
        g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, reinterpret_cast<LPHANDLER_FUNCTION>(ServiceCtrlHandler));
        if (g_StatusHandle == NULL)
            throw ServiceException("ServiceMain: RegisterServiceCtrlHandler returned error");
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
        GlobalLogger.write("ServiceMain: Exit", INFO);
        return; // TODO: add return code
    }
    
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN || SERVICE_ACCEPT_STOP || SERVICE_ACCEPT_SESSIONCHANGE;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;

    try {
        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            throw ServiceException("ServiceMain: SetServiceStatus returned error");
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
    }

    SECURITY_ATTRIBUTES jsa = GetSecurityAttributes(L"O:SYG:SYG");

    PWTS_SESSION_INFO wtsSessions = NULL;
    DWORD sessionCount = 0;
    if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &wtsSessions, &sessionCount)) {
        GlobalLogger.write(&"ServiceMain: Troubles with WTSEnumerateSessions"[GetLastError()], ERR);
    }
    else
    {
        for (DWORD i = 0; i < sessionCount; ++i)
        {
            StartProcessInSession(wtsSessions[i].SessionId);
        }
    }
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent)
{
    GlobalLogger.write("ServiceCtrlHandler: Entry", ERR);

    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:

        GlobalLogger.write("ServiceCtrlHandler: SERVICE_CONTROL_STOP Request", ERR);
        g_ServiceStatus.dwCurrentState == SERVICE_STOPPED;
        break;

        /*
         * Perform tasks necessary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            GlobalLogger.write("ServiceCtrlHandler: SetServiceStatus returned error", ERR);

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
    {
        WTSSESSION_NOTIFICATION* sessionNotification = reinterpret_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
        if (dwEventType == WTS_SESSION_LOGON || dwEventType == WTS_SESSION_CREATE || dwEventType == WTS_REMOTE_CONNECT) {
            StartProcessInSession(sessionNotification->dwSessionId);
        }
        break;
    }

    default:
        break;
    }

    GlobalLogger.write("ServiceCtrlHandler: Exit", INFO);
}

void StartProcessInSession(DWORD sessionId) {
    std::thread clientThread([sessionId]() {
        GlobalLogger.write(std::format("StartUIProcessInSession sessionId = {}", sessionId), INFO);
        HANDLE hUserToken = NULL;
        if (WTSQueryUserToken(sessionId, &hUserToken)) {
            WCHAR commandLine[] = GUI_PATH_L;
            std::wstring processSddl = std::format(L"O:SYG:SYD:(D;OICI;0x{:08X};;;WD)(A;OICI;0x{:08X};;;WD)",
                PROCESS_TERMINATE, PROCESS_ALL_ACCESS);
            std::wstring threadSddl = std::format(L"O:SYG:SYD:(D;OICI;0x{:08X};;;WD)(A;OICI;0x{:08X};;;WD)",
                THREAD_TERMINATE, THREAD_ALL_ACCESS);

            PROCESS_INFORMATION pi{};
            STARTUPINFO si{};

            SECURITY_ATTRIBUTES psa = GetSecurityAttributes(processSddl);
            SECURITY_ATTRIBUTES tsa = GetSecurityAttributes(threadSddl);
            GlobalLogger.write(std::format("Create pipe for sessionId = {}", sessionId), INFO);
            if (psa.lpSecurityDescriptor != nullptr && 
                tsa.lpSecurityDescriptor != nullptr) {
                Channel ch;
                ch.Create(hUserToken);
                GlobalLogger.write(std::format("Start UI process for sessionId = {}", sessionId), INFO);
                if (CreateProcessAsUserW(
                    hUserToken,
                    NULL,
                    commandLine,
                    &psa,
                    &tsa,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si, &pi))
                {
                    GlobalLogger.write(std::format("Process created for sessionId = {}", sessionId), INFO);
                    ULONG clientProcessId;
                    BOOL clientIdentified;
                    do {
                        BOOL fConnected = ConnectNamedPipe(ch.GetHandlePipe(), NULL) ?
                            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
                        clientIdentified = GetNamedPipeClientProcessId(ch.GetHandlePipe(), &clientProcessId);
                        if (clientIdentified) {
                            if (clientProcessId == pi.dwProcessId) break;
                            else DisconnectNamedPipe(ch.GetHandlePipe());
                        }
                    } while (true);

                    uint8_t buf[512];
                    DWORD bytesRead = 0;
                    while (ch.Read(buf, 512, bytesRead)) {
                        ImpersonateNamedPipeClient(ch.GetHandlePipe());
                        // TODO: чето делаем
                        RevertToSelf();
                        ch.Write(buf, bytesRead);
                    }
                    // 
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
                else GlobalLogger.write(std::format("Can\'t parse security descriptor for sessionId = {}: {}", sessionId, GetLastError()), ERR);

                auto sd = tsa.lpSecurityDescriptor;
                tsa.lpSecurityDescriptor = nullptr;
                LocalFree(sd);

                sd = psa.lpSecurityDescriptor;
                psa.lpSecurityDescriptor = nullptr;
                LocalFree(sd);
                }

        }
        CloseHandle(hUserToken);
    });
    clientThread.detach();
}
