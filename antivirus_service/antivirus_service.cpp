#include <Windows.h>
#include <iostream>
#include <WTSApi32.h>
#include <codecvt>

using namespace std;

#define SERVICE_NAME TEXT("NotepadOpener")
SERVICE_STATUS				ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE		hServiceStatusHandler = NULL;
HANDLE						hServiceEvent = NULL;

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpArgv);
DWORD WINAPI ServiceControlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID dwContent);
void ServiceReportStatus(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHunt);
void ServiceInit(DWORD dwArgc, LPTSTR* lpArgv);
void ServiceInstall();
void ServiceDelete();
void ServiceStart();
void ServiceStop();
bool isServiceInstalled();

int main(int argc, char* argv[])
{
    BOOL bStServiceCtrlDispatcher = FALSE;
    if (lstrcmpiA(argv[1], "install") == 0)
    {
        ServiceInstall();
    }
    else if (lstrcmpiA(argv[1], "start") == 0)
    {
        //        if (!isServiceInstalled()) ServiceInstall();
        ServiceStart();
        cout << "ServiceStart Success" << endl;

    }
    else if (lstrcmpiA(argv[1], "stop") == 0)
    {
        ServiceStop();
        cout << "ServiceStop Success" << endl;
    }
    else if (lstrcmpiA(argv[1], "delete") == 0)
    {
        ServiceDelete();
        cout << "ServiceDelete" << endl;
    }

    else
    {
        SERVICE_TABLE_ENTRY DispatchTable[] =
        {
                {const_cast<LPSTR>("NotepadOpener"), reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain)},
                {NULL,NULL}
        };

        bStServiceCtrlDispatcher = StartServiceCtrlDispatcher(DispatchTable);
        cout << bStServiceCtrlDispatcher << endl;
        /*
         *  Будет всегда FALSE, если запускать этот бинарь не как службу, а как консольное приложение, надо юзать вместе со start
         * */
        if (FALSE == bStServiceCtrlDispatcher)
            cout << "StartServiceCtrlDispatcher Failed: " << GetLastError() << endl;
        else
            cout << "StartServiceCtrlDispatcher Success" << endl;

    }

    system("PAUSE");
    return 0;
}


// Выводит кракозябры в консоль
wstring GetLastErrorAsString() {
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0) {
        return L"No error message has been recorded";
    }
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&messageBuffer), 0, NULL);

    std::wstring message(messageBuffer, size);

    LocalFree(messageBuffer);

    return message;
}

bool isServiceInstalled() {
    int ret = true;
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE); // Используем именно эту маску с точки зрения безопаности
    if (scm == NULL) {
        cerr << "Failed to open service manager to checking installed service" << endl;
        ret = false;
    }
    else {
        SC_HANDLE service = OpenService(scm, SERVICE_NAME, SERVICE_QUERY_STATUS);
        if (service == NULL)
        {
            cout << "Service is not installed. " << endl; // TODO: Сделать запись в лог
            CloseServiceHandle(scm);
            ret = false;
        }
        else std::cout << "Service is installed. ";
    }
    return ret;
}

void StartProcessInSession(DWORD sessionId) {
    HANDLE hUserToken = NULL;

    if (!WTSQueryUserToken(sessionId, &hUserToken)) {
        std::cerr << "Failed to get user token for session " << sessionId << std::endl;
        return;
    }
    LPCWSTR lpApplicationName = L"C:\\Users\\helge\\source\\repos\\my_antivirus\\antivirus_gui\\.exe";

    if (!CreateProcessAsUser(hUserToken, reinterpret_cast<LPCSTR>(lpApplicationName), NULL, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, NULL, NULL)) {
        std::cerr << "Failed to create process in session " << sessionId << std::endl;
    }

    CloseHandle(hUserToken);
}


