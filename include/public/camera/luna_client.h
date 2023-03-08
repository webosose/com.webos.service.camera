// @@@LICENSE
//
// Copyright (C) 2021, LG Electronics, All Right Reserved.
//
// No part of this source code may be communicated, distributed, reproduced
// or transmitted in any form or by any means, electronic or mechanical or
// otherwise, for any purpose, without the prior written permission of
// LG Electronics.
//
// LICENSE@@@
#pragma once

#include <glib.h>
#include <luna-service2/lunaservice.h>
#include <map>
#include <memory>
#include <string>

using Handler         = bool (*)(const char *, void *);
using RegisterHandler = bool (*)(const char *, bool, void *);

struct LSHandle;

class LunaClient
{
public:
    struct HandlerWrapper
    {
        Handler callback;
        void *data;
    };
    struct RegisterHandlerWrapper
    {
        RegisterHandler callback;
        void *data;
    };

    LunaClient(void);
    LunaClient(const char *serviceName, GMainContext *ctx = nullptr);
    virtual ~LunaClient(void);

    bool callSync(const char *uri, const char *param, std::string *result, int timeout = 30);

    bool callAsync(const char *uri, const char *param, Handler handler, void *data);

    bool subscribe(const char *uri, const char *param, unsigned long *subscribeKey, Handler handler,
                   void *data);

    bool unsubscribe(unsigned long subscribeKey);

    bool registerToService(const char *serviceName, RegisterHandler handler, void *data);

private:
    LSHandle *pHandle_{nullptr};
    GMainContext *pContext_{nullptr};
    std::map<unsigned long, std::unique_ptr<HandlerWrapper>> handlers_;
    std::map<std::string, std::unique_ptr<RegisterHandlerWrapper>> registerHandlers_;
};
