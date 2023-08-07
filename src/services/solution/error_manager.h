#pragma once

#include <map>
#include <string>

enum ErrorCode
{
    SOLLUTION_NAME_IS_REQUIRED = 100,
    SOLLUTION_NAME_IS_EMPTY    = 110,
    FAIL_TO_OPEN_PLUGIN        = 120,
    FAIL_TO_CREATE_SOLUTION    = 130,
    FAIL_TO_INIT               = 200,
    ENABLE_VALUE_IS_REQUIRED   = 300,
    FAIL_TO_ENABLE             = 310,
    FAIL_TO_RELEASE            = 400,
    FAIL_TO_SUBSCRIBE          = 500,
    ERROR_CODE_END             = 999,
};

class ErrorManager
{
    static const std::map<ErrorCode, std::string> errorMap;

public:
    static std::string GetErrorText(ErrorCode errorCode);
};