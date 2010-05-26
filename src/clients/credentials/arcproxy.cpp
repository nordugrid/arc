// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glibmm/stringutils.h>
#include <glibmm/fileutils.h>
#include <glibmm.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/DateTime.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>

#include <openssl/ui.h>

static int create_proxy_file(const std::string& path) {
  int f = -1;
  int err = 0;

  if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove proxy file " + path);
  }
  f = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, S_IRUSR | S_IWUSR);
  if (f == -1) {
    throw std::runtime_error("Failed to create proxy file " + path);
  }
  if(::chmod(path.c_str(), S_IRUSR | S_IWUSR) != 0) {
    ::unlink(path.c_str());
    ::close(f);
    throw std::runtime_error("Failed to change permissions of proxy file " + path);
  }
  return f;
}

static void write_proxy_file(const std::string& path, const std::string& content) {
  std::string::size_type off = 0;
  int f = create_proxy_file(path);
  while(off < content.length()) {
    ssize_t l = ::write(f, content.c_str(), content.length()-off);
    if(l < 0) {
      ::unlink(path.c_str());
      ::close(f);
      throw std::runtime_error("Failed to write into proxy file " + path);
    }
    off += (std::string::size_type)l;
  }
  ::close(f);
}

static void tls_process_error(Arc::Logger& logger) {
  unsigned long err;
  err = ERR_get_error();
  if (err != 0) {
    logger.msg(Arc::ERROR, "OpenSSL Error -- %s", ERR_error_string(err, NULL));
    logger.msg(Arc::ERROR, "Library  : %s", ERR_lib_error_string(err));
    logger.msg(Arc::ERROR, "Function : %s", ERR_func_error_string(err));
    logger.msg(Arc::ERROR, "Reason   : %s", ERR_reason_error_string(err));
  }
  return;
}

#define PASS_MIN_LENGTH 6
static int input_password(char *password, int passwdsz, bool verify,
                          const std::string prompt_info,
                          const std::string prompt_verify_info,
                          Arc::Logger& logger) {
  UI *ui = NULL;
  int res = 0;
  ui = UI_new();
  if (ui) {
    int ok = 0;
    char buf[256];
    memset(buf, 0, 256);
    int ui_flags = 0;
    char *prompt1 = NULL;
    char *prompt2 = NULL;
    prompt1 = UI_construct_prompt(ui, "passphrase", prompt_info.c_str());
    prompt2 = UI_construct_prompt(ui, "passphrase", prompt_verify_info.c_str());
    ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
    UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);
    ok = UI_add_input_string(ui, prompt1, ui_flags, password,
                             PASS_MIN_LENGTH, BUFSIZ - 1);
    if (ok >= 0 && verify)
      ok = UI_add_verify_string(ui, prompt2, ui_flags, buf,
                                PASS_MIN_LENGTH, BUFSIZ - 1, password);
    if (ok >= 0)
      do
        ok = UI_process(ui);
      while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));

    if (ok >= 0)
      res = strlen(password);
    if (ok == -1) {
      logger.msg(Arc::ERROR, "User interface error");
      tls_process_error(logger);
      memset(password, 0, (unsigned int)passwdsz);
      res = 0;
    }
    if (ok == -2) {
      logger.msg(Arc::ERROR, "Aborted!");
      memset(password, 0, (unsigned int)passwdsz);
      res = 0;
    }
    UI_free(ui);
    OPENSSL_free(prompt1);
    OPENSSL_free(prompt2);
  }
  return res;
}

