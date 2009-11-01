#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include "LDIFtoXML.h"

int main(int argc,char* argv[]) {
  std::string base;
  std::istream* in = &std::cin;
  std::istream* fin = NULL;
  Arc::NS ns;
  Arc::XMLNode xml(ns,"LDIFTree");
  int r = -1;
  if(argc < 2) {
    std::cerr<<"Usage: ldif2xml LDIF_base [input_file [output_file]] "<<std::endl;
    return -1;
  };
  base=argv[1];
  if(argc >= 3) in = (fin = new std::ifstream(argv[2]));
  if(*in) {
    if(ARex::LDIFtoXML(*in,base,xml)) {
      std::string s;
      xml.GetDoc(s);
      if(argc < 4) {
        std::cout<<s; r=0;
      } else {
        std::ofstream f(argv[3]);
        if(f) {
          f<<s; r=0;
        };
      };
    };
  };
  if(fin) delete fin;
  return r;
}

