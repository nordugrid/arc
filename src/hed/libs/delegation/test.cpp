#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>

#include "DelegationInterface.h"
#include <arc/message/PayloadSOAP.h>

static Arc::DelegationContainerSOAP* deleg_service = NULL;

class DirectMCC: public Arc::MCCInterface {
 public:
  Arc::MCC_Status process(Arc::Message& in,Arc::Message& out);
};

Arc::MCC_Status DirectMCC::process(Arc::Message& in,Arc::Message& out) {
  if(!deleg_service) return Arc::MCC_Status();
  if(!in.Payload()) return Arc::MCC_Status();
  Arc::PayloadSOAP* in_payload = NULL;
  try {
    in_payload = dynamic_cast<Arc::PayloadSOAP*>(in.Payload());
  } catch(std::exception& e) { };
  if(!in_payload) return Arc::MCC_Status();
  Arc::MCC_Status r;
  Arc::NS ns;
  Arc::PayloadSOAP* out_payload = new Arc::PayloadSOAP(ns);
  out.Payload(out_payload);
  std::string xml;
  in_payload->GetXML(xml);
  std::cout<<"Request:\n"<<xml<<std::endl;
  if((*in_payload)["DelegateCredentialsInit"]) {
    if(!deleg_service->DelegateCredentialsInit(*in_payload,*out_payload)) return Arc::MCC_Status();
  } else if((*in_payload)["UpdateCredentials"]) {
    std::string cred;
    if(!deleg_service->UpdateCredentials(cred,*in_payload,*out_payload)) return Arc::MCC_Status();
    std::cout<<"Delegated credentials:\n"<<cred<<std::endl;
  };
  out_payload->GetXML(xml);
  std::cout<<"Response:\n"<<xml<<std::endl;
  return Arc::MCC_Status(Arc::STATUS_OK);
}

int main(int /*argc*/,char* argv[]) {
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
  /*
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
  */
  Arc::DelegationContainerSOAP c;
  Arc::DelegationProviderSOAP p(credentials);
  deleg_service=&c;
  Arc::MessageAttributes attr;
  Arc::MessageContext context;
  DirectMCC interface;
  if(!p.DelegateCredentialsInit(interface,&attr,&context)) {
    std::cerr<<"DelegateCredentialsInit failed"<<std::cerr;
    return -1;
  };
  if(!p.UpdateCredentials(interface,&attr,&context)) {
    std::cerr<<"UpdateCredentials failed"<<std::endl;
    return -1;
  };
  return 0;
}

