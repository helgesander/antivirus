module;

#include <Windows.h>
#include <string>
import <iostream>;
#include <vector>
import Logger;
Logger GlobalLogger("C:\\Users\\helgesander\\antivirus.log");


export module FileStream;

export class FileStream {
private: 
	HANDLE fileHandle;
	DWORD fileSize;
	DWORD currentPosition;
public: 
	FileStream(const std::string&);
	~FileStream();
	bool isEOF();
	void Close();
	void ReadInt(int&);
	void ReadString(size_t, char*);
	void ReadBytes(std::vector<uint8_t>&);
	DWORD GetCurrentPosition();
	void SetCurrentPosition(DWORD);
};

DWORD FileStream::GetCurrentPosition()
{
	LARGE_INTEGER newPosition = { 0 };
	LARGE_INTEGER currentPosition = { 0 };
	SetFilePointerEx(fileHandle, newPosition, &currentPosition, FILE_CURRENT);
	return currentPosition.QuadPart;
}

void FileStream::SetCurrentPosition(DWORD position)
{
	LARGE_INTEGER newPosition = { 0 };
	LARGE_INTEGER currentPosition = { 0 };
	SetFilePointerEx(fileHandle, newPosition, &currentPosition, FILE_BEGIN);
}


void FileStream::ReadBytes(std::vector<uint8_t>& cont) {
	GlobalLogger.write(std::format("{}", cont.size()), INFO);
	DWORD cbBytesRead = 0;
	BOOL fSuccess = ReadFile(
		fileHandle,
		cont.data(),
		cont.size(),
		&cbBytesRead,
		NULL
	);
	if (!fSuccess || cont.size() != cbBytesRead)
	{
		GlobalLogger.write("Попа", INFO);
	}
	currentPosition += cbBytesRead;
}

void FileStream::ReadString(size_t length, char* str) {
	DWORD bytesRead;
	ReadFile(fileHandle, str, length, &bytesRead, NULL);
	currentPosition += bytesRead;
}

void FileStream::ReadInt(int& num) {
	DWORD bytesRead;
	ReadFile(fileHandle, &num, sizeof(int), &bytesRead, NULL);
	currentPosition += bytesRead;
}

void FileStream::Close() {
	if (fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
		fileHandle = INVALID_HANDLE_VALUE;
	}
}

FileStream::FileStream(const std::string& filename) : currentPosition(0) {
	fileHandle = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open file" << std::endl;
		return;
	}

	fileSize = GetFileSize(fileHandle, NULL);
}

FileStream::~FileStream() {
	Close();
}

bool FileStream::isEOF() {
	return currentPosition >= fileSize;
}