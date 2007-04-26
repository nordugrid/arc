#ifndef __ARC_MCCSOAP_H__
#define __ARC_MCCSOAP_H__

#include "../../libs/message/MCC.h"

namespace Arc {

/** This MCC processes parsed SOAP message from input payload.
  On input payload with PayloadRawInterface is expected. It's
 converted into PayloadSOAP and passed next MCC. Returned 
 PayloadSOAP is converted into PayloadRaw and returned to calling
 MCC. */
class MCC_SOAP_Service: public MCC
{
    public:
        /* Constructor takes configuration of MCC. 
         Currently there are no configuration parameters for this MCC */
        MCC_SOAP_Service(Arc::Config *cfg);
        virtual ~MCC_SOAP_Service(void);
        virtual MCC_Status process(Message&,Message&);
};

/* This is client side of SOAP processing MCC.
  It accepts and produces PayloadSOAP kind of payloads in it's
 process() method. Comminication to next MCC is done over payloads
 implementing PayloadRawInterface. */
class MCC_SOAP_Client: public MCC
{
    public:
        /* Constructor takes configuration of MCC. 
         Currently there are no configuration parameters for this MCC */
        MCC_SOAP_Client(Arc::Config *cfg);
        virtual ~MCC_SOAP_Client(void);
        virtual MCC_Status process(Message&,Message&);
};

} // namespace Arc

#endif /* __ARC_MCCSOAP_H__ */
