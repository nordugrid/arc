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

    Arc::MCCConfig cfg;
    Arc::ClientHTTP client(cfg, url.Host(), url.Port(),
			   url.Protocol() == "https", url.str());

    Arc::PayloadRaw request;
    Arc::PayloadRawInterface* response;

    int code;
    std::string reason;
    client.process("GET", &request, &code, reason, &response);

    std::list<FileInfo>::iterator f = files.insert(files.end(), url.Path());
    f->SetType(FileInfo::file_type_file);
    f->SetSize(response->Size());
    // need to get date somehow ...

    return true;
  }

} // namespace Arc
