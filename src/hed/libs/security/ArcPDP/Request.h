#ifndef __ARC_SEC_REQUEST_H__
#define __ARC_SEC_REQUEST_H__

#include <list>
#include <arc/loader/LoadableClass.h>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/RequestItem.h>

namespace ArcSec {

///Following is some general structures and classes for storing the request information. 
///In principle, the request structure shoud be in XML format, and also can include a few items

///ReqItemList is a container for RequestItem objects
typedef std::list<RequestItem*> ReqItemList;

///Attr contains a tuple of attribute type and value
typedef struct{
  std::string value;
  std::string type;
} Attr;

///Attrs is a container for one or more Attr
/**Attrs includes includes methonds for inserting, getting items, and counting size as well*/
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


///Base class/Interface for request, includes a container for RequestItems and some operations
/**A Request object can has a few <subjects, actions, objects> tuples, i.e. RequestItem
The Request class and any customized class which inherit from it, should be loadable, which means these classes
can be dynamically loaded according to the configuration informtation, see the example configuration below:
        <Service name="pdp.service" id="pdp_service">
            <pdp:PDPConfig>
               <......> 
               <pdp:Request name="arc.request" />
               <......>
            </pdp:PDPConfig>
        </Service>

There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Request : public Arc::LoadableClass {
protected:
  ReqItemList rlist;
public:
  /**Get all the RequestItem inside RequestItem container */
  virtual ReqItemList getRequestItems () const = 0;

  /**Set the content of the container*/
  virtual void setRequestItems (ReqItemList sl) = 0;

  /**Add request tuple from non-XMLNode*/
  virtual void addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx)=0;

  /**Set the attribute factory for the usage of Request*/
  virtual void setAttributeFactory(AttributeFactory* attributefactory) = 0;

  /**Create the objects included in Request according to the node attached to the Request object*/
  virtual void make_request() = 0;

  /**Default constructor*/
  Request () {};

  /**Constructor: Parse request information from a xml stucture in memory*/
  Request (const Arc::XMLNode*) {};

  /**Constructor: Parse request information from a file*/
  Request (const char* reqfile) {};

  /**Constructor: Parse request information from a string*/
  Request (std::string& reqstring) {};

  virtual ~Request(){};
};

} // namespace ArcSec

#endif /* __ARC_SEC_REQUEST_H__ */
