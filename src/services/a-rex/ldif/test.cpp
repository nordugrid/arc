#include <fstream>
#include <arc/XMLNode.h>

#include "LDIFtoXML.h"

int main(int args,char* argv[]) {
  std::ifstream f(argv[1]);
  std::string ldif_base = "Mds-Vo-name=local,O=Grid";
  Arc::NS ns;
  Arc::XMLNode xml(ns);
  ARex::LDIFtoXML(f,ldif_base,xml.NewChild("LDIF"));
  std::string s;
  xml.GetXML(s);
  std::cout<<s<<std::endl;
  return 0;
}

