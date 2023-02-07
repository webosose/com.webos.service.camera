#pragma once
#include <algorithm>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <nlohmann/json.hpp>
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
        strPath_         = strPath;
        strName_         = pPlugin->getName();
        strDescription_  = pPlugin->getDescription();
        strCategory_     = pPlugin->getCategory();
        strVersion_      = pPlugin->getVersion();
        strOrganization_ = pPlugin->getOrganization();
        for (size_t i = 0; i < pPlugin->getFeatureCount(); ++i)
        {
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
    j.at("path").get_to(p.strPath_);
    j.at("name").get_to(p.strName_);
    j.at("description").get_to(p.strDescription_);
    j.at("category").get_to(p.strCategory_);
    j.at("version").get_to(p.strVersion_);
    j.at("organization").get_to(p.strOrganization_);
    j.at("features").get_to(p.lstFeatures_);
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
    bool scan(void)
    {
        bool ret                  = true;
        auto szPluginPath         = std::getenv("PLUGIN_PATH");
        std::string strPluginPath = szPluginPath == nullptr ? "/usr/lib/camera" : szPluginPath;
        try
        {
            for (const auto &e : std::filesystem::directory_iterator(strPluginPath))
            {
                std::string strFileName = e.path().string();
                const char *path        = strFileName.c_str();
                struct stat sb;
                if (stat(path, &sb) == 0 && !(sb.st_mode & S_IFDIR))
                {
                    void *handle{nullptr};
                    std::cout << "PluginRegistry::scan() " << path << std::endl;
                    handle = dlopen(path, RTLD_LAZY);
                    if (!handle)
                    {
                        std::cout << "dlopen() failed: " << dlerror() << std::endl;
                        ret = false;
                        continue;
                    }

                    auto plugin_init = (plugin_entrypoint)dlsym(handle, "plugin_init");
                    if (!plugin_init)
                    {
                        std::cout << "dlsym() failed @1: " << dlerror() << std::endl;
                        ret = false;
                        continue;
                    }

                    auto pPlugin = plugin_init();

                    lstPluginInfo_.emplace_back(std::make_unique<PluginInfo>(pPlugin, strFileName));

                    delete pPlugin;
                    dlclose(handle);
                    handle = nullptr;
                }
            }
        }
        catch (std::exception &e)
        {
            std::cout << "error : " << e.what() << std::endl;
            ret = false;
        }
        return ret;
    }
    bool dump(void)
    {
        bool ret = false;
        for (auto &p : lstPluginInfo_)
        {
            std::cout << "============================" << std::endl;
            printPluginInfo(p->strName_);
            ret = true;
        }
        return ret;
    }
    bool dumpJson(void)
    {
        bool ret = true;

        std::cout << ">>>> Dump Case #1 : dump from list of plugininfo to json" << std::endl;
        json j = lstPluginInfo_;
        std::cout << std::setw(4) << j << std::endl;

        std::cout << std::endl;
        std::cout << ">>>> Dump Case #2 : dump from json to list of plugininfo" << std::endl;
        PluginInfoLst lst = j;
        for (auto &it : lst)
        {
            std::cout << "Name         : " << it->strName_ << std::endl;
            std::cout << "Description  : " << it->strDescription_ << std::endl;
            std::cout << "Category     : " << it->strCategory_ << std::endl;
            std::cout << "Version      : " << it->strVersion_ << std::endl;
            std::cout << "Organization : " << it->strOrganization_ << std::endl;
            std::cout << "Plugin Path  : " << it->strPath_ << std::endl;
            std::cout << "Number of Featrures (" << it->lstFeatures_.size() << ")" << std::endl;
            for (auto &n : it->lstFeatures_)
            {
                std::cout << "   " << n << std::endl;
            }
        }

        return ret;
    }
    bool dumpJsonFile(void)
    {
        bool ret = false;
        std::ofstream ofs;
        ofs.open("PluginRegistry.json", std::ios_base::out);
        json j = lstPluginInfo_;
        ofs << std::setw(4) << j << std::endl;
        ofs.close();
        return ret;
    }
    const char *findPluiginNameFromFeatureName(const char *szFeatureName)
    {
        for (auto &p : lstPluginInfo_)
        {
            auto it = std::find_if(std::begin(p->lstFeatures_), std::end(p->lstFeatures_),
                                   [&szFeatureName](const auto &f) { return szFeatureName == f; });
            if (it != std::end(p->lstFeatures_))
            {
                return p->strName_.c_str();
            }
        }
        return nullptr;
    }
    const char *getPluginPath(const char *szPluginName)
    {
        auto it =
            std::find_if(std::begin(lstPluginInfo_), std::end(lstPluginInfo_),
                         [&szPluginName](const auto &p) { return szPluginName == p->strName_; });

        if (it != std::end(lstPluginInfo_))
            return it->get()->strPath_.c_str();

        return nullptr;
    }

private:
    void printPluginInfo(std::string &strPluginName)
    {
        auto it =
            std::find_if(std::begin(lstPluginInfo_), std::end(lstPluginInfo_),
                         [&strPluginName](const auto &p) { return strPluginName == p->strName_; });

        if (it != std::end(lstPluginInfo_))
        {
            std::cout << "Name         : " << (*it)->strName_ << std::endl;
            std::cout << "Description  : " << (*it)->strDescription_ << std::endl;
            std::cout << "Category     : " << (*it)->strCategory_ << std::endl;
            std::cout << "Version      : " << (*it)->strVersion_ << std::endl;
            std::cout << "Organization : " << (*it)->strOrganization_ << std::endl;
            std::cout << "Plugin Path  : " << (*it)->strPath_ << std::endl;
            std::cout << "Number of Featrures (" << (*it)->lstFeatures_.size() << ")" << std::endl;
            for (auto &n : (*it)->lstFeatures_)
            {
                std::cout << "   " << n << std::endl;
            }
        }
    }

private:
    PluginInfoLst lstPluginInfo_;
};

class PluginFactory
{
    using PluginRegistryPtr = std::unique_ptr<PluginRegistry>;

public:
    PluginFactory(void)
    {
        pRegirsry_ = PluginRegistryPtr(new PluginRegistry());
        pRegirsry_->scan();
    }
    ~PluginFactory(void) {}

public:
    IFeaturePtr createFeature(const char *szFeatureName)
    {
        void *handle = nullptr;
        auto szName  = pRegirsry_->findPluiginNameFromFeatureName(szFeatureName);
        auto szPath  = pRegirsry_->getPluginPath(szName);

        std::cout << "PluginFactory::createFeature() " << szPath << std::endl;
        handle = dlopen(szPath, RTLD_LAZY);
        if (!handle)
        {
            std::cout << "dlopen() failed: " << dlerror() << std::endl;
            return nullptr;
        }

        auto plugin_init = (plugin_entrypoint)dlsym(handle, "plugin_init");
        if (!plugin_init)
        {
            std::cout << "dlsym() failed @1: " << dlerror() << std::endl;
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

public:
    bool dumpAllPlugins(void) { return pRegirsry_->dump(); }
    bool dumpAllPluginsJson(void) { return pRegirsry_->dumpJson(); }
    bool dumpAllPluginsJsonFile(void) { return pRegirsry_->dumpJsonFile(); }

private:
    PluginRegistryPtr pRegirsry_;
};
