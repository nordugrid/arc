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


  /************************************/
  //Request side
  Arc::Time t;
  std::string req_string;
  ArcLib::Credential request(t, Arc::Period(24*3600));//, 2048);// "rfc", "impersonation", "");
  request.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  std::string out_string;
  ArcLib::Credential proxy;
  ArcLib::Credential signer(cert, key, "", cafile);
  proxy.InquireRequest(req_string);
  //Put an example Arc policy as the extension of proxy certificate
  std::string oid("1.3.6.1.5.5.7.1.21");
  std::string sn("arcpolicy");
  proxy.AddCertExtObj(sn, oid);
  proxy.AddExtension("arcpolicy", policy, true);
  signer.SignRequest(&proxy, out_string); //The signer will send the signed certificate to request side

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key, signing_cert, signing_certchain;
  request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_certchain);
  out_string.append(private_key);
  out_string.append(signing_cert);
  out_string.append(signing_certchain);
  std::cout<<"Signed proxy certificate: " <<out_string<<std::endl;

  //Output into a file
  std::string proxy_file("proxy.pem");
  std::ofstream out_f(proxy_file.c_str());
  out_f.write(out_string.c_str(), out_string.size());
  out_f.close();

  /*************************************/
  //Request side
  Arc::Time t1;
  std::string req_string1;
  ArcLib::Credential request1(t1, Arc::Period(24*3600));//, 2048);// "rfc", "impersonation", "");
  request1.GenerateRequest(req_string1);
  std::cout<<"Certificate request: "<<req_string1<<std::endl;

  //Signing side
  std::string out_string1;
  ArcLib::Credential proxy1;
  ArcLib::Credential signer1(proxy_file, "", "", cafile); //use the existing proxy as the signer
  proxy1.InquireRequest(req_string1);
  std::string oid1("1.3.6.1.5.5.7.1.21");
  std::string sn1("arcpolicy");
  proxy.AddCertExtObj(sn1, oid1);
  signer1.SignRequest(&proxy1, out_string1); //The signer will send the signed certificate to request side

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key1, signing_cert1, signing_certchain1;
  request1.OutputPrivatekey(private_key1);
  signer1.OutputCertificate(signing_cert1);
  signer1.OutputCertificateChain(signing_certchain1);
  out_string1.append(private_key1);
  out_string1.append(signing_cert1);
  out_string1.append(signing_certchain1);
  std::cout<<"Signed proxy certificate: " <<out_string1<<std::endl;

  //Output into a file
  std::string proxy_file1("proxy1.pem");
  std::ofstream out_f1(proxy_file1.c_str());
  out_f1.write(out_string1.c_str(), out_string1.size());
  out_f1.close();

}

