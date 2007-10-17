#ifndef __ARC_SEC_REQUEST_H__
#define __ARC_SEC_REQUEST_H__

#include <arc/loader/LoadableClass.h>

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

//typedef std::map<int,Attr> Attrs;

class Attrs {
public:
  void addItem(Attr attr) { 
    int n = (attrs.size());  
    attrs.insert(std::pair<int, Attr>(n, attr)); 
  };
  int size() { return attrs.size();};
  Attr& getItem(int n) { return attrs[n]; };
  Attr& operator[](int n) { return attrs[n]; };
private:
  std::map<int, Attr> attrs;
};


/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Request : public Arc::LoadableClass {
protected:
  ReqItemList rlist;
public:
  virtual ReqItemList getRequestItems () const = 0;
  virtual void setRequestItems (ReqItemList sl) = 0;

  //**add request tuple from non-XMLNode*/
  virtual void addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx)=0;

  //**Default constructor*/
  Request () {};

  //**Constructor: Parse request information from a xml stucture in memory*/
  Request (const Arc::XMLNode*) {};
  virtual ~Request(){};

protected:
  //**Constructor: Parse request information from a input file*/
  Request (const char*) {};
};

} // namespace ArcSec

#endif /* __ARC_SEC_REQUEST_H__ */
