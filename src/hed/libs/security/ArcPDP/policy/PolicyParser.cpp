#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ArcPDP/policy/PolicyParser.h>
#include <fstream>
#include <iostream>
#include "ArcPolicy.h" 

using namespace Arc;
using namespace ArcSec;

PolicyParser::PolicyParser(){
}

Policy* PolicyParser::parsePolicy(const std::string filename, EvaluatorContext* ctx){
  std::string xml_str = "";
  std::string str;
  std::ifstream f(filename.c_str());

  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();
  
  XMLNode node(xml_str);
  
  return(new ArcPolicy(node, ctx));
  
}
