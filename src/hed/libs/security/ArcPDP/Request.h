#ifndef __ARC_REQUEST_H__
#define __ARC_REQUEST_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
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
  virtual ReqItemList getRequestItems () const = 0;
  virtual void setRequestItems (ReqItemList sl) = 0;

  //**Parse request information from a input file*/
  Request (const std::string&) {};

  //**Parse request information from a xml stucture in memory*/
  Request (const Arc::XMLNode&) {};
  virtual ~Request(){};
};

} // namespace Arc

#endif /* __ARC_REQUEST_H__ */
