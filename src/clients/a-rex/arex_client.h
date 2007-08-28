// arex_client.h
// A very simple client class for the A-REX service.

#ifndef __AREX_CLIENT__
#define __AREX_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include "../../libs/common/ArcConfig.h"
#include "../../libs/common/Logger.h"
#include "../../libs/common/XMLNode.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPEnvelope.h"
#include "../../hed/libs/message/PayloadSOAP.h"

// TODO: Provide documentation!!!

namespace Arc {

  class AREXClientError : public std::runtime_error {
  public:
    AREXClientError(const std::string& what);
  };

  class AREXClient {
  public:
    AREXClient(std::string configFile="") throw(AREXClientError);
    ~AREXClient();
    
    std::string submit(std::istream& jsdl_file) throw(AREXClientError);
    std::string stat(const std::string& jobid) throw(AREXClientError);
    void kill(const std::string& jobid) throw(AREXClientError);

  private:
    Arc::MCC* client_entry;
    Arc::NS arex_ns;
    static Arc::Logger logger;
  };

}

#endif