void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpArgv)
{
    cout << "ServiceMain Start" << endl;
    BOOL bServiceStatus = FALSE;

    hServiceStatusHandler = RegisterServiceCtrlHandlerEx(
        SERVICE_NAME, (LPHANDLER_FUNCTION_EX)ServiceControlHandler,
        NULL);

    if (NULL == hServiceStatusHandler)
    {
        wcout << "RegisterServiceCtrlHandlerEx Failed: " << GetLastError() << endl;
    }
    else
    {
        cout << "RegisterServiceCtrlHandlerEx Success" << endl;
    }

    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN || SERVICE_ACCEPT_STOP || SERVICE_ACCEPT_SESSIONCHANGE;
    ServiceStatus.dwServiceSpecificExitCode = 0;

    ServiceReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    bServiceStatus = SetServiceStatus(hServiceStatusHandler, &ServiceStatus);
    if (FALSE == bServiceStatus)
    {
        cout << "Service Status initial Setup FAILED = " << GetLastError() << endl;
    }
    else
    {
        cout << "Service Status initial Setup SUCCESS" << endl;
    }

    //being told
    PWTS_SESSION_INFO wtsSessions;
    DWORD sessionCount;
    if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &wtsSessions, &sessionCount))
    {
        return;
    }

    for (auto i = 1; i < sessionCount; ++i)
    {
        if (wtsSessions[i].State == WTSActive) {
            StartProcessInSession(wtsSessions[i].SessionId);
        }
    }
    ServiceInit(dwArgc, lpArgv);
    cout << "ServiceMain End" << endl;
}



DWORD WINAPI ServiceControlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID dwContent)
{
    DWORD errorCode = NO_ERROR;
    switch (dwControl)
    {
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_STOP:
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
    {
        WTSSESSION_NOTIFICATION* sessionNotification = static_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
        if (dwEventType == WTS_SESSION_LOGON || dwEventType == WTS_SESSION_CREATE || dwEventType == WTS_REMOTE_CONNECT) {
            StartProcessInSession(sessionNotification->dwSessionId);
        }
    }
    default:
        errorCode = ERROR_CALL_NOT_IMPLEMENTED;
        break;
    }
    ServiceReportStatus(ServiceStatus.dwCurrentState, errorCode, 0);
    return errorCode;
}


void ServiceInit(DWORD dwArgc, LPTSTR* lpArgv) {
    cout << "ServiceInit Start" << endl;

    hServiceEvent = CreateEvent(
        NULL, //security attribute
        TRUE, //manual reset event
        FALSE,//Non Signaled
        NULL);//name of event

    if (NULL == hServiceEvent) {
        ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
    }
    else {
        ServiceReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
    }


    //    while (1) {
    //        WaitForSingleObject(hServiceEvent, INFINITE);
    //        ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
    //    }

    cout << "ServiceInit End" << endl;
}



void ServiceReportStatus(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHunt)
{
    cout << "ServiceReportStatus Start" << endl;

    static DWORD dwCheckPoint = 1;
    BOOL bSetServiceStatus = FALSE;

    ServiceStatus.dwCurrentState = dwCurrentState;
    ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    ServiceStatus.dwWaitHint = dwWaitHunt;

    if (dwCurrentState == SERVICE_START_PENDING)
    {
        ServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN || SERVICE_ACCEPT_STOP || SERVICE_ACCEPT_SESSIONCHANGE;
    }

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
    {
        ServiceStatus.dwCheckPoint = 0;
    }
    else
    {
        ServiceStatus.dwCheckPoint = dwCheckPoint++;
    }

    bSetServiceStatus = SetServiceStatus(hServiceStatusHandler, &ServiceStatus);

    if (FALSE == bSetServiceStatus)
    {
        wcout << "Service Status Failed = " << GetLastErrorAsString() << endl;
    }
    else
    {
        cout << "Service Status SUCCESS" << endl;
    }

    cout << "ServiceReportStatus END" << endl;
} //..........ServiceReportStatus.............................//



