#ifndef __ARC_SEC_ARCREQUEST_H__
#define __ARC_SEC_ARCREQUEST_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/Request.h>

#include "ArcEvaluator.h"

///ArcRequest, Parsing the specified Arc request format

namespace ArcSec {

class ArcRequest : public Request {
friend class ArcEvaluator;

public:
  //**Get all the RequestItem inside RequestItem container */
  virtual ReqItemList getRequestItems () const;
  
  //**Set the content of the container*/  
  virtual void setRequestItems (ReqItemList sl);

  //**Add request tuple from non-XMLNode*/  
  virtual void addRequestItem(Attrs& sub, Attrs& res, Attrs& act, Attrs& ctx);

  //**Set the attribute factory for the usage of Request*/
  virtual void setAttributeFactory(AttributeFactory* attributefactory) { attrfactory = attributefactory; };

  //**Default constructor*/
  ArcRequest ();

  //**Parse request information from a xml stucture in memory*/
  ArcRequest (const Arc::XMLNode* node);

  //**Parse request information from a file*/
  ArcRequest (const char* reqfile);

  //**Parse request information from a string*/
  ArcRequest (std::string& reqstring);

  virtual ~ArcRequest();

  //**Create the objects included in Request according to the node attached to the Request object*/
  virtual void make_request();

private:
  //**AttributeFactory which is in charge of producing Attribute*/
  AttributeFactory * attrfactory;

  //**A XMLNode structure which includes the xml structure of a request*/
  Arc::XMLNode reqnode;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCREQUEST_H__ */
