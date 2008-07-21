#ifndef __ARC_IIC_H__
#define __ARC_IIC_H__

#include <string>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>

namespace Arc
{

class IIClient
{
    private:
        std::string url;
        Arc::NS ns;
        Arc::MCCConfig cfg;
        Arc::ClientSOAP *cli;
        Arc::Logger logger;

    public:
        IIClient(std::string &url);
        ~IIClient(void);
        Arc::MCC_Status Query(Arc::XMLNode &req, Arc::XMLNode *resp);
}; // class IIClient

} // namespace Arc

#endif // __ARC_IIC_H__ 
