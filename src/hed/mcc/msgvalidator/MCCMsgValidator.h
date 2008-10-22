#ifndef __ARC_MCCMSGVALIDATOR_H__
#define __ARC_MCCMSGVALIDATOR_H__

#include <arc/message/MCC.h>

namespace Arc {

  // This is a base class for Message Validator client and service MCCs. 

  class MCC_MsgValidator : public MCC {
  public:
    MCC_MsgValidator(Arc::Config *cfg);
  protected:
    static Arc::Logger logger;
    std::map<std::string,std::string> schemas;

    bool validateMessage(Message&,std::string);
    std::string getSchemaPath(std::string serviceName);
  };

/* This MCC validates messages against XML schemas. 
   It accepts and produces (i.e. inmsg/outmsg) PayloadSOAP 
   kind of payloads in it's process() method. */
class MCC_MsgValidator_Service: public MCC_MsgValidator
{
    public:
        /* Constructor takes configuration of MCC. */
        MCC_MsgValidator_Service(Arc::Config *cfg);
        virtual ~MCC_MsgValidator_Service(void);
        virtual MCC_Status process(Message&,Message&);
    private:
        static std::string getPath(std::string url);
};

} // namespace Arc

#endif /* __ARC_MCCMSGVALIDATOR_H__ */
