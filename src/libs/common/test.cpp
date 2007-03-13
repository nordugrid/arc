#include "ArcConfig.h"

int main(void)
{
    Arc::Config *config = new Arc::Config("test.xml");
    config->print();
}
