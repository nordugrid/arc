// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "JobControllerPluginARC0.h"
#include "SubmitterPluginARC0.h"

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "ARC0", "HED:SubmitterPlugin", "ARCs classic Grid Manager", 0, &Arc::SubmitterPluginARC0::Instance },
  { "ARC0", "HED:JobControllerPlugin", "ARCs classic Grid Manager", 0, &Arc::JobControllerPluginARC0::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
    if(manager && module) {
      manager->makePersistent(module);
    };
  }
}

