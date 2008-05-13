// cream_client.h

#ifndef __CREAM_CLIENT__
#define __CREAM_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/misc/ClientInterface.h>
#include <arc/URL.h>

namespace Arc{
    namespace Cream{
        class CREAMClientError : public std::runtime_error {
            public:
              CREAMClientError(const std::string& what="");
        };

        class CREAMFile {
            public:
              std::string remote;
              std::string local;
              CREAMFile(void) { };
              CREAMFile(const std::string& remote_,const std::string& local_):remote(remote_),local(local_) { };
        };

        typedef std::list<CREAMFile> CREAMFileList;
      
        class CREAMClient {
            public:
          
              CREAMClient(std::string configFile="") throw(CREAMClientError);
              CREAMClient(const Arc::URL& url,const Arc::MCCConfig& cfg) throw(CREAMClientError);
              ~CREAMClient();
            
              std::string submit(std::istream& jsdl_file,CREAMFileList& file_list,bool delegate = false) throw(CREAMClientError);
              std::string stat(const std::string& jobid) throw(CREAMClientError); // OK
              void cancel(const std::string& jobid) throw(CREAMClientError); // OK
              void purge(const std::string& jobid) throw(CREAMClientError); // OK
              Arc::ClientSOAP* SOAP(void) { return client; };
            private:
              Arc::Config* client_config;
              Arc::Loader* client_loader;    
              Arc::ClientSOAP* client;
              Arc::MCC* client_entry;
              Arc::NS cream_ns;
              static Arc::Logger logger;
        };
    } // namespace cream
} // namespace arc
#endif