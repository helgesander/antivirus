//
// Created by helgesander on 17.04.2024.
//

#include "antivirus_service.h"

import Channel;

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
            throw ServiceException(std::format("Main: StartServiceCtrlDispatcher returned error: {}", GetLastError()));
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
        return GetLastError();
    }

    GlobalLogger.write("Main: Exit", INFO);
    return OK;
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
        // TODO: fix RegisterServiceCtrlHandler(Ex) (fixed)
        g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, reinterpret_cast<LPHANDLER_FUNCTION_EX>(ServiceCtrlHandler), NULL);
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


DWORD WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent)
{
    GlobalLogger.write("ServiceCtrlHandler: Entry", ERR);
    DWORD result = ERROR_CALL_NOT_IMPLEMENTED;
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:

        GlobalLogger.write("ServiceCtrlHandler: SERVICE_CONTROL_STOP Request", ERR);
        g_ServiceStatus.dwCurrentState == SERVICE_STOPPED;
        result = NO_ERROR;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        result = NO_ERROR;
        break;
    case SERVICE_CONTROL_INTERROGATE:
        result = NO_ERROR;
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
        if (dwEventType == WTS_SESSION_LOGON) {
            WTSSESSION_NOTIFICATION* sessionNotification = reinterpret_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
            StartProcessInSession(sessionNotification->dwSessionId);
        }
        break;
    }

    GlobalLogger.write("ServiceCtrlHandler: Exit", INFO);
    return result;
}

void PrintSignaturesToFile(std::ofstream& file, const SignaturesData& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        file << std::format("\t{}: {}", i, data.at(i).name) << std::endl;
    }
}

void TerminateAllUiProcesses() { // kolhoz edition
    for (auto p : processSessions)
        TerminateProcess(p.hProcess, 0); 
}

template <typename Container>
void WriteResultsToFile(std::string filename, const Container& cont) {
    std::ofstream resultFile(filename);
    try {
        if (!resultFile.is_open())
            throw ServiceException("Cannot open file to write result of scan");
    }
    catch (ServiceException const& e) {
        GlobalLogger.write(e.what(), ERR);
    }
    // TODO: maybe fix later (one signature in file)
    if constexpr (std::is_same_v<Container, std::vector<typename Container::value_type>>) {
        resultFile << "Найденные сигнатуры:" << std::endl;
        PrintSignaturesToFile(resultFile, cont);
    }
    else if constexpr (std::is_same_v<Container, std::map<typename Container::key_type, typename Container::mapped_type>>) {
        resultFile << "Найденные сигнатуры по файлам:" << std::endl;
        for (const auto& file : cont) {
            resultFile << std::format("{}:", file.first) << std::endl;
            PrintSignaturesToFile(resultFile, file.second);
        }
    }
    resultFile.close();
}

void StartProcessInSession(DWORD sessionId) {
    std::thread clientThread([sessionId]() {
        GlobalLogger.write(std::format("StartUIProcessInSession sessionId = {}", sessionId), INFO);
        HANDLE hUserToken = NULL;
        if (WTSQueryUserToken(sessionId, &hUserToken)) {
            WCHAR commandLine[] = L"antivirus_gui_3.exe";
            WCHAR sdCommandLine[] = L"antivirus_gui_3.exe --secure-desktop";
            std::wstring processSddl = std::format(L"O:SYG:SYD:(D;OICI;0x{:08X};;;WD)(A;OICI;0x{:08X};;;WD)",
                PROCESS_TERMINATE, PROCESS_ALL_ACCESS);
            std::wstring threadSddl = std::format(L"O:SYG:SYD:(D;OICI;0x{:08X};;;WD)(A;OICI;0x{:08X};;;WD)",
                THREAD_TERMINATE, THREAD_ALL_ACCESS);

            STARTUPINFO si{};
            PROCESS_INFORMATION pi{};
            SECURITY_ATTRIBUTES psa = GetSecurityAttributes(processSddl);
            SECURITY_ATTRIBUTES tsa = GetSecurityAttributes(threadSddl);
            GlobalLogger.write(std::format("Create pipe for sessionId = {}", sessionId), INFO);
            if (psa.lpSecurityDescriptor != nullptr &&
                tsa.lpSecurityDescriptor != nullptr) {
                Channel ch;
                ch.Create(hUserToken, sessionId);
                GlobalLogger.write(std::format("Start UI process for sessionId = {}", sessionId), INFO);
                if (CreateProcessAsUserW(
                    hUserToken, NULL, commandLine, &psa, &tsa, FALSE,
                    0, NULL, NULL, &si, &pi))
                {
                    processSessions.push_back(pi);
                    GlobalLogger.write(std::format("Process created for sessionId = {}", sessionId), INFO);
                    ch.InitializeConnection(pi);
                    GlobalLogger.write(std::format("Connection from client with sessionId = {}", sessionId), INFO);

                    Scanner sc(BINARY_BASE);
                    wchar_t fileOrFolderName[MAX_PATH];
                    wchar_t fileToWrite[MAX_PATH];
                    int* signal; // TODO: maybe change to 4 
                    DWORD bytesRead = 0;
                    while (ch.Read(reinterpret_cast<uint8_t*>(signal), sizeof(int), bytesRead)) {
                        ImpersonateNamedPipeClient(ch.GetPipe());
                        switch (*signal) {
                        case EXIT: 
                        {
                            TerminateAllUiProcesses(); // pipec 
                            break;
                        }
                        case SCAN_FILE:
                        {
                            ch.Read(reinterpret_cast<uint8_t*>(fileOrFolderName), MAX_PATH, bytesRead);
                            ch.Read(reinterpret_cast<uint8_t*>(fileToWrite), MAX_PATH, bytesRead);
                            std::string filename = reinterpret_cast<char*>(fileOrFolderName);
                            SignaturesData res;
                            if (sc.scanFile(filename, res))
                            {
                                filename = reinterpret_cast<const char*>(fileToWrite);
                                WriteResultsToFile(filename, res);
                                ch.Write(reinterpret_cast<uint8_t*>(SCAN_OK), sizeof(int));
                            }
                            else 
                                ch.Write(reinterpret_cast<uint8_t*>(FIND_NOTHING), sizeof(int));
                            break;
                        }
                        case SCAN_FOLDER:
                        {
                            ch.Read(reinterpret_cast<uint8_t*>(fileOrFolderName), MAX_PATH, bytesRead);
                            ch.Read(reinterpret_cast<uint8_t*>(fileToWrite), MAX_PATH, bytesRead);
                            std::string foldername = reinterpret_cast<char*>(fileOrFolderName);
                            ScanFolderData res;
                            if (sc.scanFolder(foldername, res)) {
                                foldername = reinterpret_cast<const char*>(fileToWrite);
                                WriteResultsToFile(foldername, res);
                                ch.Write(reinterpret_cast<uint8_t*>(SCAN_OK), sizeof(int));
                            }
                            else ch.Write(reinterpret_cast<uint8_t*>(FIND_NOTHING), sizeof(int));
                            break;
                        }
                        }
                        RevertToSelf();
                    }
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
