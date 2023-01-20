// Copyright (c) 2019-2021 LG Electronics, Inc.
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
 * https://www.jsonschema.net/
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
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"pattern\": \"^(.*)$\" \
    } \
  } \
}";

const char *getPropertiesSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
    \"params\": { \
      \"type\": \"array\", \
      \"title\": \"The Params Schema\", \
      \"items\": { \
        \"type\": \"string\", \
        \"title\": \"The Items Schema\",\
        \"default\": \"\", \
        \"pattern\": \"^(.*)$\" \
      }\
    },\
    \"properties\": { \
      \"handle\": { \
        \"type\": \"integer\", \
        \"title\": \"The Handle Schema\", \
        \"default\": 0 \
      }, \
      \"id\": { \
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
      }, \
      \"subscribe\": { \
      \"type\": \"boolean\", \
      \"title\": \"The subscribe Schema\", \
      \"default\": false \
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
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    }, \
    \"mode\": { \
      \"type\": \"string\", \
      \"title\": \"The Priority Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    }, \
    \"pid\": { \
      \"type\": \"integer\", \
      \"title\": \"The Client Process Id Schema\", \
      \"default\": -1 \
    }, \
    \"sig\": { \
      \"type\": \"integer\", \
      \"title\": \"The Signal Number\", \
      \"default\": 10 \
    }, \
    \"appId\": { \
      \"type\": \"string\", \
      \"title\": \"Application Id of The Client Application\", \
      \"default\": \"\" \
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
        \"format\", \
        \"fps\" \
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
        \"fps\": { \
          \"type\": \"integer\", \
          \"title\": \"The fps Schema\", \
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
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"params\": { \
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
    \"path\": { \
      \"type\": \"string\", \
      \"title\": \"The Path Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
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
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"pid\": { \
      \"type\": \"integer\", \
      \"title\": \"The Client Id Schema\", \
      \"default\": -1 \
    } \
  } \
}";

const char *getEventNotificationSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"subscribe\" \
  ], \
  \"properties\": { \
    \"subscribe\": { \
      \"type\": \"boolean\", \
      \"title\": \"The subscribe Schema\", \
      \"default\": false \
    } \
  } \
}";

const char *getFdSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"handle\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    } \
  } \
}";

const char *getSolutionsSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"id\": { \
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    } \
  } \
}";

const char *setSolutionsSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"solutions\" \
  ], \
  \"properties\": { \
    \"handle\": { \
      \"type\": \"integer\", \
      \"title\": \"The Handle Schema\", \
      \"default\": 0 \
    }, \
    \"id\": { \
      \"type\": \"string\", \
      \"title\": \"The id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    }, \
    \"solutions\": { \
      \"type\": \"array\", \
      \"title\": \"The Solutions Schema\", \
      \"items\": { \
        \"type\": \"object\", \
        \"title\": \"A Schema\", \
        \"required\": [ \
          \"name\", \
          \"params\" \
        ], \
        \"properties\": { \
          \"name\": { \
            \"type\": \"string\", \
            \"title\": \"The name Schema\", \
            \"default\": \"\", \
            \"pattern\": \"^(.*)$\" \
          }, \
          \"params\": { \
            \"type\": \"object\", \
            \"title\": \"The params Schema\", \
            \"required\": [ \
              \"enable\" \
            ], \
            \"properties\": { \
              \"enable\": { \
                \"type\": \"boolean\", \
                \"title\": \"The enable Schema\", \
                \"default\": false \
              } \
            } \
          } \
        } \
      } \
    } \
  } \
}";

const char *getFormatSchema = "{ \
  \"type\": \"object\", \
  \"title\": \"The Root Schema\", \
  \"required\": [ \
    \"id\" \
  ], \
  \"properties\": { \
    \"id\": { \
      \"type\": \"string\", \
      \"title\": \"The Id Schema\", \
      \"default\": \"\", \
      \"pattern\": \"^(.*)$\" \
    }, \
    \"subscribe\": { \
      \"type\": \"boolean\", \
      \"title\": \"The subscribe Schema\", \
      \"default\": false \
    } \
  } \
}";
#endif /*SRC_SERVICE_JSON_SCHEMA_H_*/
