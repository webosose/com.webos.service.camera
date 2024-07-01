#define LOG_TAG "DacPolicy"
#include "camera_dac_policy.h"
#include "camera_log.h"
#include <algorithm>
#include <errno.h>
#include <pwd.h>
#include <sstream>
#include <stdio.h> // debug
#include <string.h>
#include <sys/acl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const std::string str_dcim_dir   = "/media/internal/DCIM";
const std::string str_camera_dir = "/media/internal/DCIM/Camera";

void AclRule::init(void)
{
    dir             = "";
    owner_uid       = -1;
    owner_gid       = -1;
    base_user_perm  = "";
    base_group_perm = "";
    acl_user_perms.clear();
    acl_group_perms.clear();
    mask       = "";
    other_perm = "";
}

void AclRule::log(void)
{
    std::string acl_entry_;
    PLOGI("# file: %s", dir.c_str());
    PLOGI("# owner: %d", owner_uid);
    PLOGI("# group: %d", owner_gid);
    PLOGI("user::%s", base_user_perm.c_str());
    PLOGI("group::%s", base_group_perm.c_str());
    for (unsigned long i = 0; i < acl_user_perms.size(); i++)
    {
        acl_entry_ = "user:" + acl_user_perms[i].uname + ":" + acl_user_perms[i].perm;
        PLOGI("%s", acl_entry_.c_str());
    }
    for (unsigned long i = 0; i < acl_group_perms.size(); i++)
    {
        acl_entry_ = "group:" + acl_group_perms[i].uname + ":" + acl_group_perms[i].perm;
        PLOGI("%s", acl_entry_.c_str());
    }
    PLOGI("mask::%s", mask.c_str());
    PLOGI("other::%s", other_perm.c_str());
}

std::vector<std::string> CameraDacPolicy::split(const std::string &str, char delim)
{
    std::istringstream iss(str);
    std::string token;

    std::vector<std::string> result;
    while (getline(iss, token, delim))
    {
        result.push_back(token);
    }

    return result;
}

void CameraDacPolicy::logAclText(const char *acltext)
{
    PLOGI("ACL text:");
    std::string input              = acltext;
    std::vector<std::string> lines = split(input, '\n');
    for (unsigned long i = 0; i < lines.size(); i++)
    {
        PLOGI("%s", lines[i].c_str());
    }
}

const char *CameraDacPolicy::getUsername(int uid)
{
    struct passwd *pws = getpwuid((uid_t)uid);
    if (pws)
    {
        return pws->pw_name;
    }
    return NULL;
}

bool CameraDacPolicy::getAcl(const char *file, int *owner_uid, int *owner_gid)
{
    struct stat st;
    acl_t acl;
    char *acl_text;

    aclText_ = "";

    if (stat(file, &st) != 0)
    {
        PLOGE("%s: %s\n", file, strerror(errno));
        return false;
    }

    acl = acl_get_file(file, ACL_TYPE_ACCESS);
    if (acl == NULL)
    {
        PLOGE("getting acl of %s: %s\n", file, strerror(errno));
        return false;
    }
    acl_text = acl_to_text(acl, NULL);
    acl_free(acl);

    if (acl_text)
    {
        aclText_ = acl_text;
    }

    *owner_uid = st.st_uid;
    *owner_gid = st.st_gid;

    if (acl_text)
    {
        logAclText(acl_text);
    }

    return true;
}

bool CameraDacPolicy::setAcl(const char *rule, const char *file)
{
    acl_t acl;

    acl = acl_from_text(rule);
    if (acl == NULL)
    {
        PLOGE("%s: %s\n", rule, strerror(errno));
        return false;
    }
    if (acl_valid(acl) != 0)
    {
        PLOGE("`%s': invalid/incomplete acl\n", rule);
        acl_free(acl);
        return false;
    }

    if (acl_set_file(file, ACL_TYPE_ACCESS, acl) != 0)
    {
        PLOGE("setting acl of %s: %s\n", file, strerror(errno));
        acl_free(acl);
        return false;
    }

    acl_free(acl);
    PLOGI("acl_set_file done\n");
    return true;
}

