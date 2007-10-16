#ifndef __ARC_SEC_ARCREQUEST_H__
#define __ARC_SEC_ARCREQUEST_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/Request.h>

/** ArcRequest, Parsing the specified Arc request format*/

namespace ArcSec {

//typedef std::list<RequestItem*> ReqItemList;

class ArcRequest : public Request {

public:
  virtual ReqItemList getRequestItems () const;
  virtual void setRequestItems (ReqItemList sl);
  
  virtual void addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx);

  void setAttributeFactory(AttributeFactory* attributefactory) { attrfactory = attributefactory; };

  //**Default constructor*/
  ArcRequest ();
  
  //**Parse request information from a file*/
  ArcRequest (const char* filename);

  //**Parse request information from a xml stucture in memory*/
  ArcRequest (const Arc::XMLNode* node);
  virtual ~ArcRequest();

  void make_request();

private:
  AttributeFactory * attrfactory;
  Arc::XMLNode reqnode;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCREQUEST_H__ */
