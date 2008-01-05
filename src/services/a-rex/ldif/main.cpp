#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include "LDIFtoXML.h"

int main(int argc,char* argv[]) {
  bool r = false;
  std::string base;
  std::istream* in = &std::cin;
  Arc::NS ns;
  Arc::XMLNode xml(ns,"LDIFTree");
  if(argc < 2) {
    std::cerr<<"Usage: ldif2xml LDIF_base [input_file [output_file]] "<<std::endl;
    return -1;
  };
  base=argv[1];
  if(argc >= 3) in = new std::ifstream(argv[2]);
  if(!(*in)) return -1;
  if(!ARex::LDIFtoXML(*in,base,xml)) return -1;
  std::string s;
  xml.GetDoc(s);
  if(argc < 4) {
    std::cout<<s;
  } else {
    std::ofstream f(argv[3]);
    if(!f) return -1;
    f<<s;
  };
  return 0;
}

