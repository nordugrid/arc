#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/Service.h>

class TestService: public Arc::Service {
    public:
        TestService(Arc::Config *cfg);
        virtual ~TestService(void);
        virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&);
};

TestService::TestService(Arc::Config* cfg):Arc::Service(cfg) {
}

TestService::~TestService() {
}

Arc::MCC_Status TestService::process(Arc::Message&,Arc::Message&) {
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* servicearg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!servicearg) return NULL;
    return new TestService((Arc::Config*)(*servicearg));
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "testservice", "HED:SERVICE", 0, &get_service },
    { NULL, NULL, 0, NULL }
};

