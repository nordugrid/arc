// cream_client.h

#ifndef __CREAM_CLIENT__
#define __CREAM_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <glibmm.h>
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

#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

namespace Arc{
    namespace Cream{
    
        // OpenSSLFunctions    
        time_t ASN1_UTCTIME_get(const ASN1_UTCTIME *s);
        const long getCertTimeLeft( std::string pxfile );
        time_t GRSTasn1TimeToTimeT(char *asn1time, size_t len);
        int makeProxyCert(char **proxychain, char *reqtxt, char *cert, char *key, int minutes);
        std::string checkPath(std::string p);
        std::string getProxy();
        std::string getTrustedCerts();
        
        struct creamJobInfo {
            std::string jobId;
            std::string creamURL;
            std::string ISB_URI;
            std::string OSB_URI;
        };
        
        class CREAMClientError : public std::runtime_error {
            public:
                CREAMClientError(const std::string& what="");
        };

        class CREAMClient {
            public:
                CREAMClient(const Arc::URL& url,const Arc::MCCConfig& cfg) throw(CREAMClientError);
                ~CREAMClient();
                void setDelegationId(std::string delegId) { this->delegationId = delegId; };
                void createDelegation(std::string& delegation_id) throw(CREAMClientError);
                void destroyDelegation(std::string& delegation_id) throw(CREAMClientError);
                creamJobInfo submit(std::string& jsdl_text) throw(CREAMClientError);
                std::string stat(const std::string& jobid) throw(CREAMClientError);
                void cancel(const std::string& jobid) throw(CREAMClientError);
                void purge(const std::string& jobid) throw(CREAMClientError);
                
                // Data moving attributes
                std::string cache_path;
                std::string job_root;
                
            private:
                Arc::ClientSOAP* client;
                Arc::NS cream_ns;
                std::string delegationId;
                static Arc::Logger logger;
                

                creamJobInfo registerJob(std::string& jdl_text) throw(CREAMClientError);
                void startJob(const std::string& jobid) throw(CREAMClientError);
                void putFiles(const std::vector< std::pair< std::string, std::string > >& fileList, const creamJobInfo job) throw(CREAMClientError);
                void getFiles() throw(CREAMClientError);
                
        };
    } // namespace cream
} // namespace arc
#endif
