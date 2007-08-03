#ifndef __ARC_ARCREQUESTITEM_H__
#define __ARC_ARCREQUESTITEM_H_

#include "RequestItem.h"
#include "XMLNode.h"
#include <list>
#include "Attribute.h"

namespace Arc {

/**<subjects, actions, objects, envs> tuple */
/**Specified ArcRequestItem which can parse Arc request formate*/
class ArcRequestItem : public RequestItem{
public:
  RequestItem(const Arc::XMLNode& root);
  virtual ~RequestItem(void) { };

public:
  virtual SubList getSubjects () const;
  virtual void setSubjects (const SubList& sl);
  virtual ResList getResources () const;
  virtual void setResources (const ResList& rl);
  virtual ActList getActions () const;
  virtual void setAction (const ActList& actions);
  virtual EnvList getEnvironments () const;
  virtual void setEnvironmets (const EnvList& ctx);

};

} // namespace Arc

#endif /* __ARC_ARCREQUESTITEM_H__ */