void ServiceInstall(void)
{
    cout << "ServiceInstall Start" << endl;

    SC_HANDLE		hScOpenSCManager = NULL;
    SC_HANDLE		hScCreateService = NULL;
    DWORD			dwGetModuleFileName = 0;
    TCHAR			szPath[MAX_PATH];

    dwGetModuleFileName = GetModuleFileName(NULL, szPath, MAX_PATH);
    if (0 == dwGetModuleFileName)
    {
        cout << "Service Installation Failed" << GetLastError() << endl;
    }
    else
    {
        cout << "Successfully install the file\n" << endl;
    }

    hScOpenSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hScOpenSCManager)
    {
        cout << "OpenSCManager Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "OpenSCManager Success" << endl;
    }

    hScCreateService = CreateService(
        hScOpenSCManager,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        szPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

    if (NULL == hScCreateService)
    {
        cout << "CreateSeervice Failed = " << GetLastError() << endl;
        CloseServiceHandle(hScOpenSCManager);
    }
    else
    {
        cout << "CreateService Success" << endl;
    }

    CloseServiceHandle(hScCreateService);
    CloseServiceHandle(hScOpenSCManager);

    cout << "ServiceInstall End" << endl;
}


void ServiceDelete()
{
    cout << "ServiceDelete Start" << endl;

    SC_HANDLE		hScOpenSCManager = NULL;
    SC_HANDLE		hScOpenService = NULL;
    BOOL			bDeleteService = FALSE;

    hScOpenSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hScOpenSCManager)
    {
        cout << "OpenSCManager Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "OpenSCManager Success" << endl;
    }

    hScOpenService = OpenService(
        hScOpenSCManager,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS);

    if (NULL == hScOpenService)
    {
        cout << "OpenService Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "OpenService Success " << endl;
    }

    bDeleteService = DeleteService(hScOpenService);

    if (FALSE == bDeleteService)
    {
        cout << "Delete Service Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "Delete Service Success" << endl;
    }

    CloseServiceHandle(hScOpenService);
    CloseServiceHandle(hScOpenSCManager);
    cout << "ServiceDelete End" << endl;

}


void ServiceStart()
{
    cout << "Inside ServiceStart function" << endl;

    BOOL			bStartService = FALSE;
    SERVICE_STATUS_PROCESS		SvcStatusProcess;
    SC_HANDLE		hOpenSCManager = NULL;
    SC_HANDLE		hOpenService = NULL;
    BOOL			bQueryServiceStatus = FALSE;
    DWORD			dwBytesNeeded;

    hOpenSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hOpenSCManager)
    {
        cout << "hOpenSCManager Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "hOpenSCManager Success" << endl;
    }

    hOpenService = OpenService(
        hOpenSCManager,
        SERVICE_NAME,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hOpenService)
    {
        cout << "OpenServcie Failed = " << GetLastError() << endl;
        CloseServiceHandle(hOpenSCManager);
    }
    else
    {
        cout << "OpenServcie SUCCESS" << endl;
    }

    bQueryServiceStatus = QueryServiceStatusEx(
        hOpenService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&SvcStatusProcess,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded);

    if (FALSE == bQueryServiceStatus)
    {
        cout << "QueryService Failed = " << GetLastError() << endl;
        CloseServiceHandle(hOpenService);
        CloseServiceHandle(hOpenSCManager);
    }
    else
    {
        cout << "QueryService Success" << endl;
    }

    if ((SvcStatusProcess.dwCurrentState != SERVICE_STOPPED) &&
        (SvcStatusProcess.dwCurrentState != SERVICE_STOP_PENDING))
    {
        cout << "service is already running" << endl;
    }
    else
    {
        cout << "Service is already stopped" << endl;
    }

    while (SvcStatusProcess.dwCurrentState == SERVICE_STOP_PENDING)
    {
        bQueryServiceStatus = QueryServiceStatusEx(
            hOpenService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&SvcStatusProcess,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded);

        if (FALSE == bQueryServiceStatus)
        {
            cout << "Query Service Failed = " << GetLastError() << endl;
            CloseServiceHandle(hOpenService);
            CloseServiceHandle(hOpenSCManager);
        }
        else
        {
            cout << "query Service Success" << endl;
        }
    }

    bStartService = StartService(
        hOpenService,
        NULL,
        NULL);

    if (FALSE == bStartService)
    {
        cout << "StartService Failed = " << GetLastError() << endl;
        CloseServiceHandle(hOpenService);
        CloseServiceHandle(hOpenSCManager);
    }
    else
    {
        cout << "StartService Success" << endl;
    }

    bQueryServiceStatus = QueryServiceStatusEx(
        hOpenService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&SvcStatusProcess,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded);

    if (FALSE == bQueryServiceStatus)
    {
        cout << "QueryService Failed = " << GetLastError() << endl;
        CloseServiceHandle(hOpenService);
        CloseServiceHandle(hOpenSCManager);
    }
    else
    {
        cout << "QueryService Success" << endl;
    }

    if (SvcStatusProcess.dwCurrentState == SERVICE_RUNNING)
    {
        cout << "Service Started Running..." << endl;
    }
    else
    {
        cout << "Service Running Failed  = " << GetLastError() << endl;
        CloseServiceHandle(hOpenService);
        CloseServiceHandle(hOpenSCManager);
    }

    CloseServiceHandle(hOpenService);
    CloseServiceHandle(hOpenSCManager);
    cout << "ServiceStart end" << endl;
}

