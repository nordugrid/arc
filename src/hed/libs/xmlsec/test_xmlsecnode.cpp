#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include "XMLSecNode.h"
#include "XmlSecUtils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>


int main(void)
{
    std::string xml_str = "";
    std::string str;
    std::ifstream f("testxmlsec.xml");

    // load content of file
    while (f >> str) {
        xml_str.append(str);
        xml_str.append(" ");
    }
    f.close();
    
    Arc::XMLNode node(xml_str);
    node.GetXML(str);
    std::cout<<"Original node: "<<std::endl<<str<<std::endl;

    Arc::XMLSecNode secnode(node);

    Arc::init_xmlsec();

    //Sign the node
    std::string idname("ID");
    secnode.AddSignatureTemplate(idname, Arc::XMLSecNode::RSA_SHA1);  
    std::string privkey("../../../tests/echo/testkey-nopass.pem");
    std::string cert("../../../tests/echo/testcert.pem");  
    if(secnode.SignNode(privkey,cert)) {
      std::cout<<"Succeed to sign the signature under the node"<<std::endl;
      secnode.GetXML(str);
      std::cout<<"Signed node: "<<std::endl<<str<<std::endl;
    }
    else { Arc::final_xmlsec(); return 0; }
   
    //Verify the signature
    std::string cafile("../../../tests/echo/testcacert.pem");
    std::string capath("");
    if(secnode.VerifyNode(idname, cafile, capath, false, true)) {
      std::cout<<"Succeed to verify the signature under the node"<<std::endl;
    }

    Arc::final_xmlsec();
    return 0;
}