int main(int argc, char *argv[]) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcproxy");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(" ",
                            istring("The arcproxy command creates a proxy from a key/certificate pair for use in\n"
                                    "the ARC middleware"),
                            istring("Supported constraints are:\n"
                                    "  validityStart=time (e.g. 2008-05-29T10:20:30Z; if not specified, start from now)\n"
                                    "  validityEnd=time\n"
                                    "  validityPeriod=time (e.g. 43200 or 12h or 12H; if both validityPeriod and validityEnd not specified, the default is 12 hours)\n"
                                    "  vomsACvalidityPeriod=time (e.g. 43200 or 12h or 12H; if not specified, validityPeriod is used\n"
                                    "  proxyPolicy=policy content\n"
                                    "  proxyPolicyFile=policy file"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to proxy file"),
                    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "cert", istring("path to certificate file"),
                    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to private key file"),
                    istring("path"), key_path);

  std::string ca_dir;
  options.AddOption('T', "cadir", istring("path to trusted certificate directory, only needed for voms client functionality"),
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

  std::list<std::string> orderlist;
  options.AddOption('o', "order", istring("group<:role>. Specify ordering of attributes \n"
                    "              Example: --order /knowarc.eu/coredev:Developer,/knowarc.eu/testers:Tester \n"
                    "              or: --order /knowarc.eu/coredev:Developer --order /knowarc.eu/testers:Tester \n"
                    " Note that it does not make sense to specify the order if you have two or more different voms server specified"),
                    istring("string"), orderlist);

  bool use_gsi_comm = false;
  options.AddOption('G', "gsicom", istring("use GSI communication protocol for contacting VOMS services"), use_gsi_comm);

  bool use_gsi_proxy = false;
  options.AddOption('O', "old", istring("use GSI proxy (RFC 3820 compliant proxy is default)"), use_gsi_proxy);

  bool info = false;
  options.AddOption('I', "info", istring("print all information about this proxy. \n"
                                         "              In order to show the Identity (DN without CN as subfix for proxy) \n"
                                         "              of the certificate, the 'trusted certdir' is needed."
                                         ),
                    info);

  std::string user_name; //user name to myproxy server
  options.AddOption('U', "user", istring("username to myproxy server"),
                    istring("string"), user_name);

  std::string passphrase; //passphrase to myproxy server
//  options.AddOption('R', "pass", istring("passphrase to myproxy server"),
//                    istring("string"), passphrase);

  std::string myproxy_server; //url of myproxy server
  options.AddOption('L', "myproxysrv", istring("hostname[:port] of myproxy server"),
                    istring("string"), myproxy_server);

  std::string myproxy_command; //command to myproxy server
  options.AddOption('M', "myproxycmd", istring("command to myproxy server. The command can be PUT and GET.\n"
                                               "              PUT/put -- put a delegated credential to myproxy server; \n"
                                               "              GET/get -- get a delegated credential from myproxy server, \n"
                                               "              credential (certificate and key) is not needed in this case. \n"
                                               "              myproxy functionality can be used together with voms\n"
                                               "              functionality.\n"
                                               ),
                    istring("string"), myproxy_command);

  std::list<std::string> constraintlist;
  options.AddOption('c', "constraint", istring("proxy constraints"),
                    istring("string"), constraintlist);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile, Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials));
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return EXIT_FAILURE;
  }

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcproxy", VERSION) << std::endl;
    return EXIT_SUCCESS;
  }

  Arc::User user;

  try {
    if (params.size() != 0)
      throw std::invalid_argument("Wrong number of arguments!");

    // TODO: Conver to UserConfig. But only after UserConfig is extended.
    if (key_path.empty())
      key_path = Arc::GetEnv("X509_USER_KEY");
    if (key_path.empty())
      key_path = usercfg.KeyPath();
    if (key_path.empty())
#ifndef WIN32
      key_path = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "userkey.pem";
#else
      key_path = std::string(g_get_home_dir()) + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "userkey.pem";
#endif
    if (!Glib::file_test(key_path, Glib::FILE_TEST_IS_REGULAR))
      key_path = "";

    if (cert_path.empty())
      cert_path = Arc::GetEnv("X509_USER_CERT");
    if (cert_path.empty())
      cert_path = usercfg.CertificatePath();
    if (cert_path.empty())
#ifndef WIN32
      cert_path = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "usercert.pem";
#else
      cert_path = std::string(g_get_home_dir()) + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "usercert.pem";
#endif
    if (!Glib::file_test(cert_path, Glib::FILE_TEST_IS_REGULAR))
      cert_path = "";

    if (proxy_path.empty())
      proxy_path = Arc::GetEnv("X509_USER_PROXY");
    if (proxy_path.empty())
      proxy_path = usercfg.ProxyPath();
    if (proxy_path.empty())
      proxy_path = Glib::build_filename(Glib::get_tmp_dir(), "x509up_u" + Arc::tostring(user.get_uid()));

    if (ca_dir.empty())
      ca_dir = Arc::GetEnv("X509_CERT_DIR");
    if (ca_dir.empty())
      ca_dir = usercfg.CACertificatesDirectory();
    if (ca_dir.empty()) {
      ca_dir = std::string(g_get_home_dir()) + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
    if (ca_dir.empty()) {
      ca_dir = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
#ifndef WIN32
    if (ca_dir.empty()) {
      ca_dir = "/etc/grid-security/certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
#endif
    if (ca_dir.empty()) {
      ca_dir = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "grid-security" + G_DIR_SEPARATOR_S + "certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
    if (ca_dir.empty()) {
      ca_dir = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
    if (ca_dir.empty()) {
      ca_dir = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + "share" + G_DIR_SEPARATOR_S + "certificates";
      if (!Glib::file_test(ca_dir, Glib::FILE_TEST_IS_DIR))
        ca_dir = "";
    }
  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    tls_process_error(logger);
    return EXIT_FAILURE;
  }

  if (ca_dir.empty()) {
    logger.msg(Arc::ERROR, "Cannot find the CA Certificate Directory path, "
               "please setup environment X509_CERT_DIR, "
               "or cacertificatesdirectory in configuration file");
    return EXIT_FAILURE;
  }

  const Arc::Time now;

  if (info) {
    std::vector<std::string> voms_attributes;
    bool res = false;

    if (proxy_path.empty()) {
      logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                 "please setup environment X509_USER_PROXY, "
                 "or proxypath in configuration file");
      return EXIT_FAILURE;
    }
    else if (!(Glib::file_test(proxy_path, Glib::FILE_TEST_EXISTS))) {
      logger.msg(Arc::ERROR, "Cannot find file at %s for getting the proxy. "
                 "Please make sure this file exists.", proxy_path);
      return EXIT_FAILURE;
    }

    Arc::Credential holder(proxy_path, "", ca_dir, "");
    std::cout << Arc::IString("Subject: %s", holder.GetDN()) << std::endl;
    std::cout << Arc::IString("Identity: %s", holder.GetIdentityName()) << std::endl;
    if (holder.GetEndTime() < now)
      std::cout << Arc::IString("Time left for proxy: Proxy expired") << std::endl;
    else if (holder.GetVerification())
      std::cout << Arc::IString("Time left for proxy: %s", (holder.GetEndTime() - now).istr()) << std::endl;
    else
      std::cout << Arc::IString("Time left for proxy: Proxy not valid") << std::endl;
    std::cout << Arc::IString("Proxy path: %s", proxy_path) << std::endl;
    std::cout << Arc::IString("Proxy type: %s", certTypeToString(holder.GetType())) << std::endl;

    std::vector<std::string> voms_trust_dn;
    res = parseVOMSAC(holder, "", "", voms_trust_dn, voms_attributes, false);
    if (!res)
      logger.msg(Arc::ERROR, "VOMS attribute parsing failed");

    return EXIT_SUCCESS;
  }

  if (cert_path.empty() || key_path.empty()) {
    if (cert_path.empty())
      logger.msg(Arc::ERROR, "Cannot find the user certificate path, "
                 "please setup environment X509_USER_CERT, "
                 "or certificatepath in configuration file");
    if (key_path.empty())
      logger.msg(Arc::ERROR, "Cannot find the user private key path, "
                 "please setup environment X509_USER_KEY, "
                 "or keypath in configuration file");
    return EXIT_FAILURE;
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
  if ((constraints["validityEnd"].empty()) &&
      (constraints["validityPeriod"].empty()))
    constraints["validityPeriod"] = "43200";

  //If the period is formated with hours, e.g., 12h, then change
  //it into seconds
  if (!(constraints["validityPeriod"].empty()) &&
      ((constraints["validityPeriod"].rfind("h") != std::string::npos) ||
       (constraints["validityPeriod"].rfind("H") != std::string::npos))) {
    unsigned long tmp;
    tmp = strtoll(constraints["validityPeriod"].c_str(), NULL, 0);
    tmp = tmp * 3600;
    std::string strtmp = Arc::tostring(tmp);
    constraints["validityPeriod"] = strtmp;
  }

  if (!(constraints["vomsACvalidityPeriod"].empty()) &&
      ((constraints["vomsACvalidityPeriod"].rfind("h") != std::string::npos) ||
       (constraints["vomsACvalidityPeriod"].rfind("H") != std::string::npos))) {
    unsigned long tmp;
    tmp = strtoll(constraints["vomsACvalidityPeriod"].c_str(), NULL, 0);
    tmp = tmp * 3600;
    std::string strtmp = Arc::tostring(tmp);
    constraints["vomsACvalidityPeriod"] = strtmp;
  }

  //Set the default proxy validity lifetime to 12 hours if there is
  //no validity lifetime provided by command caller
  if (constraints["vomsACvalidityPeriod"].empty()) {
    if ((constraints["validityEnd"].empty()) &&
        (constraints["validityPeriod"].empty()))
      constraints["vomsACvalidityPeriod"] = "43200";
    else if ((constraints["validityEnd"].empty()) &&
             (!(constraints["validityPeriod"].empty())))
      constraints["vomsACvalidityPeriod"] = constraints["validityPeriod"];
    else
      constraints["vomsACvalidityPeriod"] = constraints["validityStart"].empty() ? (Arc::Time(constraints["validityEnd"]) - now) : (Arc::Time(constraints["validityEnd"]) - Arc::Time(constraints["validityStart"]));
  }

  std::string voms_period = Arc::tostring(Arc::Period(constraints["vomsACvalidityPeriod"]).GetPeriod());

  Arc::OpenSSLInit();

  //If the "GET" myproxy command is given, try to get a delegated
  //certificate from the myproxy server.
  //For "GET" command, certificate and key are not needed, and
  //anonymous GSSAPI is used (GSS_C_ANON_FLAG)
  try {
    if (myproxy_command == "get" || myproxy_command == "GET") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of myproxy server is missing");
      if (user_name.empty())
        throw std::invalid_argument("Username to myproxy server is missing");
//      if (passphrase.empty())
//        throw std::invalid_argument("Passphrase to myproxy server is missing");
      std::string prompt1 = "MyProxy server";
      char password[256];
      int res = input_password(password, 256, false, prompt1, "", logger);
      if (!res)
        throw std::invalid_argument("Error entering passphrase");
      passphrase = password;
      std::string proxy_cred_str_pem;
      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      myproxyopt["lifetime"] = "43200";
      if(!cstore.Retrieve(myproxyopt,proxy_cred_str_pem))
        throw std::invalid_argument("Failed to retrieve proxy from MyProxy service");
      write_proxy_file(proxy_path,proxy_cred_str_pem);

      //return EXIT_SUCCESS;
      //Assign proxy_path to cert_path and key_path,
      //so the later voms functionality can use the proxy_path
      //to create proxy with voms AC extension. In this
      //case, "--cert" and "--key" is not needed.
      cert_path = proxy_path;
      key_path = proxy_path;
    }

  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    tls_process_error(logger);
    return EXIT_FAILURE;
  }

  //Create proxy or voms proxy
  try {
    std::cout << Arc::IString("Your identity: %s", Arc::Credential(cert_path, "", "", "").GetDN()) << std::endl;

    Arc::Credential signer(cert_path, key_path, ca_dir, "");
    if((signer.GetVerification()) == false) {
      if (now > signer.GetEndTime())
        std::cerr << Arc::IString("Proxy generation failed: Certificate has expired.") << std::endl;
      else if (now < signer.GetStartTime())
        std::cerr << Arc::IString("Proxy generation failed: Certificate is not valid yet.") << std::endl;
      else
        std::cerr << Arc::IString("Proxy generation failed: Unable to verify certificate.") << std::endl;
      return EXIT_FAILURE;
    }

    std::string private_key, signing_cert, signing_cert_chain;

    Arc::Time start = constraints["validityStart"].empty() ? Arc::Time() : Arc::Time(constraints["validityStart"]);
    Arc::Period period1 = constraints["validityEnd"].empty() ? Arc::Period(std::string("43200")) : (Arc::Time(constraints["validityEnd"]) - start);
    Arc::Period period = constraints["validityPeriod"].empty() ? period1 : (Arc::Period(constraints["validityPeriod"]));
    int keybits = 1024;
    std::string req_str;
    std::string policy;
    policy = constraints["proxyPolicy"].empty() ? constraints["proxyPolicyFile"] : constraints["proxyPolicy"];
    //Arc::Credential cred_request(start, period, keybits, "rfc", policy.empty() ? "inheritAll" : "anylanguage", policy, -1);
    Arc::Credential cred_request(start - Arc::Period(300), period, keybits);
    cred_request.GenerateRequest(req_str);
    cred_request.OutputPrivatekey(private_key);
    signer.OutputCertificate(signing_cert);
    signer.OutputCertificateChain(signing_cert_chain);

    if (!vomslist.empty()) { //If we need to generate voms proxy

      //Generate a temporary self-signed proxy certificate
      //to contact the voms server
      std::string proxy_cert;
      if (!signer.SignRequest(&cred_request, proxy_cert))
        throw std::runtime_error("Failed to sign proxy");
      proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);
      write_proxy_file(proxy_path,proxy_cert);
/*
      signer.SignRequest(&cred_request, proxy_path.c_str());
      std::ofstream out_f(proxy_path.c_str(), std::ofstream::app);
      out_f.write(private_key.c_str(), private_key.size());
      out_f.write(signing_cert.c_str(), signing_cert.size());
      out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
      out_f.close();
*/
      //Parse the voms server and command from command line
      std::multimap<std::string, std::string> server_command_map;
      for (std::list<std::string>::iterator it = vomslist.begin();
           it != vomslist.end(); it++) {
        size_t p;
        std::string voms_server;
        std::string command;
        p = (*it).find(":");
        voms_server = (p == std::string::npos) ? (*it) : (*it).substr(0, p);
        command = (p == std::string::npos) ? "" : (*it).substr(p + 1);
        server_command_map.insert(std::pair<std::string, std::string>(voms_server, command));
      }

      //Parse the 'vomses' file to find configure lines corresponding to
      //the information from the command line
      if (vomses_path.empty())
        vomses_path = Arc::GetEnv("X509_VOMS_DIR");
      if (vomses_path.empty())
        vomses_path = usercfg.VOMSServerPath();

      if (vomses_path.empty()) {
        vomses_path = "/etc/grid-security/vomses";
        if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
          vomses_path = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "grid-security" + G_DIR_SEPARATOR_S + "vomses";
          if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
            vomses_path = user.Home() + G_DIR_SEPARATOR_S + ".vomses";
            if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
              vomses_path = G_DIR_SEPARATOR_S;
              vomses_path.append("etc").append(G_DIR_SEPARATOR_S).append("grid-security").append(G_DIR_SEPARATOR_S).append("vomses");
              if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
                vomses_path = G_DIR_SEPARATOR_S;
                vomses_path.append("etc").append(G_DIR_SEPARATOR_S).append("vomses");
                if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
                  vomses_path = user.Home() + G_DIR_SEPARATOR_S + ".voms" + G_DIR_SEPARATOR_S + "vomses";
                  if (!Glib::file_test(vomses_path, Glib::FILE_TEST_IS_REGULAR)) {
                    std::string tmp1 = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "grid-security" + G_DIR_SEPARATOR_S + "vomses";
                    std::string tmp2 = user.Home() + G_DIR_SEPARATOR_S + ".vomses";
                    std::string tmp3 = G_DIR_SEPARATOR_S;
                    tmp3.append("etc").append(G_DIR_SEPARATOR_S).append("grid-security").append(G_DIR_SEPARATOR_S).append("vomses");
                    std::string tmp4 = G_DIR_SEPARATOR_S;
                    tmp4.append("etc").append(G_DIR_SEPARATOR_S).append("vomses");
                    std::string tmp5 = user.Home() + G_DIR_SEPARATOR_S + ".voms" + G_DIR_SEPARATOR_S + "vomses";
                    logger.msg(Arc::ERROR, "Cannot find vomses at %s, %s, %s, %s and %s",
                               tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), tmp4.c_str(), tmp5.c_str());
                    return EXIT_FAILURE;
                  }
                }
              }
            }
          }
        }
      }

      std::ifstream in_f(vomses_path.c_str());
      std::map<std::string, std::string> matched_voms_line;
      std::string voms_line;
      while (true) {
        voms_line.clear();
        std::getline<char>(in_f, voms_line, '\n');
        if (voms_line.empty())
          break;
        if((voms_line.size() >= 1) && (voms_line[0] == '#')) continue;

        size_t pos = voms_line.rfind("\"");
        std::string str;
        if (pos != std::string::npos) {
          voms_line.erase(pos);
          pos = voms_line.rfind("\"");
          if (pos != std::string::npos) {
            str = voms_line.substr(pos + 1);
            for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
                 it != server_command_map.end(); it++) {
              std::string voms_server = (*it).first;
              if (str == voms_server) {
                matched_voms_line[voms_server] = voms_line;
                break;
              };
            };
          };
        };

        //you can also use the acronym name of the voms server
        size_t pos1 = voms_line.find("\"");
        if (pos1 != std::string::npos) {
          size_t pos2 = voms_line.find("\"", pos1+1);
          if (pos2 != std::string::npos) {
            std::string str1 = voms_line.substr(pos1+1, pos2-pos1-1);
            for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
                 it != server_command_map.end(); it++) {
              std::string voms_server = (*it).first;
              if (str1 == voms_server) {
                matched_voms_line[str] = voms_line;
                break;
              };
            };
          };
        };

      }
      //Judge if we can not find any of the voms server in the command line from 'vomses' file
      //if(matched_voms_line.empty()) {
      //  logger.msg(Arc::ERROR, "Cannot get voms server information from file: %s ", vomses_path.c_str());
      // throw std::runtime_error("Cannot get voms server information from file: " + vomses_path);
      //}
      if (matched_voms_line.size() != server_command_map.size())
        for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
             it != server_command_map.end(); it++)
          if (matched_voms_line.find((*it).first) == matched_voms_line.end())
            logger.msg(Arc::ERROR, "Cannot get voms server %s information from file: %s ",
                       (*it).first, vomses_path.c_str());

      //Contact the voms server to retrieve attribute certificate
      std::string voms_server;
      std::list<std::string> command_list;
      ArcCredential::AC **aclist = NULL;
      std::string acorder;
      Arc::MCCConfig cfg;
      cfg.AddProxy(proxy_path);
      cfg.AddCADir(ca_dir);

      for (std::map<std::string, std::string>::iterator it = matched_voms_line.begin();
           it != matched_voms_line.end(); it++) {
        std::map<std::string, std::string>::iterator it = matched_voms_line.begin();
        voms_server = (*it).first;
        voms_line = (*it).second;
        int count = server_command_map.count(voms_server);
        logger.msg(Arc::DEBUG, "There are %d commands to the same voms server %s\n", count, voms_server.c_str());

        std::multimap<std::string, std::string>::iterator command_it;
        for(command_it = server_command_map.equal_range(voms_server).first; command_it!=server_command_map.equal_range(voms_server).second; ++command_it) {
          command_list.push_back((*command_it).second);
        }

        size_t p = 0;
        for (int i = 0; i < 3; i++) {
          p = voms_line.find("\"", p);
          if (p == std::string::npos) {
            logger.msg(Arc::ERROR, "Cannot get voms server address information from voms line: %s\"", voms_line.c_str());
            throw std::runtime_error("Cannot get voms server address information from voms line:" + voms_line + "\"");
          }
          p = p + 1;
        }

        size_t p1 = voms_line.find("\"", p + 1);
        std::string address = voms_line.substr(p, p1 - p);
        p = voms_line.find("\"", p1 + 1);
        p1 = voms_line.find("\"", p + 1);
        std::string port = voms_line.substr(p + 1, p1 - p - 1);
        logger.msg(Arc::INFO, "Contacting VOMS server (named %s): %s on port: %s",
                   voms_server.c_str(), address.c_str(), port.c_str());
        std::cout << Arc::IString("Contacting VOMS server (named %s): %s on port: %s", voms_server, address, port) << std::endl;

        std::string send_msg;
        send_msg.append("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms>");
        std::string command;

        for(std::list<std::string>::iterator c_it = command_list.begin(); c_it != command_list.end(); c_it++) {
          std::string command_2server;
          command = *c_it;
          if (command.empty())
            command_2server.append("G/").append(voms_server);
          else if (command == "all" || command == "ALL")
            command_2server.append("A");
          else if (command == "list")
            command_2server.append("N");
          else {
            std::string::size_type pos = command.find("/Role=");
            if (pos == 0)
              command_2server.append("R").append(command.substr(pos + 6));
            else if (pos != std::string::npos && pos > 0)
              command_2server.append("B").append(command.substr(0, pos)).append(":").append(command.substr(pos + 6));
            else if(command[0] == '/')
              command_2server.append("G").append(command);
          }
          send_msg.append("<command>").append(command_2server).append("</command>");
        }

        std::string ordering;
        for(std::list<std::string>::iterator o_it = orderlist.begin(); o_it != orderlist.end(); o_it++) {
          ordering.append(o_it == orderlist.begin() ? "" : ",").append(*o_it);
        }
        logger.msg(Arc::VERBOSE, "Try to get attribute from voms server with order: %s ", ordering.c_str());
        send_msg.append("<order>").append(ordering).append("</order>");

        send_msg.append("<lifetime>").append(voms_period).append("</lifetime></voms>");
        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), use_gsi_comm ? Arc::GSISec : Arc::SSL3Sec, usercfg.Timeout());
        Arc::PayloadRaw request;
        request.Insert(send_msg.c_str(), 0, send_msg.length());
        Arc::PayloadStreamInterface *response = NULL;
        Arc::MCC_Status status = client.process(&request, &response, true);
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
        char ret_buf[1024];
        memset(ret_buf, 0, 1024);
        int len;
        do {
          len = 1024;
          response->Get(&ret_buf[0], len);
          ret_str.append(ret_buf, len);
          memset(ret_buf, 0, 1024);
        } while (len == 1024);
        logger.msg(Arc::VERBOSE, "Returned msg from voms server: %s ", ret_str.c_str());
        Arc::XMLNode node(ret_str);

        if (ret_str.find("error") != std::string::npos) {
          std::string str = node["error"]["item"]["message"];
          throw std::runtime_error("Cannot get any AC or attributes info from voms server: " + voms_server + ";\n       Returned msg from voms server: " + str);
        }

        //Put the return attribute certificate into proxy certificate as the extension part
        std::string codedac;
        if (command == "list")
          codedac = (std::string)(node["bitstr"]);
        else
          codedac = (std::string)(node["ac"]);
        std::string decodedac;
        int size;
        char *dec = NULL;
        dec = Arc::VOMSDecode((char*)(codedac.c_str()), codedac.length(), &size);
        decodedac.append(dec, size);
        if (dec != NULL) {
          free(dec);
          dec = NULL;
        }

        if (command == "list") {
          //logger.msg(Arc::INFO, "The attribute information from voms server: %s is list as following:\n%s",
          //           voms_server.c_str(), decodedac.c_str());
          std::cout << Arc::IString("The attribute information from voms server: %s is list as following", voms_server) << std::endl << decodedac;
          if (response)
            delete response;
          return EXIT_SUCCESS;
        }

        if (response)
          delete response;

        Arc::addVOMSAC(aclist, acorder, decodedac);
      }

      //Put the returned attribute certificate into proxy certificate
      if (aclist != NULL)
        cred_request.AddExtension("acseq", (char**)aclist);
      if (!acorder.empty())
        cred_request.AddExtension("order", acorder);
    }

    if (!use_gsi_proxy)
      cred_request.SetProxyPolicy("rfc", policy.empty() ? "inheritAll" : "anylanguage", policy, -1);
    else
      cred_request.SetProxyPolicy("gsi2", "", "", -1);

    std::string proxy_cert;
    if (!signer.SignRequest(&cred_request, proxy_cert))
      throw std::runtime_error("Failed to sign proxy");

    proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);

    write_proxy_file(proxy_path,proxy_cert);

    Arc::Credential proxy_cred(proxy_path, proxy_path, ca_dir, "");
    Arc::Time left = proxy_cred.GetEndTime();
    std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
    std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

    //return EXIT_SUCCESS;
  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    tls_process_error(logger);
    return EXIT_FAILURE;
  }

  //Delegate the former self-delegated credential to
  //myproxy server
  try {
    if (myproxy_command == "put" || myproxy_command == "PUT") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of myproxy server is missing");
      if (user_name.empty())
        throw std::invalid_argument("Username to myproxy server is missing");
//      if (passphrase.empty())
//        throw std::invalid_argument("Passphrase to myproxy server is missing");
      std::string prompt1 = "MyProxy server";
      std::string prompt2 = "MyProxy server";
      char password[256];
      int res = input_password(password, 256, true, prompt1, prompt2, logger);
      if (!res)
        throw std::invalid_argument("Error entering passphrase");
      passphrase = password;

      std::string proxy_cred_str_pem;
      std::ifstream proxy_cred_file(proxy_path.c_str());
      if(!proxy_cred_file) 
        throw std::invalid_argument("Failed to read proxy file "+proxy_path);
      std::getline(proxy_cred_file,proxy_cred_str_pem,'\0');
      if(proxy_cred_str_pem.empty())
        throw std::invalid_argument("Failed to read proxy file "+proxy_path);
      proxy_cred_file.close();

      Arc::CredentialStore cstore(usercfg,Arc::URL("myproxy://"+myproxy_server));
      std::map<std::string,std::string> myproxyopt;
      myproxyopt["username"] = user_name;
      myproxyopt["password"] = passphrase;
      myproxyopt["lifetime"] = "43200";
      if(!cstore.Store(myproxyopt,proxy_cred_str_pem))
        throw std::invalid_argument("Failed to delegate proxy to MyProxy service");

      return EXIT_SUCCESS;
    }
  } catch (std::exception& err) {
    logger.msg(Arc::ERROR, err.what());
    tls_process_error(logger);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}
