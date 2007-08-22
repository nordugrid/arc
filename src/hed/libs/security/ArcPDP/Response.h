#ifndef __ARC_RESPONSE_H__
#define __ARC_RESPONSE_H__

#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

/** Class for Response*/

namespace Arc {

//typedef std::list<Arc::Policy*> Policies;
typedef std::map<Arc::RequestItem*, Policy*> MatchedItem, ResponseItem;

/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Response {
protect:
  ResponseItemList rlist;
public:
  virtual ResponseItemList getResponseItems () const {};
  virtual void setResponseItems (const ResponseItemList* rl) {};

  Request () {};

  void merge(Response*);

  virtual ~Response();
};

} // namespace Arc

#endif /* __ARC_RESPONSE_H__ */
