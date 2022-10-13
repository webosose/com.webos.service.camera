#ifndef ADDON_SINGLETON_PLUGIN_IMPLEMENTATION_TEMPLATE_H_
#define ADDON_SINGLETON_PLUGIN_IMPLEMENTATION_TEMPLATE_H_

template <typename T>
class TemplateSingleton
{
protected:
    TemplateSingleton()
    {
    }
    virtual ~TemplateSingleton()
    {
    }
public:
    static T * getInstance()
    {
        static T instance;
        return &instance;
    }
};

#endif /* ADDON_SINGLETON_PLUGIN_IMPLEMENTATION_TEMPLATE_H_ */
