#ifndef __ARC_TESTPLUGIN_H__
#define __ARC_TESTPLUGIN_H__

namespace TestPlugin {

class TestClass 
{
    public:
        TestClass(void);
        ~TestClass(void);
        int a;
        void testfunc(int b);
};

}; // namespace TestPlugin

#endif /* __ARC_TESTPLUGIN_H__ */
