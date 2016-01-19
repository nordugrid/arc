#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/MCC.h>
/*
#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/SecAttr.h>
#include <arc/loader/Plugin.h>

#include "PayloadHTTP.h"
#include "MCCHTTP.h"
*/

class TestMCC: public Arc::MCC {
    public:
        TestMCC(Arc::Config *cfg, Arc::PluginArgument *parg);
        virtual ~TestMCC(void);
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

TestMCC::TestMCC(Arc::Config* cfg, Arc::PluginArgument *parg):Arc::MCC(cfg,parg) {
}

TestMCC::~TestMCC() {
}

Arc::MCC_Status TestMCC::process(Arc::Message&,Arc::Message&) {
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::Plugin* get_mcc(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new TestMCC((Arc::Config*)(*mccarg),arg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "testmcc", "HED:MCC", NULL, 0, &get_mcc },
    { NULL, NULL, NULL, 0, NULL }
};

