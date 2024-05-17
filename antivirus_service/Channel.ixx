//
// Created by helgesander on 17.04.2024.
//

module ;

#include <string>
#include <locale>
#include <codecvt>
#include "antivirus_service.h"

import Logger;

export module Channel;

export enum MESSAGES {
	EXIT,
	SCAN_FILE,
	SCAN_FOLDER,
	SCAN_OK,
	FIND_NOTHING
};


export class Channel {
public:
	~Channel();
	void Create(HANDLE, DWORD);
	bool Read(uint8_t*, uint64_t, DWORD&);
	bool Write(uint8_t*, uint64_t);
	void InitializeConnection(PROCESS_INFORMATION); 
	HANDLE GetPipe() { return hPipe; }
private:
	HANDLE hPipe;
	std::wstring pipeName;
	std::wstring pipeSddl;
};


void Channel::Create(HANDLE userToken, DWORD sessionId) {
	pipeSddl = std::format(L"O:SYG:SYD:(A;OICI;GA;;;{})",
		GetUserSid(userToken));
	SECURITY_ATTRIBUTES npsa = GetSecurityAttributes(pipeSddl);
	pipeName = std::format(L"\\\\.\\pipe\\antivirus_{}", sessionId); // TODO: change in client (fixed)
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

	std::string str = converter.to_bytes(pipeName);
	GlobalLogger.write(std::format("Create new pipe with name {}", str), INFO);
	hPipe = CreateNamedPipeW(
		pipeName.c_str(),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | 
		PIPE_WAIT,
		1,
		512,
		512,
		0,
		&npsa);
	try {
		if (hPipe == INVALID_HANDLE_VALUE) {
			throw ServiceException("Failed to create named pipe. Error code: " + GetLastError());
		}
	}
	catch (const ServiceException& e) {
		GlobalLogger.write(e.what(), ERR);
	}
}

Channel::~Channel(){
	if (!FlushFileBuffers(hPipe)) {
		GlobalLogger.write("FlushFileBuffers failed with error " + GetLastError(), ERR);
	}

	if (!DisconnectNamedPipe(hPipe)) {
		GlobalLogger.write("DisconnectNamedPipe failed with error " + GetLastError(), ERR);
	}
	CloseHandle(hPipe);
	hPipe = INVALID_HANDLE_VALUE;
}

bool Channel::Read(uint8_t* data, uint64_t length, DWORD& bytesRead) {
	bytesRead = 0;
	BOOL fSuccess = ReadFile(
		hPipe,
		data,
		length,
		&bytesRead,
		NULL
	);
	if (!fSuccess || bytesRead == 0) {
		return false;
	}
	return true;
}

bool Channel::Write(uint8_t* data, uint64_t length) {
	DWORD cbWritten = 0;
	BOOL fSuccess = WriteFile(
		hPipe,
		data,
		length,
		&cbWritten,
		NULL
	);
	if (!fSuccess || length != cbWritten) {
		return false;
	}
	return true;
}

void Channel::InitializeConnection(PROCESS_INFORMATION pi) {
	ULONG clientProcessId;
	BOOL clientIdentified;
	do {
		BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		clientIdentified = GetNamedPipeClientProcessId(hPipe, &clientProcessId);
		if (clientIdentified) {
			if (clientProcessId == pi.dwProcessId) break;
			else DisconnectNamedPipe(hPipe);
		}
	} while (true);
}
