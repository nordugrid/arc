#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <termios.h>
#endif
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>


static Arc::Logger& logger = Arc::Logger::rootLogger;

static void tls_process_error(void) {
   unsigned long err;
   err = ERR_get_error();
   if (err != 0)
   {
     logger.msg(Arc::ERROR, "OpenSSL Error -- %s", ERR_error_string(err, NULL));
     logger.msg(Arc::ERROR, "Library  : %s", ERR_lib_error_string(err));
     logger.msg(Arc::ERROR, "Function : %s", ERR_func_error_string(err));
     logger.msg(Arc::ERROR, "Reason   : %s", ERR_reason_error_string(err));
   }
   return;
}

int main(int argc, char* argv[]){

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options("", "",
			    istring("Supported constraints are:\n"
				    "  validityStart=time\n"
				    "  validityEnd=time\n"
				    "  validityPeriod=time\n"
                                    "  vomsACvalidityStart=time\n"
                                    "  vomsACvalidityEnd=time\n"
                                    "  vomsACvalidityPeriod=time\n"
				    "  proxyPolicy=policy content\n"
                                    "  proxyPolicyFile=policy file"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to proxy file"),
		    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "certificate", istring("path to certificate file"),
		    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to private key file"),
		    istring("path"), key_path);

  std::string ca_dir;
  options.AddOption('T', "trusted certdir", istring("path to trusted certificate directory"),
                    istring("path"), ca_dir);

  std::string vomses_path;
  options.AddOption('V', "vomses", istring("path to voms server configuration file"),
                    istring("path"), vomses_path);

  std::string voms;
  options.AddOption('S', "voms", istring("voms<:command>. Specify voms server. :command is optional, and is used to ask for specific attributes(e.g: roles)"),
                    istring("string"), voms);

  std::list<std::string> constraintlist;
  options.AddOption('c', "constraint", istring("proxy constraints"),
		    istring("string"), constraintlist);

  std::string conffile;
  options.AddOption('z', "conffile",
		    istring("configuration file (default ~/.arc/client.xml)"),
		    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcproxy", VERSION) << std::endl;
    return 0;
  }

  std::map<std::string, std::string> constraints;
  for (std::list<std::string>::iterator it = constraintlist.begin();
       it != constraintlist.end(); it++) {
    std::string::size_type pos = it->find('=');
    if (pos != std::string::npos)
      constraints[it->substr(0, pos)] = it->substr(pos + 1);
    else
      constraints[*it] = "";
  }

  //Set the default proxy validity lifetime to 12 hours if there is 
  //no validity lifetime provided by command caller
  if((constraints["validityEnd"].empty()) && 
     (constraints["validityPeriod"].empty()))
    constraints["validityPeriod"] = "43200";

  SSL_load_error_strings();
  SSL_library_init();

  try{
    if (params.size()!=0)
      throw std::invalid_argument("Wrong number of arguments!");

    Arc::User user;

    if (key_path.empty())
      key_path = Arc::GetEnv("X509_USER_KEY");
    if (key_path.empty())
      key_path = (std::string)usercfg.ConfTree()["KeyPath"];
    if (key_path.empty())
      key_path = user.get_uid() == 0 ? "/etc/grid-security/hostkey.pem" :
	user.Home() + "/.globus/userkey.pem";

    if (cert_path.empty())
      cert_path = Arc::GetEnv("X509_USER_CERT");
    if (cert_path.empty())
      cert_path = (std::string)usercfg.ConfTree()["CertificatePath"];
    if (cert_path.empty())
      cert_path = user.get_uid() == 0 ? "/etc/grid-security/hostcert.pem" :
	user.Home() + "/.globus/usercert.pem";

    if (proxy_path.empty())
      proxy_path = Arc::GetEnv("X509_USER_PROXY");
    if (proxy_path.empty())
      proxy_path = (std::string)usercfg.ConfTree()["ProxyPath"];
    if (proxy_path.empty())
      proxy_path = "/tmp/x509up_u" + Arc::tostring(user.get_uid());

    if (ca_dir.empty())
      ca_dir = (std::string)usercfg.ConfTree()["CACertificatesDir"];
    if (ca_dir.empty())
      ca_dir = user.get_uid() == 0 ? "/etc/grid-security/certificates" :
        user.Home() + "/.globus/certificates";

    if (vomses_path.empty())
      vomses_path = (std::string)usercfg.ConfTree()["VOMSServerPath"];

    if(vomses_path.empty()) {
#ifndef WIN32
      struct termios to;
      tcgetattr(STDIN_FILENO,&to);
      to.c_lflag &= ~ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
      Arc::DelegationProvider provider(cert_path,key_path,&std::cin);
      to.c_lflag |= ECHO; tcsetattr(STDIN_FILENO,TCSANOW,&to);
#else
      Arc::DelegationProvider provider(cert_path,key_path,&std::cin);
#endif
      if(!provider) {
        throw std::runtime_error("Failed to acquire credentials");
      };
      Arc::DelegationConsumer consumer;
      if(!consumer) {
        throw std::runtime_error("Failed to generate new private key");
      };
      std::string request;
      if(!consumer.Request(request)) {
        throw std::runtime_error("Failed to generate certificate request");
      };
      std::string proxy = provider.Delegate(request,constraints);
      if(!consumer.Acquire(proxy)) {
        throw std::runtime_error("Failed to generate proxy");
      };
      int f = ::open(proxy_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
      if(f == -1) {
        throw std::runtime_error("Failed to open proxy file " + proxy_path);
      };
      if(::write(f,proxy.c_str(),proxy.length()) != proxy.length()) {
        throw std::runtime_error("Failed to write into proxy file " + proxy_path);
      };
      ::close(f);
      return EXIT_SUCCESS;
    }
    else {
      Arc::Credential signer(cert_path, key_path, ca_dir, "");
      std::string private_key, signing_cert, signing_cert_chain;

      Arc::Time start = constraints["validityStart"].empty() ? Arc::Time() : Arc::Time(constraints["validityStart"]);
      Arc::Period period1 = constraints["validityEnd"].empty() ? Arc::Period(std::string("43200")) : (Arc::Time(constraints["validityEnd"]) - start);
      Arc::Period period = constraints["validityPeriod"].empty() ? period1 : (Arc::Period(constraints["validityPeriod"]));
      int keybits = 1024;
      std::string req_str;
      std::string policy;
      policy = constraints["proxyPolicy"].empty() ? constraints["proxyPolicyFile"] : constraints["proxyPolicy"];
      Arc::Credential cred_request(start, period, keybits, "rfc", policy.empty() ? "inheritAll" : "anylanguage", policy, -1);

      cred_request.GenerateRequest(req_str);

      //Generate a temporary self-signed proxy certificate
      //to contact the voms server
      //Arc::Credential proxy;
      //proxy.InquireRequest(req_str);
      //signer.SignRequest(&proxy, proxy_path.c_str());
      signer.SignRequest(&cred_request, proxy_path.c_str());
      cred_request.OutputPrivatekey(private_key);
      signer.OutputCertificate(signing_cert);
      signer.OutputCertificateChain(signing_cert_chain);
      std::ofstream out_f(proxy_path.c_str(), std::ofstream::app);
      out_f.write(private_key.c_str(), private_key.size());
      out_f.write(signing_cert.c_str(), signing_cert.size());
      out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
      out_f.close();

      //Contact the voms server to retrieve attribute certificate
      std::string voms_server;
      std::string command;
      size_t p;
      p = voms.find(":");
      voms_server = (p == std::string::npos) ? voms : voms.substr(0, p);
      command = (p == std::string::npos) ? "" : voms.substr(p+1);
      std::ifstream in_f(vomses_path.c_str());
      std::string voms_line;
      while(true) {
        voms_line.clear();
        std::getline<char>(in_f,voms_line,'\n');
        if(voms_line.empty()) break;
        size_t pos = voms_line.rfind("\"");
        if(pos != std::string::npos) {
          voms_line.erase(pos);
          pos = voms_line.rfind("\"");
          if(pos != std::string::npos) {
            std::string str = voms_line.substr(pos+1);
            if(str == voms_server) break;
          }
        }
      };
      if(voms_line.empty()) {
        logger.msg(Arc::ERROR, "Can not get voms server information from file: %s ", vomses_path.c_str());
        throw std::runtime_error("Can not get voms server information from file: " + vomses_path); 
      }
      p = 0;
      for(int i=0; i<3; i++) {
        p = voms_line.find("\"", p);
        if(p == std::string::npos) {
          logger.msg(Arc::ERROR, "Can not get voms server address information from voms line: %s\"", voms_line.c_str());
          throw std::runtime_error("Can not get voms server address information from voms line:"+voms_line+"\"");
        }
        p = p+1;
      }
      size_t p1 = voms_line.find("\"",p+1);
      std::string address = voms_line.substr(p, p1-p);
      p = voms_line.find("\"",p1+1);
      p1 = voms_line.find("\"",p+1);
      std::string port = voms_line.substr(p+1, p1-p-1);
      logger.msg(Arc::INFO, "Contacting to VOMS server (named %s): %s on port: %s",voms_server.c_str(),address.c_str(),port.c_str());

      std::string send_msg;
      send_msg.append("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>");
      std::string command_2server;
      if(command.empty()) command_2server.append("G/").append(voms);
      else if(command == "all" || command == "ALL") command_2server.append("A");
      else if(command == "list") command_2server.append("N");
      else {
        std::string::size_type pos = command.find("/Role=");
        if(pos == 0)
          command_2server.append("R").append(command.substr(pos+6));
        else if(pos != std::string::npos && pos > 0)
          command_2server.append("B").append(command.substr(0,pos)).append(":").append(command.substr(pos+6));
      }
      send_msg.append(command_2server).append("</command><lifetime>43200</lifetime></voms>");
      Arc::MCCConfig cfg;
      cfg.AddProxy(proxy_path);
      cfg.AddCADir(ca_dir);
      Arc::ClientTCP client(cfg, address, atoi(port.c_str()), Arc::GSISec);
      Arc::PayloadRaw request;
      request.Insert(send_msg.c_str(),0,send_msg.length());
      Arc::PayloadStreamInterface *response = NULL;
      Arc::MCC_Status status = client.process(&request, &response,true);
      if (!status) {
        logger.msg(Arc::ERROR, (std::string)status);
        if (response)
          delete response;
        return 1;
      }
      if (!response) {
        logger.msg(Arc::ERROR, "No stream response from voms server");
        return 1;
      }
      std::string ret_str;
      int length;
      char ret_buf[1024];
      memset(ret_buf,0,1024);
      int len;
      do {
        len = 1024;
        response->Get(&ret_buf[0], len);
        ret_str.append(ret_buf,len);
        memset(ret_buf,0,1024);
      }while(len == 1024);
      logger.msg(Arc::DEBUG, "Returned msg from voms server: %s ", ret_str.c_str());

      //Put the return attribute certificate into proxy certificate as the extension part
      Arc::XMLNode node(ret_str);
      std::string codedac;
      if(command == "list")
        codedac = (std::string)(node["bitstr"]);
      else 
        codedac = (std::string)(node["ac"]);
      //std::cout<<"Coded AC: "<<codedac<<std::endl;
      std::string decodedac;
      int size;
      char* dec = NULL;
      dec = Arc::VOMSDecode((char *)(codedac.c_str()), codedac.length(), &size);
      decodedac.append(dec,size);
      if(dec!=NULL) { free(dec); dec = NULL; }
      //std::cout<<"Decoded AC: "<<decodedac<<std::endl;

      if(command == "list") {
        logger.msg(Arc::INFO, "The attribute information from voms server: %s is list as following:\n%s", voms_server.c_str(), decodedac.c_str());
        if(response) delete response;
        return EXIT_SUCCESS;  
      }

      ArcCredential::AC** aclist = NULL;
      Arc::addVOMSAC(aclist, decodedac);

      //Arc::Credential proxy1;
      //proxy1.InquireRequest(req_str);

      //proxy1.AddExtension("acseq", (char**) aclist);
      if(aclist != NULL)
        cred_request.AddExtension("acseq", (char**) aclist);
      //signer.SignRequest(&proxy1, proxy_path.c_str());
      signer.SignRequest(&cred_request, proxy_path.c_str());

      std::ofstream out_f1(proxy_path.c_str(), std::ofstream::app);
      out_f1.write(private_key.c_str(), private_key.size());
      out_f1.write(signing_cert.c_str(), signing_cert.size());
      out_f1.write(signing_cert_chain.c_str(), signing_cert_chain.size());
      out_f1.close();

      if(response) delete response;

      return EXIT_SUCCESS;
    }
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    tls_process_error();
    return EXIT_FAILURE;
  }
}
