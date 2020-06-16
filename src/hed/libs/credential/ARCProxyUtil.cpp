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

/*
#ifdef HAVE_NSS
#include <arc/credential/NSSUtil.h>
#endif
*/

#include "ARCProxyUtil.h"

using namespace ArcCredential;

namespace Arc {

#define VOMS_LINE_NICKNAME (0)
#define VOMS_LINE_HOST (1)
#define VOMS_LINE_PORT (2)
#define VOMS_LINE_SN (3)
#define VOMS_LINE_NAME (4)
#define VOMS_LINE_NUM (5)

static bool is_file(std::string path);

static bool is_dir(std::string path);

static std::string tokens_to_string(std::vector<std::string> tokens);

static int create_proxy_file(const std::string& path);

static void write_proxy_file(const std::string& path, const std::string& content);

static void remove_proxy_file(const std::string& path);

static void remove_cert_file(const std::string& path);

static void tls_process_error(Arc::Logger& logger);

static int input_password(char *password, int passwdsz, bool verify,
                          const std::string& prompt_info,
                          const std::string& prompt_verify_info,
                          Arc::Logger& logger);

static Arc::Credential* create_cred_object(Arc::Logger& logger, const std::string& cert_path,
		const std::string& key_path);

static void create_tmp_proxy(const std::string& tmp_proxy_path, Arc::Credential& signer);

static void create_proxy(Arc::Logger& logger, std::string& proxy_cert, Arc::Credential& signer,
    const std::string& proxy_policy, Arc::Time& proxy_start, Arc::Period& proxy_period,
    const std::string& vomsacseq, bool use_gsi_proxy, int keybits);

static std::string get_proxypolicy(const std::string& policy_source);

static std::string get_cert_dn(const std::string& cert_file);

static std::vector<std::string> search_vomses(std::string path);

static bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    const std::list<std::string>& vomslist, const std::string& vomses_path, Arc::Logger& logger);

static bool contact_voms_servers(const std::list<std::string>& vomslist, const std::list<std::string>& orderlist,
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
      const std::string& ca_dir, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq);

static bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command,
      const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
      const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
      std::list<std::string>& vomslist, std::string& vomses_path, const std::string& proxy_path,
      Arc::UserConfig& usercfg, Arc::Logger& logger);

static bool create_tmp_proxy_from_nssdb(const std::string& nssdb_path,
      const std::string& tmp_proxy_path, Arc::Logger& logger);

static bool create_proxy_from_nssdb(const std::list<std::string>& vomslist, Arc::Logger& logger);

#ifdef HAVE_NSS
static void get_default_nssdb_path(std::vector<std::string>& nss_paths);

