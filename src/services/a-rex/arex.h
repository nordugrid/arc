#ifndef __ARC_AREX_H__
#define __ARC_AREX_H__

#include "message/Service.h"

namespace ARex {

class ARexGMConfig;

class ARexService: public Arc::Service {
 protected:
  Arc::NS ns_;
  Arc::MCC_Status CreateActivity(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetActivityStatuses(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status TerminateActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetActivityDocuments(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status GetFactoryAttributesDocument(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status StopAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status StartAcceptingNewActivities(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status ChangeActivityStatus(ARexGMConfig& config,Arc::XMLNode& in,Arc::XMLNode& out);
  Arc::MCC_Status make_fault(Arc::Message& outmsg);
 public:
  ARexService(Arc::Config *cfg);
  virtual ~ARexService(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
};

}; // namespace ARex

#endif

