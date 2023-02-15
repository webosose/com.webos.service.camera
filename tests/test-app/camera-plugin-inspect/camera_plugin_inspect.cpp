#include <iostream>
#include <plugin_factory.hpp>
#include <plugin_interface.hpp>

class PluginScanner : public PluginRegistry
{
public:
    PluginScanner(void) {}
    virtual ~PluginScanner(void) {}

public:
    bool scan(void)
    {
        bool ret = true;

        try
        {
            for (const auto &e : std::filesystem::directory_iterator(getPluginDir()))
            {
                if (!e.is_regular_file())
                    continue;
                if (e.is_symlink())
                    continue;

                std::string strFileName = e.path().string();
                const char *path        = strFileName.c_str();
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
                    dlclose(handle);
                    handle = nullptr;
                    continue;
                }

                auto pPlugin = plugin_init();

                lstPluginInfo_.emplace_back(std::make_unique<PluginInfo>(pPlugin, strFileName));

                delete pPlugin;
                dlclose(handle);
                handle = nullptr;
            }
        }
        catch (std::exception &e)
        {
            std::cout << "error : " << e.what() << std::endl;
            ret = false;
        }
        return ret;
    }

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
        json j   = lstPluginInfo_;
        std::cout << std::setw(4) << j << std::endl;
        return ret;
    }

    bool dumpJsonFile(void)
    {
        bool ret = false;
        std::ofstream ofs;
        ofs.open(getRegistryFilePath(), std::ios_base::out);
        json j = lstPluginInfo_;
        ofs << std::setw(4) << j << std::endl;
        ofs.close();
        return ret;
    }
};

int main(int argc, char *argv[])
{
    PluginScanner scan;
    scan.scan();
    scan.dump();
    scan.dumpJson();
    scan.dumpJsonFile();
    return 0;
}
