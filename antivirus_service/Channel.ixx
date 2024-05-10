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
	SCAN_FOLDER
};

export class Channel {
public:
	Channel();
	~Channel() noexcept;
	void Create(HANDLE);
	bool Read(uint8_t*, uint64_t, DWORD&);
	bool Write(uint8_t*, uint64_t);
	HANDLE GetHandlePipe();
private:
	HANDLE hPipe;
	std::wstring pipeName;
	std::wstring pipeSddl;
};


HANDLE Channel::GetHandlePipe() {
	return hPipe;
}

void Channel::Create(HANDLE userToken, DWORD wtsSession) {
	pipeSddl = std::format(L"O:SYG:SYD:(A;OICI;GA;;;{})",
		wtsSession);
	SECURITY_ATTRIBUTES npsa = GetSecurityAttributes(pipeSddl);
	pipeName = std::format(L"\\\\.\\pipe\\{}", wtsToken);
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

Channel::Channel() {};

Channel::~Channel() noexcept {
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
	if (!fSuccess || bytesRead == 0)
		return false;
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
	if (!fSuccess || length != cbWritten)
		return false;
	return true;
}

