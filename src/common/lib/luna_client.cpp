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

#include "luna_client.h"
#include "camera_constants.h"
#include <camera_log.h>
#include <glib.h>
#include <system_error>

struct AutoLSError : LSError
{
    AutoLSError(void) { LSErrorInit(this); }
    ~AutoLSError(void)
    {
        try
        {
            LSErrorFree(this);
        }
        catch (const std::system_error &e)
        {
            PMLOG_ERROR(CONST_MODULE_LC, "Caught a system_error with code %d meaning %s",
                        e.code().value(), e.what());
        }
        catch (std::bad_cast &err)
        {
            PMLOG_ERROR(CONST_MODULE_LC, "Caught a bad_cast %s", err.what());
        }
    }
};

LunaClient::LunaClient(void)
{
    AutoLSError error = {};
    error.message     = nullptr;
    if (!LSRegister(nullptr, &pHandle_, &error))
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
    }

    if (!LSGmainContextAttach(pHandle_, g_main_context_default(), &error))
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
    }
}

LunaClient::LunaClient(const char *serviceName, GMainContext *ctx)
{
    AutoLSError error = {};
    error.message     = nullptr;
    if (!LSRegister(serviceName, &pHandle_, &error))
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
    }

    if (ctx == nullptr)
        pContext_ = g_main_context_default();
    else
        pContext_ = ctx;

    if (!LSGmainContextAttach(pHandle_, pContext_, &error))
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
    }
}

LunaClient::~LunaClient(void)
{
    try
    {
        AutoLSError error = {};
        LSUnregister(pHandle_, &error);
    }
    catch (const std::exception &e)
    {
        return;
    }
}

bool LunaClient::callSync(const char *uri, const char *param, std::string *result, int timeout)
{
    AutoLSError error  = {};
    error.message      = nullptr;
    bool ret           = false;
    LSMessageToken tok = 0;

    struct Ctx
    {
        Ctx(std::string *pstrResult) : pstrResult_(pstrResult) {}
        bool bRet_{false};
        std::string *pstrResult_{nullptr};
        bool bDone_{false};
    } ctx(result);

    PMLOG_INFO(CONST_MODULE_LC, "[%p] uri=%s, param=%s, timeout=%d", g_thread_self(), uri, param,
               timeout);
    ret = LSCallOneReply(
        pHandle_, uri, param,
        +[](LSHandle *h, LSMessage *m, void *d)
        {
            Ctx *pCtx = static_cast<Ctx *>(d);
            // 1. Check whether error with including time out.
            if (!LSMessageIsHubErrorMessage(m))
                pCtx->bRet_ = true;
            // 2. Processing message
            pCtx->pstrResult_->assign(LSMessageGetPayload(m));
            // 3. Notify
            pCtx->bDone_ = true;
            PMLOG_INFO(CONST_MODULE_LC, "[%p] reply\n", g_thread_self());
            return pCtx->bRet_;
        },
        &ctx, &tok, &error);

    if (ret != true)
    {
        PMLOG_ERROR(CONST_MODULE_LC, "[%p] LunaClient ERROR: %s\n", g_thread_self(), error.message);
    }

    if (ret == true)
    {
        ret = LSCallSetTimeout(pHandle_, tok, timeout, &error);
    }

    if (ret == true)
    {
        if (pContext_ == g_main_context_default())
        {
            while (!ctx.bDone_)
            {
                if (FALSE == g_main_context_iteration(pContext_, FALSE))
                    continue;
            }
        }
        else
        {
            int cnt = 0;
            while (!ctx.bDone_ && cnt < timeout)
            {
                g_usleep(1000);
                cnt++;
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LC, "[%p] ret=%d, bRet_=%d", g_thread_self(), ret, ctx.bRet_);
    return ret && ctx.bRet_;
}

bool LunaClient::callAsync(const char *uri, const char *param, Handler handler, void *data)
{
    AutoLSError error       = {};
    error.message           = nullptr;
    bool ret                = false;
    HandlerWrapper *wrapper = new HandlerWrapper;
    wrapper->callback       = handler;
    wrapper->data           = data;

    PMLOG_DEBUG("uri=%s, param=%s", uri, param);
    ret = LSCallOneReply(
        pHandle_, uri, param,
        +[](LSHandle *h, LSMessage *m, void *d)
        {
            HandlerWrapper *wrapper = (HandlerWrapper *)d;
            wrapper->callback(LSMessageGetPayload(m), wrapper->data);
            delete wrapper;
            return true;
        },
        (void *)wrapper, NULL, &error);

    if (!ret)
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
        delete wrapper;
    }

    PMLOG_DEBUG("ret=%d", ret);
    return ret;
}

bool LunaClient::registerToService(const char *serviceName, RegisterHandler handler, void *data)
{
    struct RegisterHandlerWrapper
    {
        RegisterHandler callback;
        void *data;
    };

    AutoLSError error               = {};
    error.message                   = nullptr;
    bool ret                        = false;
    RegisterHandlerWrapper *wrapper = new RegisterHandlerWrapper;
    wrapper->callback               = handler;
    wrapper->data                   = data;

    PMLOG_DEBUG("serviceName=%s", serviceName);
    ret = LSRegisterServerStatusEx(
        pHandle_, serviceName,
        +[](LSHandle *h, const char *s, bool b, void *d)
        {
            RegisterHandlerWrapper *wrapper = (RegisterHandlerWrapper *)d;
            wrapper->callback(s, b, wrapper->data);
            delete wrapper;
            return true;
        },
        (void *)wrapper, NULL, &error);

    if (!ret)
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
        delete wrapper;
    }

    PMLOG_DEBUG("ret=%d", ret);
    return ret;
}

bool LunaClient::subscribe(const char *uri, const char *param, unsigned long *subscribeKey,
                           Handler handler, void *data)
{
    AutoLSError error       = {};
    error.message           = nullptr;
    bool ret                = false;
    HandlerWrapper *wrapper = new HandlerWrapper;
    wrapper->callback       = handler;
    wrapper->data           = data;

    PMLOG_DEBUG("uri=%s, param=%s", uri, param);
    ret = LSCall(
        pHandle_, uri, param,
        +[](LSHandle *h, LSMessage *m, void *d)
        {
            HandlerWrapper *wrapper = (HandlerWrapper *)d;
            wrapper->callback(LSMessageGetPayload(m), wrapper->data);
            return true;
        },
        (void *)wrapper, subscribeKey, &error);

    if (!ret)
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: %s\n", error.message);
        delete wrapper;
        return false;
    }

    handlers_[*subscribeKey] = std::unique_ptr<HandlerWrapper>(wrapper);

    PMLOG_DEBUG("ret=%d, subscribeKey=%ld", ret, *subscribeKey);
    return ret;
}

bool LunaClient::unsubscribe(unsigned long subscribeKey)
{
    AutoLSError error = {};

    PMLOG_DEBUG("subscribeKey=%ld", subscribeKey);
    if (!LSCallCancel(pHandle_, subscribeKey, &error))
    {
        PMLOG_ERROR(CONST_MODULE_LC, "LunaClient ERROR: subscribeKey = %ld", subscribeKey);
        handlers_.erase(subscribeKey);
        return false;
    }

    handlers_.erase(subscribeKey);
    PMLOG_DEBUG("ret=%d", true);
    return true;
}
