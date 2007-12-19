#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <arc/Logger.h>
#include <arc/message/PayloadRaw.h>
#include <arc/data/DataBufferPar.h>
#include <arc/misc/ClientInterface.h>
#include "DataPointHTTP.h"

namespace Arc {

  Logger DataPointHTTP::logger(DataPoint::logger, "HTTP");

  DataPointHTTP::DataPointHTTP(const URL& url) : DataPointDirect(url) {}

  DataPointHTTP::~DataPointHTTP() {}

  bool DataPointHTTP::list_files(std::list<FileInfo>& files, bool) {

    MCCConfig cfg;
    ClientHTTP client(cfg, url.Host(), url.Port(),
                      url.Protocol() == "https", url.str());

    PayloadRaw request;
    PayloadRawInterface* response;

    ClientHTTP::Info info;
    client.process("GET", &request, &info, &response);

    std::list<FileInfo>::iterator f = files.insert(files.end(), url.Path());
    f->SetType(FileInfo::file_type_file);
    f->SetSize(info.size);
    f->SetCreated(info.lastModified);

    return true;
  }

  bool DataPointHTTP::start_reading(DataBufferPar& buffer) {
    return false;
  }

  bool DataPointHTTP::start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb) {
    return false;
  }

} // namespace Arc
