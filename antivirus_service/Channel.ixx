//
// Created by helgesander on 17.04.2024.
//

module ;

#include <Windows.h>
#include <string>
#include "exceptions.h"

import Logger;

export module Channel;

export class Channel {
public:
	Channel(const std::wstring& name);
	~Channel() noexcept;
	Channel GetInstance();
private:
	HANDLE hPipe;
	std::wstring pipeName;
};


Channel::Channel(const std::wstring& name) : pipeName(name) {
	hPipe = CreateNamedPipeW(
		name.c_str(),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		512,
		512,
		0,
		NULL
	);
	try {
		if (hPipe == INVALID_HANDLE_VALUE) {
			throw ServiceException("Failed to create named pipe. Error code: " + GetLastError());
		}
	}
	catch (const ServiceException& e) {
		GlobalLogger.write(e.what(), ERR);
	}
}

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

export Channel GlobalChannel(L"\\\\.\\pipe\\antivirus_channel");