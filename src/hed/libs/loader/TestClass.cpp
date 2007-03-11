#include "TestClass.h"
#include "Plugin.h"

namespace TestPlugin {

TestClass::TestClass(void)
{
	a = 1;
}

TestClass::~TestClass(void) 
{

}

void TestClass::testfunc(int b)
{
	a = b;
}

void *init(void) 
{
    return new TestClass();
}

}; // namespace TestClass

/* MCC plugin descriptor */
mcc_descriptor descriptor = {
    0,                  /* version */
    TestPlugin::init    /* init function */
};
