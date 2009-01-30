#include <arc/message/MessageAuth.h>

/*
<?xml version="1.0" encoding="UTF-8"?>
<xsd:schema
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns="http://www.nordugrid.org/schemas/InfoFilter/2008"
  xmlns:if="http://www.nordugrid.org/schemas/InfoFilter/2008"
  targetNamespace="http://www.nordugrid.org/schemas/InfoFilter/2008"
  elementFormDefault="qualified">

    <xsd:complexType name="InfoFilterDefinition_Type">
        <!-- This element defines information document filtering definition. -->
        <xsd:sequence>
            <!-- Filter contains one authorization policy to by applied -->
            <xsd:any minOccurs="1" maxOccurs="1"/>
        </xsd:sequence>
        <!-- Attribute 'id' defines reference used by InfoFilterTag elements -->
        <xsd:attribute name="id" type="xsd:string" use="optional"/>
    </xsd:complexType>
    <xsd:element name="InfoFilterDefinition" type="if:InfoFilterDefinition_Type"/>

    <!-- InfoFilterTag refers to Filter which has to be applied to current node -->
    <xsd:attribute name="InfoFilterTag">
        <xsd:simpleType>
          <xsd:restriction base="xsd:string"/>
        </xsd:simpleType>
    </xsd:attribute>

</xsd:schema>
*/

namespace Arc {

typedef std::list< std::pair<std::string,XMLNode> > InfoFilterPolicies;

/// Filters information document according to identity of requestor
/** Identity is compared to policies stored inside information 
document and external ones. Parts of document which do not pass 
policy evaluation are removed. */
class InfoFilter {
 private:
  MessageAuth& id_;
 public:
  /// Creates object and associates identity
  /** Associated identity is not copied, hence passed argument must not be 
     destroyed while this method is used. */
  InfoFilter(MessageAuth& id);
  /// Filter information document according to internal policies
  /** In provided document all policies and nodes which have their policies
     evaluated to negative result are removed. */
  bool Filter(XMLNode doc) const;
  /// Filter information document according to internal and external policies
  /** In provided document all policies and nodes which have their policies
     evaluated to negative result are removed. External policies are provided 
     in policies argument. First element of every pair is XPath defining to 
     which XML node policy must be applied. Second element is policy itself.
     Argument ns defines XML namespaces for XPath evaluation. */
  bool Filter(XMLNode doc,const InfoFilterPolicies& policies,const Arc::NS& ns) const;
};

} // namespace Arc

