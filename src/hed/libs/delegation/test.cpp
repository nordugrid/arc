#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>

#include "DelegationInterface.h"

int main(int argc,char* argv[]) {
  std::string credentials;
  std::ifstream ic(argv[1]);
  for(;!ic.eof();) {
    char buf[256];
    ic.get(buf,sizeof(buf),0);
    if(ic.gcount() <= 0) break;
    credentials.append(buf,ic.gcount());
  };
  std::cerr<<"Credentials:"<<std::endl;
  std::cerr<<credentials<<std::endl;
  std::string s;
  Arc::DelegationConsumer c;
  c.Backup(s);
  std::cerr<<"Private key:\n"<<s<<std::endl;
  c.Request(s);
  std::cerr<<"Request:\n"<<s<<std::endl;
  Arc::DelegationProvider p(credentials);
  s=p.Delegate(s);
  std::cerr<<"Delegation:\n"<<s<<std::endl;
  c.Acquire(s);
  std::ofstream oc("proxy.pem");
  oc<<s<<std::endl;
  return 0;
}

