#include "service_utils.h"

#include <string.h>
#include "constants.h"
#include "camera_types.h"
#include "command_manager.h"
#include "device_manager.h"
#include "command_service_util.h"
#include "service_main.h"

int gbSubscribePDM = 0;
// for dependency services
static int gbServiceReady = 0; // systemProperty, PDM service data
DEVICE_LIST_T arrDevList[MAX_DEVICE];

CommandManager *devcmd = CommandManager::getInstance();
DeviceManager *dInfo = DeviceManager::getInstance();

static void __subscribe_get_list(int *pCamDev, int *pMicDev, int *pCamSupport, int *pMicSupport)
{
    CAMERA_PRINT_INFO("%s:%d] started!", __FUNCTION__, __LINE__);

    LSError lserror;
    struct json_object *pOutJson = NULL, *pOutJsonChild1 = NULL, *pOutJsonChild2 = NULL;
    char strKey[CONST_MAX_STRING_LENGTH];
    char arrList[20][CONST_MAX_STRING_LENGTH];
    int supportList[20];
    int i;
    int nCamCount;
    int nMicCount;

    pOutJson = json_object_new_object();
    nCamCount = 0;
    for (i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
    {
        if (pCamDev[i] == CONST_VARIABLE_INITIALIZE)
            break;

        snprintf(arrList[nCamCount], CONST_MAX_STRING_LENGTH, "%s://%s/%s%d",
        CONST_SERVICE_URI_NAME, CONST_SERVICE_NAME_CAMERA,
        CONST_DEVICE_NAME_CAMERA, pCamDev[i]);
        supportList[nCamCount] = pCamSupport[i];
        nCamCount++;
    }

    nMicCount = 0;
    for (i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
    {
        if (pMicDev[i] == CONST_VARIABLE_INITIALIZE)
            break;

        snprintf(arrList[nCamCount + nMicCount], CONST_MAX_STRING_LENGTH, "%s://%s/%s%d",
        CONST_SERVICE_URI_NAME,
        CONST_SERVICE_NAME_CAMERA, CONST_DEVICE_NAME_MIC, pMicDev[i]);
        supportList[nCamCount + nMicCount] = pMicSupport[i];
        nMicCount++;
    }

    pOutJsonChild1 = json_object_new_array();

    for (i = 0; i < nCamCount; i++)
    {
        pOutJsonChild2 = json_object_new_object();
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_URI,
                json_object_new_string(arrList[i]));
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_TYPE,
                json_object_new_string(_GetTypeString(DEVICE_CAMERA)));
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_SUPPORT,
                json_object_new_boolean(supportList[i]));
        json_object_array_add(pOutJsonChild1, pOutJsonChild2);
    }

    for (i = nCamCount; i < nCamCount + nMicCount; i++)
    {
        pOutJsonChild2 = json_object_new_object();
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_URI,
                json_object_new_string(arrList[i]));
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_TYPE,
                json_object_new_string(_GetTypeString(DEVICE_MICROPHONE)));
        json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_SUPPORT,
                json_object_new_boolean(supportList[i]));
        json_object_array_add(pOutJsonChild1, pOutJsonChild2);
    }

    json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
            json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
    json_object_object_add(pOutJson, CONST_PARAM_NAME_URILIST, pOutJsonChild1);

    PMLOG_INFO(CONST_MODULE_LUNA" %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return;
}

static void __remove_subscription_item(DEVICE_TYPE_T nCategory, int nDevNum)
{
    //free(arrKeyList);
    CAMERA_PRINT_INFO("%s:%d] Ended!!!", __FUNCTION__, __LINE__);
    return;
}

