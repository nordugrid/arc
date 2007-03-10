#ifndef __ARC_TESTPLUGIN_H__
#define __ARC_TESTPLUGIN_H__

namespace TestPlugin {
    public:
        TestPlugin();
        ~TestPlugin();
        void test(void);
    private:
        int a;
        void _test(void);
}; // namespace TestPlugin

#endif /* __ARC_TESTPLUGIN_H__ */
