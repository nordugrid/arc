#include "PolicyParser.h"
#include <fstream>
#include <iostream>
#include "ArcPolicy.h" 

PolicyParser::PolicyParser(){
}

Policy* PolicyParser::parsePolicy(const std::string filename){
  std::string xml_str = "";
  std::string str;
  std::ifstream f(filename);

  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();
  
  XMLNode node(xml_str);
  
  return(new ArcPolicy(node));
  
}
