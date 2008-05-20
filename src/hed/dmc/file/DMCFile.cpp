#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DMCFile.h"
#include "DataPointFile.h"

namespace Arc {

  Logger DMCFile::logger(DMC::logger, "File");

  DMCFile::DMCFile(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCFile::~DMCFile() {
    Unregister(this);
  }

  DMC *DMCFile::Instance(Arc::Config *cfg, Arc::ChainContext *) {
    return new DMCFile(cfg);
  }

  DataPoint *DMCFile::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "file")
      return NULL;
    return new DataPointFile(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  {"file", 0, &Arc::DMCFile::Instance},
  {NULL, 0, NULL}
};