void ServiceStop()
{
    cout << "Inside Service Stop" << endl;

    SERVICE_STATUS_PROCESS		SvcStatusProcess;
    SC_HANDLE					hScOpenSCManager = NULL;
    SC_HANDLE					hScOpenService = NULL;
    BOOL						bQueryServiceStatus = TRUE;
    BOOL						bControlService = TRUE;
    DWORD						dwBytesNeeded;

    //шаг 1
    hScOpenSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hScOpenSCManager)
    {
        cout << "OpenSCManager Failed = " << GetLastError() << endl;
    }
    else
    {
        cout << "OpenSCManager Success" << endl;
    }

    //шаг 2
    hScOpenService = OpenService(
        hScOpenSCManager,
        SERVICE_NAME,
        SC_MANAGER_ALL_ACCESS);

    if (NULL == hScOpenService)
    {
        cout << "OpenService Failed = " << GetLastError() << endl;
        CloseServiceHandle(hScOpenSCManager);
    }
    else
    {
        cout << "OpenService Success" << endl;
    }


    bQueryServiceStatus = QueryServiceStatusEx(
        hScOpenService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&SvcStatusProcess,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded);

    if (FALSE == bQueryServiceStatus)
    {
        cout << "QueryService Failed = " << GetLastError() << endl;
        CloseServiceHandle(hScOpenService);
        CloseServiceHandle(hScOpenSCManager);
    }
    else
    {
        cout << "QueryService Success" << endl;
    }

    bControlService = ControlService(
        hScOpenService,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS)&SvcStatusProcess);

    if (TRUE == bControlService)
    {
        cout << "Control Service Success" << endl;
    }
    else
    {
        cout << "Control Service Failed = " << GetLastError() << endl;
        CloseServiceHandle(hScOpenService);
    }

    while (SvcStatusProcess.dwCurrentState != SERVICE_STOPPED)
    {
        bQueryServiceStatus = QueryServiceStatusEx(
            hScOpenService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&SvcStatusProcess,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded);

        if (TRUE == bQueryServiceStatus)
        {
            cout << "Queryservice FAiled = " << GetLastError() << endl;
            CloseServiceHandle(hScOpenService);
            CloseServiceHandle(hScOpenSCManager);
        }
        else
        {
            cout << "Query Service Success" << endl;
        }

        if (SvcStatusProcess.dwCurrentState == SERVICE_STOPPED)
        {
            cout << "Service stopped successfully" << endl;
            break;
        }
        else
        {
            cout << "Service stopped Faield = " << GetLastError() << endl;
            CloseServiceHandle(hScOpenService);
            CloseServiceHandle(hScOpenSCManager);
        }
    }

    CloseServiceHandle(hScOpenService);
    CloseServiceHandle(hScOpenSCManager);
    cout << "Service Stop" << endl;

}
