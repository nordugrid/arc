#ifndef __ARC_SEC_REQUESTITEM_H__
#define __ARC_SEC_REQUESTITEM_H__

#include <list>
#include <arc/XMLNode.h>
#include "attr/AttributeFactory.h"
#include "attr/RequestAttribute.h"

namespace ArcSec {

///Attribute containers, which includes a few RequestAttribute objects
/** Why do we need such containers?
    A Subject node could be like below, include a few attributes at the same time: 
       <Subject>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</Attribute>
       </Subject>
    Or only include one attribute:
       <Subject AttributeId="urn:arc:subject:dn" Type="X500DN">/O=NorduGrid/OU=UIO/CN=test</Subject>
    Or include a few the same types of attributes at the same time:
       <Subject Type="xsd:string">
          <Attribute AttributeId="urn:arc:subject:voms-attribute">administrator</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute">/O=NorduGrid/OU=UIO/CN=admin</Attribute>
       </Subject>

    Note, <Subject> (or others) node with more than one <Attribute>s means the <Subject> owns all the included attributes at the same time. 
    e.g. a person with email: abc@xyz and DN:/O=XYZ/OU=ABC/CN=theguy and role: administrator
    However, Parallel <Subject>s inside one SubList (see below about definition if ***List) does not means there is any relationship between 
    these <Subject>s.

    Then if there are two examples of <Subject> here:
       Subject1:
       <Subject>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</Attribute>
       </Subject>

      and,
       Subject2:
       <Subject AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=test</Subject>

       Subject3:
       <Subject AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Subject>

     the former one will be explained as the <Subject1, Action, Resource, Context> request tuple has two attributes at the same time
     the later one will be explained as the two <Subject2, Action, Resource, Context>, <Subject3, Action, Resource, Context> independently has 
     one attribute. 
     If we consider the Policy side, a policy snipet example like this:
     <Rule>
      <Subjects>
        <Subject Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</Subject>
        <Subject Type="xsd:string">administrator</Subject>
      </Subjects>
      <Resources>......</Resources>
      <Actions>......</Actions>
      <Conditions>......</Conditions>
     </Rule>
     then all of the Subject1 Subject2 Subject3 will satisfy the <Subjects> in policy.
     but if the policy snipet is like this:
     <Rule>
      <Subjects>
        <Subject>
          <SubFraction Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</SubFraction>
          <SubFraction Type="xsd:string">administrator</SubFraction>
        </Subject>
      </Subjects>
      <Resources>......</Resources>
      <Actions>......</Actions>
      <Conditions>......</Conditions>
     </Rule>
     then only Subject1 can satisfy the <Subjects> in policy.


    A complete request item could be like:
    <RequestItem>
        <Subject AttributeId="urn:arc:subject:dn" Type="string">/O=NorduGrid/OU=UIO/CN=test</Subject>
        <Subject AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Subject>
        <Subject>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">guest</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=anonymous</Attribute>
        </Subject>
        <Resource AttributeId="urn:arc:resource:file" Type="string">file://home/test</Resource>
        <Action AttributeId="urn:arc:action:file-action" Type="string">read</Action>
        <Action AttributeId="urn:arc:action:file-action" Type="string">copy</Action>
        <Context AttributeId="urn:arc:context:date" Type="period">2007-09-10T20:30:20/P1Y1M</Context>
    </RequestItem>
    
    Here putting a few <Subject>s <Resource>s <Action>s  or <Context>s together (inside one RequestItem) is only for the convinient of 
    expression (there is no logical relationship between them). For more than one <<Subject>, <Resource>, <Action>, <Context>> tuples,  
    if there is one element (e.g. <Subject>) which is different to each other, you can put these tuples together by using one tuple  
    <<Subject1>,<Subject2>, <Resource>, <Action>, <Context>> tuple, and don't need to write a few tuples.
*/
typedef std::list<RequestAttribute*> Subject, Resource, Action, Context;

///Containers, which include a few Subject, Resource, Action or Context objects
typedef std::list<Subject> SubList;
typedef std::list<Resource> ResList;
typedef std::list<Action> ActList;
typedef std::list<Context> CtxList; 

///Interface for request item container, <subjects, actions, objects, ctxs> tuple
class RequestItem{
 public:
  /**Constructor
  @param node  The XMLNode structure of the request item
  @param attributefactory  The AttributeFactory which will be used to generate RequestAttribute 
  */  
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

