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

  //***/
  ArcRequest (AttributeFactory* attrfactory);
  
  //**Parse request information from a file*/
  ArcRequest (const std::string& filename, AttributeFactory* attrfactory);

  //**Parse request information from a xml stucture in memory*/
  ArcRequest (Arc::XMLNode& node, AttributeFactory* attrfactory);
  virtual ~ArcRequest();

private:
  void make_request(Arc::XMLNode& node);

private:
  AttributeFactory * attrfactory;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCREQUEST_H__ */
