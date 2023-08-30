#include "error_manager.h"

const std::map<ErrorCode, std::string> ErrorManager::errorMap = {
    {SOLLUTION_NAME_IS_REQUIRED, "Solution name is required."},
    {SOLLUTION_NAME_IS_EMPTY, "Solution name is empty."},
    {FAIL_TO_OPEN_PLUGIN, "Fail to open plugin."},
    {FAIL_TO_CREATE_SOLUTION, "Fail to create a solution."},
    {FAIL_TO_INIT, "Fail to initialize."},
    {ENABLE_VALUE_IS_REQUIRED, "Enable value is required."},
    {FAIL_TO_ENABLE, "Fail to enable."},
    {FAIL_TO_RELEASE, "Fail to release."},
    {FAIL_TO_SUBSCRIBE, "Fail to subscribe."},
};

std::string ErrorManager::GetErrorText(ErrorCode errorCode)
{
    auto it = errorMap.find(errorCode);
    if (it != errorMap.end())
    {
        return it->second;
    }
    return "Unknown error.";
}