// Copyright (c) 2023 LG Electronics, Inc.
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

#ifndef CAMERA_SECURITY_DAC_POLICY_H_
#define CAMERA_SECURITY_DAC_POLICY_H_

#include <string>
#include <vector>

struct AclEntry
{
    std::string uname;
    std::string perm;
};

struct AclRule
{
    std::string dir;
    int owner_uid;
    int owner_gid;
    std::string base_user_perm;
    std::string base_group_perm;
    std::vector<AclEntry> acl_user_perms;
    std::vector<AclEntry> acl_group_perms;
    std::string mask;
    std::string other_perm;
    void init(void);
    void log(void);
};

class CameraDacPolicy
{
private:
    std::string aclText_;
    AclRule dcim_rule_;
    AclRule camera_rule_;

    std::vector<std::string> split(const std::string &str, char delim);
    const char *getUsername(int uid);

    bool getAcl(const char *file, int *owner_id, int *group_id);
    bool setAcl(const char *rule, const char *file);
    void logAclText(const char *acltext);
    bool getCurrentRule(const char *dir, AclRule &rule);
    void createTextRule(const AclRule &r, std::string &text);
    void createCaptureTextRule(int uid, std::string &text);
    bool prepare(int uid);

public:
    static CameraDacPolicy &getInstance()
    {
        static CameraDacPolicy obj;
        return obj;
    }

    bool checkCredential(int uid, std::string &path);
    bool apply(int uid);
};

#endif /* CAMERA_SECURITY_DAC_POLICY_H_ */