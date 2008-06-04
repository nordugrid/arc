#ifndef __ARCSEC_EVALUATORLOADER_H__
#define __ARCSEC_EVALUATORLOADER_H__

#include <list>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/Source.h>

namespace ArcSec {
//EvaluatorLoader is implemented as a helper class for loading different Evaluator objects, like ArcEvaluator
class EvaluatorLoader {
 public:
  EvaluatorLoader();
  Evaluator* getEvaluator(const std::string& classname);
  Request* getRequest(const std::string& classname, const Source& requestsource);
  Policy* getPolicy(const std::string& classname, const Source& policysource);
 protected:
  static Arc::Logger logger;
 private:
  std::list<Arc::XMLNode> class_config_list_;
};

} //namespace ArcSec

#endif /* __ARCSEC_EVALUATORLOADER_H__ */

