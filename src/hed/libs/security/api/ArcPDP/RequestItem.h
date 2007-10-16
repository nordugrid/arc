#ifndef __ARC_SEC_REQUESTITEM_H__
#define __ARC_SEC_REQUESTITEM_H__

#include <list>
#include <arc/XMLNode.h>
#include "attr/AttributeFactory.h"
#include "attr/RequestAttribute.h"

namespace ArcSec {

//typedef std::list<Attribute*> Subject, Resource, Action, Context;
typedef std::list<RequestAttribute*> Subject, Resource, Action, Context;
typedef std::list<Subject> SubList;
typedef std::list<Resource> ResList;
typedef std::list<Action> ActList;
typedef std::list<Context> CtxList; 

/**<subjects, actions, objects, ctxs> tuple */
class RequestItem{
 public:
  RequestItem(Arc::XMLNode&, AttributeFactory*){};
  virtual ~RequestItem(){};

protected:
  SubList subjects;
  ResList actions;
  ActList resources;
  CtxList contexts;

public:
  virtual SubList getSubjects () const  = 0;
  virtual void setSubjects (const SubList& sl) = 0;
  virtual ResList getResources () const  = 0;
  virtual void setResources (const ResList& rl) = 0;
  virtual ActList getActions () const  = 0;
  virtual void setActions (const ActList& al) = 0;
  virtual CtxList getContexts () const  = 0;
  virtual void setContexts (const CtxList& ctx) = 0;

};

} // namespace Arc

#endif /* __ARC_SEC_REQUESTITEM_H__ */

