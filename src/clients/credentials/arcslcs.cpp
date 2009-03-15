// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
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
#include <openssl/x509.h>
#include <openssl/bio.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/credential/Credential.h>
#include <arc/client/ClientSAML2SSO.h>
#include <arc/message/MCC.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/URL.h>
#include <arc/client/UserConfig.h>

static Arc::Logger& logger = Arc::Logger::rootLogger;

static void tls_process_error(void) {
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

int main(int argc, char *argv[]) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options("", "", "");

  std::string slcs_url;
  options.AddOption('S', "url", istring("URL of SLCS service"),
                    istring("url"), slcs_url);

  std::string idp_name;
  options.AddOption('I', "idp", istring("IdP name"),
                    istring("string"), idp_name);

  std::string username;
  options.AddOption('U', "user", istring("User account to IdP"),
                    istring("string"), username);

  std::string password;
  options.AddOption('P', "password", istring("Password for user account to IdP"),
                    istring("string"), password);

  int keysize = 0;
  options.AddOption('Z', "keysize", istring("Key size of the private key (512, 1024, 2048)"),
                    istring("number"), keysize);

  std::string keypass;
  options.AddOption('K', "keypass", istring("Private key passphrase"),
                    istring("passphrase"), keypass);

  int lifetime = 0;
  options.AddOption('L', "lifetime", istring("Lifetime of the certificate, start with current time, hour as unit"),
                    istring("period"), lifetime);

  std::string storedir;
  options.AddOption('D', "storedir", istring("Store directory for key and signed certificate"),
                    istring("directory"), storedir);

  std::string conffile;
  options.AddOption('c', "conffile",
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

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (slcs_url.empty() && usercfg.ConfTree()["SLCSURL"])
    slcs_url = (std::string)usercfg.ConfTree()["SLCSURL"];

  if (idp_name.empty() && usercfg.ConfTree()["IdPname"])
    idp_name = (std::string)usercfg.ConfTree()["IdPname"];

  if (username.empty() && usercfg.ConfTree()["Username"])
    username = (std::string)usercfg.ConfTree()["Username"];

  if (password.empty() && usercfg.ConfTree()["Password"])
    password = (std::string)usercfg.ConfTree()["Password"];

  if (keysize == 0 && usercfg.ConfTree()["Keysize"]) {
    std::string keysize_str = (std::string)usercfg.ConfTree()["Keysize"];
    keysize = strtol(keysize_str.c_str(), NULL, 0);
  }
  if (keysize == 0)
    keysize = 1024;

  if (keypass.empty() && usercfg.ConfTree()["Keypass"])
    keypass = (std::string)usercfg.ConfTree()["Keypass"];

  if (lifetime == 0 && usercfg.ConfTree()["CertLifetime"]) {
    std::string lifetime_str = (std::string)usercfg.ConfTree()["CertLifetime"];
    lifetime = strtol(lifetime_str.c_str(), NULL, 0);
  }
  if (lifetime == 0)
    lifetime = 12;

  if (storedir.empty() && usercfg.ConfTree()["StoreDir"])
    storedir = (std::string)usercfg.ConfTree()["StoreDir"];

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcslcs", VERSION) << std::endl;
    return 0;
  }

  try {
    if (params.size() != 0)
      throw std::invalid_argument("Wrong number of arguments!");

    Arc::User user;

    //certificate and key file to be generated by SLCS service
    std::string key_path, cert_path;

    if (storedir.empty()) {
      key_path = Arc::GetEnv("X509_USER_KEY");
      if (key_path.empty())
        key_path = user.get_uid() == 0 ? "/etc/grid-security/hostkey.pem" :
                   user.Home() + "/.globus/userkey.pem";
    }
    else
      key_path = storedir + "/userkey.pem";

    if (storedir.empty()) {
      cert_path = Arc::GetEnv("X509_USER_CERT");
      if (cert_path.empty())
        cert_path = user.get_uid() == 0 ? "/etc/grid-security/hostcert.pem" :
                    user.Home() + "/.globus/usercert.pem";
    }
    else
      cert_path = storedir + "/usercert.pem";
    std::string cert_req_path = cert_path.substr(0, (cert_path.size() - 13)) + "/usercert_request.pem";

    //Generate certificate request
    Arc::Time t;
    Arc::Credential request(t, Arc::Period(lifetime * 3600), keysize, "EEC");
    std::string cert_req_str;
    if (!request.GenerateRequest(cert_req_str))
      throw std::runtime_error("Failed to generate certificate request");
    std::ofstream out_cert_req(cert_req_path.c_str(), std::ofstream::out);
    out_cert_req.write(cert_req_str.c_str(), cert_req_str.size());
    out_cert_req.close();
    std::string private_key;
    //If the passphrase for protecting private key has not been set,
    // the private key will not be protected by passphrase.
    if (keypass.empty())
      request.OutputPrivatekey(private_key);
    else
      request.OutputPrivatekey(private_key, true, keypass);
    std::ofstream out_key(key_path.c_str(), std::ofstream::out);
    out_key.write(private_key.c_str(), private_key.size());
    out_key.close();

    //Send soap message to service, implicitly including the SAML2SSO profile
    Arc::MCCConfig mcc_cfg;  //TODO: ClientAuthn should be off here
    //mcc_cfg.AddPrivateKey("testkey-nopass.pem");
    //mcc_cfg.AddCertificate("testcert.pem");
    //mcc_cfg.AddCAFile("cacert.pem");
    //mcc_cfg.AddCADir("certificates");

    //CA path or CA directory for certifying the server's credential
    //Note: this client itself will not provide credential (server side
    //will not verify client's certificate).
    std::string trusted_ca_path, trusted_ca_dir;
    trusted_ca_path = (std::string)usercfg.ConfTree()["CACertificatePath"];
    if (!trusted_ca_path.empty())
      mcc_cfg.AddCAFile(trusted_ca_path);
    trusted_ca_dir = (std::string)usercfg.ConfTree()["CACertificatesDir"];
    if (trusted_ca_dir.empty())
      trusted_ca_dir = user.get_uid() == 0 ? "/etc/grid-security/certificates" :
                       user.Home() + "/.globus/certificates";
    if (!trusted_ca_dir.empty())
      mcc_cfg.AddCADir(trusted_ca_dir);

    Arc::URL url(slcs_url);

    Arc::ClientSOAPwithSAML2SSO *client_soap = NULL;
    client_soap = new Arc::ClientSOAPwithSAML2SSO(mcc_cfg, url);
    logger.msg(Arc::INFO, "Creating and sending soap request");

    Arc::NS slcs_ns;
    slcs_ns["slcs"] = "http://www.nordugrid.org/schemas/slcs";
    Arc::PayloadSOAP req_soap(slcs_ns);
    req_soap.NewChild("GetSLCSCertificateRequest").NewChild("X509Request") = cert_req_str;

    Arc::PayloadSOAP *resp_soap = NULL;
    if (client_soap) {
      Arc::MCC_Status status = client_soap->process(&req_soap, &resp_soap, idp_name, username, password);
      if (!status) {
        logger.msg(Arc::ERROR, "SOAP with SAML2SSO invokation failed");
        throw std::runtime_error("SOAP with SAML2SSO invokation failed");
      }
      if (resp_soap == NULL) {
        logger.msg(Arc::ERROR, "There was no SOAP response");
        throw std::runtime_error("There was no SOAP response");
      }
    }
    {
      std::string xml_soap;
      resp_soap->GetXML(xml_soap);
      std::cout << "XML: " << xml_soap << std::endl;
    }

    std::string cert_str = (std::string)((*resp_soap)["GetSLCSCertificateResponse"]["X509Certificate"]);
    std::string ca_str = (std::string)((*resp_soap)["GetSLCSCertificateResponse"]["CACertificate"]);
    if (resp_soap)
      delete resp_soap;
    if (client_soap)
      delete client_soap;

    //Output the responded slcs certificate and CA certificate
    std::ofstream out_cert(cert_path.c_str(), std::ofstream::out);
    out_cert.write(cert_str.c_str(), cert_str.size());
    out_cert.close();

    char ca_name[20];
    if (!ca_str.empty()) {
      BIO *ca_bio = BIO_new_mem_buf((void*)(ca_str.c_str()), ca_str.length());
      X509 *ca_cert = PEM_read_bio_X509(ca_bio, NULL, NULL, NULL);
      unsigned long ca_hash;
      ca_hash = X509_subject_name_hash(ca_cert);
      snprintf(ca_name, 20, "%08lx.0", ca_hash);
    }
    std::string ca_path = cert_path.substr(0, (cert_path.size() - 13)) + "/certificates/" + ca_name;
    std::ofstream out_ca(ca_path.c_str(), std::ofstream::out);
    out_ca.write(ca_str.c_str(), ca_str.size());
    out_ca.close();

    return EXIT_SUCCESS;
  } catch (std::exception& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
    tls_process_error();
    return EXIT_FAILURE;
  }
}
