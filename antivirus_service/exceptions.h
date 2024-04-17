#pragma once

#include <exception>
#include <string>

class ServiceException : public std::exception
{
private:
    std::string m_errorMessage;

public:
    ServiceException(const std::string& errorMessage) : m_errorMessage(errorMessage) {}

    const char* what() const throw()
    {
        return m_errorMessage.c_str();
    }
};

enum RETURN_ERRORS {
   OK,
   // TODO: 
};
