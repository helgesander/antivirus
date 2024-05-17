module;

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <format>
#include <algorithm>
#include <filesystem>
#include "Windows.h"
#include <wincrypt.h>

std::string GenerateByteString(std::vector<uint8_t>& cont);

import Logger;
import FileStream;
Logger LocalLogger("C:\\Users\\helgesander\\antivirus.log");

export module Scanner;

namespace fs = std::filesystem;

#define ID_SIZE 10
#define BUFSIZE 32 

enum class HashType {
    HashSha1, HashMd5, HashSha256
};

export struct Signature {
    char name[BUFSIZE];
    int signatureLength;
    std::vector<uint8_t> signature;
    std::vector<uint8_t> hash;
    int offsetBegin;
    int offsetEnd;
}; // TODO: add hash 

export using SignaturesData = std::map<std::vector<uint8_t>, Signature>;
export using ScanFileResult = std::vector<std::string>;
export using ScanFolderResult = std::map<std::string, ScanFileResult>;

export class Scanner {
public:
    Scanner(const std::string&);
    bool scanFile(std::string const&, ScanFileResult&);
    bool scanFolder(std::string const&, ScanFolderResult&);
private:
    SignaturesData malwareBase;
    // Private methods
    void loadSignatureFromFile(FileStream&, Signature&);
    bool checkFolder(std::string const&);
    std::vector<uint8_t> GetDataHash(const uint8_t*, const size_t, HashType);
};

std::vector<uint8_t> Scanner::GetDataHash(const uint8_t* data, const size_t data_size, HashType hashType)
{
    HCRYPTPROV hProv = NULL;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        return {};
    }
    BOOL hash_ok = FALSE;
    HCRYPTPROV hHash = NULL;
    switch (hashType) {
    case HashType::HashSha1: hash_ok = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash); break;
    case HashType::HashMd5: hash_ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash); break;
    case HashType::HashSha256: hash_ok = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash); break;
    }
    if (!hash_ok) {
        CryptReleaseContext(hProv, 0);
        return {};
    }
    if (!CryptHashData(hHash, static_cast<const BYTE*>(data), data_size, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }
    DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHashSize, &dwCount, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }
    std::vector<uint8_t> buffer(cbHashSize);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(&buffer[0]), &cbHashSize, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return buffer;
}

Scanner::Scanner(const std::string& namefile) {
    LocalLogger.write("check0", INFO);
    FileStream stream(namefile);
    char id[] = "FILIPPOVA";
    char tempIdentificator[ID_SIZE];
    stream.ReadString(ID_SIZE, tempIdentificator);
    //if (!strcmp(id, tempIdentificator)) {
    //    LocalLogger.write("Base is not valid", INFO);
    //    throw std::runtime_error("Base is not valid");
    //}

    while (!stream.isEOF()) {
        Signature sig;
        loadSignatureFromFile(stream, sig);
        // LocalLogger.write(std::format("{} {} {} {} {}", sig.name, sig.signatureLength, reinterpret_cast<int>(sig.signature.data()), sig.offsetBegin, sig.offsetEnd), INFO);
        malwareBase[sig.hash] = sig;
    }
    LocalLogger.write("check2", INFO);
}

void Scanner::loadSignatureFromFile(FileStream& stream, Signature& sig) {
    stream.ReadString(BUFSIZE, sig.name);
    stream.ReadInt(sig.signatureLength);
    sig.signature.resize(sig.signatureLength);
    stream.ReadBytes(sig.signature);
    stream.ReadBytes(sig.hash);
    stream.ReadInt(sig.offsetBegin);
    stream.ReadInt(sig.offsetEnd);
}

bool Scanner::scanFile(std::string const& filename, ScanFileResult& whichSignatureInside)
{
    FileStream stream(filename);
    bool exist = false;
    for (auto sig : malwareBase) {
        stream.SetCurrentPosition(sig.second.offsetBegin);
        std::vector<uint8_t> data(sig.second.signatureLength);
        stream.ReadBytes(data);
        auto hash = GetDataHash(sig.second.signature.data(), sig.second.signatureLength, HashType::HashMd5);
        if (hash == sig.second.hash) {
            exist = true;
            whichSignatureInside.push_back(sig.second.name);
        }
    }
    return exist;
}

bool Scanner::scanFolder(std::string const& foldername, ScanFolderResult& result) {
    bool ret = false;
    if (checkFolder(foldername)) {
        ScanFileResult temp;
        for (const auto& file : fs::directory_iterator(foldername)) {
            // TODO: fix scan folder (add check binary file)
            if (scanFile(file.path().string(), temp)) {
                ret = true;
                result[file.path().string()] = temp;
            }
        }
    }
    return ret;
}

bool Scanner::checkFolder(std::string const& foldername) {
    bool ret = false;
    if (fs::exists(foldername) and fs::is_directory(foldername))
        ret = true;
    return ret;
}

// FOR CHECK
std::string GenerateByteString(std::vector<uint8_t> &cont) {
    std::string res;
    for (uint8_t byte : cont)
        res += std::format("{:02x}", byte);
    return res;
}