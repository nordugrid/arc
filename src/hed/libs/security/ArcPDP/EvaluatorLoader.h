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
///EvaluatorLoader is implemented as a helper class for loading different Evaluator objects, like ArcEvaluator
/**The object loading is based on the configuration information about evaluator, including information for 
factory class, request, policy and evaluator itself */
class EvaluatorLoader {
 public:
  EvaluatorLoader();
  /**Get evaluator object according to the class name*/
  Evaluator* getEvaluator(const std::string& classname);
  /**Get request object according to the class name, based on the request source*/
  Request* getRequest(const std::string& classname, const Source& requestsource);
  /**Get policy object according to the class name, based on the policy source*/
  Policy* getPolicy(const std::string& classname, const Source& policysource);
  /**Get proper policy object according to the policy source*/
  Policy* getPolicy(const Source& policysource);
 protected:
  static Arc::Logger logger;
 private:
  /**configuration information for loading objects; there could 
  be more than one suits of configuration*/
  std::list<Arc::XMLNode> class_config_list_;
};

} //namespace ArcSec

#endif /* __ARCSEC_EVALUATORLOADER_H__ */

