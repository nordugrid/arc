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
#include <arc/communication/ClientInterface.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credential/Credential.h>
#include <arc/credential/CertUtil.h>
#include <arc/credentialstore/CredentialStore.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/FileUtils.h>

#include <openssl/ui.h>

#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif

using namespace ArcCredential;

static bool contact_voms_servers(std::list<std::string>& vomslist, std::list<std::string>& orderlist,
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period, 
      Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq);

static bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command,
      const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
      const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
      std::list<std::string>& vomslist, std::string& vomses_path, const std::string& proxy_path, 
      Arc::UserConfig& usercfg, Arc::Logger& logger);

static void create_tmp_proxy(const std::string& tmp_proxy_path, Arc::Credential& signer);

static void create_proxy(std::string& proxy_cert, Arc::Credential& signer,     
    const std::string& proxy_policy, Arc::Time& proxy_start, Arc::Period& proxy_period, 
    const std::string& vomsacseq, bool use_gsi_proxy, int keybits);

static std::string get_proxypolicy(const std::string& policy_source);

static int create_proxy_file(const std::string& path) {
  int f = -1;

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

static void remove_proxy_file(const std::string& path) {
  if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove proxy file " + path);
  }
}

static void remove_cert_file(const std::string& path) {
  if((::unlink(path.c_str()) != 0) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove certificate file " + path);
  }
}

static void tls_process_error(Arc::Logger& logger) {
  unsigned long err;
  err = ERR_get_error();
  if (err != 0) {
    logger.msg(Arc::ERROR, "OpenSSL error -- %s", ERR_error_string(err, NULL));
    logger.msg(Arc::ERROR, "Library  : %s", ERR_lib_error_string(err));
    logger.msg(Arc::ERROR, "Function : %s", ERR_func_error_string(err));
    logger.msg(Arc::ERROR, "Reason   : %s", ERR_reason_error_string(err));
  }
  return;
}

#define PASS_MIN_LENGTH (4)
static int input_password(char *password, int passwdsz, bool verify,
                          const std::string& prompt_info,
                          const std::string& prompt_verify_info,
                          Arc::Logger& logger) {
  UI *ui = NULL;
  int res = 0;
  ui = UI_new();
  if (ui) {
    int ok = 0;
    char* buf = new char[passwdsz];
    memset(buf, 0, passwdsz);
    int ui_flags = 0;
    char *prompt1 = NULL;
    char *prompt2 = NULL;
    prompt1 = UI_construct_prompt(ui, "passphrase", prompt_info.c_str());
    ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
    UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);
    ok = UI_add_input_string(ui, prompt1, ui_flags, password,
                             0, passwdsz - 1);
    if (ok >= 0) {
      do {
        ok = UI_process(ui);
      } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));
    }

    if (ok >= 0) res = strlen(password);

    if (ok >= 0 && verify) {
      UI_free(ui);
      ui = UI_new();
      if(!ui) {
        ok = -1;
      } else {
        // TODO: use some generic password strength evaluation
        if(res < PASS_MIN_LENGTH) {
          UI_add_info_string(ui, "WARNING: Your password is too weak (too short)!\n"
                                 "Make sure this is really what You wanted to enter.\n");
        }
        prompt2 = UI_construct_prompt(ui, "passphrase", prompt_verify_info.c_str());
        ok = UI_add_verify_string(ui, prompt2, ui_flags, buf,
                                  0, passwdsz - 1, password);
        if (ok >= 0) {
          do {
            ok = UI_process(ui);
          } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));
        }
      }
    }

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
    if(ui) UI_free(ui);
    delete[] buf;
    if(prompt1) OPENSSL_free(prompt1);
    if(prompt2) OPENSSL_free(prompt2);
  }
  return res;
}

static bool is_file(std::string path) {
  if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR))
    return true;
  return false;
}

static bool is_dir(std::string path) {
  if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
    return true;
  return false;
}

static std::vector<std::string> search_vomses(std::string path) {
  std::vector<std::string> vomses_files;
  if(is_file(path)) vomses_files.push_back(path);
  else if(is_dir(path)) {
    //if the path 'vomses' is a directory, search all of the files under this directory,
    //i.e.,  'vomses/voA'  'vomses/voB'
    std::string path_header = path;
    std::string fullpath;
    Glib::Dir dir(path);
    for(Glib::Dir::iterator i = dir.begin(); i != dir.end(); i++ ) {
      fullpath = path_header + G_DIR_SEPARATOR_S + *i;
      if(is_file(fullpath)) vomses_files.push_back(fullpath);
      else if(is_dir(fullpath)) {         
        std::string sub_path = fullpath;
        //if the path is a directory, search the all of the files under this directory,
        //i.e., 'vomses/extra/myprivatevo'
        Glib::Dir subdir(sub_path);
        for(Glib::Dir::iterator j = subdir.begin(); j != subdir.end(); j++ ) {
          fullpath = sub_path + G_DIR_SEPARATOR_S + *j;
          if(is_file(fullpath)) vomses_files.push_back(fullpath);
          //else if(is_dir(fullpath)) { //if it is again a directory, the files under it will be ignored }
        }
      }
    }
  }
  return vomses_files;
}

#define VOMS_LINE_NICKNAME (0)
#define VOMS_LINE_HOST (1)
#define VOMS_LINE_PORT (2)
#define VOMS_LINE_SN (3)
#define VOMS_LINE_NAME (4)
#define VOMS_LINE_NUM (5)

static bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    std::list<std::string>& vomslist, std::string& vomses_path, Arc::UserConfig& usercfg, Arc::Logger& logger);

static std::string tokens_to_string(std::vector<std::string> tokens) {
  std::string s;
  for(int n = 0; n<tokens.size(); ++n) {
    s += "\""+tokens[n]+"\" ";
  };
  return s;
}

#ifdef HAVE_NSS
static void get_default_nssdb_path(std::vector<std::string>& nss_paths) {
  const Arc::User user;
  // The profiles.ini could exist under firefox, seamonkey and thunderbird
  std::vector<std::string> profiles_homes;

#ifndef WIN32
  std::string home_path = user.Home();
#else
  std::string home_path = Glib::get_home_dir();
#endif

  std::string profiles_home;

#if defined(_MACOSX)
  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "Firefox";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "SeaMonkey";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Thunderbird";
  profiles_homes.push_back(profiles_home);

#elif defined(WIN32)
  //Windows Vista and Win7
  profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "Firefox";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "SeaMonkey";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "AppData" G_DIR_SEPARATOR_S "Roaming" G_DIR_SEPARATOR_S "Thunderbird";
  profiles_homes.push_back(profiles_home);

  //WinXP and Win2000
  profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "Firefox";
  profiles_homes.push_back(profiles_home);    

  profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Mozilla" G_DIR_SEPARATOR_S "SeaMonkey";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "Application Data" G_DIR_SEPARATOR_S "Thunderbird";
  profiles_homes.push_back(profiles_home);

#else //Linux
  profiles_home = home_path + G_DIR_SEPARATOR_S ".mozilla" G_DIR_SEPARATOR_S "firefox";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S ".mozilla" G_DIR_SEPARATOR_S "seamonkey";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S ".thunderbird";
  profiles_homes.push_back(profiles_home);
#endif

  std::vector<std::string> pf_homes;
  // Remove the unreachable directories
  for(int i=0; i<profiles_homes.size(); i++) {
    std::string pf_home;
    pf_home = profiles_homes[i];
    struct stat st;
    if(::stat(pf_home.c_str(), &st) != 0) continue;
    if(!S_ISDIR(st.st_mode)) continue; 
    if(user.get_uid() != st.st_uid) continue;
    pf_homes.push_back(pf_home);
  }

  // Record the profiles.ini path, and the directory it belongs to.
  std::map<std::string, std::string> ini_home;
  // Remove the unreachable "profiles.ini" files
  for(int i=0; i<pf_homes.size(); i++) {
    std::string pf_file = pf_homes[i] + G_DIR_SEPARATOR_S "profiles.ini";
    struct stat st;
    if(::stat(pf_file.c_str(),&st) != 0) continue; 
    if(!S_ISREG(st.st_mode)) continue; 
    if(user.get_uid() != st.st_uid) continue;
    ini_home[pf_file] = pf_homes[i];
  }

  // All of the reachable profiles.ini files will be parsed to get 
  // the nss configuration information (nss db location).
  // All of the information about nss db location  will be 
  // merged together for users to choose
  std::map<std::string, std::string>::iterator it;
  for(it = ini_home.begin(); it != ini_home.end(); it++) {
    std::string pf_ini = (*it).first;
    std::string pf_home = (*it).second;

    std::string profiles;
    std::ifstream in_f(pf_ini.c_str());
    std::getline<char>(in_f, profiles, '\0');

    std::list<std::string> lines;
    Arc::tokenize(profiles, lines, "\n");

    // Parse each [Profile]
    for (std::list<std::string>::iterator i = lines.begin(); i != lines.end(); ++i) {
      std::vector<std::string> inivalue;
      Arc::tokenize(*i, inivalue, "=");
      if((inivalue[0].find("Profile") != std::string::npos) && 
         (inivalue[0].find("StartWithLast") == std::string::npos)) {
        bool is_relative = false;
        std::string path;
        std::advance(i, 1);
        for(; i != lines.end();) {
          inivalue.clear();
          Arc::tokenize(*i, inivalue, "=");   
          if (inivalue.size() == 2) {
            if (inivalue[0] == "IsRelative") {
              if(inivalue[1] == "1") is_relative = true;
              else is_relative = false;
            }
            if (inivalue[0] == "Path") path = inivalue[1];
          }
          if(inivalue[0].find("Profile") != std::string::npos) { i--; break; }
          std::advance(i, 1);
        }
        std::string nss_path;
        if(is_relative) nss_path = pf_home + G_DIR_SEPARATOR_S + path;
        else nss_path = path;

        struct stat st;
        if((::stat(nss_path.c_str(),&st) == 0) && (S_ISDIR(st.st_mode))
            && (user.get_uid() == st.st_uid))
          nss_paths.push_back(nss_path);
        if(i == lines.end()) break;
      }
    }
  }
  return; 
} 

