#ifndef __ARC_SEC_ARCREQUESTITEM_H__
#define __ARC_SEC_ARCREQUESTITEM_H__

#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/RequestItem.h>

namespace ArcSec {

/**<subjects, actions, objects, ctxs> tuple */
/**Specified ArcRequestItem which can parse Arc request formate*/
class ArcRequestItem : public RequestItem{
public:
  ArcRequestItem(Arc::XMLNode& node, AttributeFactory* attrfactory);
  virtual ~ArcRequestItem();

public:
  virtual SubList getSubjects () const;
  virtual void setSubjects (const SubList& sl);
  virtual ResList getResources () const;
  virtual void setResources (const ResList& rl);
  virtual ActList getActions () const;
  virtual void setActions (const ActList& actions);
  virtual CtxList getContexts () const;
  virtual void setContexts (const CtxList& ctx);

};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCREQUESTITEM_H__ */

