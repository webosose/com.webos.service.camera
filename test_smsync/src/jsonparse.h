#ifndef _CUSTOM_JSON_PARSE_H_
#define _CUSTOM_JSON_PARSE_H_

#include <pbnjson.hpp>

pbnjson::JValue convertStringToJson(const char *rawData);
std::string convertJsonToString(const pbnjson::JValue json);


#endif