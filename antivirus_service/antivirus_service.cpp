//
// Created by helgesander on 17.04.2024.
//

#include "antivirus_service.h"
#include <WTSApi32.h>

import Channel;
import Logger;


int _tmain(int argc, TCHAR* argv[])
{
    GlobalLogger.write("Main: Entry", INFO);

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
            {const_cast<LPWSTR>(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            {NULL, NULL}
    };
    try {
        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
            throw ServiceException("Main: StartServiceCtrlDispatcher returned error");
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
        return GetLastError();
    }

    GlobalLogger.write("Main: Exit", INFO);
    return OK;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    GlobalLogger.write("ServiceMain: Entry", INFO); // инициализируем логгер
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

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN || SERVICE_ACCEPT_STOP || SERVICE_ACCEPT_SESSIONCHANGE;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    try {
        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            throw ServiceException("ServiceMain: SetServiceStatus returned error");
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
    }

    PWTS_SESSION_INFO wtsSessions = NULL;
    DWORD sessionCount = 0;
    if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &wtsSessions, &sessionCount)) {
        GlobalLogger.write(&"ServiceMain: Troubles with WTSEnumerateSessions"[GetLastError()], ERR);
    }
    else
    {
        // ПОШЛИ ЖЕСТКИЕ КОСТЫЛИ
        GlobalLogger.write(&"ServiceMain: I think he can do it, sessionCount = "[sessionCount], INFO);

        for (DWORD i = 0; i < sessionCount; ++i)
        {
            HANDLE userToken;
            if (WTSQueryUserToken(wtsSessions[i].SessionId, &userToken))
            {
                PROCESS_INFORMATION pi = {};
                STARTUPINFO si = {};
                WCHAR path[] = GUI_PATH_L;
                if (!CreateProcessAsUser(userToken, NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                    GlobalLogger.write(&"ServiceMain: Troubles with CreateProcessAsUser"[GetLastError()], ERR);
                else
                    GlobalLogger.write("ServiceMain: Gui is running", INFO);
                CloseHandle(userToken);
            }
            else
                GlobalLogger.write(&"ServiceMain: Troubles with WTSQueryUserToken"[GetLastError()], ERR);
        }
        WTSFreeMemory(wtsSessions);
    }

    GlobalLogger.write("ServiceMain: Nadeus eto rabotaet...", INFO);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        try {
            GlobalLogger.write("CreateEvent returned error", ERR);

            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            g_ServiceStatus.dwWin32ExitCode = GetLastError();
            g_ServiceStatus.dwCheckPoint = 1;

            if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
                throw ServiceException("SetServiceStatus returned error");
        }
        catch (ServiceException const& e) {
            GlobalLogger.write("ServiceMain: Exit", INFO);
        }
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        GlobalLogger.write("ServiceMain: SetServiceStatus returned error", ERR);
    }

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    GlobalLogger.write("ServiceMain: Waiting for Worker Thread to complete", INFO);

    WaitForSingleObject(hThread, INFINITE);

    GlobalLogger.write("ServiceMain: Worker Thread Stop Event signaled", INFO);


    /*
     * Perform any cleanup tasks
     */
    GlobalLogger.write("ServiceMain: Performing Cleanup Operations", ERR);

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        GlobalLogger.write("ServiceMain: ServiceStatus returned error", ERR);
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent)
{
    GlobalLogger.write("ServiceCtrlHandler: Entry", ERR);

    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:

        GlobalLogger.write("ServiceCtrlHandler: SERVICE_CONTROL_STOP Request", ERR);
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
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


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    GlobalLogger.write("ServiceWorkerThread: Entry", INFO);

    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        // пока ничего...
    }

    GlobalLogger.write("ServiceWorkerThread: Exit", ERR);

    return ERROR_SUCCESS;
}

void StartProcessInSession(DWORD sessionId) {
    GlobalLogger.write("Entry to StartProcessInSession", INFO);
    HANDLE hUserToken = NULL;
    if (!WTSQueryUserToken(sessionId, &hUserToken)) {
        std::string logMessage = "Failed to get user token for session " + sessionId;
        GlobalLogger.write(logMessage, ERR);
        return;
    }
    LPCWSTR lpApplicationName = GUI_PATH_L;
    if (!CreateProcessAsUser(hUserToken, lpApplicationName, NULL, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, NULL, NULL)) {
        GlobalLogger.write("Failed to create process in session " + sessionId, ERR);
    }
    CloseHandle(hUserToken);
}