static void __process_cam_event(DEVICE_EVENT_STATE_T nCamEvent, int nCamNum, int bPowerOnConnect,
        int bBuiltIn, int bUpdated, char *pPowerStatus)
{
    CAMERA_PRINT_INFO(
            "%s:%d] Started(nCamEvent: %d, nCamNum: %d, bBuiltIn: %d, bUpdated: %d, pPowerStatus: %s)!!!",
            __FUNCTION__, __LINE__, nCamEvent, nCamNum, bBuiltIn, bUpdated, pPowerStatus);

    int arrCamDev[CONST_MAX_DEVICE_COUNT], arrMicDev[CONST_MAX_DEVICE_COUNT];
    int arrCamSupport[CONST_MAX_DEVICE_COUNT], arrMicSupport[CONST_MAX_DEVICE_COUNT];

    switch (nCamEvent)
    {
    case DEVICE_EVENT_STATE_PLUGGED:

        devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        break;
    case DEVICE_EVENT_STATE_UNPLUGGED:
        __remove_subscription_item(DEVICE_CAMERA, nCamNum);

        devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        break;
    case DEVICE_EVENT_STATE_UNSUPPORTED_PLUGGED:
        if (dInfo->isUpdatedList())
        {
            devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
            __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        }
        break;
    case DEVICE_EVENT_STATE_UNSUPPORTED_UNPLUGGED:
        if (dInfo->isUpdatedList())
        {
            devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
            __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        }
        break;
    case DEVICE_EVENT_STATE_DUPLICATED_PLUGGED:
        if (dInfo->isUpdatedList())
        {
            devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
            __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        }
        break;
    case DEVICE_EVENT_STATE_DUPLICATED_UNPLUGGED:
        if (dInfo->isUpdatedList())
        {
            devcmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
            __subscribe_get_list(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);
        }
        break;
    default:
        PMLOG_INFO(CONST_MODULE_LUNA, "%s:%d] unknown event!!!(nCamEvent: %d)", __FUNCTION__,
                __LINE__, nCamEvent);
    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!!", __FUNCTION__, __LINE__);
    return;
}

static void __process_mic_event(DEVICE_EVENT_STATE_T nMicEvent, int nMicNum)
{
    CAMERA_PRINT_INFO("%s:%d] Started(nMicEvent: %d, nMicNum: %d)!!!", __FUNCTION__, __LINE__,
            nMicEvent, nMicNum);

    switch (nMicEvent)
    {
    case DEVICE_EVENT_STATE_UNPLUGGED:
        // Microphone device  subscribe
        //__removeSubscriptionItem(DEVICE_MICROPHONE, nMicNum);
        break;
    default:
        PMLOG_ERROR(CONST_MODULE_LUNA, "%s:%d] unknown event!!!(nMicEvent: %d)", __FUNCTION__,
                __LINE__, nMicEvent);
    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!!", __FUNCTION__, __LINE__);
    return;
}

static bool __filter_update_device_list(LSHandle *lsHandle, LSMessage *message, void *user_data)

{
    CAMERA_PRINT_INFO("%s:%d] started!", __FUNCTION__, __LINE__);

    bool ret = false;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL, *pInJsonChild2 = NULL,
            *pInJsonChild3 = NULL;

    // json parsing
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!pInJson)
    {
        CAMERA_PRINT_INFO("%s:%d] json parsing error!", __FUNCTION__, __LINE__);
    }
    else
    {
        CAMERA_PRINT_INFO("%s:%d] %s", __FUNCTION__, __LINE__, json_object_to_json_string(pInJson));
        PMLOG_INFO(CONST_MODULE_LUNA, "Get Device list to pdm service: %s",
                json_object_to_json_string(pInJson));

        int i;

        int bRetValue;
        int nListCount;
        int nCamCount;

        int nDeviceNum = -1;
        int nPortNum = -1;
        char *pVendorName = NULL;
        char *pProductName = NULL;
        char *pSerialNumber = NULL;
        char *pDeviceType = NULL;
        char *pDeviceSubtype = NULL;
        int isPowerOnConnect = 0;
        char *pPowerStatus = NULL;

        //DEVICE_LIST_T arrDevList[10];

        DEVICE_EVENT_STATE nCamEvent;
        DEVICE_EVENT_STATE nMicEvent;
        int bBuiltIn;
        int bUpdated;
        int nCamNum;
        int nMicNum;
        int bPowerOnConnect;

        if (json_object_object_get_ex(pInJson, CONST_PARAM_NAME_RETURNVALUE, &pInJsonChild1))
        {
            bRetValue = json_object_get_boolean(pInJsonChild1);

            if (bRetValue <= 0)
            {
                CAMERA_PRINT_INFO("%s:%d] PDM is not a good state! CameraService's API also stop!",
                        __FUNCTION__, __LINE__);
            }
            else
            {
                gbSubscribePDM = 1;

                if (json_object_object_get_ex(pInJson,
                CONST_PARAM_NAME_POWER_STATUS, &pInJsonChild1))
                {
                    pPowerStatus = (char *) json_object_get_string(pInJsonChild1);
                }

                if (json_object_object_get_ex(pInJson,
                CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST, &pInJsonChild1))
                {
                    nListCount = json_object_array_length(pInJsonChild1);
                    nCamCount = 0;

                    for (i = 0; i < nListCount; i++)
                    {
                        pInJsonChild2 = json_object_array_get_idx(pInJsonChild1, i);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_DEVICE_NUM, &pInJsonChild3))
                            nDeviceNum = json_object_get_int(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_USB_PORT_NUM, &pInJsonChild3))
                            nPortNum = json_object_get_int(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_VENDOR_NAME, &pInJsonChild3))
                            pVendorName = (char *) json_object_get_string(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_PRODUCT_NAME, &pInJsonChild3))
                            pProductName = (char *) json_object_get_string(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_SERIAL_NUMBER, &pInJsonChild3))
                            pSerialNumber = (char *) json_object_get_string(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_DEVICE_TYPE, &pInJsonChild3))
                            pDeviceType = (char *) json_object_get_string(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_DEVICE_SUBTYPE, &pInJsonChild3))
                            pDeviceSubtype = (char *) json_object_get_string(pInJsonChild3);

                        if (json_object_object_get_ex(pInJsonChild2,
                        CONST_PARAM_NAME_IS_POWER_ON_CONNECT, &pInJsonChild3))
                            isPowerOnConnect = json_object_get_boolean(pInJsonChild3);

                        if (pDeviceType != NULL && strncmp(pDeviceType, "CAM", 3) == 0)
                        {
                            arrDevList[nCamCount].nDeviceNum = nDeviceNum;
                            arrDevList[nCamCount].nPortNum = nPortNum;
                            strncpy(arrDevList[nCamCount].strVendorName, pVendorName,
                            CONST_MAX_STRING_LENGTH - 1);
                            arrDevList[nCamCount].strVendorName[CONST_MAX_STRING_LENGTH - 1] = '\0';
                            strncpy(arrDevList[nCamCount].strProductName, pProductName,
                            CONST_MAX_STRING_LENGTH - 1);
                            arrDevList[nCamCount].strProductName[CONST_MAX_STRING_LENGTH - 1] =
                                    '\0';
                            strncpy(arrDevList[nCamCount].strSerialNumber, pSerialNumber,
                            CONST_MAX_STRING_LENGTH - 1);
                            arrDevList[nCamCount].strSerialNumber[CONST_MAX_STRING_LENGTH - 1] =
                                    '\0';
                            strncpy(arrDevList[nCamCount].strDeviceType, pDeviceType,
                            CONST_MAX_STRING_LENGTH - 1);
                            arrDevList[nCamCount].strDeviceType[CONST_MAX_STRING_LENGTH - 1] = '\0';
                            strncpy(arrDevList[nCamCount].strDeviceSubtype, pDeviceSubtype,
                            CONST_MAX_STRING_LENGTH - 1);
                            arrDevList[nCamCount].strDeviceSubtype[CONST_MAX_STRING_LENGTH - 1] =
                                    '\0';
                            arrDevList[nCamCount].isPowerOnConnect = isPowerOnConnect;

                            nCamCount++;
                        }
                    }

                    nCamEvent = nMicEvent = DEVICE_EVENT_NONE;
                    nCamNum = 0;
                    bPowerOnConnect = 0;
                    nMicNum = 0;
                    bBuiltIn = 0;
                    bUpdated = 0;

                    if (dInfo->updateList(arrDevList, nCamCount, &nCamEvent, &nMicEvent)
                            == DEVICE_OK)
                    {
                        // processing cam event
                        __process_cam_event(nCamEvent, nCamNum, bPowerOnConnect, bBuiltIn, bUpdated,
                                pPowerStatus);

                        // processing mic event
                        __process_mic_event(nMicEvent, nMicNum);
                        if ((!gbServiceReady) && (0 < nCamCount))
                        {
                            gbServiceReady = 1;
                        }
                    }
                    else
                    {
                        CAMERA_PRINT_INFO("%s:%d] HAL Update error", __FUNCTION__, __LINE__);
                    }
                }
            }
        }
    }

    LSMessageUnref(message);
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}

void _sender_get_nonstorage_device_list(void)
{
    CAMERA_PRINT_INFO("%s:%d] started", __FUNCTION__, __LINE__);

    if (gbSubscribePDM)
    {
        CAMERA_PRINT_INFO("%s:%d] PDM already subscribed!", __FUNCTION__, __LINE__);
        return;
    }

    struct json_object *pOutJson = NULL;

    pOutJson = json_object_new_object();
    json_object_object_add(pOutJson, CONST_PARAM_NAME_SUBSCRIBE,
            json_object_new_boolean(CONST_PARAM_VALUE_TRUE));

    PMLOG_INFO(CONST_MODULE_LUNA, "Request Device list to pdm service");

    // Command Service Luna Call
    CAMERA_COMMANDSERVICE_Push(CONST_LS_SERVICE_NAME_PDM, NULL,
    CONST_LS_SERVICE_FUNCTION_NAME_GET_ATTACHED_NONSTORAGE_DEVICE_LIST,
            json_object_to_json_string(pOutJson), 1, __filter_update_device_list);

    //H_CAMERA_JSON_PUT(pOutJson);

    CAMERA_PRINT_INFO("%s:%d] ended", __FUNCTION__, __LINE__);
    return;
}

