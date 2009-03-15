// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

#include "DataPointFile.h"
#include "DMCFile.h"

namespace Arc {

  Logger DMCFile::logger(DMC::logger, "File");

  DMCFile::DMCFile(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCFile::~DMCFile() {
    Unregister(this);
  }

  Plugin* DMCFile::Instance(PluginArgument *arg) {
    Arc::DMCPluginArgument *dmcarg =
      arg ? dynamic_cast<Arc::DMCPluginArgument*>(arg) : NULL;
    if (!dmcarg)
      return NULL;
    return new DMCFile((Arc::Config*)(*dmcarg));
  }

  DataPoint* DMCFile::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "file")
      return NULL;
    return new DataPointFile(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "file", "HED:DMC", 0, &Arc::DMCFile::Instance },
  { NULL, NULL, 0, NULL }
};