static void get_nss_certname(std::string& certname, Arc::Logger& logger);
#endif



  static Logger ARCProxyUtilLogger(Logger::getRootLogger(), "ARCProxyUtil");

  ARCProxyUtil::ARCProxyUtil() : keybits(2048), sha(SHA1) {

  }

  bool ARCProxyUtil::Contact_VOMS_Server(bool use_nssdb, const std::list<std::string>& vomslist,
      const std::list<std::string>& orderlist, bool use_gsi_comm, bool use_http_comm,
      const std::string& voms_period, std::string vomsacseq) {
    if(vomslist.empty()) {
      ARCProxyUtilLogger.msg(Arc::ERROR, "VOMS command is empty");
      return false;
    }

	//Generate a temporary self-signed proxy certificate
    //to contact the voms server
    std::string tmp_proxy_path;
    tmp_proxy_path = Glib::build_filename(Glib::get_tmp_dir(), std::string("tmp_proxy.pem"));
    if(!use_nssdb) {
	  Arc::Credential* signer = create_cred_object(ARCProxyUtilLogger, cert_path, key_path);
	  if(!signer) return false;
      create_tmp_proxy(tmp_proxy_path, *signer);
      if(signer != NULL) delete signer;
    }
    else {
      bool res = create_tmp_proxy_from_nssdb(nssdb_path, tmp_proxy_path, ARCProxyUtilLogger);
      if(!res) return false;
    }

    //Contact voms server to create voms proxy
    bool ret = contact_voms_servers(vomslist, orderlist, vomses_path, use_gsi_comm,
        use_http_comm, voms_period, ca_dir, ARCProxyUtilLogger, tmp_proxy_path, vomsacseq);
    remove_proxy_file(tmp_proxy_path);

	return ret;
  }

  bool ARCProxyUtil::Create_Proxy(bool use_nssdb, const std::string& proxy_policy,
          Arc::Time& proxy_start, Arc::Period& proxy_period,
          const std::string& vomsacseq) {
    bool res = false;
    if(!use_nssdb) {
	  Arc::Credential* signer = create_cred_object(ARCProxyUtilLogger, cert_path, key_path);
	  if(!signer) return false;

      std::cout << Arc::IString("Your identity: %s", signer->GetIdentityName()) << std::endl;

      std::string proxy_cert;
      res = create_proxy(ARCProxyUtilLogger, proxy_cert, *signer, proxy_policy, proxy_start, proxy_period,
          vomsacseq, false, keybits);
      delete signer;
    }
    else {

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

static std::string tokens_to_string(std::vector<std::string> tokens) {
  std::string s;
  for(int n = 0; n<tokens.size(); ++n) {
    s += "\""+tokens[n]+"\" ";
  };
  return s;
}

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

static Arc::Credential* create_cred_object(Arc::Logger& logger, const std::string& cert_path,
		const std::string& key_path) {
  Arc::Credential* cred = NULL;
  const Arc::Time now;
  cred = new Arc::Credential(cert_path, key_path, "", "");
  if (cred->GetIdentityName().empty()) {
    logger.msg(Arc::ERROR, "Proxy generation failed: No valid certificate found.");
    delete cred; cred = NULL;
    return false;
  }
  EVP_PKEY* pkey = cred->GetPrivKey();
  if(!pkey) {
    logger.msg(Arc::ERROR, "Proxy generation failed: No valid private key found.");
    delete cred; cred = NULL;
    return false;
  }
  if(pkey) EVP_PKEY_free(pkey);
  //std::cout << Arc::IString("Your identity: %s", signer.GetIdentityName()) << std::endl;
  if (now > cred->GetEndTime()) {
  	logger.msg(Arc::ERROR, "Proxy generation failed: Certificate has expired.");
    delete cred; cred = NULL;
    return false;
  }
  else if (now < cred->GetStartTime()) {
    logger.msg(Arc::ERROR, "Proxy generation failed: Certificate is not valid yet.");
    delete cred; cred = NULL;
    return false;
  }
  return cred;
}

static void create_tmp_proxy(const std::string& tmp_proxy_path, Arc::Credential& signer) {
  int keybits = 2048;
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

static bool create_proxy(Arc::Logger& logger, std::string& proxy_cert, Arc::Credential& signer,
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
    if (!r) logger.msg(Arc::ERROR, "Failed to add VOMS AC extension. Your proxy may be incomplete.");
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

  if (!signer.SignRequest(&cred_request, proxy_cert)) {
    logger.msg(Arc::ERROR, "Failed to sign proxy");
    return false;
  }

  proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);

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

static bool find_matched_vomses(std::map<std::string, std::vector<std::vector<std::string> > > &matched_voms_line /*output*/,
    std::multimap<std::string, std::string>& server_command_map /*output*/,
    std::list<std::string>& vomses /*output*/,
    const std::list<std::string>& vomslist, const std::string& vomses_path, Arc::Logger& logger) {
  //Parse the voms server and command from command line
  for (std::list<std::string>::const_iterator it = vomslist.begin();
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

static bool contact_voms_servers(const std::list<std::string>& vomslist, const std::list<std::string>& orderlist,
      std::string& vomses_path, bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
      const std::string& ca_dir, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq) {

  std::map<std::string, std::vector<std::vector<std::string> > > matched_voms_line;
  std::multimap<std::string, std::string> server_command_map;
  std::list<std::string> vomses;
  if(!find_matched_vomses(matched_voms_line, server_command_map, vomses, vomslist, vomses_path, logger))
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
      for(std::list<std::string>::const_iterator o_it = orderlist.begin(); o_it != orderlist.end(); o_it++) {
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
        Arc::ClientHTTP client(cfg, voms_url);//, usercfg.Timeout());
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
        Arc::ClientTCP client(cfg, address, atoi(port.c_str()), use_gsi_comm ? Arc::GSISec : Arc::SSL3Sec);//, usercfg.Timeout());
        Arc::PayloadRaw request;
        request.Insert(send_msg.c_str(), 0, send_msg.length());
        Arc::PayloadStreamInterface *response = NULL;
        Arc::MCC_Status status = client.process(&request, &response, true);
        if (!status) {
          //logger.msg(Arc::ERROR, (std::string)status);
          if (response) delete response;
          std::cout << Arc::IString("The VOMS server with the information:\n\t%s\ncan not be reached, please make sure it is available", tokens_to_string(voms_line)) << std::endl;
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
        std::cout << Arc::IString("There are %d servers with the same name: %s in your vomses file, but none of them can be reached, or can return valid message. But proxy without VOMS AC extension will still be generated.", voms_lines.size(), voms_server) << std::endl;
    }
  }

  return true;
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
      find_matched_vomses(matched_voms_line, server_command_map, vomses, vomslist, vomses_path, logger);
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


#ifdef HAVE_NSS
static void get_default_nssdb_path(std::vector<std::string>& nss_paths) {
  const Arc::User user;
  // The profiles.ini could exist under firefox, seamonkey and thunderbird
  std::vector<std::string> profiles_homes;

  std::string home_path = user.Home();

  std::string profiles_home;

#if defined(_MACOSX)
  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "Firefox";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Application Support" G_DIR_SEPARATOR_S "SeaMonkey";
  profiles_homes.push_back(profiles_home);

  profiles_home = home_path + G_DIR_SEPARATOR_S "Library" G_DIR_SEPARATOR_S "Thunderbird";
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
  std::list<ArcAuthNSS::certInfo> certInfolist;
  ArcAuthNSS::nssListUserCertificatesInfo(certInfolist);
  if(certInfolist.size()) {
    std::cout<<Arc::IString("There are %d user certificates existing in the NSS database",
      certInfolist.size())<<std::endl;
  }
  int n = 1;
  std::list<ArcAuthNSS::certInfo>::iterator it;
  for(it = certInfolist.begin(); it != certInfolist.end(); it++) {
    ArcAuthNSS::certInfo cert_info = (*it);
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

static std::string get_nssdb_path(Arc::Logger& logger) {
  // Get the nss db paths from firefox's profile.ini file
  std::vector<std::string> nssdb_paths;
  get_default_nssdb_path(nssdb_paths);
  if(nssdb_paths.empty()) {
    logger.msg(Arc::ERROR, "The NSS database can not be detected in the Firefox profile");
    return std::string();
  }

  // Let user to choose which profile to use
  // if multiple profiles exist
    std::string nssdb;
  if(nssdb_paths.size()) {
    std::cout<<Arc::IString("There are %d NSS base directories where the certificate, key, and module databases live",
    nssdb_paths.size())<<std::endl;
  }
  for(int i=0; i < nssdb_paths.size(); i++) {
    std::cout<<Arc::IString("Number %d is: %s", i+1, nssdb_paths[i])<<std::endl;
  }
  std::cout << Arc::IString("Please choose the NSS database you would like to use (1-%d): ", nssdb_paths.size());
  if(nssdb_paths.size() == 1) { nssdb = nssdb_paths[0]; }
  char c;
  while(true && (nssdb_paths.size()>1)) {
    c = getchar();
    int num = c - '0';
    if((num<=nssdb_paths.size()) && (num>=1)) {
      configdir = nssdb_paths[num-1];
      break;
    }
  }

  std::cout<< Arc::IString("NSS database to be accessed: %s\n", nssdb.c_str());
  return nssdb;
}

static bool create_tmp_proxy_from_nssdb(const std::string& nssdb_path,
    const std::string& tmp_proxy_path, Arc::Logger& logger) {
  bool res = ArcAuthNSS::nssInit(nssdb_path);

  char* slotpw = NULL;
  //The nss db under firefox profile seems to not be protected by any passphrase by default
  const char* trusts = "u,u,u";

  // Generate CSR
  std::string proxy_csrfile = "proxy.csr";
  std::string proxy_keyname = "proxykey";
  std::string proxy_privk_str;

  // Generate CSR
  std::string passwd = "";
  ArcAuthNSS::Context ctx(ArcAuthNSS::Context::EmptyContext);
  ArcAuthNSS::NSS::CSRContext proxy_request(ctx, nssdb_path, passwd);
  res = proxy_request.createRequest(proxy_key_name);

  if(!res) {
	logger.msg(Arc::ERROR, "Failed to generate X509 request with NSS");
    return false;
  }

  std::string issuername;
  get_nss_certname(issuername, logger);

  // Create tmp proxy cert
  int duration = 12;
  res = ArcAuthNSS::nssCreateCert(proxy_csrfile, issuername, NULL, duration, "", tmp_proxy_path, ascii);
  if(!res) {
    logger.msg(Arc::ERROR, "Failed to create X509 certificate with NSS");
    return false;
  }
  std::string tmp_proxy_cred_str;
  std::ifstream tmp_proxy_cert_s(tmp_proxy_path.c_str());
  std::getline(tmp_proxy_cert_s, tmp_proxy_cred_str,'\0');
  tmp_proxy_cert_s.close();

  // Export EEC
  std::string cert_file = "cert.pem";
  res = ArcAuthNSS::nssExportCertificate(issuername, cert_file);
  if(!res) {
    logger.msg(Arc::ERROR, "Failed to export X509 certificate from NSS DB");
    return false;
  }
  std::string eec_cert_str;
  std::ifstream eec_s(cert_file.c_str());
  std::getline(eec_s, eec_cert_str,'\0');
  eec_s.close();
  remove_cert_file(cert_file);

  // Compose tmp proxy file
  tmp_proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
  write_proxy_file(tmp_proxy_path, tmp_proxy_cred_str);

  return true;
}



Arc::Logger& logger, std::string& proxy_cert, Arc::Credential& signer,
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
    if (!r) logger.msg(Arc::ERROR, "Failed to add VOMS AC extension. Your proxy may be incomplete.");
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

  if (!signer.SignRequest(&cred_request, proxy_cert)) {
    logger.msg(Arc::ERROR, "Failed to sign proxy");
    return false;
  }

  proxy_cert.append(private_key).append(signing_cert).append(signing_cert_chain);

  return true;

static bool create_proxy_from_nssdb(Arc::Logger& logger, std::string& proxy_cert, Arc::Credential& signer,
	    const std::string& proxy_policy, Arc::Time& proxy_start, Arc::Period& proxy_period,
	    const std::string& vomsacseq, bool use_gsi_proxy, int keybits) {

  // Create proxy with VOMS AC
  std::string proxy_certfile = "myproxy.pem";



  /////
  char* slotpw = NULL;
   //The nss db under firefox profile seems to not be protected by any passphrase by default
   bool ascii = true;
   const char* trusts = "u,u,u";

   // Generate CSR
   std::string proxy_csrfile = "proxy.csr";
   std::string proxy_keyname = "proxykey";
   std::string proxy_privk_str;
   res = ArcAuthNSS::nssGenerateCSR(proxy_keyname, "CN=Test,OU=ARC,O=EMI", slotpw, proxy_csrfile, proxy_privk_str, ascii);
   if(!res) {
 	logger.msg(Arc::ERROR, "Failed to generate X509 request with NSS");
     return false;
   }

   std::string issuername;
   get_nss_certname(issuername, logger);

   // Create tmp proxy cert
   int duration = 12;
   res = ArcAuthNSS::nssCreateCert(proxy_csrfile, issuername, NULL, duration, "", tmp_proxy_path, ascii);
   if(!res) {
     logger.msg(Arc::ERROR, "Failed to create X509 certificate with NSS");
     return false;
   }
   std::string tmp_proxy_cred_str;
   std::ifstream tmp_proxy_cert_s(tmp_proxy_path.c_str());
   std::getline(tmp_proxy_cert_s, tmp_proxy_cred_str,'\0');
   tmp_proxy_cert_s.close();

   // Export EEC
   std::string cert_file = "cert.pem";
   res = ArcAuthNSS::nssExportCertificate(issuername, cert_file);
   if(!res) {
     logger.msg(Arc::ERROR, "Failed to export X509 certificate from NSS DB");
     return false;
   }
   std::string eec_cert_str;
   std::ifstream eec_s(cert_file.c_str());
   std::getline(eec_s, eec_cert_str,'\0');
   eec_s.close();
   remove_cert_file(cert_file);

   // Compose tmp proxy file
   tmp_proxy_cred_str.append(proxy_privk_str).append(eec_cert_str);
   write_proxy_file(tmp_proxy_path, tmp_proxy_cred_str);


  ////



  // Let user to choose which credential to use
  if(issuername.empty()) get_nss_certname(issuername, logger);
  std::cout<<Arc::IString("Certificate to use is: %s", issuername)<<std::endl;

  int duration;
  duration = validityPeriod.GetPeriod() / 3600;

  std::string vomsacseq_asn1;
  if(!vomsacseq.empty()) Arc::VOMSACSeqEncode(vomsacseq, vomsacseq_asn1);
  res = ArcAuthNSS::nssCreateCert(proxy_csrfile, issuername, "", duration, vomsacseq_asn1, proxy_certfile, ascii);
  if(!res) {
    logger.msg(Arc::ERROR,"Failed to create X509 certificate with NSS");
    return false;
  }

  const char* proxy_certname = "proxycert";
  res = ArcAuthNSS::nssImportCert(slotpw, proxy_certfile, proxy_certname, trusts, ascii);
  if(!res) {
    logger.msg(Arc::ERROR,"Failed to import X509 certificate into NSS DB");
    return false;
  }

  //Compose the proxy certificate
  if(!proxy_path.empty())Arc::SetEnv("X509_USER_PROXY", proxy_path);
  Arc::UserConfig usercfg(conffile,
      Arc::initializeCredentialsType(Arc::initializeCredentialsType::NotTryCredentials));
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed to initialize the credential configuration");
    return false;
  }
  if(proxy_path.empty()) proxy_path = usercfg.ProxyPath();
  usercfg.ProxyPath(proxy_path);
  std::string cert_file = "cert.pem";
  res = ArcAuthNSS::nssExportCertificate(issuername, cert_file);
  if(!res) {
    logger.msg(Arc::ERROR, "Failed to export X509 certificate from NSS DB");
    return false;
  }

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

    return true;
  }

#endif

}


