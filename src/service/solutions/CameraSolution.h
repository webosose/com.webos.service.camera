/*
 * Mobile Communication Company, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2017 by LG Electronics Inc.
 *
 * All rights reserved. No part of this work may be reproduced, stored in a
 * retrieval system, or transmitted by any means without prior written
 * Permission of LG Electronics Inc.
 */

#ifndef __LGCAMERASOLUTION_H__
#define __LGCAMERASOLUTION_H__

// System dependencies

#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "camera_hal_if.h"
#include <string>

#ifndef ABS
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#endif
#ifndef MIN
#define MIN(x , y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x , y) ((x) > (y) ? (x) : (y))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) MAX (MIN (x, max), min)
#endif

#define FEATURE_LG_AUTOCONTRAST
//#define FEATURE_LG_FACEDETECTION

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Result Codes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The result code type to be returned from functions. Must match ordering and size CamxResultStrings.

static const int32_t ResultSuccess           = 0;    ///< Operation was successful
static const int32_t ResultEFailed           = 1;    ///< Operation encountered unspecified error
static const int32_t ResultEUnsupported      = 2;    ///< Operation is not supported
static const int32_t ResultEInvalidState     = 3;    ///< Invalid state
static const int32_t ResultEInvalidArg       = 4;    ///< Invalid argument
static const int32_t ResultEInvalidPointer   = 5;    ///< Invalid memory pointer
static const int32_t ResultENoSuch           = 6;    ///< No such item exists or is valid
static const int32_t ResultEOutOfBounds      = 7;    ///< Out of bounds
static const int32_t ResultENoMemory         = 8;    ///< Out of memory
static const int32_t ResultETimeout          = 9;    ///< Operation timed out
static const int32_t ResultENoMore           = 10;   ///< No more items available
static const int32_t ResultENeedMore         = 11;   ///< Operation requires more
static const int32_t ResultEExists           = 12;   ///< Item exists
static const int32_t ResultEPrivLevel        = 13;   ///< Privileges are insufficient for requested operation
static const int32_t ResultEResource         = 14;   ///< Resources are insufficient for requested operation
static const int32_t ResultEUnableToLoad     = 15;   ///< Unable to load library/object
static const int32_t ResultEInProgress       = 16;   ///< Operation is already in progress
static const int32_t ResultETryAgain         = 17;   ///< Could not complete request; try again
static const int32_t ResultEBusy             = 18;   ///< Device or resource busy
static const int32_t ResultEReentered        = 19;   ///< Non re-entrant API re-entered
static const int32_t ResultEReadOnly         = 20;   ///< Cannot change read-only object or parameter
static const int32_t ResultEOverflow         = 21;   ///< Value too large for defined data type
static const int32_t ResultEOutOfDomain      = 22;   ///< Math argument or result out of domain
static const int32_t ResultEInterrupted      = 23;   ///< Waitable call is interrupted
static const int32_t ResultEWouldBlock       = 24;   ///< Operation would block
static const int32_t ResultETooManyUsers     = 25;   ///< Too many users
static const int32_t ResultENotImplemented   = 26;   ///< Function or method is not implemented
static const int32_t ResultEDisabled         = 27;   ///< Feature disabled
static const int32_t ResultECancelledRequest = 28;   ///< Request is in cancelled state
// Increment the count below if adding additional result codes.
static const int32_t ResultCount             = 29;   ///< The number of result codes. Not to be used.

static const unsigned int InvalidId                    = 0xFFFFFFFF;   ///< Defining an Invalid Handle


class CameraSolutionManager;

class CameraSolution {
    public:
        CameraSolution(CameraSolutionManager *mgr);
        virtual ~CameraSolution();

        // interface for snapshot
        virtual void initialize(stream_format_t streamformat, int key) = 0;
        virtual bool isSupported() = 0;
        virtual bool isEnabled() = 0;
        virtual void setSupportStatus(bool supportStatus) = 0;
        virtual void setEnableValue(bool enableValue) = 0;
        virtual bool needThread() = 0;
        virtual void startThread(stream_format_t streamformat, int key) = 0;
        virtual std::string getSolutionStr() = 0;
        virtual void processForSnapshot(void* inBuf,        stream_format_t streamformat) = 0;
        virtual void processForPreview(void* inBuf, stream_format_t streamformat) = 0;
        virtual void release() = 0;

        CameraSolutionManager *m_manager;
        int dump_framecnt;
        bool needDebugInfo;
    private:

};

#endif //__LGCAMERASOLUTION_H__
