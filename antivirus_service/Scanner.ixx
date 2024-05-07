module; 

#include "antivirus_service.h"

export module Scanner;

import <vector>;
import Logger;
import <format>;
import <map>;

export class Scanner {
public:
	Scanner(std::string &);
	~Scanner();
    std::string readStringFromBinaryBase();
private:
    std::string signature;
	std::ifstream binaryBase;
};

std::string Scanner::readStringFromBinaryBase() {
    std::string line;
    char ch;

    if(binaryBase) {
        while(binaryBase.get(ch) && ch != '\n' && ch != '\0') {
            line += ch;
        }

        binaryBase.close();
    } else {
        GlobalLogger.write("Something with binary file...", ERR);
    }

    return line;
}

Scanner::Scanner(std::string &name) : binaryBase(name, std::ios::binary){
	if (!binaryBase.is_open())
		throw std::runtime_error("Ну и дела другалек");
	signature = readStringFromBinaryBase();
    GlobalLogger.write(std::format("Signature = {}", signature), INFO);
}