bool CameraDacPolicy::getCurrentRule(const char *dir, AclRule &rule)
{
    if (dir == nullptr)
    {
        PLOGI("dir is nullptr");
        return false;
    }

    rule.init();

    if (!getAcl(dir, &rule.owner_uid, &rule.owner_gid))
    {
        return false;
    }

    rule.dir = dir;

    std::vector<std::string> lines = split(aclText_, '\n');
    for (unsigned long i = 0; i < lines.size(); i++)
    {
        std::vector<std::string> tokens = split(lines[i], ':');
        if (tokens.size() == 3)
        {
            if (tokens[0] == "user")
            {
                if (tokens[1].empty())
                {
                    rule.base_user_perm = tokens[2];
                }
                else
                {
                    rule.acl_user_perms.push_back({tokens[1], tokens[2]});
                }
            }
            else if (tokens[0] == "group")
            {
                if (tokens[1].empty())
                {
                    rule.base_group_perm = tokens[2];
                }
                else
                {
                    rule.acl_group_perms.push_back({tokens[1], tokens[2]});
                }
            }
            else if (tokens[0] == "mask")
            {
                if (tokens[1].empty())
                {
                    rule.mask = tokens[2];
                }
            }
            else if (tokens[0] == "other")
            {
                if (tokens[1].empty())
                {
                    rule.other_perm = tokens[2];
                }
            }
        }
    }

    rule.log();

    return true;
}

void CameraDacPolicy::createTextRule(const AclRule &r, std::string &text)
{
    text = "";
    if (!r.base_user_perm.empty())
    {
        text += "user::" + r.base_user_perm;
    }
    if (!text.empty())
    {
        for (unsigned long i = 0; i < r.acl_user_perms.size(); i++)
        {
            text += ",user:" + r.acl_user_perms[i].uname + ":" + r.acl_user_perms[i].perm;
        }
        if (!r.base_group_perm.empty())
        {
            text += ",group::" + r.base_group_perm;
        }
        for (unsigned long i = 0; i < r.acl_group_perms.size(); i++)
        {
            text += ",group:" + r.acl_group_perms[i].uname + ":" + r.acl_group_perms[i].perm;
        }
        if (!r.mask.empty())
        {
            text += ",mask::" + r.mask;
        }
        if (!r.other_perm.empty())
        {
            text += ",other::" + r.other_perm;
        }
    }
}

void CameraDacPolicy::createCaptureTextRule(int uid, std::string &text)
{
    text = "user::rwx,user:" + std::to_string(uid) + ":rwx,group::---,mask::rwx,other::---";
}

bool CameraDacPolicy::prepare(int uid)
{
    const char *uname = getUsername(uid);
    if (uname == nullptr)
    {
        return false;
    }
    std::string str_uname = uname;

    auto it = std::find_if(dcim_rule_.acl_user_perms.begin(), dcim_rule_.acl_user_perms.end(),
                           [=](const AclEntry &p) { return p.uname == str_uname; });
    if (it == dcim_rule_.acl_user_perms.end())
    {
        // Add ACL for this new client.
        dcim_rule_.acl_user_perms.push_back({uname, "r-x"});
        if (dcim_rule_.mask.empty())
        {
            dcim_rule_.mask = "r-x";
        }
    }
    else
    {
        if (it->perm != "r-x")
        {
            it->perm = "r-x";
        }
    }

    it = std::find_if(dcim_rule_.acl_group_perms.begin(), dcim_rule_.acl_group_perms.end(),
                      [=](const AclEntry &p) { return p.uname == str_uname; });
    if (it != dcim_rule_.acl_group_perms.end())
    {
        // we apply DAC not using ACL_GROUP but using ACL_USER entry.
        dcim_rule_.acl_group_perms.erase(it);
        if (dcim_rule_.mask.empty())
        {
            dcim_rule_.mask = "r-x";
        }
    }

    it = std::find_if(camera_rule_.acl_user_perms.begin(), camera_rule_.acl_user_perms.end(),
                      [=](const AclEntry &p) { return p.uname == str_uname; });
    if (it == camera_rule_.acl_user_perms.end())
    {
        camera_rule_.acl_user_perms.push_back({uname, "r-x"});
        if (camera_rule_.mask.empty())
        {
            camera_rule_.mask = "r-x";
        }
    }
    else
    {
        if (it->perm != "r-x")
        {
            it->perm = "r-x";
        }
    }

    it = std::find_if(camera_rule_.acl_group_perms.begin(), camera_rule_.acl_group_perms.end(),
                      [=](const AclEntry &p) { return p.uname == str_uname; });
    if (it != camera_rule_.acl_group_perms.end())
    {
        // we apply DAC using ACL_USER entry only.
        camera_rule_.acl_group_perms.erase(it);
        if (camera_rule_.mask.empty())
        {
            camera_rule_.mask = "r-x";
        }
    }

    PLOGI("prepare : DCIM rule updated as:\n");
    dcim_rule_.log();

    PLOGI("prepare : Camera rule updated as:\n");
    camera_rule_.log();

    return true;
}

