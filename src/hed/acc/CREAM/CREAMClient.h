// cream_client.h

#ifndef __CREAM_CLIENT__
#define __CREAM_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/URL.h>
#include <arc/client/JobDescription.h>

#include <arc/data/DMC.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataCache.h>
#include <arc/data/URLMap.h>

namespace Arc{
    namespace Cream{
    
        struct creamJobInfo {
            std::string jobId;
            std::string creamURL;
            std::string ISB_URI;
            std::string OSB_URI;
        };

        class CREAMClient {
            public:
	  CREAMClient(const URL& url,const MCCConfig& cfg);
                ~CREAMClient();
                void setDelegationId(const std::string& delegId) { this->delegationId = delegId; };
                bool createDelegation(const std::string& delegation_id);
                bool destroyDelegation(const std::string& delegation_id);
                bool submit(const std::string& jsdl_text, creamJobInfo& info);
                bool stat(const std::string& jobid, std::string& status);
                bool cancel(const std::string& jobid);
                bool purge(const std::string& jobid);
                
                // Data moving attributes
                std::string job_root;
                
            private:
                ClientSOAP* client;
                NS cream_ns;
                std::string proxyPath;
                std::string delegationId;
                static Logger logger;
                

                bool registerJob(const std::string& jdl_text, creamJobInfo& info);
                bool startJob(const std::string& jobid);
                void putFiles(const std::vector< std::pair< std::string, std::string > >& fileList, const creamJobInfo job);
                
        };
    } // namespace cream
} // namespace arc
#endif
