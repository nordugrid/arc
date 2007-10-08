#ifndef __ARC_SEC_REQUEST_H__
#define __ARC_SEC_REQUEST_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "RequestItem.h"

/** Basic class for Request*/

namespace ArcSec {

typedef std::list<RequestItem*> ReqItemList;

typedef struct{
  std::string value;
  std::string type;
} Attr;
typedef std::list<Attr> Attrs;


/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Request {
protected:
  ReqItemList rlist;
public:
  virtual ReqItemList getRequestItems () const = 0;
  virtual void setRequestItems (ReqItemList sl) = 0;

  //**add request tuple from non-XMLNode*/
  virtual void addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx)=0;

  //**Constructor: Parse request information from a input file*/
  Request (const std::string&) {};

  //**Constructor: Parse request information from a xml stucture in memory*/
  Request (const Arc::XMLNode&) {};
  virtual ~Request(){};
};

} // namespace ArcSec

#endif /* __ARC_SEC_REQUEST_H__ */