bool CameraDacPolicy::apply(int uid)
{
    if (!getCurrentRule(str_dcim_dir.c_str(), dcim_rule_))
    {
        return false;
    }

    if (!getCurrentRule(str_camera_dir.c_str(), camera_rule_))
    {
        return false;
    }

    if (!prepare(uid))
    {
        return false;
    }

    std::string str_uid         = std::to_string(uid);
    std::string str_capture_dir = str_camera_dir + "/" + str_uid;

    if (mkdir(str_capture_dir.c_str(), 0700) != 0)
    {
        if (errno != EEXIST)
        {
            return false;
        }
    }

    std::string dcim_rule_text;
    createTextRule(dcim_rule_, dcim_rule_text);
    PLOGI("DCIM rule = %s", dcim_rule_text.c_str());
    setAcl(dcim_rule_text.c_str(), str_dcim_dir.c_str());

    std::string camera_rule_text;
    createTextRule(camera_rule_, camera_rule_text);
    PLOGI("DCIM/Camera rule = %s", camera_rule_text.c_str());
    setAcl(camera_rule_text.c_str(), str_camera_dir.c_str());

    std::string capture_rule_text;
    createCaptureTextRule(uid, capture_rule_text);
    PLOGI("DCIM/Camera/%d rule = %s", uid, capture_rule_text.c_str());
    setAcl(capture_rule_text.c_str(), str_capture_dir.c_str());

    int dum_uid = -1, dum_gid = -1;
    getAcl(str_dcim_dir.c_str(), &dum_uid, &dum_gid);
    getAcl(str_camera_dir.c_str(), &dum_uid, &dum_gid);
    getAcl(str_capture_dir.c_str(), &dum_uid, &dum_gid);

    return true;
}

bool CameraDacPolicy::checkCredential(int uid, std::string &path)
{
    if (uid == -1 && (path.empty() || strstr(path.c_str(), "/tmp")))
    {
        PLOGI("DAC will not be installed to the capture path");
        return true;
    }

    if (!getUsername(uid))
    {
        PLOGE("Invalid uid: %d", uid);
        return false;
    }

    std::string str_uid = std::to_string(uid);

    PLOGI("input path: %s", path.c_str());

    if (strstr(path.c_str(), str_camera_dir.c_str()))
    {
        if (path == str_camera_dir)
        {
            path += "/" + str_uid + "/";
            PLOGI("Result path: %s", path.c_str());
            return true;
        }
        else if (path == (str_camera_dir + "/"))
        {
            path += str_uid + "/";
            PLOGI("Result path: %s", path.c_str());
            return true;
        }
        else
        {
            std::vector<std::string> path_hierarchy = split(path, '/');
            if (path_hierarchy.size() == 7)
            // path should be /media/internal/DCIM/Camera/<uid>/<somefilename with extension>
            {
                if (path_hierarchy[4] == "Camera" && path_hierarchy[5] == std::to_string(uid))
                {
                    PLOGI("Result path: %s", path.c_str());
                    return true;
                }
            }
        }
    }

    return false;
}