#ifndef __ARC_MCCMSGVALIDATOR_H__
#define __ARC_MCCMSGVALIDATOR_H__

#include <arc/message/MCC.h>

namespace Arc {

  // This is a base class for Message Validator client and service MCCs. 

  class MCC_MSGVALIDATOR : public MCC {
  public:
    MCC_MSGVALIDATOR(Arc::Config *cfg);
  protected:
    static Arc::Logger logger;
    std::map<std::string,std::string> schemas;

    bool validateMessage(Message&,std::string);
    std::string getSchemaPath(std::string serviceName);
  };

// This MCC validates messages against XML schemas.
class MCC_MSGVALIDATOR_Service: public MCC_MSGVALIDATOR
{
    public:
        /* Constructor takes configuration of MCC. */
        MCC_MSGVALIDATOR_Service(Arc::Config *cfg);
        virtual ~MCC_MSGVALIDATOR_Service(void);
        virtual MCC_Status process(Message&,Message&);
    private:
        static std::string getPath(std::string url);
};

/* This is client side of MSGVALIDATOR processing MCC.
   It accepts and produces PayloadSOAP kind of payloads in it's
   process() method. Communication to next MCC is done over payloads
   implementing PayloadRawInterface. */
class MCC_MSGVALIDATOR_Client: public MCC_MSGVALIDATOR
{
    public:
        /* Constructor takes configuration of MCC. */
        MCC_MSGVALIDATOR_Client(Arc::Config *cfg);
        virtual ~MCC_MSGVALIDATOR_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCMSGVALIDATOR_H__ */
