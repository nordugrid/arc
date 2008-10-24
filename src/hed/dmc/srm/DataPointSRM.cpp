#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/fileutils.h>
#include <glibmm/thread.h>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataCallback.h>

#include "DataPointSRM.h"

namespace Arc {

  Logger DataPointSRM::logger(DataPoint::logger, "SRM");

  DataPointSRM::DataPointSRM(const URL& url)
    : DataPointDirect(url) {}

  DataPointSRM::~DataPointSRM() {}

  DataStatus DataPointSRM::Check() {}

  DataStatus DataPointSRM::Remove() {}

  DataStatus DataPointSRM::StartReading(DataBufferPar& buf) {}

  DataStatus DataPointSRM::StopReading() {}

  DataStatus DataPointSRM::StartWriting(DataBufferPar& buf,
					DataCallback *space_cb) {}

  DataStatus DataPointSRM::StopWriting() {}

  DataStatus DataPointSRM::ListFiles(std::list<FileInfo>& files,
				     bool long_list,
				     bool resolve) {}

} // namespace Arc
