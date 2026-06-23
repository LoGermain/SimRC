
#pragma once
#include "../macros.h"
#include <exception>
#include <string>
namespace RS21::direct_input {

class WIN_API EffectException : public std::exception
{
public:
    EffectException(const std::string& errorMessage);
    EffectException(const char* errorMessage);
    virtual const char* what() const override{return m_errorStr.c_str();}
private:
    std::string m_errorStr;
};

}//namespace

