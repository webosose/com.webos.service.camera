#pragma once
#include <algorithm>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <json_utils.h>
#include <list>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <plugin_interface.hpp>
#include <string>
#include <sys/stat.h>

using namespace nlohmann;
struct PluginInfo
{
    using FeatureList = std::list<std::string>;

    PluginInfo(void) {}

    PluginInfo(IPlugin *pPlugin, std::string strPath)
    {
        strPath_ = strPath;
        if (pPlugin->getName() != nullptr)
            strName_ = pPlugin->getName();
        if (pPlugin->getDescription() != nullptr)
            strDescription_ = pPlugin->getDescription();
        if (pPlugin->getCategory() != nullptr)
            strCategory_ = pPlugin->getCategory();
        if (pPlugin->getVersion() != nullptr)
            strVersion_ = pPlugin->getVersion();
        if (pPlugin->getOrganization() != nullptr)
            strOrganization_ = pPlugin->getOrganization();
        for (size_t i = 0; i < pPlugin->getFeatureCount(); ++i)
        {
            if (pPlugin->getFeatureName(i) != nullptr)
                lstFeatures_.emplace_back(pPlugin->getFeatureName(i));
        }
    }

    ~PluginInfo(void) {}
    std::string strPath_;
    std::string strName_;
    std::string strDescription_;
    std::string strCategory_;
    std::string strVersion_;
    std::string strOrganization_;
    FeatureList lstFeatures_;
};

using PluginInfoPtr = std::unique_ptr<PluginInfo>;
using PluginInfoLst = std::list<PluginInfoPtr>;
using IFeaturePtr   = std::unique_ptr<IFeature, std::function<void(IFeature *)>>;

inline void to_json(json &j, const PluginInfo &p)
{
    j = json{{"path", p.strPath_},
             {"name", p.strName_},
             {"description", p.strDescription_},
             {"category", p.strCategory_},
             {"version", p.strVersion_},
             {"organization", p.strOrganization_},
             {"features", p.lstFeatures_}};
}

inline void from_json(const json &j, PluginInfo &p)
{
    p.strPath_         = get_optional<std::string>(j, "path").value_or("");
    p.strName_         = get_optional<std::string>(j, "name").value_or("");
    p.strDescription_  = get_optional<std::string>(j, "description").value_or("");
    p.strCategory_     = get_optional<std::string>(j, "category").value_or("");
    p.strVersion_      = get_optional<std::string>(j, "version").value_or("");
    p.strOrganization_ = get_optional<std::string>(j, "organization").value_or("");
    p.lstFeatures_ =
        get_optional<PluginInfo::FeatureList>(j, "features").value_or(PluginInfo::FeatureList());
}

inline void to_json(json &j, const PluginInfoLst &l)
{
    for (auto &p : l)
    {
        j.push_back(*p);
    }
}

inline void from_json(const json &j, PluginInfoLst &l)
{
    for (auto &p : j)
    {
        l.emplace_back(std::make_unique<PluginInfo>(p));
    }
}

class PluginRegistry
{
public:
    PluginRegistry(void) {}
    virtual ~PluginRegistry(void) {}

public:
    bool readPluginRegistry(void)
    {
        std::ifstream ifs(getRegistryFilePath());
        json jdata = json::parse(ifs, nullptr, false);

        if (jdata.is_discarded())
        {
            std::cout << "registry parsing error : " << getRegistryFilePath() << std::endl;
            return false;
        }

        if (lstPluginInfo_.size() > 0)
            lstPluginInfo_.clear();
        lstPluginInfo_ = jdata;

        return true;
    }

    const char *findPluginPathHaveFeature(const char *szFeatureName)
    {
        for (auto &p : lstPluginInfo_)
        {
            auto it = std::find_if(std::begin(p->lstFeatures_), std::end(p->lstFeatures_),
                                   [&szFeatureName](const auto &f) { return szFeatureName == f; });
            if (it != std::end(p->lstFeatures_))
                return p->strPath_.c_str();
        }
        return nullptr;
    }

    std::vector<std::string> getFeatureList(const char *szCategoryName)
    {
        std::vector<std::string> list;
        if (szCategoryName == nullptr)
            return list;

        for (auto &p : lstPluginInfo_)
        {
            if (p->strCategory_ == szCategoryName)
            {
                for (auto &f : p->lstFeatures_)
                {
                    list.push_back(f);
                }
            }
        }
        return list;
    }

protected:
    const char *getPluginDir(void)
    {
        return getenv("CAMERA_PLUGIN_PATH").value_or("/usr/lib/camera");
    }

    const char *getRegistryFilePath(void)
    {
        return getenv("CAMERA_REGISTRY_PATH")
            .value_or("/usr/lib/camera/camera_plugin_registry.json");
    }

private:
    std::optional<const char *> getenv(const char *szEnv)
    {
        if (std::getenv(szEnv) != nullptr)
            return std::getenv(szEnv);
        else
            return {};
    }

protected:
    PluginInfoLst lstPluginInfo_;
};

class PluginFactory
{
public:
    PluginFactory(void) { oRegirsry_.readPluginRegistry(); }
    ~PluginFactory(void) {}

public:
    IFeaturePtr createFeature(const char *szFeatureName)
    {
        auto szPath = oRegirsry_.findPluginPathHaveFeature(szFeatureName);
        if (szPath == nullptr)
            return nullptr;

        std::cout << "PluginFactory::createFeature() " << szPath << std::endl;
        void *handle = dlopen(szPath, RTLD_LAZY);
        if (!handle)
        {
            char *error{nullptr};
            if ((error = dlerror()) != nullptr)
                std::cout << "dlopen() failed: " << dlerror() << std::endl;
            return nullptr;
        }

        auto plugin_init = (plugin_entrypoint)dlsym(handle, "plugin_init");
        if (!plugin_init)
        {
            char *error{nullptr};
            if ((error = dlerror()) != nullptr)
                std::cout << "dlsym() failed @1: " << dlerror() << std::endl;

            dlclose(handle);
            return nullptr;
        }

        // TODO : To keep the plugin instance any where
        std::unique_ptr<IPlugin> pPlugin(plugin_init());
        auto pFeature = pPlugin->createFeature(szFeatureName);

        return IFeaturePtr(reinterpret_cast<IFeature *>(pFeature),
                           [handle](IFeature *p)
                           {
                               delete p;
                               if (dlclose(handle) != 0)
                                   std::cout << "failed to run dlcose" << std::endl;
                           });
    }

    std::vector<std::string> getFeatureList(const char *szCategoryName)
    {
        return oRegirsry_.getFeatureList(szCategoryName);
    }

private:
    PluginRegistry oRegirsry_;
};
