// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef JSON_SCHEMA_H_
#define JSON_SCHEMA_H_

/*
* Note : The strings are generated online from Json object
* Any change in string should be taken care else schema validation
* will fail
*/

const char *getCameraListSchema = "{ "
                                  "\"type\": \"object\", "
                                  "\"title\": \"The Root Schema\" }";

const char *getInfoSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
  \"id\" \
  ], \
  \"properties\": { \
    \"id\": { \
      \"$id\": \"#/properties/id\", \
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"pattern\": \"^(.*)$\" \
    } \
  } \
}";

const char *getPropertiesSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
    \"required\": [ \
      \"handle\" \
    ], \
    \"properties\": { \
      \"handle\": { \
        \"$id\": \"#/properties/handle\", \
        \"type\": \"integer\", \
        \"title\": \"The Handle Schema\", \
        \"default\": 0 \
      } \
    } \
}";

const char *openSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"id\" \
  ], \
  \"properties\": { \
    \"id\": { \
      \"$id\": \"#/properties/id\", \
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    } \
  } \
}";

const char *setFormatSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\", \
    \"params\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"params\": { \
      \"type\": \"object\", \
      \"title\": \"The Params Schema\", \
      \"required\": [ \
        \"width\", \
        \"height\", \
        \"format\" \
      ], \
      \"properties\": { \
        \"width\": { \
          \"type\": \"integer\", \
          \"title\": \"The Width Schema\", \
          \"default\": 0 \
        }, \
        \"height\": { \
          \"type\": \"integer\", \
          \"title\": \"The Height Schema\", \
          \"default\": 0 \
        }, \
        \"format\": { \
          \"type\": \"string\", \
          \"title\": \"The Format Schema\" \
        } \
      } \
    } \
  } \
}";

const char *setPropertiesSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\", \
    \"params\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"$id\": \"#/properties/handle\", \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"params\": { \
      \"$id\": \"#/properties/params\", \
      \"type\": \"object\", \
      \"title\": \"The Params Schema\" \
    } \
  } \
}";

const char *startCaptureSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\", \
    \"params\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"params\": { \
      \"type\": \"object\", \
      \"title\": \"The Params Schema\", \
      \"required\": [ \
        \"width\", \
        \"height\", \
        \"format\", \
        \"mode\" \
      ], \
      \"properties\": { \
        \"width\": { \
          \"type\": \"integer\", \
          \"title\": \"The Width Schema\", \
          \"default\": 0 \
        }, \
        \"height\": { \
          \"type\": \"integer\", \
          \"title\": \"The Height Schema\", \
          \"default\": 0 \
        }, \
        \"format\": { \
          \"type\": \"string\", \
          \"title\": \"The Format Schema\", \
          \"pattern\": \"^(.*)$\" \
        }, \
        \"mode\": { \
          \"type\": \"string\", \
          \"title\": \"The Mode Schema\", \
          \"pattern\": \"^(.*)$\" \
        }, \
        \"nimage\": { \
          \"type\": \"integer\", \
          \"title\": \"The Nimage Schema\", \
          \"default\": 0 \
        } \
      } \
    } \
  } \
}";

const char *startPreviewSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\", \
    \"params\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"params\": { \
      \"type\": \"object\", \
      \"title\": \"The Params Schema\", \
      \"required\": [ \
        \"type\", \
        \"source\" \
      ], \
      \"properties\": { \
        \"type\": { \
          \"type\": \"string\", \
          \"title\": \"The Type Schema\", \
          \"pattern\": \"^(.*)$\" \
        }, \
        \"source\": { \
          \"type\": \"string\", \
          \"title\": \"The Source Schema\", \
          \"pattern\": \"^(.*)$\" \
        } \
      } \
    } \
  } \
}";

const char *stopCapturePreviewCloseSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"$id\": \"#/properties/handle\", \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": -1 \
    } \
  } \
}";

#endif /*SRC_SERVICE_JSON_SCHEMA_H_*/
