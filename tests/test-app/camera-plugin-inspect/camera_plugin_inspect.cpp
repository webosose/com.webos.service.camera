#include <glib.h>
#include <iostream>
#include <plugin_factory.hpp>
#include <plugin_interface.hpp>

class PluginScanner : public PluginRegistry
{
public:
    PluginScanner(void) {}
    virtual ~PluginScanner(void) {}

public:
    bool scanPluginPath(void)
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

    std::string findPluginNameHaveFeature(const char *szFeatureName)
    {
        for (auto &p : lstPluginInfo_)
        {
            auto it = std::find_if(std::begin(p->lstFeatures_), std::end(p->lstFeatures_),
                                   [&szFeatureName](const auto &f) { return szFeatureName == f; });
            if (it != std::end(p->lstFeatures_))
                return p->strName_;
        }
        return "";
    }

    std::string getCategory(const char *szFeatureName)
    {
        for (auto &p : lstPluginInfo_)
        {
            auto it = std::find_if(std::begin(p->lstFeatures_), std::end(p->lstFeatures_),
                                   [&szFeatureName](const auto &f) { return szFeatureName == f; });
            if (it != std::end(p->lstFeatures_))
                return p->strCategory_;
        }
        return "";
    }

    std::vector<std::string> getFeatureList(void)
    {
        std::vector<std::string> vecFeatures;
        for (auto &p : lstPluginInfo_)
        {
            for (auto &f : p->lstFeatures_)
            {
                vecFeatures.emplace_back(f);
            }
        }
        return vecFeatures;
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
        std::cout << "Check : " << g_path_get_dirname(getRegistryFilePath()) << std::endl;
        if (!g_file_test(g_path_get_dirname(getRegistryFilePath()), G_FILE_TEST_IS_DIR))
        {
            std::cout << "Make Path : " << g_path_get_dirname(getRegistryFilePath()) << std::endl;
            g_mkdir_with_parents(g_path_get_dirname(getRegistryFilePath()), 0755);
        }

        std::ofstream ofs;
        ofs.open(getRegistryFilePath(), std::ios_base::out);
        json j = lstPluginInfo_;
        ofs << std::setw(4) << j << std::endl;
        ofs.flush();
        ofs.close();
        ret = true;

        return ret;
    }
};

struct OptionParser
{
    gboolean bScanPluginPath_{TRUE};
    gboolean bReadRegistry_{FALSE};
    gboolean bVerifyFeature_{FALSE};
    gchar **vszFeatureNames_{NULL};
    guint nFeatureNames_{0};

public:
    OptionParser(void) {}
    ~OptionParser(void) {}

public:
    void parse(int &argc, char **&argv)
    {
        GError *err            = NULL;
        GOptionEntry options[] = {
            {"read_registry", 0, 0, G_OPTION_ARG_NONE, &bReadRegistry_,
             "Read informations from camera plugin registry file.", NULL},
            {"verify_feature", 0, 0, G_OPTION_ARG_NONE, &bVerifyFeature_,
             "Verify features are running properly.", NULL},
            {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &vszFeatureNames_, NULL},
            {NULL}};

        g_set_prgname("camera-plugin-inspect");

        auto ctx = g_option_context_new("[feature name]");
        g_option_context_add_main_entries(ctx, options, NULL);
        if (!g_option_context_parse(ctx, &argc, &argv, &err))
        {
            g_print("Error initializing: %s\n", err->message ? err->message : "(NULL)");
            g_clear_error(&err);
            g_option_context_free(ctx);
            return;
        }
        g_option_context_free(ctx);

        if (vszFeatureNames_ != nullptr)
            nFeatureNames_ = g_strv_length(vszFeatureNames_);

        if (bReadRegistry_ == TRUE)
            bScanPluginPath_ = FALSE;
    }
};

void verifyFeature(PluginFactory &fact, const char *szFeatureName)
{
    std::cout << "====" << std::endl;

    auto pFeature    = fact.createFeature(szFeatureName);
    void *pInterface = nullptr;

    if (pFeature == nullptr)
    {
        std::cout << "Feature " << szFeatureName << " is not available." << std::endl;
        return;
    }

    std::cout << "[TEST #1] Interface Query for " << szFeatureName;
    pFeature->queryInterface(szFeatureName, &pInterface);
    if (pInterface != nullptr)
        std::cout << " - OK" << std::endl;
    else
        std::cout << " - FAIL" << std::endl;

    std::cout << "[TEST #2] Unload Feature for " << szFeatureName << std::endl;
}

int main(int argc, char *argv[])
{
    OptionParser opt;
    opt.parse(argc, argv);

    PluginScanner scan;

    if (opt.bScanPluginPath_)
    {
        scan.scanPluginPath();
        scan.dumpJsonFile();
    }
    else
    {
        scan.readPluginRegistry();
    }

    if (opt.nFeatureNames_ == 0)
    {
        if (!opt.bVerifyFeature_)
        {
            scan.dump();
            return 0;
        }

        std::vector<std::string> vecFeatures = scan.getFeatureList();

        PluginFactory fact;
        for (auto &feature : vecFeatures)
            verifyFeature(fact, feature.c_str());
        return 0;
    }

    if (opt.bVerifyFeature_)
    {
        PluginFactory fact;
        for (char **f = opt.vszFeatureNames_; *f != NULL; f++)
            verifyFeature(fact, *f);

        return 0;
    }

    for (char **f = opt.vszFeatureNames_; *f != NULL; f++)
    {
        std::string strPluginName = scan.findPluginNameHaveFeature(*f);
        scan.printPluginInfo(strPluginName);
    }

    return 0;
}
