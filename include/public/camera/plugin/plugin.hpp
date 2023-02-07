#pragma once
#include <algorithm>
#include <map>
#include <plugin_interface.hpp>
#include <string>
#include <vector>

class Plugin : public IPlugin
{
private:
    using FeaturePair = std::pair<std::string, void *(*)(void)>;
    using FeatureList = std::vector<FeaturePair>;

public:
    Plugin() {}
    virtual ~Plugin(void) {}

public:
    void setName(const char *szName) { strName_ = szName; }
    void setDescription(const char *szDescription) { strDescription_ = szDescription; }
    void setCategory(const char *szCategory) { strCategory_ = szCategory; }
    void setVersion(const char *szVersion) { strVersion_ = szVersion; }
    void setOrganization(const char *szOrganization) { strOrganization_ = szOrganization; }
    template <typename T>
    void registerFeature(const char *szName)
    {
        lstFeatures_.push_back(std::make_pair(
            szName, +[](void) -> void * { return new (std::nothrow) T; }));
    }

public:
    virtual const char *getName(void) override { return strName_.c_str(); }
    virtual const char *getDescription(void) override { return strDescription_.c_str(); }
    virtual const char *getCategory(void) override { return strCategory_.c_str(); }
    virtual const char *getVersion(void) override { return strVersion_.c_str(); }
    virtual const char *getOrganization(void) override { return strOrganization_.c_str(); }
    virtual const size_t getFeatureCount(void) override { return lstFeatures_.size(); }
    virtual const char *getFeatureName(const int nIndex) override
    {
        return lstFeatures_[nIndex].first.c_str();
    }
    virtual IFeature *createFeature(const char *szName) override
    {
        auto it = std::find_if(std::begin(lstFeatures_), std::end(lstFeatures_),
                               [&szName](const auto &p) { return p.first == szName; });
        if (it == std::end(lstFeatures_))
            return nullptr;

        return reinterpret_cast<IFeature *>(it->second());
    }

private:
    std::string strName_;
    std::string strDescription_;
    std::string strCategory_;
    std::string strVersion_;
    std::string strOrganization_;
    FeatureList lstFeatures_;
};
