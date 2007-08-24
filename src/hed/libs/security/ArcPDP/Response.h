#ifndef __ARC_RESPONSE_H__
#define __ARC_RESPONSE_H__

#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"
#include "EvaluationCtx.h"
#include "./policy/Policy.h"

/** Class for Response*/

namespace Arc {

typedef std::list<Arc::Policy*> Policies;
typedef std::pair<Arc::RequestTuple, Policies> ResponseItem;
typedef std::map<Arc::RequestTuple, Policies> ResponseList;

/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Response {
protected:
  ResponseList rlist;
public:
  virtual ResponseList getResponseItems () const {};
  virtual void setResponseItems (const ResponseList rl) { rlist = rl;};
  virtual addResponseItem(const ResponseItem item) { rlist.insert(item); };

  Response () {};

  virtual ~Response();
};

} // namespace Arc

#endif /* __ARC_RESPONSE_H__ */
