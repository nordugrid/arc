// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/ArcConfig.h>
#include <arc/UserConfig.h>
#include <arc/communication/ClientInterface.h>
#include <arc/credential/Credential.h>

#include "CredentialStore.h"


namespace Arc {

using namespace Arc;

#define logger (Logger::getRootLogger())

CredentialStore::~CredentialStore(void) {
}

CredentialStore::CredentialStore(const URL& url):valid(false) {
  if(!url) return;
  if(url.Protocol() != "myproxy") return;
  UserConfig ucfg;
  set(url.Host(),url.Port(),ucfg);
}

CredentialStore::CredentialStore(const UserConfig& cfg, const URL& url):valid(false) {
  if(!url) return;
  if(url.Protocol() != "myproxy") return;
  set(url.Host(),url.Port(),cfg);
}

void CredentialStore::set(const std::string& host_, int port_, const UserConfig& cfg) {
  if(port_ <= 0) port_ = default_port;
  if(host_.empty()) return;
  timeout = 10;
  cfg.ApplyToConfig(concfg);
  host = host_;
  port = port_;
  valid = true;
}

static std::string read_response(PayloadStreamInterface& stream,bool nullterm = true) {
  std::string resp;
  do {
    char buf[256];
    int len = sizeof(buf);
    if(!stream.Get(buf,len)) break;
    //std::cout<<"Response: "<<buf<<std::endl;

    if(nullterm) {
      char* p = (char*)memchr(buf,0,len);
      if(p) {
        resp.append(buf,p-buf);
        break;
      };
    };
    resp.append(buf,len);
    if(!nullterm) if(len!=sizeof(buf)) break;
  } while(true);
  return resp;
}

static std::map<std::string,std::string> parse_response(const std::string& str) {
  std::map<std::string,std::string> vals;
  std::list<std::string> lines;
  tokenize(str,lines,"\r\n");
  for(std::list<std::string>::iterator line = lines.begin();
                          line != lines.end();++line) {
    std::list<std::string> tokens;

    //trick to deal with the token in response like:
    //CRED_OWNER=/O=Grid/OU=ARC/OU=localdomain/CN=User
    std::string::size_type pos = (*line).find_first_of("=");
    if(pos!=std::string::npos)(*line).replace(pos,1,"+");

    tokenize(*line,tokens,"+");
    if(tokens.size() != 2) continue;
    vals[*tokens.begin()] += vals[*tokens.begin()].empty()? (*(++tokens.begin())) : ("; " + *(++tokens.begin()));
  }
  return vals;
}

static std::map<std::string,std::string> parse_response(PayloadStreamInterface& stream) {
  return parse_response(read_response(stream));
}

bool compose_msg(std::string& msg, const std::map<std::string,std::string>& options, bool dummy_pass = false) {
  std::string credname;
  std::string username;
  std::string password;
  std::string newpassword;
  std::string trusted_retriever;
  int lifetime = 0;
  std::map<std::string,std::string>::const_iterator val;
  val = options.find("credname");
  if(val != options.end()) credname = val->second;
  val = options.find("username");
  if(val != options.end()) username = val->second;
  val = options.find("password");
  if(val != options.end()) password = val->second;
  val = options.find("newpassword");
  if(val != options.end()) newpassword = val->second;
  val = options.find("lifetime");
  if(val != options.end()) lifetime = stringto<int>(val->second);
  val = options.find("retriever_trusted");
  if(val != options.end()) trusted_retriever = val->second;
 
  if(username.empty()) return false;
  //USERNAME must be provided; PASSPHRASE could be empty 
  //if RETRIEVER_TRUSTED is provided.
  msg.append("USERNAME="+username+"\n");
  //if(!password.empty()) {

  if(!dummy_pass)
    msg.append("PASSPHRASE="+password+"\n");
  //}
  else
    msg.append("PASSPHRASE=DUMMY-PASSPHRASE\n");

  if(!newpassword.empty()) {
    msg.append("NEW_PHRASE="+newpassword+"\n");
  }

  if(lifetime > 0) {
    msg.append("LIFETIME="+tostring(lifetime)+"\n");
  }
  else {
    msg.append("LIFETIME=0").append("\n");
  }
  //The other attributes are optional.
  if(!credname.empty()) {
    msg.append("CRED_NAME="+credname+"\n");
  }
  if(!trusted_retriever.empty()) {
    msg.append("RETRIEVER_TRUSTED="+trusted_retriever+"\n");
  }

  for(std::map<std::string,std::string>::const_iterator it = options.begin(); it != options.end(); it++) {
    if((*it).first.find("vomsname") != std::string::npos) {
      std::string vomsname = (*it).second;
      if(!vomsname.empty()) {
        msg.append("VONAME="+vomsname+"\n");
      }
    }
  }
  for(std::map<std::string,std::string>::const_iterator it = options.begin(); it != options.end(); it++) {
    if((*it).first.find("vomses") != std::string::npos) {
      std::string vomses_line = (*it).second;
      if(!vomses_line.empty()) {
        msg.append("VOMSES="+vomses_line+"\n");
      }
    }
  }

  msg.append("\0");

  return true;
}

bool CredentialStore::Store(const std::map<std::string,std::string>& options,const std::string& cred, bool if_delegate, const Arc::Time deleg_start, const Arc::Period deleg_period) {
  if(!valid) return false;
  //if(if_delegate) {
    std::string msg("VERSION=MYPROXYv2\nCOMMAND=1\n");
  //}
  //else {
  //  std::string msg("VERSION=MYPROXYv2\nCOMMAND=5\n");
    //TODO
  //}
  if(!compose_msg(msg, options)) return false;

  PayloadStreamInterface *response = NULL;
  std::map<std::string,std::string> rvals;
  ClientTCP client(concfg,host,port,GSISec,timeout);
  {
    PayloadRaw request;
    request.Insert(msg.c_str(),0,msg.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  rvals = parse_response(*response);
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    delete response; response=NULL;
    return false;
  }
  std::string credrequest = read_response(*response,false);
  delete response; response=NULL;
  
  Arc::Credential proxy(deleg_start,deleg_period,1024);
  // DER binary request
  if(!proxy.InquireRequest(credrequest, false, true)) return false;
  std::string signercred = cred;
  std::string signerkey;
  bool isfile = false;
  if(signercred.empty()) {
    isfile = true;
    signercred = concfg.proxy;
    if(signercred.empty()) {
      signercred = concfg.cert;
      signerkey = concfg.key;
    }
  }
  Arc::Credential signer(signercred,signerkey,concfg.cadir,concfg.cafile,concfg.systemca,"",isfile);
  std::string credresp;
  std::string credtmp;
  if(!signer.SignRequest(&proxy,credtmp,true)) return false;
  unsigned char ncerts = signer.GetCertNumofChain() + 2;
  credresp.append((char*)&ncerts,1);
  credresp.append(credtmp);
  credtmp.clear();
  if(!signer.OutputCertificate(credtmp,true)) return false;
  credresp.append(credtmp);
  if(ncerts > 2) {
    credtmp.clear();
    if(!signer.OutputCertificateChain(credtmp,true)) return false;
    credresp.append(credtmp);
  };
  {
    PayloadRaw request;
    request.Insert(credresp.c_str(),0,credresp.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  rvals = parse_response(*response);
  delete response; response=NULL;
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    return false;
  }
  return true;
}

bool CredentialStore::Retrieve(const std::map<std::string,std::string>& options, std::string& cred, bool if_delegate) {
  if(!valid) return false;
  std::string msg("VERSION=MYPROXYv2\nCOMMAND=0\n");
  if(!compose_msg(msg, options)) return false;

  PayloadStreamInterface *response = NULL;
  ClientTCP client(concfg,host,port,GSISec,timeout);
  {
    PayloadRaw request;
    request.Insert(msg.c_str(),0,msg.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  std::map<std::string,std::string> rvals = parse_response(*response);
  delete response; response=NULL;
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    return false;
  }
  std::string credrequest;
  Arc::Credential requester(Time(), 0 ,1024);
  requester.GenerateRequest(credrequest,true);
  {
    PayloadRaw request;
    request.Insert(credrequest.c_str(),0,credrequest.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  std::string credresp = read_response(*response,false);
  delete response; response=NULL;
  if(credresp.empty()) return false;
  credresp.erase(0,1);
  std::string credkey;
  requester.OutputPrivatekey(credkey);
  Credential proxy(credresp,credkey,concfg.cadir,concfg.cafile,concfg.systemca,"",false);
  cred.clear();
  std::string credtmp;
  proxy.OutputCertificate(credtmp);
  if(credtmp.empty()) return false;
  cred.append(credtmp);
  credtmp.clear();
  proxy.OutputPrivatekey(credtmp);
  if(credtmp.empty()) return false;
  cred.append(credtmp);
  credtmp.clear();
  proxy.OutputCertificateChain(credtmp);
  cred.append(credtmp);
  return true;
}

bool CredentialStore::Info(const std::map<std::string,std::string>& options,std::string& respinfo){
  if(!valid) return false;
  std::string msg("VERSION=MYPROXYv2\nCOMMAND=2\n");
  if(!compose_msg(msg, options,true)) return false;

  PayloadStreamInterface *response = NULL;
  std::map<std::string,std::string> rvals;
  ClientTCP client(concfg,host,port,GSISec,timeout);
  {
    PayloadRaw request;
    request.Insert(msg.c_str(),0,msg.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  rvals = parse_response(*response);
  delete response; response=NULL;
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    return false;
  }
  std::string owner = rvals["CRED_OWNER"];
  respinfo.append("owner: ").append(rvals["CRED_OWNER"]).append("\n");

  std::string et_str = rvals["CRED_END_TIME"];
  long end = stringto<long>(et_str);
  Time endtime(end);
  Time now;
  Period left(endtime-now);
  std::string left_str = left;
  respinfo.append("time left: ").append(left_str).append("\n");
  std::cout<<respinfo;
  return true;
}

bool CredentialStore::ChangePassword(const std::map<std::string,std::string>& options) {
  if(!valid) return false;
  std::string msg("VERSION=MYPROXYv2\nCOMMAND=4\n");
  if(!compose_msg(msg, options)) return false;

  PayloadStreamInterface *response = NULL;
  std::map<std::string,std::string> rvals;
  ClientTCP client(concfg,host,port,GSISec,timeout);
  {
    PayloadRaw request;
    request.Insert(msg.c_str(),0,msg.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  rvals = parse_response(*response);
  delete response; response=NULL;
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    return false;
  }

  return true;
}

bool CredentialStore::Destroy(const std::map<std::string,std::string>& options) {
  if(!valid) return false;
  std::string msg("VERSION=MYPROXYv2\nCOMMAND=3\n");
  if(!compose_msg(msg, options)) return false;

  PayloadStreamInterface *response = NULL;
  std::map<std::string,std::string> rvals;
  ClientTCP client(concfg,host,port,GSISec,timeout);
  {
    PayloadRaw request;
    request.Insert(msg.c_str(),0,msg.length());
    MCC_Status status = client.process(&request,&response,true);
    if(!status) {
      if(response) delete response;
      return false;
    };
    if(!response) {
      return false;
    };
  };
  rvals = parse_response(*response);
  delete response; response=NULL;
  if(rvals["RESPONSE"] != "0") {
    logger.msg(ERROR, "MyProxy failure: %s",rvals["ERROR"]);
    return false;
  }

  return true;
}

} // namespace Arc

