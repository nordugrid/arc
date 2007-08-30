#ifndef __ARC_REQUEST_H__
#define __ARC_REQUEST_H__

#include <list>
#include "common/XMLNode.h"
#include "common/Logger.h"
#include "RequestItem.h"

/** Basic class for Request*/

namespace Arc {

typedef std::list<RequestItem*> ReqItemList;

/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Request {
protected:
  ReqItemList rlist;
public:
  virtual ReqItemList getRequestItems () const {};
  virtual void setRequestItems (const ReqItemList* sl) {};

  //**Parse request information from a input file*/
  Request (const std::string& filename) {};

  //**Parse request information from a xml stucture in memory*/
  Request (const Arc::XMLNode& node) {};
  virtual ~Request();
};

} // namespace Arc

#endif /* __ARC_REQUEST_H__ */
