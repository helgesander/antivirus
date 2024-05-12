module;

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <format>
#include <algorithm>
#include <filesystem>

export module Scanner;

namespace fs = std::filesystem;

#define ID_SIZE 10
#define BUFSIZE 32 

export struct Signature {
    char name[BUFSIZE];
    uint16_t signatureLength;
    std::vector<uint8_t> signature;
    int offsetBegin;
    int offsetEnd;
}; // TODO: add hash 

export using SignaturesData = std::vector<Signature>; // TODO: fix with using
export using ScanFolderData = std::map<std::string, SignaturesData>;

export class Scanner {
public:
    Scanner(const std::string&);
    SignaturesData getBase() { return malwareBase; };
    bool scanFile(std::string const&, SignaturesData&);
    bool scanFolder(std::string const&, ScanFolderData&);
private:
    SignaturesData malwareBase;
    std::string identificator;
    // Private methods
    void loadSignatureFromFile(std::ifstream&, Signature&);
    int getSizeOfFile(std::ifstream&);
    bool checkFolder(std::string const&);
};

Scanner::Scanner(const std::string& namefile) {
    std::ifstream binaryBase(namefile, std::ios::binary);
    char temp_identificator[ID_SIZE];
    binaryBase.read((char*)&temp_identificator, sizeof(temp_identificator));
    identificator = temp_identificator;

    while (!binaryBase.eof()) {
        Signature sig;
        loadSignatureFromFile(binaryBase, sig);
        malwareBase.push_back(sig);
    }
    binaryBase.close();
}

void Scanner::loadSignatureFromFile(std::ifstream& file, Signature& sig) {
    file.read((char*)&sig.name, sizeof(sig.name));
    file.read((char*)&sig.signatureLength, sizeof(sig.signatureLength));
    file.read((char*)sig.signature.data(), sig.signatureLength);
    file.read((char*)&sig.offsetBegin, sizeof(sig.offsetBegin));
    file.read((char*)&sig.offsetEnd, sizeof(sig.offsetEnd));
}

bool Scanner::scanFile(std::string const& filename, SignaturesData& whichSignatureInside)
{
    //TODO: fix file reading
    std::ifstream checkingFile(filename, std::ios::binary);
    std::vector<uint8_t> dataByOffset;
    bool exist = false;
    for (auto signatureObject : malwareBase) {
        checkingFile.seekg(signatureObject.offsetBegin, std::ios::beg);
        checkingFile.read((char*)dataByOffset.data(), getSizeOfFile(checkingFile) - signatureObject.offsetEnd); // TODO: check offset from end
        auto it = std::search(dataByOffset.begin(), dataByOffset.end(),
            signatureObject.signature.begin(), signatureObject.signature.end());
        if (it != signatureObject.signature.end()) {
            exist = true;
            whichSignatureInside.push_back(signatureObject);
        }
    }
    return exist;
}

bool Scanner::scanFolder(std::string const& foldername, ScanFolderData& result) {
    bool ret = false;
    if (checkFolder(foldername)) {
        SignaturesData temp;
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

int Scanner::getSizeOfFile(std::ifstream& file) {
    size_t sizeBefore = file.tellg();
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(sizeBefore, std::ios::beg);
    return size;
}

bool Scanner::checkFolder(std::string const& foldername) {
    bool ret = false;
    if (fs::exists(foldername) and fs::is_directory(foldername))
        ret = true;
    return ret;
}