static void get_nss_certname(std::string& certname, Arc::Logger& logger) {
  std::list<AuthN::certInfo> certInfolist;
  AuthN::nssListUserCertificatesInfo(certInfolist);
  if(certInfolist.size()) {
    std::cout<<Arc::IString("There are %d user certificates existing in the NSS database",
      certInfolist.size())<<std::endl;
  }
  int n = 1;
  std::list<AuthN::certInfo>::iterator it;
  for(it = certInfolist.begin(); it != certInfolist.end(); it++) {
    AuthN::certInfo cert_info = (*it);
    std::string sub_dn = cert_info.subject_dn;
    std::string cn_name;
    std::string::size_type pos1, pos2;
    pos1 = sub_dn.find("CN=");
    if(pos1 != std::string::npos) {
      pos2 = sub_dn.find(",", pos1);
      if(pos2 != std::string::npos) 
        cn_name = " ("+sub_dn.substr(pos1+3, pos2-pos1-3) + ")";
    }
    std::cout<<Arc::IString("Number %d is with nickname: %s%s", n, cert_info.certname, cn_name)<<std::endl;
    Arc::Time now;
    std::string msg;
    if(now > cert_info.end) msg = "(expired)";
    else if((now + 300) > cert_info.end) msg = "(will be expired in 5 min)";
    else if((now + 3600*24) > cert_info.end) {
      Arc::Period left(cert_info.end - now);
      msg = std::string("(will be expired in ") + std::string(left) + ")";
    }
    std::cout<<Arc::IString("    expiration time: %s ", cert_info.end.str())<<msg<<std::endl;
    //std::cout<<Arc::IString("    certificate dn:  %s", cert_info.subject_dn)<<std::endl;
    //std::cout<<Arc::IString("    issuer dn:       %s", cert_info.issuer_dn)<<std::endl;
    //std::cout<<Arc::IString("    serial number:   %d", cert_info.serial)<<std::endl;
    logger.msg(Arc::INFO, "    certificate dn:  %s", cert_info.subject_dn);
    logger.msg(Arc::INFO, "    issuer dn:       %s", cert_info.issuer_dn);
    logger.msg(Arc::INFO, "    serial number:   %d", cert_info.serial);
    n++;
  }

  std::cout << Arc::IString("Please choose the one you would use (1-%d): ", certInfolist.size());
  if(certInfolist.size() == 1) { it = certInfolist.begin(); certname = (*it).certname; }
  char c;
  while(true && (certInfolist.size()>1)) {
    c = getchar();
    int num = c - '0';
    if((num<=certInfolist.size()) && (num>=1)) {
      it = certInfolist.begin();
      std::advance(it, num-1);
      certname = (*it).certname;
      break;
    }
  }
}

#endif

int main(int argc, char *argv[]) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcproxy");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(" ",
                            istring("The arcproxy command creates a proxy from a key/certificate pair which can\n"
                                    "then be used to access grid resources."),
                            istring("Supported constraints are:\n"
                                    "  validityStart=time (e.g. 2008-05-29T10:20:30Z; if not specified, start from now)\n"
                                    "  validityEnd=time\n"
                                    "  validityPeriod=time (e.g. 43200 or 12h or 12H; if both validityPeriod and validityEnd\n"
                                    "  not specified, the default is 12 hours for local proxy, and 168 hours for delegated\n"
                                    "  proxy on myproxy server)\n"
                                    "  vomsACvalidityPeriod=time (e.g. 43200 or 12h or 12H; if not specified, the default\n"
                                    "  is the minimum value of 12 hours and validityPeriod)\n"
                                    "  myproxyvalidityPeriod=time (lifetime of proxies delegated by myproxy server,\n"
                                    "  e.g. 43200 or 12h or 12H; if not specified, the default is the minimum value of\n"
                                    "  12 hours and validityPeriod (which is lifetime of the delegated proxy on myproxy server))\n"
                                    "  proxyPolicy=policy content\n"
                                    "  proxyPolicyFile=policy file"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to the proxy file"),
                    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "cert", istring("path to the certificate file, it can be either PEM, DER, or PKCS12 formated"),
                    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to the private key file, if the certificate is in PKCS12 format, then no need to give private key"),
                    istring("path"), key_path);

  std::string ca_dir;
  options.AddOption('T', "cadir", istring("path to the trusted certificate directory, only needed for the VOMS client functionality"),
                    istring("path"), ca_dir);

  std::string voms_dir;
  options.AddOption('s', "vomsdir", istring("path to the top directory of VOMS *.lsc files, only needed for the VOMS client functionality"),
                    istring("path"), voms_dir);

  std::string vomses_path;
  options.AddOption('V', "vomses", istring("path to the VOMS server configuration file"),
                    istring("path"), vomses_path);

  std::list<std::string> vomslist;
  options.AddOption('S', "voms", istring("voms<:command>. Specify VOMS server (More than one VOMS server \n"
                                         "              can be specified like this: --voms VOa:command1 --voms VOb:command2). \n"
                                         "              :command is optional, and is used to ask for specific attributes(e.g: roles)\n"
                                         "              command options are:\n"
                                         "              all --- put all of this DN's attributes into AC; \n"
                                         "              list ---list all of the DN's attribute, will not create AC extension; \n"
                                         "              /Role=yourRole --- specify the role, if this DN \n"
                                         "                               has such a role, the role will be put into AC \n"
                                         "              /voname/groupname/Role=yourRole --- specify the VO, group and role; if this DN \n"
                                         "                               has such a role, the role will be put into AC \n"
                                         ),
                    istring("string"), vomslist);

  std::list<std::string> orderlist;
  options.AddOption('o', "order", istring("group<:role>. Specify ordering of attributes \n"
                    "              Example: --order /knowarc.eu/coredev:Developer,/knowarc.eu/testers:Tester \n"
                    "              or: --order /knowarc.eu/coredev:Developer --order /knowarc.eu/testers:Tester \n"
                    " Note that it does not make sense to specify the order if you have two or more different VOMS servers specified"),
                    istring("string"), orderlist);

  bool use_gsi_comm = false;
  options.AddOption('G', "gsicom", istring("use GSI communication protocol for contacting VOMS services"), use_gsi_comm);

  bool use_http_comm = false;
  options.AddOption('H', "httpcom", istring("use HTTP communication protocol for contacting VOMS services that provide RESTful access \n"
                                            "               Note for RESTful access, \'list\' command and multiple VOMS server are not supported\n"
                                            ), 
                    use_http_comm);

  bool use_gsi_proxy = false;
  options.AddOption('O', "old", istring("use GSI proxy (RFC 3820 compliant proxy is default)"), use_gsi_proxy);

  bool info = false;
  options.AddOption('I', "info", istring("print all information about this proxy. \n"
                                         "              In order to show the Identity (DN without CN as suffix for proxy) \n"
                                         "              of the certificate, the 'trusted certdir' is needed."
                                         ),
                    info);

  bool remove_proxy = false;
  options.AddOption('r', "remove", istring("remove proxy"), remove_proxy);

  std::string user_name; //user name to MyProxy server
  options.AddOption('U', "user", istring("username to MyProxy server"),
                    istring("string"), user_name);

  bool use_empty_passphrase = false; //if use empty passphrase to myproxy server
  options.AddOption('N', "nopassphrase", istring("don't prompt for a credential passphrase, when retrieve a \n"
                                         "              credential from on MyProxy server. \n"
                                         "              The precondition of this choice is the credential is PUT onto\n"
                                         "              the MyProxy server without a passphrase by using -R (--retrievable_by_cert) \n"
                                         "              option when being PUTing onto Myproxy server. \n"
                                         "              This option is specific for the GET command when contacting Myproxy server."
                                         ),
                    use_empty_passphrase);
  
  std::string retrievable_by_cert; //if use empty passphrase to myproxy server
  options.AddOption('R', "retrievable_by_cert", istring("Allow specified entity to retrieve credential without passphrase.\n"
                                         "              This option is specific for the PUT command when contacting Myproxy server."
                                         ),
                    istring("string"), retrievable_by_cert);

  std::string myproxy_server; //url of MyProxy server
  options.AddOption('L', "myproxysrv", istring("hostname[:port] of MyProxy server"),
                    istring("string"), myproxy_server);

  std::string myproxy_command; //command to myproxy server
  options.AddOption('M', "myproxycmd", istring("command to MyProxy server. The command can be PUT or GET.\n"
                                               "              PUT/put/Put -- put a delegated credential to the MyProxy server; \n"
                                               "              GET/get/Get -- get a delegated credential from the MyProxy server, \n"
                                               "              credential (certificate and key) is not needed in this case. \n"
                                               "              MyProxy functionality can be used together with VOMS\n"
                                               "              functionality.\n"
                                               "              voms and vomses can be used for Get command if VOMS attributes\n"
                                               "              is required to be included in the proxy.\n"
                                               ),
                    istring("string"), myproxy_command);

