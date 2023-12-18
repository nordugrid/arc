// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/FileUtils.h>
#include <arc/communication/ClientInterface.h>
#include <arc/credentialstore/ClientVOMS.h>
#include <arc/credentialstore/ClientVOMSRESTful.h>
#include <arc/credential/VOMSConfig.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>

#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif

#include "arcproxy.h"

using namespace ArcCredential;

typedef enum {
  pass_all,
  pass_private_key,
  pass_myproxy,
  pass_myproxy_new,
  pass_nss
} pass_destination_type;

extern  std::map<pass_destination_type, Arc::PasswordSource*> passsources;

static std::string get_cert_dn(const std::string& cert_file) {
  std::string dn_str;
  Arc::Credential cert(cert_file, "", "", "", false);
  dn_str = cert.GetIdentityName();
  return dn_str;
}

bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command, 
      const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
      const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
      std::list<std::string>& vomslist,
      std::string& vomses_path,
      const std::string& proxy_path, 
      Arc::UserConfig& usercfg, Arc::Logger& logger) {
  
  std::string user_name = myproxy_user_name;
  std::string key_path, cert_path, ca_dir;
  key_path = usercfg.KeyPath();
  cert_path = usercfg.CertificatePath();
  ca_dir = usercfg.CACertificatesDirectory();
  if(user_name.empty()) {
    user_name = get_cert_dn(proxy_path);
  }
  if(user_name.empty() && !cert_path.empty()) {
    user_name = get_cert_dn(cert_path);
  }

  //If the "INFO" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (Arc::lower(myproxy_command) == "info") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string respinfo;

      //if(usercfg.CertificatePath().empty()) usercfg.CertificatePath(cert_path);
      //if(usercfg.KeyPath().empty()) usercfg.KeyPath(key_path);
      if(usercfg.ProxyPath().empty() && !proxy_path.empty()) usercfg.ProxyPath(proxy_path);
      else {
        if(usercfg.CertificatePath().empty() && !cert_path.empty()) usercfg.CertificatePath(cert_path);
        if(usercfg.KeyPath().empty() && !key_path.empty()) usercfg.KeyPath(key_path);
      }      
      if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(ca_dir);

      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      if(!cstore.Info(myproxyopt,respinfo))
        throw std::invalid_argument("Failed to get info from MyProxy service");

      std::cout << Arc::IString("Succeeded to get info from MyProxy server") << std::endl;
      std::cout << respinfo << std::endl;
      return true;
    }

  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    return false;
  }

  //If the "NEWPASS" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (Arc::lower(myproxy_command) == "newpass") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string passphrase;
      if(passsources[pass_myproxy]->Get(passphrase, 4, 256) != Arc::PasswordSource::PASSWORD) 
        throw std::invalid_argument("Error entering passphrase");
     
      std::string newpassphrase;
      if(passsources[pass_myproxy_new]->Get(newpassphrase, 4, 256) != Arc::PasswordSource::PASSWORD) 
        throw std::invalid_argument("Error entering passphrase");
     
      if(usercfg.ProxyPath().empty() && !proxy_path.empty()) usercfg.ProxyPath(proxy_path);
      else {
        if(usercfg.CertificatePath().empty() && !cert_path.empty()) usercfg.CertificatePath(cert_path);
        if(usercfg.KeyPath().empty() && !key_path.empty()) usercfg.KeyPath(key_path);
      }
      if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(ca_dir);

      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      myproxyopt["newpassword"] = newpassphrase;
      if(!cstore.ChangePassword(myproxyopt))
        throw std::invalid_argument("Failed to change password MyProxy service");

      std::cout << Arc::IString("Succeeded to change password on MyProxy server") << std::endl;

      return true;
    }

  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    return false;
  }

  //If the "DESTROY" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (Arc::lower(myproxy_command) == "destroy") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string passphrase;
      if(passsources[pass_myproxy]->Get(passphrase, 4, 256) != Arc::PasswordSource::PASSWORD) 
        throw std::invalid_argument("Error entering passphrase");

      std::string respinfo;

      if(usercfg.ProxyPath().empty() && !proxy_path.empty()) usercfg.ProxyPath(proxy_path);
      else {
        if(usercfg.CertificatePath().empty() && !cert_path.empty()) usercfg.CertificatePath(cert_path);
        if(usercfg.KeyPath().empty() && !key_path.empty()) usercfg.KeyPath(key_path);
      }
      if(usercfg.CACertificatesDirectory().empty()) usercfg.CACertificatesDirectory(ca_dir);

      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      if(!cstore.Destroy(myproxyopt))
        throw std::invalid_argument("Failed to destroy credential on MyProxy service");

      std::cout << Arc::IString("Succeeded to destroy credential on MyProxy server") << std::endl;

      return true;
    }
  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    return false;
  }

  //If the "GET" myproxy command is given, try to get a delegated
  //certificate from the myproxy server.
  //For "GET" command, certificate and key are not needed, and
  //anonymous GSSAPI is used (GSS_C_ANON_FLAG)
  try {
    if (Arc::lower(myproxy_command) == "get") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string passphrase;
      if(!use_empty_passphrase) {
        if(passsources[pass_myproxy]->Get(passphrase, 4, 256) != Arc::PasswordSource::PASSWORD)
          throw std::invalid_argument("Error entering passphrase");
      }

      std::string proxy_cred_str_pem;
     
      Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::SkipCredentials);
      Arc::UserConfig usercfg_tmp(cred_type);
      usercfg_tmp.CACertificatesDirectory(usercfg.CACertificatesDirectory());

      Arc::CredentialStore cstore(usercfg_tmp,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      myproxyopt["lifetime"] = myproxy_period;
      // According to the protocol of myproxy, the "Get" command can
      // include the information about vo name, so that myproxy server
      // can contact voms server to retrieve AC for myproxy client 
      // See 2.4 of http://grid.ncsa.illinois.edu/myproxy/protocol/
      // "When VONAME appears in the message, the server will generate VOMS
      // proxy certificate using VONAME and VOMSES, or the server's VOMS server information."
      class vomses_match: public Arc::VOMSConfig::filter {
       private:
        int seq_;
        const std::list<std::string>& vomses_;
        std::map<std::string,std::string>& opts_;
       public:
        bool match(const Arc::VOMSConfigLine& line) {
          for(std::list<std::string>::const_iterator voms = vomses_.begin();
                         voms != vomses_.end(); ++voms) {
            if((line.Name() == *voms) || (line.Alias() == *voms)) {
              opts_["vomsname"+Arc::tostring(seq_)] = *voms;
              opts_["vomses"+Arc::tostring(seq_)] = line.Str();
              ++seq_;
              break;
            };
          };
          // Because rsult is stored imeediately there is no sense to keep matched lines in 
          // VOMSConfig object.
          return false;
        };
        vomses_match(const std::list<std::string>& vomses, std::map<std::string,std::string> opts):
                 seq_(0),vomses_(vomses),opts_(opts) { };
      };

      Arc::VOMSConfig voms_config(vomses_path, vomses_match(vomslist,myproxyopt));

      if(!cstore.Retrieve(myproxyopt,proxy_cred_str_pem))
        throw std::invalid_argument("Failed to retrieve proxy from MyProxy service");
      write_proxy_file(proxy_path,proxy_cred_str_pem);

      //Assign proxy_path to cert_path and key_path,
      //so the later voms functionality can use the proxy_path
      //to create proxy with voms AC extension. In this
      //case, "--cert" and "--key" is not needed.
      cert_path = proxy_path;
      key_path = proxy_path;
      std::cout << Arc::IString("Succeeded to get a proxy in %s from MyProxy server %s", proxy_path, myproxy_server) << std::endl;

      return true;
    }

  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    return false;
  }

  //Delegate the former self-delegated credential to
  //myproxy server
  try {
    if (Arc::lower(myproxy_command) == "put") {

      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if (user_name.empty()) 
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string prompt1 = "MyProxy server";
      std::string passphrase;
      if(retrievable_by_cert.empty()) {
        if(passsources[pass_myproxy_new]->Get(passphrase, 4, 256) != Arc::PasswordSource::PASSWORD)
          throw std::invalid_argument("Error entering passphrase");
      }

      std::string proxy_cred_str_pem;
      std::ifstream proxy_cred_file(proxy_path.c_str());
      if(!proxy_cred_file)
        throw std::invalid_argument("Failed to read proxy file "+proxy_path);
      std::getline(proxy_cred_file,proxy_cred_str_pem,'\0');
      if(proxy_cred_str_pem.empty())
        throw std::invalid_argument("Failed to read proxy file "+proxy_path);
      proxy_cred_file.close();

      usercfg.ProxyPath(proxy_path);
      if(usercfg.CACertificatesDirectory().empty()) { usercfg.CACertificatesDirectory(ca_dir); }

      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      myproxyopt["lifetime"] = myproxy_period;
      if(!retrievable_by_cert.empty()) {
        myproxyopt["retriever_trusted"] = retrievable_by_cert;
      }
      if(!cstore.Store(myproxyopt,proxy_cred_str_pem,true,proxy_start,proxy_period))
        throw std::invalid_argument("Failed to delegate proxy to MyProxy service");

      remove_proxy_file(proxy_path);

      std::cout << Arc::IString("Succeeded to put a proxy onto MyProxy server") << std::endl;

      return true;
    }
  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    remove_proxy_file(proxy_path);
    return false;
  }
  return true;
}

