#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>

#include <arc/XMLNode.h>
#include <arc/arclib/Credential.h>

int main(void) {
  std::string cert("./cert.pem");
  std::string key("./key.pem");
  std::string cafile("./ca.pem");
  Arc::XMLNode policy_nd("\
    <Policy\
      xmlns=\"http://www.nordugrid.org/ws/schemas/policy-arc\"\ 
      PolicyId='sm-example:policy1'\ 
      CombiningAlg='Deny-Overrides'>\
     <Rule RuleId='rule1' Effect='Permit'>\
      <Description>\
         Sample Permit rule for Storage_manager service \
      </Description>\
      <Subjects>\
         <Subject Type='string'>/O=NorduGrid/OU=UIO/CN=test</Subject>\
         <Subject Type='string'>/vo.knowarc/usergroupA</Subject>\
         <Subject>\
            <Attribute Type='string'>/O=Grid/OU=KnowARC/CN=XYZ</Attribute>\
            <Attribute Type='string'>urn:mace:shibboleth:examples</Attribute>\
         </Subject>\
      </Subjects>\
      <Resources>\
         <Resource Type='string'>file://home/test</Resource>\
      </Resources>\
      <Actions Type='string'>\
         <Action>read</Action>\
         <Action>stat</Action>\
         <Action>list</Action>\
      </Actions>\
      <Conditions>\
         <Condition Type='period'>2007-09-10T20:30:20/P1Y1M</Condition>\
      </Conditions>\
     </Rule>\
    </Policy>");
  std::string policy;
  policy_nd.GetXML(policy);

  Arc::Time t;
  std::string req_string;
  std::string out_string;

  //Request side
  ArcLib::Credential request(t, Arc::Period(24*3600));//, 2048);// "rfc", "impersonation", "");
  request.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  ArcLib::Credential proxy;
  ArcLib::Credential signer(cert, key, "", cafile);
  proxy.InquireRequest(req_string);
  //Put an example Arc policy as the extension of proxy certificate
  std::string oid("1.3.6.1.5.5.7.1.21");
  std::string sn("arcpolicy");
  proxy.AddCertExtObj(sn, oid);
  proxy.AddExtension("arcpolicy", policy);

  signer.SignRequest(&proxy, out_string);
  std::cout<<"Signed proxy certificate: " <<out_string<<std::endl;
}