#ifdef HAVE_NSS
  bool use_nssdb = false;
  options.AddOption('F', "nssdb", istring("use NSS credential database in default Mozilla profiles, \n"
                                          "              including Firefox, Seamonkey and Thunderbird.\n"), use_nssdb);
#endif

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

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcproxy", VERSION) << std::endl;
    return EXIT_SUCCESS;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  // This ensure command line args overwrite all other options
  if(!cert_path.empty())Arc::SetEnv("X509_USER_CERT", cert_path);
  if(!key_path.empty())Arc::SetEnv("X509_USER_KEY", key_path);
  if(!proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", proxy_path);
  if(!ca_dir.empty())Arc::SetEnv("X509_CERT_DIR", ca_dir);

  // Set default, predefined or guessed credentials. Also check if they exist.
#ifdef HAVE_NSS
  Arc::UserConfig usercfg(conffile,
        Arc::initializeCredentialsType(use_nssdb ? Arc::initializeCredentialsType::NotTryCredentials 
        : Arc::initializeCredentialsType::TryCredentials));
#else
  Arc::UserConfig usercfg(conffile,
        Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
#endif
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization.");
    return EXIT_FAILURE;
  }

  // Check for needed credentials objects
  // Can proxy be used for? Could not find it in documentation.
  // Key and certificate not needed if only printing proxy information
  if (!(myproxy_command == "get" || myproxy_command == "GET" || myproxy_command == "Get")) {
    if((usercfg.CertificatePath().empty() || 
        (
         usercfg.KeyPath().empty() && 
         (usercfg.CertificatePath().find(".p12") == std::string::npos)
        )
       ) && !(info || remove_proxy)) {
      logger.msg(Arc::ERROR, "Failed to find certificate and/or private key or files have improper permissions or ownership.");
      logger.msg(Arc::ERROR, "You may try to increase verbosity to get more information.");
      return EXIT_FAILURE;
    }
  }

  if(!vomslist.empty() || !myproxy_command.empty()) {
    // For external communication CAs are needed
    if(usercfg.CACertificatesDirectory().empty()) {
      logger.msg(Arc::ERROR, "Failed to find CA certificates");
      logger.msg(Arc::ERROR, "Cannot find the CA certificates directory path, "
                 "please set environment variable X509_CERT_DIR, "
                 "or cacertificatesdirectory in a configuration file.");
      logger.msg(Arc::ERROR, "You may try to increase verbosity to get more information.");
      logger.msg(Arc::ERROR, "The CA certificates directory is required for "
                 "contacting VOMS and MyProxy servers.");
      return EXIT_FAILURE;
    }
  }
  // Proxy is special case. We either need default or predefined path.
  // No guessing or testing is needed. 
  // By running credentials initialization once more all set values
  // won't change. But proxy will get default value if not set.
  {
    Arc::UserConfig tmpcfg(conffile,
          Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
    if(proxy_path.empty()) proxy_path = tmpcfg.ProxyPath();
    usercfg.ProxyPath(proxy_path);
  }
  // Get back all paths
  if(key_path.empty()) key_path = usercfg.KeyPath();
  if(cert_path.empty()) cert_path = usercfg.CertificatePath();
  if(ca_dir.empty()) ca_dir = usercfg.CACertificatesDirectory();
  if(voms_dir.empty()) voms_dir = Arc::GetEnv("X509_VOMS_DIR");

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (timeout > 0) usercfg.Timeout(timeout);

  Arc::User user;

  if (!params.empty()) {
    logger.msg(Arc::ERROR, "Wrong number of arguments!");
    return EXIT_FAILURE;
  }

  const Arc::Time now;
  
  if (remove_proxy) {
    if (proxy_path.empty()) {
      logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                 "please setup environment X509_USER_PROXY, "
                 "or proxypath in a configuration file");
      return EXIT_FAILURE;
    } else if (!(Glib::file_test(proxy_path, Glib::FILE_TEST_EXISTS))) {
        logger.msg(Arc::ERROR, "Cannot remove proxy file at %s, because it's not there", proxy_path);
        return EXIT_FAILURE;
    }
    if((unlink(proxy_path.c_str()) != 0) && (errno != ENOENT)) {
        logger.msg(Arc::ERROR, "Cannot remove proxy file at %s", proxy_path);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  if (info) {
    std::vector<Arc::VOMSACInfo> voms_attributes;
    bool res = false;

    if (proxy_path.empty()) {
      logger.msg(Arc::ERROR, "Cannot find the path of the proxy file, "
                 "please setup environment X509_USER_PROXY, "
                 "or proxypath in a configuration file");
      return EXIT_FAILURE;
    }
    else if (!(Glib::file_test(proxy_path, Glib::FILE_TEST_EXISTS))) {
      logger.msg(Arc::ERROR, "Cannot find file at %s for getting the proxy. "
                 "Please make sure this file exists.", proxy_path);
      return EXIT_FAILURE;
    }

    Arc::Credential holder(proxy_path, "", "", "");
    std::cout << Arc::IString("Subject: %s", holder.GetDN()) << std::endl;
    std::cout << Arc::IString("Issuer: %s", holder.GetIssuerName()) << std::endl;
    std::cout << Arc::IString("Identity: %s", holder.GetIdentityName()) << std::endl;
    if (holder.GetEndTime() < now)
      std::cout << Arc::IString("Time left for proxy: Proxy expired") << std::endl;
    else if (now < holder.GetStartTime())
      std::cout << Arc::IString("Time left for proxy: Proxy not valid yet") << std::endl;
    else
      std::cout << Arc::IString("Time left for proxy: %s", (holder.GetEndTime() - now).istr()) << std::endl;
    std::cout << Arc::IString("Proxy path: %s", proxy_path) << std::endl;
    std::cout << Arc::IString("Proxy type: %s", certTypeToString(holder.GetType())) << std::endl;

    Arc::VOMSTrustList voms_trust_dn;
    voms_trust_dn.AddRegex(".*");
    res = parseVOMSAC(holder, ca_dir, "", voms_dir, voms_trust_dn, voms_attributes, true, true);
    // Not printing error message because parseVOMSAC will print everything itself
    //if (!res) logger.msg(Arc::ERROR, "VOMS attribute parsing failed");
    for(int n = 0; n<voms_attributes.size(); ++n) {
      if(voms_attributes[n].attributes.size() > 0) {
        std::cout<<"====== "<<Arc::IString("AC extension information for VO ")<<
                   voms_attributes[n].voname<<" ======"<<std::endl;
        if(voms_attributes[n].status & Arc::VOMSACInfo::ParsingError) {
          std::cout << Arc::IString("Error detected while parsing this AC")<<std::endl;
          if(voms_attributes[n].status & Arc::VOMSACInfo::X509ParsingFailed) {
            std::cout << "Failed parsing X509 structures ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::ACParsingFailed) {
            std::cout << "Failed parsing Attribute Certificate structures ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::InternalParsingFailed) {
            std::cout << "Failed parsing VOMS structures ";
          }
          std::cout << std::endl;
        }
        if(voms_attributes[n].status & Arc::VOMSACInfo::ValidationError) {
          std::cout << Arc::IString("AC is invalid: ");
          if(voms_attributes[n].status & Arc::VOMSACInfo::CAUnknown) {
            std::cout << "CA of VOMS service is not known ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::CertRevoked) {
            std::cout << "VOMS service certificate is revoked ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::CertRevoked) {
            std::cout << "LSC matching/processing failed ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::TrustFailed) {
            std::cout << "Failed to match configured trust chain ";
          }
          if(voms_attributes[n].status & Arc::VOMSACInfo::TimeValidFailed) {
            std::cout << "Out of time restrictions ";
          }
          std::cout << std::endl;
        }
        std::cout << "VO        : "<<voms_attributes[n].voname << std::endl;
        std::cout << "subject   : "<<voms_attributes[n].holder << std::endl;
        std::cout << "issuer    : "<<voms_attributes[n].issuer << std::endl;
        bool found_uri = false;
        for(int i = 0; i < voms_attributes[n].attributes.size(); i++) {
          std::string attr = voms_attributes[n].attributes[i];
          std::string::size_type pos;
          if((pos = attr.find("hostname=")) != std::string::npos) {
            if(found_uri) continue;
            std::string str = attr.substr(pos+9);
            std::string::size_type pos1 = str.find("/");
            if(pos1 != std::string::npos) str = str.substr(0, pos1);
            std::cout << "uri       : " << str <<std::endl;
            found_uri = true;
          }
          else {
            if(attr.find("Role=") == std::string::npos)
              std::cout << "attribute : " << attr <<"/Role=NULL/Capability=NULL" <<std::endl;
            else if(attr.find("Capability=") == std::string::npos)
              std::cout << "attribute : " << attr <<"/Capability=NULL" <<std::endl;
            else
              std::cout << "attribute : " << attr <<std::endl;
          }

          if((pos = attr.find("Role=")) != std::string::npos) {
            std::string str = attr.substr(pos+5);
            std::cout << "attribute : role = " << str << " (" << voms_attributes[n].voname << ")"<<std::endl;
          }

          //std::cout << "attribute : "<<voms_attributes[n].attributes[i]<<std::endl;
          //do not display those attributes that have already been displayed 
          //(this can happen when there are multiple voms server )
        }
        Arc::Time ct;
        if(ct < voms_attributes[n].from) {
          std::cout << Arc::IString("Time left for AC: AC is not valid yet")<<std::endl;
        } else if(ct > voms_attributes[n].till) {
          std::cout << Arc::IString("Time left for AC: AC has expired")<<std::endl;
        } else {
          std::cout << Arc::IString("Time left for AC: %s", (voms_attributes[n].till-ct).istr())<<std::endl;
        }
      }
/*
      Arc::Time now;
      Arc::Time till = voms_attributes[n].till;
      if(now < till)
        std::cout << Arc::IString("Timeleft for AC: %s", (till-now).istr())<<std::endl;
      else
        std::cout << Arc::IString("AC has been expired for: %s", (now-till).istr())<<std::endl;
*/
    }
    return EXIT_SUCCESS;
  }

  if ((cert_path.empty() || key_path.empty()) && 
      ((myproxy_command == "PUT") || (myproxy_command == "put") || (myproxy_command == "Put"))) {
    if (cert_path.empty())
      logger.msg(Arc::ERROR, "Cannot find the user certificate path, "
                 "please setup environment X509_USER_CERT, "
                 "or certificatepath in a configuration file");
    if (key_path.empty())
      logger.msg(Arc::ERROR, "Cannot find the user private key path, "
                 "please setup environment X509_USER_KEY, "
                 "or keypath in a configuration file");
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

  //proxy validity period
  //Set the default proxy validity lifetime to 12 hours if there is
  //no validity lifetime provided by command caller
  // Set default values first
  // TODO: Is default validityPeriod since now or since validityStart?
  Arc::Time validityStart = now; // now by default
  Arc::Period validityPeriod(12*60*60);
  if (myproxy_command == "put" || myproxy_command == "PUT" || myproxy_command == "Put") {
    //For myproxy PUT operation, the proxy should be 7 days according to the default 
    //definition in myproxy implementation.
    validityPeriod = 7*24*60*60;
  }
  // Acquire constraints. Check for valid values and conflicts.
  if((!constraints["validityStart"].empty()) && 
     (!constraints["validityEnd"].empty()) &&
     (!constraints["validityPeriod"].empty())) {
    std::cerr << Arc::IString("The start, end and period can't be set simultaneously") << std::endl;
    return EXIT_FAILURE;
  }
  if(!constraints["validityStart"].empty()) {
    validityStart = Arc::Time(constraints["validityStart"]);
    if (validityStart == Arc::Time(Arc::Time::UNDEFINED)) {
    std::cerr << Arc::IString("The start time that you set: %s can't be recognized.", (std::string)constraints["validityStart"]) << std::endl;
      return EXIT_FAILURE;
    }
  }
  if(!constraints["validityPeriod"].empty()) {
    validityPeriod = Arc::Period(constraints["validityPeriod"]);
    if (validityPeriod.GetPeriod() <= 0) {
      std::cerr << Arc::IString("The period that you set: %s can't be recognized.", (std::string)constraints["validityPeriod"]) << std::endl;
      return EXIT_FAILURE;
    }
  }
  if(!constraints["validityEnd"].empty()) {
    Arc::Time validityEnd = Arc::Time(constraints["validityEnd"]);
    if (validityEnd == Arc::Time(Arc::Time::UNDEFINED)) {
      std::cerr << Arc::IString("The end time that you set: %s can't be recognized.", (std::string)constraints["validityEnd"]) << std::endl;
      return EXIT_FAILURE;
    }
    if(!constraints["validityPeriod"].empty()) {
      // If period is explicitely set then start is derived from end and period
      validityStart = validityEnd - validityPeriod;
    } else {
      // otherwise start - optionally - and end are set, period is derived
      if(validityEnd < validityStart) {
        std::cerr << Arc::IString("The end time that you set: %s is before start time:%s.", (std::string)validityEnd,(std::string)validityStart) << std::endl;
        // error
        return EXIT_FAILURE;
      }
      validityPeriod = validityEnd - validityStart;
    }
  }
  // Here we have validityStart and validityPeriod defined
  Arc::Time validityEnd = validityStart + validityPeriod;
  // Warn user about strange times but do not prevent user from doing anything legal
  if(validityStart < now) {
    std::cout << Arc::IString("WARNING: The start time that you set: %s is before current time: %s", (std::string)validityStart, (std::string)now) << std::endl;
  }
  if(validityEnd < now) {
    std::cout << Arc::IString("WARNING: The end time that you set: %s is before current time: %s", (std::string)validityEnd, (std::string)now) << std::endl;
  }

  //voms AC valitity period
  //Set the default voms AC validity lifetime to 12 hours if there is
  //no validity lifetime provided by command caller
  Arc::Period vomsACvalidityPeriod(12*60*60);
  if(!constraints["vomsACvalidityPeriod"].empty()) {
    vomsACvalidityPeriod = Arc::Period(constraints["vomsACvalidityPeriod"]);
    if (vomsACvalidityPeriod.GetPeriod() == 0) {
      std::cerr << Arc::IString("The VOMS AC period that you set: %s can't be recognized.", (std::string)constraints["vomsACvalidityPeriod"]) << std::endl;
      return EXIT_FAILURE;
    }
  } else {
    if(validityPeriod < vomsACvalidityPeriod) vomsACvalidityPeriod = validityPeriod;
    // It is strange that VOMS AC may be valid less than proxy itself.
    // Maybe it would be more correct to have it valid by default from 
    // now till validityEnd.
  }
  std::string voms_period = Arc::tostring(vomsACvalidityPeriod.GetPeriod());

  //myproxy validity period.
  //Set the default myproxy validity lifetime to 12 hours if there is
  //no validity lifetime provided by command caller
  Arc::Period myproxyvalidityPeriod(12*60*60);
  if(!constraints["myproxyvalidityPeriod"].empty()) {
    myproxyvalidityPeriod = Arc::Period(constraints["myproxyvalidityPeriod"]);
    if (myproxyvalidityPeriod.GetPeriod() == 0) {
      std::cerr << Arc::IString("The MyProxy period that you set: %s can't be recognized.", (std::string)constraints["myproxyvalidityPeriod"]) << std::endl;
      return EXIT_FAILURE;
    }
  } else {
    if(validityPeriod < myproxyvalidityPeriod) myproxyvalidityPeriod = validityPeriod;
    // see vomsACvalidityPeriod
  }
  std::string myproxy_period = Arc::tostring(myproxyvalidityPeriod.GetPeriod());



#ifdef HAVE_NSS
  //Using nss db dominate other option
  if(use_nssdb) {
    // Get the nss db paths from firefox's profile.ini file
    std::vector<std::string> nssdb_paths;
    get_default_nssdb_path(nssdb_paths);
    if(nssdb_paths.empty()) {
      std::cout << Arc::IString("The NSS database can not be detected in the Firefox profile") << std::endl;
      return EXIT_FAILURE;
    }

    // Let user to choose which profile to use
    // if multiple profiles exist
    bool res;
    std::string configdir;
    if(nssdb_paths.size()) {
      std::cout<<Arc::IString("There are %d NSS base directories where the certificate, key, and module datbases live",
        nssdb_paths.size())<<std::endl;
    }
    for(int i=0; i < nssdb_paths.size(); i++) {
      std::cout<<Arc::IString("Number %d is: %s", i+1, nssdb_paths[i])<<std::endl;
    }
    std::cout << Arc::IString("Please choose the NSS database you would use (1-%d): ", nssdb_paths.size());
    if(nssdb_paths.size() == 1) { configdir = nssdb_paths[0]; }
    char c;
    while(true && (nssdb_paths.size()>1)) {
      c = getchar();
      int num = c - '0';
      if((num<=nssdb_paths.size()) && (num>=1)) {
        configdir = nssdb_paths[num-1];
        break;
      }
    }

    res = AuthN::nssInit(configdir);
    std::cout<< Arc::IString("NSS database to be accessed: %s\n", configdir.c_str());

    char* slotpw = NULL; //"secretpw";  
    //The nss db under firefox profile seems to not be protected by any passphrase by default
    bool ascii = true;
    const char* trusts = "u,u,u";

    // Generate CSR
    std::string proxy_csrfile = "proxy.csr";
    std::string proxy_keyname = "proxykey";
    std::string proxy_privk_str;
    res = AuthN::nssGenerateCSR(proxy_keyname, "CN=Test,OU=ARC,O=EMI", slotpw, proxy_csrfile, proxy_privk_str, ascii);
    if(!res) return EXIT_FAILURE;

    // Create a temporary proxy and contact voms server
    std::string issuername;
    std::string vomsacseq;
    if (!vomslist.empty()) {
      std::string tmp_proxy_path;
      tmp_proxy_path = Glib::build_filename(Glib::get_tmp_dir(), std::string("tmp_proxy.pem"));
      get_nss_certname(issuername, logger);
      
      // Create tmp proxy cert
      int duration = 12;
      res = AuthN::nssCreateCert(proxy_csrfile, issuername, NULL, duration, "", tmp_proxy_path, ascii);
      if(!res) return EXIT_FAILURE;
      std::string tmp_proxy_cred_str;
      std::ifstream tmp_proxy_cert_s(tmp_proxy_path.c_str());
      std::getline(tmp_proxy_cert_s, tmp_proxy_cred_str,'\0');
      tmp_proxy_cert_s.close();

      // Export EEC
      std::string cert_file = "cert.pem";
      res = AuthN::nssExportCertificate(issuername, cert_file);
      if(!res) return EXIT_FAILURE;
      std::string eec_cert_str;
      std::ifstream eec_s(cert_file.c_str());
      std::getline(eec_s, eec_cert_str,'\0');
      eec_s.close();
      remove_cert_file(cert_file);

      // Compose tmp proxy file
      tmp_proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
      write_proxy_file(tmp_proxy_path, tmp_proxy_cred_str);

      contact_voms_servers(vomslist, orderlist, vomses_path, use_gsi_comm,
          use_http_comm, voms_period, usercfg, logger, tmp_proxy_path, vomsacseq);
      remove_proxy_file(tmp_proxy_path);
    }

    // Create proxy with VOMS AC
    std::string proxy_certfile = "myproxy.pem";

    // Let user to choose which credential to use 
    if(issuername.empty()) get_nss_certname(issuername, logger);
    std::cout<<Arc::IString("Certificate to use is: %s", issuername)<<std::endl;

    int duration;
    duration = validityPeriod.GetPeriod() / 3600;

    std::string vomsacseq_asn1;
    if(!vomsacseq.empty()) Arc::VOMSACSeqEncode(vomsacseq, vomsacseq_asn1);
    res = AuthN::nssCreateCert(proxy_csrfile, issuername, "", duration, vomsacseq_asn1, proxy_certfile, ascii);
    if(!res) return EXIT_FAILURE;

    const char* proxy_certname = "proxycert";
    res = AuthN::nssImportCert(slotpw, proxy_certfile, proxy_certname, trusts, ascii);
    if(!res) return EXIT_FAILURE;

    //Compose the proxy certificate 
    if(!proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", proxy_path);
    Arc::UserConfig usercfg(conffile,
        Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
    if (!usercfg) {
      logger.msg(Arc::ERROR, "Failed configuration initialization.");
      return EXIT_FAILURE;
    }
    if(proxy_path.empty()) proxy_path = usercfg.ProxyPath();
    usercfg.ProxyPath(proxy_path);
    std::string cert_file = "cert.pem";
    res = AuthN::nssExportCertificate(issuername, cert_file);
    if(!res) return EXIT_FAILURE;

    std::string proxy_cred_str;
    std::ifstream proxy_s(proxy_certfile.c_str());
    std::getline(proxy_s, proxy_cred_str,'\0');
    proxy_s.close();
    // Remove the proxy file, because the content
    // is recorded and then writen into proxy path,
    // together with private key, and the issuer of proxy.
    remove_proxy_file(proxy_certfile);

    std::string eec_cert_str;
    std::ifstream eec_s(cert_file.c_str());
    std::getline(eec_s, eec_cert_str,'\0');
    eec_s.close();
    remove_cert_file(cert_file);

    proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
    write_proxy_file(proxy_path, proxy_cred_str);

    Arc::Credential proxy_cred(proxy_path, proxy_path, "", "");
    Arc::Time left = proxy_cred.GetEndTime();
    std::cout << Arc::IString("Proxy generation succeeded") << std::endl;
    std::cout << Arc::IString("Your proxy is valid until: %s", left.str(Arc::UserTime)) << std::endl;

    return EXIT_SUCCESS;
  }
#endif


  Arc::OpenSSLInit();

  Arc::Time proxy_start = validityStart;
  Arc::Period proxy_period = validityPeriod;
  if (constraints["validityStart"].empty() && constraints["validityEnd"].empty()) {
    // If start/end is not explicitely specified then add 5 min back gap.
    proxy_start = proxy_start - Arc::Period(300);
    proxy_period.SetPeriod(proxy_period.GetPeriod() + 300);
  }
  std::string policy;
  policy = constraints["proxyPolicy"].empty() ? constraints["proxyPolicyFile"] : constraints["proxyPolicy"];

  if (!myproxy_command.empty() && (myproxy_command != "put" && myproxy_command != "PUT" && myproxy_command != "Put")) {
    bool res = contact_myproxy_server(myproxy_server, myproxy_command, 
      user_name, use_empty_passphrase, myproxy_period, retrievable_by_cert, 
      proxy_start, proxy_period, vomslist, vomses_path, proxy_path, usercfg, logger);
    if (res && ((myproxy_command == "get") || (myproxy_command == "GET") || (myproxy_command == "Get"))) {
      // IF the myproxy command is "Get", and voms command is given,
      // then we need to check if the proxy returned from myproxy server
      // includes VOMS AC, if not, we will use the returned proxy to
      // directly contact VOMS server to generate a proxy-on-proxy with 
      // VOMS AC included.
      Arc::Credential holder(proxy_path, "", "", "");
      Arc::VOMSTrustList voms_trust_dn;
      voms_trust_dn.AddRegex(".*");
      std::vector<Arc::VOMSACInfo> voms_attributes;
      bool r = parseVOMSAC(holder, ca_dir, "", voms_dir, voms_trust_dn, voms_attributes, true, true);
      if (!r) logger.msg(Arc::ERROR, "VOMS attribute parsing failed");
      if(voms_attributes.size() == 0) {
        logger.msg(Arc::INFO, "Myproxy server did not return proxy with VOMS AC included");
        std::string vomsacseq;
        contact_voms_servers(vomslist, orderlist, vomses_path, use_gsi_comm,
            use_http_comm, voms_period, usercfg, logger, proxy_path, vomsacseq);
        if(!vomsacseq.empty()) {
          Arc::Credential signer(proxy_path, proxy_path, "", "");
          std::string proxy_cert;
          create_proxy(proxy_cert, signer, policy, proxy_start, proxy_period, 
              vomsacseq, use_gsi_proxy, 1024);
          write_proxy_file(proxy_path, proxy_cert);
        }
      }
      return EXIT_SUCCESS;
    }
    else return EXIT_FAILURE;
  }

  //Create proxy or voms proxy
  try {
    Arc::Credential signer(cert_path, key_path, "", "");
    if (signer.GetIdentityName().empty()) {
      std::cerr << Arc::IString("Proxy generation failed: No valid certificate found.") << std::endl;
      return EXIT_FAILURE;
    }
#ifndef WIN32
    EVP_PKEY* pkey = signer.GetPrivKey();
    if(!pkey) {
      std::cerr << Arc::IString("Proxy generation failed: No valid private key found.") << std::endl;
      return EXIT_FAILURE;
    }
    if(pkey) EVP_PKEY_free(pkey);
#endif
    std::cout << Arc::IString("Your identity: %s", signer.GetIdentityName()) << std::endl;
    if (now > signer.GetEndTime()) {
      std::cerr << Arc::IString("Proxy generation failed: Certificate has expired.") << std::endl;
      return EXIT_FAILURE;
    }
    else if (now < signer.GetStartTime()) {
      std::cerr << Arc::IString("Proxy generation failed: Certificate is not valid yet.") << std::endl;
      return EXIT_FAILURE;
    }

    std::string vomsacseq;
    if (!vomslist.empty()) {
      //Generate a temporary self-signed proxy certificate
      //to contact the voms server
      std::string tmp_proxy_path;
      tmp_proxy_path = Glib::build_filename(Glib::get_tmp_dir(), std::string("tmp_proxy.pem"));
      create_tmp_proxy(tmp_proxy_path, signer);
      contact_voms_servers(vomslist, orderlist, vomses_path, use_gsi_comm,
          use_http_comm, voms_period, usercfg, logger, tmp_proxy_path, vomsacseq);
      remove_proxy_file(tmp_proxy_path);
    }

    std::string proxy_cert;
    create_proxy(proxy_cert, signer, policy, proxy_start, proxy_period,      
        vomsacseq, use_gsi_proxy, 1024);

    //If myproxy command is "Put", then the proxy path is set to /tmp/myproxy-proxy.uid.pid 
    if (myproxy_command == "put" || myproxy_command == "PUT" || myproxy_command == "Put")
      proxy_path = Glib::build_filename(Glib::get_tmp_dir(), "myproxy-proxy."
                   + Arc::tostring(user.get_uid()) + Arc::tostring((int)(getpid())));
    write_proxy_file(proxy_path,proxy_cert);

    Arc::Credential proxy_cred(proxy_path, proxy_path, "", "");
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
  if (myproxy_command == "put" || myproxy_command == "PUT" || myproxy_command == "Put") {
    bool res = contact_myproxy_server( myproxy_server, myproxy_command,
      user_name, use_empty_passphrase, myproxy_period, retrievable_by_cert,
      proxy_start, proxy_period, vomslist, vomses_path, proxy_path, usercfg, logger);
    if(res) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}

static void create_tmp_proxy(const std::string& tmp_proxy_path, Arc::Credential& signer) {
  int keybits = 1024;
  Arc::Time now;
  Arc::Period period = 3600 * 12 + 300;
  std::string req_str;
  Arc::Credential tmp_proxyreq(now-Arc::Period(300), period, keybits);
  tmp_proxyreq.GenerateRequest(req_str);

  std::string proxy_private_key, proxy_cert, signing_cert, signing_cert_chain;
  tmp_proxyreq.OutputPrivatekey(proxy_private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);

  if (!signer.SignRequest(&tmp_proxyreq, proxy_cert))
    throw std::runtime_error("Failed to sign proxy");
  proxy_cert.append(proxy_private_key).append(signing_cert).append(signing_cert_chain);
  write_proxy_file(tmp_proxy_path, proxy_cert);
}

static void create_proxy(std::string& proxy_cert, Arc::Credential& signer, 
    const std::string& proxy_policy, Arc::Time& proxy_start, Arc::Period& proxy_period, 
    const std::string& vomsacseq, bool use_gsi_proxy, int keybits) {

  std::string private_key, signing_cert, signing_cert_chain;
  std::string req_str;

  Arc::Credential cred_request(proxy_start, proxy_period, keybits);
  cred_request.GenerateRequest(req_str);
  cred_request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);

  //Put the voms attribute certificate into proxy certificate
  if (!vomsacseq.empty()) {
    bool r = cred_request.AddExtension("acseq", (char**)(vomsacseq.c_str()));
    if (!r) std::cout << Arc::IString("Failed to add VOMS AC extension. Your proxy may be incomplete.") << std::endl;
  }

  if (!use_gsi_proxy) {
    if(!proxy_policy.empty()) {
      cred_request.SetProxyPolicy("rfc", "anylanguage", proxy_policy, -1);
    } else if(CERT_IS_LIMITED_PROXY(signer.GetType())) {
      // Gross hack for globus. If Globus marks own proxy as limited
      // it expects every derived proxy to be limited or at least
      // independent. Independent proxies has little sense in Grid
      // world. So here we make our proxy globus-limited to allow
      // it to be used with globus code.
      cred_request.SetProxyPolicy("rfc", "limited", proxy_policy, -1);
    } else {
      cred_request.SetProxyPolicy("rfc", "inheritAll", proxy_policy, -1);
    }
  } else {
    cred_request.SetProxyPolicy("gsi2", "", "", -1);
  }

  if (!signer.SignRequest(&cred_request, proxy_cert))
    throw std::runtime_error("Failed to sign proxy");

  proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);

}

static bool contact_voms_servers(std::list<std::string>& vomslist, std::list<std::string>& orderlist, 
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period, 
      Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq) {

  std::string ca_dir;
  ca_dir = usercfg.CACertificatesDirectory();

  std::map<std::string, std::vector<std::vector<std::string> > > matched_voms_line;
  std::multimap<std::string, std::string> server_command_map;
  std::list<std::string> vomses;
  if(!find_matched_vomses(matched_voms_line, server_command_map, vomses, vomslist, vomses_path, usercfg, logger))
    return false;

  //Contact the voms server to retrieve attribute certificate
  Arc::MCCConfig cfg;
  cfg.AddProxy(tmp_proxy_path);
  cfg.AddCADir(ca_dir);

  for (std::map<std::string, std::vector<std::vector<std::string> > >::iterator it = matched_voms_line.begin();
       it != matched_voms_line.end(); it++) {
    std::string voms_server;
    std::list<std::string> command_list;
    voms_server = (*it).first;
    std::vector<std::vector<std::string> > voms_lines = (*it).second;

    bool succeeded = false; 
    //a boolean value to indicate if there is valid message returned from voms server, by using the current voms_line  
    for (std::vector<std::vector<std::string> >::iterator line_it = voms_lines.begin(); 
        line_it != voms_lines.end(); line_it++) {
      std::vector<std::string> voms_line = *line_it;
      int count = server_command_map.count(voms_server);
      logger.msg(Arc::DEBUG, "There are %d commands to the same VOMS server %s", count, voms_server);

      std::multimap<std::string, std::string>::iterator command_it;
      for(command_it = server_command_map.equal_range(voms_server).first; 
          command_it!=server_command_map.equal_range(voms_server).second; ++command_it) {
        command_list.push_back((*command_it).second);
      }

      std::string address;
      if(voms_line.size() > VOMS_LINE_HOST) address = voms_line[VOMS_LINE_HOST];
      if(address.empty()) {
          logger.msg(Arc::ERROR, "Cannot get VOMS server address information from vomses line: \"%s\"", tokens_to_string(voms_line));
          throw std::runtime_error("Cannot get VOMS server address information from vomses line: \"" + tokens_to_string(voms_line) + "\"");
      }

      std::string port;
      if(voms_line.size() > VOMS_LINE_PORT) port = voms_line[VOMS_LINE_PORT];

      std::string voms_name;
      if(voms_line.size() > VOMS_LINE_NAME) voms_name = voms_line[VOMS_LINE_NAME];
  
      logger.msg(Arc::INFO, "Contacting VOMS server (named %s): %s on port: %s",
                voms_name, address, port);
      std::cout << Arc::IString("Contacting VOMS server (named %s): %s on port: %s", voms_name, address, port) << std::endl;

      std::string send_msg;
      send_msg.append("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms>");
      std::string command;

      for(std::list<std::string>::iterator c_it = command_list.begin(); c_it != command_list.end(); c_it++) {
        std::string command_2server;
        command = *c_it;
        if (command.empty())
          command_2server.append("G/").append(voms_name);
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
      logger.msg(Arc::VERBOSE, "Try to get attribute from VOMS server with order: %s", ordering);
      send_msg.append("<order>").append(ordering).append("</order>");
      send_msg.append("<lifetime>").append(voms_period).append("</lifetime></voms>");
      logger.msg(Arc::VERBOSE, "Message sent to VOMS server %s is: %s", voms_name, send_msg);
       
      std::string ret_str;
      if(use_http_comm) { 
        // Use http to contact voms server, for the RESRful interface provided by voms server
        // The format of the URL: https://moldyngrid.org:15112/generate-ac?fqans=/testbed.univ.kiev.ua/blabla/Role=test-role&lifetime=86400
        // fqans is composed of the voname, group name and role, i.e., the "command" for voms.
        std::string url_str;
        if(!command.empty()) url_str = "https://" + address + ":" + port + "/generate-ac?" + "fqans=" + command + "&lifetime=" + voms_period;
        else url_str = "https://" + address + ":" + port + "/generate-ac?" + "lifetime=" + voms_period;
        Arc::URL voms_url(url_str);
        Arc::ClientHTTP client(cfg, voms_url, usercfg.Timeout());
        client.RelativeURI(true);
        Arc::PayloadRaw request;
        Arc::PayloadRawInterface* response;
        Arc::HTTPClientInfo info;
        Arc::MCC_Status status = client.process("GET", &request, &info, &response);
        if (!status) {
          if (response) delete response;
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
          continue; //There could be another voms replicated server with the same name exists
        }
        if (!response) {
          logger.msg(Arc::ERROR, "No HTTP response from VOMS server");
          continue;
        }
        if(response->Content() != NULL) ret_str.append(response->Content());
        if (response) delete response;
        logger.msg(Arc::VERBOSE, "Returned message from VOMS server: %s", ret_str);
      }
      else {
        // Use GSI or TLS to contact voms server 
        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), use_gsi_comm ? Arc::GSISec : Arc::SSL3Sec, usercfg.Timeout());
        Arc::PayloadRaw request;
        request.Insert(send_msg.c_str(), 0, send_msg.length());
        Arc::PayloadStreamInterface *response = NULL;
        Arc::MCC_Status status = client.process(&request, &response, true);
        if (!status) {
          //logger.msg(Arc::ERROR, (std::string)status);
          if (response) delete response;
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\"\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
          continue; //There could be another voms replicated server with the same name exists
        }
        if (!response) {
          logger.msg(Arc::ERROR, "No stream response from VOMS server");
          continue;
        }
        char ret_buf[1024];
        int len = sizeof(ret_buf);
        while(response->Get(ret_buf, len)) {
          ret_str.append(ret_buf, len);
          len = sizeof(ret_buf);
        };
        if (response) delete response;
        logger.msg(Arc::VERBOSE, "Returned message from VOMS server: %s", ret_str);
      }

      Arc::XMLNode node;
      Arc::XMLNode(ret_str).Exchange(node);
      if((!node) || ((bool)(node["error"]))) {
        if((bool)(node["error"])) {
          std::string str = node["error"]["item"]["message"];
          std::string::size_type pos;
          std::string tmp_str = "The validity of this VOMS AC in your proxy is shortened to";
          if((pos = str.find(tmp_str))!= std::string::npos) {
            std::string tmp = str.substr(pos + tmp_str.size() + 1); 
            std::cout << Arc::IString("The validity duration of VOMS AC is shortened from %s to %s, due to the validity constraint on voms server side.\n", voms_period, tmp);
          }
          else {
            std::cout << Arc::IString("Cannot get any AC or attributes info from VOMS server: %s;\n       Returned message from VOMS server: %s\n", voms_server, str);
            break; //since the voms servers with the same name should be looked as the same for robust reason, the other voms server that can be reached could returned the same message. So we exists the loop, even if there are other backup voms server exist.
          }
        }
        else {
          std::cout << Arc::IString("Returned message from VOMS server %s is: %s\n", voms_server, ret_str);
          break;
        } 
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
      if (dec != NULL) {
        decodedac.append(dec, size);
        free(dec);
        dec = NULL;
      }

      if (command == "list") {
        std::cout << Arc::IString("The attribute information from VOMS server: %s is list as following:", voms_server) << std::endl << decodedac << std::endl;
        return true;
      }

      vomsacseq.append(VOMS_AC_HEADER).append("\n");
      vomsacseq.append(codedac).append("\n");
      vomsacseq.append(VOMS_AC_TRAILER).append("\n");

      succeeded = true; break;
    }//end of the scanning of multiple vomses lines with the same name
    if(succeeded == false) {
      if(voms_lines.size() > 1) 
        std::cout << Arc::IString("There are %d servers with the same name: %s in your vomses file, but all of them can not be reached, or can not return valid message. But proxy without VOMS AC extension will still be generated.", voms_lines.size(), voms_server) << std::endl;
    }
  }

  return true;
}
  

static std::string get_proxypolicy(const std::string& policy_source) {
  std::string policystring;
  if(policy_source.empty()) return std::string();
  if(Glib::file_test(policy_source, Glib::FILE_TEST_EXISTS)) {
    //If the argument is a location which specifies a file that
    //includes the policy content
    if(Glib::file_test(policy_source, Glib::FILE_TEST_IS_REGULAR)) {
      std::ifstream fp;
      fp.open(policy_source.c_str());
      if(!fp) {
        std::cout << Arc::IString("Error: can't open policy file: %s", policy_source.c_str()) << std::endl;
        return std::string();
      }
      fp.unsetf(std::ios::skipws);
      char c;
      while(fp.get(c))
        policystring += c;
      fp.close();
    }
    else {
      std::cout << Arc::IString("Error: policy location: %s is not a regular file", policy_source.c_str()) <<std::endl;
      return std::string();
    }
  }
  else {
    //Otherwise the argument should include the policy content
    policystring = policy_source;
  }
  return policystring;
}

static std::string get_cert_dn(const std::string& cert_file) {
  std::string dn_str;
  Arc::Credential cert(cert_file, "", "", "");
  dn_str = cert.GetIdentityName();
  return dn_str;
}

static bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command, 
      const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
      const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
      std::list<std::string>& vomslist, std::string& vomses_path, const std::string& proxy_path, 
      Arc::UserConfig& usercfg, Arc::Logger& logger) {
  
  std::string user_name = myproxy_user_name;
  std::string key_path, cert_path, ca_dir;
  key_path = usercfg.KeyPath();
  cert_path = usercfg.CertificatePath();
  ca_dir = usercfg.CACertificatesDirectory();

  //If the "INFO" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (myproxy_command == "info" || myproxy_command == "INFO" || myproxy_command == "Info") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if(user_name.empty()) {
        user_name = get_cert_dn(proxy_path);
      }
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
    tls_process_error(logger);
    return false;
  }

  //If the "NEWPASS" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (myproxy_command == "newpass" || myproxy_command == "NEWPASS" || myproxy_command == "Newpass" || myproxy_command == "NewPass") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if(user_name.empty()) {
        user_name = get_cert_dn(proxy_path);
      }
      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string prompt1 = "MyProxy server";
      char password[256];
      std::string passphrase;
      int res = input_password(password, 256, false, prompt1, "", logger);
      if (!res)
        throw std::invalid_argument("Error entering passphrase");
      passphrase = password;
     
      std::string prompt2 = "MyProxy server";
      char newpassword[256];
      std::string newpassphrase;
      res = input_password(newpassword, 256, true, prompt1, prompt2, logger);
      if (!res)
        throw std::invalid_argument("Error entering passphrase");
      newpassphrase = newpassword;
     
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
    tls_process_error(logger);
    return false;
  }

  //If the "DESTROY" myproxy command is given, try to get the 
  //information about the existence of stored credentials 
  //on the myproxy server.
  try {
    if (myproxy_command == "destroy" || myproxy_command == "DESTROY" || myproxy_command == "Destroy") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if(user_name.empty()) {
        user_name = get_cert_dn(proxy_path);
      }
      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string prompt1 = "MyProxy server";
      char password[256];
      std::string passphrase;
      int res = input_password(password, 256, false, prompt1, "", logger);
      if (!res)
        throw std::invalid_argument("Error entering passphrase");
      passphrase = password;

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
    tls_process_error(logger);
    return false;
  }

  //If the "GET" myproxy command is given, try to get a delegated
  //certificate from the myproxy server.
  //For "GET" command, certificate and key are not needed, and
  //anonymous GSSAPI is used (GSS_C_ANON_FLAG)
  try {
    if (myproxy_command == "get" || myproxy_command == "GET" || myproxy_command == "Get") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");

      if(user_name.empty()) {
        user_name = get_cert_dn(proxy_path);
      }
      if (user_name.empty())
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string prompt1 = "MyProxy server";
      char password[256];

      std::string passphrase = password;
      if(!use_empty_passphrase) {
        int res = input_password(password, 256, false, prompt1, "", logger);
        if (!res)
          throw std::invalid_argument("Error entering passphrase");
        passphrase = password;
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
      char seq = '0';
      for (std::list<std::string>::iterator it = vomslist.begin();
           it != vomslist.end(); it++) {
        size_t p;
        std::string voms_server;
        p = (*it).find(":");
        voms_server = (p == std::string::npos) ? (*it) : (*it).substr(0, p);
        myproxyopt[std::string("vomsname").append(1, seq)] = voms_server;
        seq++;
      }
      seq = '0';
      // vomses can be specified, so that myproxy server could use it to contact voms server
      std::list<std::string> vomses;  
      // vomses --- Store matched vomses lines, only the 
      //vomses line that matches the specified voms name is included.
      std::map<std::string, std::vector<std::vector<std::string> > > matched_voms_line;
      std::multimap<std::string, std::string> server_command_map;
      find_matched_vomses(matched_voms_line, server_command_map, vomses, vomslist, vomses_path, usercfg, logger);
      for (std::list<std::string>::iterator it = vomses.begin();
           it != vomses.end(); it++) {
        std::string vomses_line;
        vomses_line = (*it);
        myproxyopt[std::string("vomses").append(1, seq)] = vomses_line; 
        seq++;
      }

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
    tls_process_error(logger);
    return false;
  }

  //Delegate the former self-delegated credential to
  //myproxy server
  try {
    if (myproxy_command == "put" || myproxy_command == "PUT" || myproxy_command == "Put") {
      if (myproxy_server.empty())
        throw std::invalid_argument("URL of MyProxy server is missing");
      if(user_name.empty()) {
        user_name = get_cert_dn(proxy_path);
      }
      if (user_name.empty()) 
        throw std::invalid_argument("Username to MyProxy server is missing");

      std::string prompt1 = "MyProxy server";
      std::string prompt2 = "MyProxy server";
      char password[256];
      std::string passphrase;
      if(retrievable_by_cert.empty()) {
        int res = input_password(password, 256, true, prompt1, prompt2, logger);
        if (!res)
          throw std::invalid_argument("Error entering passphrase");
        passphrase = password;
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
    tls_process_error(logger);
    remove_proxy_file(proxy_path);
    return false;
  }
  return true;
}

static bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    std::list<std::string>& vomslist, std::string& vomses_path, Arc::UserConfig& usercfg, Arc::Logger& logger) {
  //Parse the voms server and command from command line
  for (std::list<std::string>::iterator it = vomslist.begin();
      it != vomslist.end(); it++) {
    size_t p;
    std::string voms_server;
    std::string command;
    p = (*it).find(":");
    //here user could give voms name or voms nick name
    voms_server = (p == std::string::npos) ? (*it) : (*it).substr(0, p);
    command = (p == std::string::npos) ? "" : (*it).substr(p + 1);
    server_command_map.insert(std::pair<std::string, std::string>(voms_server, command));
  }

  //Parse the 'vomses' file to find configure lines corresponding to
  //the information from the command line
  if (vomses_path.empty())
    vomses_path = usercfg.VOMSESPath();
  if (vomses_path.empty()) {
    logger.msg(Arc::ERROR, "$X509_VOMS_FILE, and $X509_VOMSES are not set;\nUser has not specify the location for vomses information;\nThere is also not vomses location information in user's configuration file;\nCannot find vomses in default locations: ~/.arc/vomses, ~/.voms/vomses, $ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses, /etc/vomses, /etc/grid-security/vomses, and the location at the corresponding sub-directory");
    return false;
  }

  //the 'vomses' location could be one single files; 
  //or it could be a directory which includes multiple files, such as 'vomses/voA', 'vomses/voB', etc.
  //or it could be a directory which includes multiple directories that includes multiple files,
  //such as 'vomses/atlas/voA', 'vomses/atlas/voB', 'vomses/alice/voa', 'vomses/alice/vob', 
  //'vomses/extra/myprivatevo', 'vomses/mypublicvo'
  std::vector<std::string> vomses_files;
  //If the location is a file
  if(is_file(vomses_path)) vomses_files.push_back(vomses_path);
  //If the locaton is a directory, all the files and directories will be scanned
  //to find the vomses information. The scanning will not stop until all of the
  //files and directories are all scanned.
  else {
    std::vector<std::string> files;
    files = search_vomses(vomses_path);
    if(!files.empty())vomses_files.insert(vomses_files.end(), files.begin(), files.end());
    files.clear();
  }


  for(std::vector<std::string>::iterator file_i = vomses_files.begin(); file_i != vomses_files.end(); file_i++) {
    std::string vomses_file = *file_i;
    std::ifstream in_f(vomses_file.c_str());
    std::string voms_line;
    while (true) {
      voms_line.clear();
      std::getline<char>(in_f, voms_line, '\n');
      if (voms_line.empty())
        break;
      if((voms_line.size() >= 1) && (voms_line[0] == '#')) continue;

      bool has_find = false; 
      //boolean value to record if the vomses server information has been found in this vomses line
      std::vector<std::string> voms_tokens;
      Arc::tokenize(voms_line,voms_tokens," \t","\"");
      if(voms_tokens.size() != VOMS_LINE_NUM) {
        // Warning: malformed voms line
        logger.msg(Arc::WARNING, "VOMS line contains wrong number of tokens (%u expected): \"%s\"", (unsigned int)VOMS_LINE_NUM, voms_line);
      }
      if(voms_tokens.size() > VOMS_LINE_NAME) {
        std::string str = voms_tokens[VOMS_LINE_NAME];

        for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
             it != server_command_map.end(); it++) {
          std::string voms_server = (*it).first;
          if (str == voms_server) {
            matched_voms_line[voms_server].push_back(voms_tokens);
            vomses.push_back(voms_line);
            has_find = true;
            break;
          };
        };
      };

      if(!has_find) {
        //you can also use the nick name of the voms server
        if(voms_tokens.size() > VOMS_LINE_NAME) {
          std::string str1 = voms_tokens[VOMS_LINE_NAME];
          for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
               it != server_command_map.end(); it++) {
            std::string voms_server = (*it).first;
            if (str1 == voms_server) {
              matched_voms_line[voms_server].push_back(voms_tokens);
              vomses.push_back(voms_line);
              break;
            };
          };
        };
      };
    };
  };//end of scanning all of the vomses files

  //Judge if we can not find any of the voms server in the command line from 'vomses' file
  //if(matched_voms_line.empty()) {
  //  logger.msg(Arc::ERROR, "Cannot get voms server information from file: %s", vomses_path);
  // throw std::runtime_error("Cannot get voms server information from file: " + vomses_path);
  //}
  //if (matched_voms_line.size() != server_command_map.size())
  for (std::multimap<std::string, std::string>::iterator it = server_command_map.begin();
       it != server_command_map.end(); it++)
    if (matched_voms_line.find((*it).first) == matched_voms_line.end())
      logger.msg(Arc::ERROR, "Cannot get VOMS server %s information from the vomses files",
                 (*it).first);
  return true;
}


