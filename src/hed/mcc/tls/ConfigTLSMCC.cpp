#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/miscutils.h>

#include "PayloadTLSStream.h"

#include "ConfigTLSMCC.h"

namespace Arc {

static void config_VOMS_add(XMLNode cfg,std::vector<std::string>& vomscert_trust_dn) {
  XMLNode nd = cfg["VOMSCertTrustDNChain"];
  for(;(bool)nd;++nd) {
    XMLNode cnd = nd["VOMSCertTrustDN"];
    if((bool)cnd) {
      for(;(bool)cnd;++cnd) {
        vomscert_trust_dn.push_back((std::string)cnd);
      }
      vomscert_trust_dn.push_back("----NEXT CHAIN----");
    } else {
      XMLNode rnd = nd["VOMSCertTrustRegex"];
      if(rnd) {
        std::string rgx = (std::string)rnd;
        if(rgx[0] != '^') rgx.insert(0,"^");
        if(rgx[rgx.length()-1] != '$') rgx+="$";
        vomscert_trust_dn.push_back(rgx);
        vomscert_trust_dn.push_back("----NEXT CHAIN----");
      }
    }
  }
}

// This class is collection of configuration information

ConfigTLSMCC::ConfigTLSMCC(XMLNode cfg,Logger& logger,bool client) {
  client_authn_ = true;
  cert_file_ = (std::string)(cfg["CertificatePath"]);
  key_file_ = (std::string)(cfg["KeyPath"]);
  ca_file_ = (std::string)(cfg["CACertificatePath"]);
  ca_dir_ = (std::string)(cfg["CACertificatesDir"]);
  globus_policy_ = (((std::string)(cfg["CACertificatesDir"].Attribute("PolicyGlobus"))) == "true");
  globus_gsi_ = (((std::string)(cfg["GSI"])) == "globus");
  globusio_gsi_ = (((std::string)(cfg["GSI"])) == "globusio");
  handshake_ = (cfg["Handshake"] == "SSLv3")?ssl3_handshake:tls_handshake;
  proxy_file_ = (std::string)(cfg["ProxyPath"]);
  
  std::vector<std::string> gridSecDir (2);
  gridSecDir[0] = G_DIR_SEPARATOR_S + std::string("etc");
  gridSecDir[1] = "grid-security";
  std::string gridSecurityDir = Glib::build_path(G_DIR_SEPARATOR_S, gridSecDir);

  if(!client) {
    
    if(cert_file_.empty()) cert_file_= Glib::build_filename(gridSecurityDir, "hostcert.pem");
    if(key_file_.empty()) key_file_= Glib::build_filename(gridSecurityDir, "hostkey.pem");
    // Use VOMS trust DN of server certificates specified in configuration
    config_VOMS_add(cfg,vomscert_trust_dn_);
    // Look for those configured in separate files
    // TODO: should those file be reread on every connection
    XMLNode locnd = cfg["VOMSCertTrustDNChainsLocation"];
    for(;(bool)locnd;++locnd) {
      std::string filename = (std::string)locnd;
      std::ifstream file(filename.c_str());
      if (!file) {
        logger.msg(ERROR, "Can not read file %s with list of trusted VOMS DNs", filename);
        continue;
      };
      XMLNode node;
      file >> node;
      config_VOMS_add(node,vomscert_trust_dn_);
    };
    std::string vproc = cfg["VOMSProcessing"];
    if(vproc == "relaxed") {
      voms_processing_ = relaxed_voms;
    } else if(vproc == "standard") {
      voms_processing_ = standard_voms;
    } else if(vproc == "strict") {
      voms_processing_ = strict_voms;
    } else if(vproc == "noerrors") {
      voms_processing_ = noerrors_voms;
    } else {
      voms_processing_ = standard_voms;
    }
    //If ClientAuthn is explicitly set to be "false" in configuration,
    //then client/authentication is not required, which means client 
    //side does not need to provide certificate and key in its configuration.
    //The default value of ClientAuthn is "true"
    if (((std::string)((cfg)["ClientAuthn"])) == "false") client_authn_ = false;
  } else {
    //If both CertificatePath and ProxyPath have not beed configured, 
    //client side can not provide certificate for server side. Then server 
    //side should not require client authentication
    if(cert_file_.empty() && proxy_file_.empty()) client_authn_ = false;
  };
  if(ca_dir_.empty() && ca_file_.empty()) ca_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "certificates";
  if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
}

bool ConfigTLSMCC::Set(SSL_CTX* sslctx,Logger& logger) {
  if((!ca_file_.empty()) || (!ca_dir_.empty())) {
    if(!SSL_CTX_load_verify_locations(sslctx, ca_file_.empty()?NULL:ca_file_.c_str(), ca_dir_.empty()?NULL:ca_dir_.c_str())) {
      logger.msg(ERROR, "Can not assign CA location - %s",ca_dir_.empty()?ca_file_:ca_dir_);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  };
  if(!cert_file_.empty()) {
    // Try to load proxy then PEM and then ASN1 certificate
    if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file_.c_str()) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load certificate file - %s",cert_file_);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  };
  if(!key_file_.empty()) {
    if((SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
       (SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
      logger.msg(ERROR, "Can not load key file - %s",key_file_);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  };
  if((!key_file_.empty()) && (!cert_file_.empty()))
    if(!(SSL_CTX_check_private_key(sslctx))) {
      logger.msg(ERROR, "Private key %s does not match certificate %s",key_file_,cert_file_);
      PayloadTLSStream::HandleError(logger);
      return false;
    };
  return true;
}

} // namespace Arc

