// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "JobControllerPluginGRIDFTPJOB.h"
#include "SubmitterPluginGRIDFTPJOB.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "GRIDFTPJOB", "HED:SubmitterPlugin", "ARCs classic Grid Manager", 0, &Arc::SubmitterPluginGRIDFTPJOB::Instance },
  { "GRIDFTPJOB", "HED:JobControllerPlugin", "ARCs classic Grid Manager", 0, &Arc::JobControllerPluginGRIDFTPJOB::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
    if(manager && module) {
      manager->makePersistent(module);
    };
  }
}

