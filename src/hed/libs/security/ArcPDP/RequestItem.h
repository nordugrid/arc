#ifndef __ARC_REQUESTITEM_H__
#define __ARC_REQUESTITEM_H_

#include "XMLNode.h"
#include <list>
#include "Attribute.h"

namespace Arc {

typedef std::list<Attribute*> Subject, Resource, Action, Environment;
typedef std::list<Subject> SubList;
typedef std::list<Resource> ResList;
typedef std::list<Action> ActList;
typedef std::list<Environment> EnvList; 

/**<subjects, actions, objects, envs> tuple */
class RequestItem{
 public:
  RequestItem(const Arc::XMLNode& node);
  virtual ~RequestItem(void) { };

protect:
  SubList subjects;
  ResList actions;
  ActList objects;
  EnvList envs;

public:
  virtual SubList getSubjects () const;
  virtual void setSubjects (const SubList& sl);
  virtual ResList getResources () const;
  virtual void setResources (const ResList& rl);
  virtual ActList getActions () const;
  virtual void setAction (const ActList& al);
  virtual EnvList getEnvironments () const;
  virtual void setEnvironmets (const EnvList& ctx);

};

} // namespace Arc

#endif /* __ARC_REQUEST_H__ */

