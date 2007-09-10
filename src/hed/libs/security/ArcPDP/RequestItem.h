#ifndef __ARC_REQUESTITEM_H__
#define __ARC_REQUESTITEM_H__

#include <list>
#include "attr/RequestAttribute.h"
#include "common/XMLNode.h"

namespace Arc {

//typedef std::list<Attribute*> Subject, Resource, Action, Context;
typedef std::list<RequestAttribute*> Subject, Resource, Action, Context;
typedef std::list<Subject> SubList;
typedef std::list<Resource> ResList;
typedef std::list<Action> ActList;
typedef std::list<Context> CtxList; 

/**<subjects, actions, objects, ctxs> tuple */
class RequestItem{
 public:
  RequestItem(XMLNode& node){};
  virtual ~RequestItem(){};

protected:
  SubList subjects;
  ResList actions;
  ActList resources;
  CtxList contexts;

public:
  virtual SubList getSubjects () const {};
  virtual void setSubjects (const SubList& sl) {};
  virtual ResList getResources () const {};
  virtual void setResources (const ResList& rl) {};
  virtual ActList getActions () const {};
  virtual void setAction (const ActList& al) {};
  virtual CtxList getContexts () const {};
  virtual void setContexts (const CtxList& ctx) {};

};

} // namespace Arc

#endif /* __ARC_REQUESTITEM_H__ */

