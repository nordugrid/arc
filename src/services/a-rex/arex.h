#ifndef __ARC_AREX_H__
#define __ARC_AREX_H__

#include "../../hed/libs/message/Service.h"

namespace ARex {

class ARexService: public Arc::Service {
 protected:
  Arc::XMLNode::NS ns_;
  Arc::MCC_Status CreateActivity(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetActivityStatuses(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status TerminateActivities(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetActivityDocuments(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetFactoryAttributesDocument(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status StopAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status StartAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status ChangeActivityStatus(Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status make_fault(Arc::Message& outmsg);
 public:
  ARexService(Arc::Config *cfg);
  virtual ~ARexService(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
};

}; // namespace ARex

#endif

