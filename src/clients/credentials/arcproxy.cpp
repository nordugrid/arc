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
#include <glibmm/stringutils.h>
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
				    "  validityStart=time (e.g. 2008-05-29T10:20:30Z; if not specified, start from now)\n"
				    "  validityEnd=time\n"
				    "  validityPeriod=time (e.g. 43200; if both validityPeriod and validityEnd not specified, the default is 12 hours)\n"
                                    "  vomsACvalidityPeriod=time (e.g. 43200; if not specified, validityPeriod is used\n"
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
  options.AddOption('T', "trusted certdir", istring("path to trusted certificate directory, only needed for voms client functionality"),
                    istring("path"), ca_dir);

  std::string vomses_path;
  options.AddOption('V', "vomses", istring("path to voms server configuration file"),
                    istring("path"), vomses_path);

  std::list<std::string> vomslist;
  options.AddOption('S', "voms", istring("voms<:command>. Specify voms server (More than one voms server \n"
                    "              can be specified like this: --voms VOa:command1 --voms VOb:command2). \n"
                    "              :command is optional, and is used to ask for specific attributes(e.g: roles)\n"
                    "              command option is:\n"
                    "              all --- put all of this DN's attributes into AC; \n"
                    "              list ---list all of the DN's attribute,will not create AC extension; \n"
                    "              /Role=yourRole --- specify the role, if this DN \n"
                    "                               has such a role, the role will be put into AC \n"
                    "              /voname/groupname/Role=yourRole --- specify the vo,group and role if this DN \n"
                    "                               has such a role, the role will be put into AC \n"
                    ),
                    istring("string"), vomslist);

  bool info = false;
  options.AddOption('I', "info", istring("print all information about this proxy. \n"
                    "              In order to show the Identity (DN without CN as subfix for proxy) \n"
                    "              of the certificate, the 'trusted certdir' is needed."
                    ),
                    info);

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
    return EXIT_FAILURE;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcproxy", VERSION) << std::endl;
    return EXIT_SUCCESS;
  }

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
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    tls_process_error();
    return EXIT_FAILURE;
  }

  if(info) {
    std::vector<std::string> voms_attributes;
    bool res = false;
    
    Arc::Credential holder(proxy_path, "", ca_dir, "");
    std::cout<<"Subject:  "<<holder.GetDN()<<std::endl;
    std::cout<<"Identity: "<<holder.GetIdentityName()<<std::endl;
    std::cout<<"Timeleft for proxy: "<<(holder.GetEndTime() - Arc::Time())<<std::endl;

    std::vector<std::string> voms_trust_dn;
    res = parseVOMSAC(holder, "", "", voms_trust_dn, voms_attributes, false);
    if(!res) {
      logger.msg(Arc::ERROR,"VOMS attribute parsing failed");
    };

    return EXIT_SUCCESS;
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

  //Set the default proxy validity lifetime to 12 hours if there is
  //no validity lifetime provided by command caller
  if(constraints["vomsACvalidityPeriod"].empty()) {
    if((constraints["validityEnd"].empty()) &&
       (constraints["validityPeriod"].empty()))
      constraints["vomsACvalidityPeriod"] = "43200";
    else if((constraints["validityEnd"].empty()) &&
       (!(constraints["validityPeriod"].empty())))
      constraints["vomsACvalidityPeriod"] = constraints["validityPeriod"];
    else
      constraints["vomsACvalidityPeriod"] = constraints["validityStart"].empty() ? (Arc::Time(constraints["validityEnd"]) - Arc::Time()) : (Arc::Time(constraints["validityEnd"]) - Arc::Time(constraints["validityStart"]));
  }

  std::string voms_period = Glib::Ascii::dtostr(Arc::Period(constraints["vomsACvalidityPeriod"]).GetPeriod());

  SSL_load_error_strings();
  SSL_library_init();

  try{
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
      //Arc::Credential cred_request(start, period, keybits, "rfc", policy.empty() ? "inheritAll" : "anylanguage", policy, -1);
      Arc::Credential cred_request(start, period, keybits);
      cred_request.GenerateRequest(req_str);

      //Generate a temporary self-signed proxy certificate
      //to contact the voms server
      signer.SignRequest(&cred_request, proxy_path.c_str());
      cred_request.OutputPrivatekey(private_key);
      signer.OutputCertificate(signing_cert);
      signer.OutputCertificateChain(signing_cert_chain);
      std::ofstream out_f(proxy_path.c_str(), std::ofstream::app);
      out_f.write(private_key.c_str(), private_key.size());
      out_f.write(signing_cert.c_str(), signing_cert.size());
      out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
      out_f.close();

      //Parse the voms server and command from command line
      std::map<std::string, std::string> server_command_map;
      for (std::list<std::string>::iterator it = vomslist.begin();
        it != vomslist.end(); it++) {
        size_t p;
        std::string voms_server;
        std::string command;
        p = (*it).find(":");
        voms_server = (p == std::string::npos) ? (*it) : (*it).substr(0, p);
        command = (p == std::string::npos) ? "" : (*it).substr(p+1);
        server_command_map[voms_server] = command;
      }

      //Parse the 'vomses' file to find configure lines corresponding to 
      //the information from the command line
      std::ifstream in_f(vomses_path.c_str());
      std::map<std::string, std::string> matched_voms_line;
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
            for (std::map<std::string, std::string>::iterator it = server_command_map.begin();
                 it != server_command_map.end(); it++) {
              std::string voms_server = (*it).first;
              if(str == voms_server) { matched_voms_line[voms_server] = voms_line; break; }
            };
          };
        };
      };
      //Judge if we can not find any of the voms server in the command line from 'vomses' file
      //if(matched_voms_line.empty()) {
      //  logger.msg(Arc::ERROR, "Can not get voms server information from file: %s ", vomses_path.c_str());
      // throw std::runtime_error("Can not get voms server information from file: " + vomses_path); 
      //}
      if(matched_voms_line.size() != server_command_map.size()) {
        for (std::map<std::string, std::string>::iterator it = server_command_map.begin();
             it != server_command_map.end(); it++) {
          if(matched_voms_line.find((*it).first) == matched_voms_line.end())
            logger.msg(Arc::ERROR, "Can not get voms server %s information from file: %s ", 
              (*it).first, vomses_path.c_str());
        };
      };

      //Contact the voms server to retrieve attribute certificate
      std::string voms_server;
      std::string command;
      ArcCredential::AC** aclist = NULL;
      Arc::MCCConfig cfg;
      cfg.AddProxy(proxy_path);
      cfg.AddCADir(ca_dir);

      //for (std::map<std::string, std::string>::iterator it = matched_voms_line.begin();
      //       it !=  matched_voms_line.end(); it++) {
        std::map<std::string, std::string>::iterator it = matched_voms_line.begin();
        voms_server = (*it).first;
        voms_line = (*it).second;
        command = server_command_map[voms_server];
        size_t p = 0;
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
        logger.msg(Arc::INFO, "Contacting to VOMS server (named %s): %s on port: %s",
            voms_server.c_str(),address.c_str(),port.c_str());

        std::string send_msg;
        send_msg.append("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>");
        std::string command_2server;
        if(command.empty()) command_2server.append("G/").append(voms_server);
        else if(command == "all" || command == "ALL") command_2server.append("A");
        else if(command == "list") command_2server.append("N");
        else {
          std::string::size_type pos = command.find("/Role=");
          if(pos == 0)
            command_2server.append("R").append(command.substr(pos+6));
          else if(pos != std::string::npos && pos > 0)
            command_2server.append("B").append(command.substr(0,pos)).append(":").append(command.substr(pos+6));
        }
        send_msg.append(command_2server).append("</command><lifetime>").append(voms_period).append("</lifetime></voms>");
        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), Arc::GSISec);
        Arc::PayloadRaw request;
        request.Insert(send_msg.c_str(),0,send_msg.length());
        Arc::PayloadStreamInterface *response = NULL;
        Arc::MCC_Status status = client.process(&request, &response,true);
        if (!status) {
          logger.msg(Arc::ERROR, (std::string)status);
          if (response)
            delete response;
            return EXIT_FAILURE;
        }
        if (!response) {
          logger.msg(Arc::ERROR, "No stream response from voms server");
          return EXIT_FAILURE;
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
          logger.msg(Arc::INFO, "The attribute information from voms server: %s is list as following:\n%s", 
            voms_server.c_str(), decodedac.c_str());
          if(response) delete response;
          return EXIT_SUCCESS;  
        }

        if(response) delete response;

        Arc::addVOMSAC(aclist, decodedac);
      //}

      //Put the returned attribute certificate into proxy certificate
      if(aclist != NULL)
        cred_request.AddExtension("acseq", (char**) aclist);
      cred_request.SetProxyPolicy("rfc", policy.empty() ? "inheritAll" : "anylanguage", policy, -1);
      signer.SignRequest(&cred_request, proxy_path.c_str());

      std::ofstream out_f1(proxy_path.c_str(), std::ofstream::app);
      out_f1.write(private_key.c_str(), private_key.size());
      out_f1.write(signing_cert.c_str(), signing_cert.size());
      out_f1.write(signing_cert_chain.c_str(), signing_cert_chain.size());
      out_f1.close();

      return EXIT_SUCCESS;
    }
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    tls_process_error();
    return EXIT_FAILURE;
  }
}
