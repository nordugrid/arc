#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unordered_map>
#include <sys/stat.h>
#include <glibmm/miscutils.h>
#include <openssl/err.h>
#include <openssl/dh.h> // For DH_* in newer OpenSSL

#include <arc/credential/Credential.h>
#include <arc/Utils.h>

#include "PayloadTLSStream.h"

#include "ConfigTLSMCC.h"


// For early OpenSSL 1.0.0
#ifndef SSL_OP_NO_TLSv1_1
#define SSL_OP_NO_TLSv1_1 SSL_OP_NO_TLSv1
#endif

namespace ArcMCCTLS {

using namespace Arc;

Arc::Logger ConfigTLSMCC::logger(Arc::Logger::getRootLogger(), "MCC.TLS.Config");

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

ConfigTLSMCC::ConfigTLSMCC(XMLNode cfg,bool client) {
  protocol_options_ = 0;
  curve_nid_ = NID_undef; // so far best seems to be NID_X25519, but let OpenSSL choose by default
  client_authn_ = true;
  system_ca_dir_ = (((std::string)(cfg["SystemCADir"])) == "true");
  system_ca_file_ = (((std::string)(cfg["SystemCAFile"])) == "true");
  grid_ca_ = !(((std::string)(cfg["GridCA"])) == "false");
  allow_insecure_ = (((std::string)(cfg["AllowInsecure"])) == "true");
  if(grid_ca_) {
    ca_file_ = (std::string)(cfg["CACertificatePath"]);
    ca_dir_ = (std::string)(cfg["CACertificatesDir"]);
    globus_policy_ = (((std::string)(cfg["CACertificatesDir"].Attribute("PolicyGlobus"))) == "true");
  } else {
    globus_policy_ = false;
  }
  cert_file_ = (std::string)(cfg["CertificatePath"]);
  key_file_ = (std::string)(cfg["KeyPath"]);
  voms_dir_ = (std::string)(cfg["VOMSDir"]);
  globus_gsi_ = (((std::string)(cfg["GSI"])) == "globus");
  globusio_gsi_ = (((std::string)(cfg["GSI"])) == "globusio");
  handshake_ = (cfg["Handshake"] == "SSLv3")?ssl3_handshake:tls_handshake;
  proxy_file_ = (std::string)(cfg["ProxyPath"]);
  credential_ = (std::string)(cfg["Credential"]);
  cipher_list_ = (std::string)(cfg["Ciphers"]);
  cipher_suites_ = (std::string)(cfg["CipherSuites"]);
  server_ciphers_priority_ = (((std::string)(cfg["Ciphers"].Attribute("ServerPriority"))) == "true");
  dhparam_file_ = (std::string)(cfg["DHParamFile"]);
  if(cipher_list_.empty()) {
    // Now we rely on system's security policy and do not set any ciphers by default.
    // Explicitely set Cipher overwrites other security options.
    // Encyption option now has limited functionality. Only option which is still
    // supported is "off" to allow connection without encryption if strongly needed.
    if(cfg["Encryption"] == "off") {
      cipher_list_ = "eNULL:!aNULL";
    } else if(client) {
      // For clients allow env variable to set default ciphers
      cipher_list_ = GetEnv("ARC_CIPHERS_STRING");
    }
    // For TLSv1.3 ciphersuites we fully rely on system policy.
    // There is no way to override anything here.
  }
  if(client) {
    protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    hostname_ = (std::string)(cfg["Hostname"]);
    XMLNode protocol_node = cfg["Protocol"];
    while((bool)protocol_node) {
      std::string protocol = (std::string)protocol_node;
      if(!protocol.empty()) {
        if(protocol.length() > 255) protocol.resize(255);    
        protocols_.append(1,(char)protocol.length());
        protocols_.append(protocol);
      }
      ++protocol_node;
    }
  } else {
    // For enabled protocols we also rely on system policies by default.
    XMLNode protocol_node = cfg["Protocol"];
    if((bool)protocol_node) {
      // start from all disallowed (all we know about)
      protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3;
      while((bool)protocol_node) {
        std::string protocol = (std::string)protocol_node;
        std::list<std::string> tokens;
        Arc::tokenize(protocol, tokens);
        for(std::list<std::string>::iterator token = tokens.begin(); token != tokens.end(); ++token) {
          std::string str = Arc::trim(*token);
          if(str == "SSLv2") {
            protocol_options_ &= ~SSL_OP_NO_SSLv2;
          } else if(str == "SSLv3") {
            protocol_options_ &= ~SSL_OP_NO_SSLv3;
          } else if(str == "TLSv1.0") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1;
          } else if(str == "TLSv1.1") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1_1;
          } else if(str == "TLSv1.2") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1_2;
          };
        };
        ++protocol_node;
      };
    };

    XMLNode curve_node = cfg["Curve"];
    if((bool)curve_node) {
      int nid = OBJ_sn2nid(((std::string)curve_node).c_str());
      if (nid != NID_undef) {
        curve_nid_ = nid;
      }
    }
    if(server_ciphers_priority_) {
      protocol_options_ |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    }
  }

  std::vector<std::string> gridSecDir (2);
  gridSecDir[0] = G_DIR_SEPARATOR_S + std::string("etc");
  gridSecDir[1] = "grid-security";
  std::string gridSecurityDir = Glib::build_path(G_DIR_SEPARATOR_S, gridSecDir);

  if(!client) {
    // Default location of server certificate/key

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
        failure_ = "Can not read file "+filename+" with list of trusted VOMS DNs";
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
  if(grid_ca_ && ca_dir_.empty() && ca_file_.empty()) ca_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "certificates";
  if(voms_dir_.empty()) voms_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "vomsdir";
  if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
}

namespace {
  class X509_STORE_KEY: public std::string {
  public:
    X509_STORE_KEY(bool system_ca_dir, bool system_ca_file, bool grid_ca, std::string const& ca_file, std::string const& ca_dir) {
       append(system_ca_dir?"1":"0", 2);
       append(system_ca_file?"1":"0", 2);
       append(grid_ca?"1":"0", 2);
       append(ca_file.c_str(), ca_file.length()+1);
       append(ca_dir.c_str(), ca_dir.length()+1);
    }
  };

  typedef std::unordered_map<std::string, Credential::X509_STORERef> X509_STORE_CACHE;
  X509_STORE_CACHE x509_stores_;
}

Credential::X509_STORERef ConfigTLSMCC::BuildStore() const {
  Credential::X509_STORERef store(X509_STORE_new());
  if (!store) return store;

  if(system_ca_dir_) {
    logger.msg(VERBOSE, "Using CA default folder location");
    X509_LOOKUP* lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
    if (!lookup) return {};
    X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
  }
  if (system_ca_file_) {
      logger.msg(VERBOSE, "Using CA default file location");
      X509_LOOKUP* lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
      if (!lookup) return {};
      X509_LOOKUP_load_file(lookup, ca_file_.c_str(), X509_FILETYPE_DEFAULT);
  }
  if(grid_ca_ && (!ca_file_.empty() || !ca_dir_.empty())) {
    if(!ca_file_.empty()) {
      logger.msg(VERBOSE, "Using CA file: %s",ca_file_);
      X509_LOOKUP* lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
      if (!lookup) return {};
      if (X509_LOOKUP_load_file(lookup, ca_file_.c_str(), X509_FILETYPE_PEM) <= 0) return {};
    }
    if(!ca_dir_.empty()) {
      logger.msg(VERBOSE, "Using CA dir: %s",ca_dir_);
      X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
      if (!lookup) return {};
      X509_LOOKUP_add_dir(lookup, ca_dir_.c_str(), X509_FILETYPE_PEM);
    }
  }
  return store;
}

// Currently reuse_store is not used (=false) in favor of direct caching loaded CRLs
bool ConfigTLSMCC::Set(SSL_CTX * sslctx, bool reuse_store) {
  if (reuse_store) {
    // Re-use cached store or create new one  
    X509_STORE_KEY key(system_ca_dir_, system_ca_file_, grid_ca_, ca_file_, ca_dir_);
    auto store_it = x509_stores_.find(key);
    if (store_it == x509_stores_.end()) {
      auto store = BuildStore();
      if (!store) return false;
      store_it = x509_stores_.emplace(key, std::move(store)).first;
      if (store_it == x509_stores_.end()) return false; // paranoia check
    }
    // The sslctx is supposed to be called with unconfigured store, so we can safely replace it with new one.
    // The operations on store itself are protected from multithreaded access by lock member of X509_STORE object
    // so sharing same store among multiple X509_STORE_CTX and SSL_CTX is possible.
    SSL_CTX_set1_cert_store(sslctx, store_it->second);
  } else {
    auto store = BuildStore();
    if (!store) return false;
    SSL_CTX_set1_cert_store(sslctx, store);
  }

  if(!credential_.empty()) {
    // First try to use in-memory credential
    Credential cred(credential_, credential_, ca_dir_, ca_file_, system_ca_dir_, system_ca_file_, grid_ca_, Credential::NoPassword(), false);
    if (!cred) {
      failure_ = "Failed to read in-memory credentials";
      return false;
    }
    EVP_PKEY* key = cred.GetPrivKey();
    if (SSL_CTX_use_PrivateKey(sslctx, key) != 1) {
      failure_ = "Can not load key from in-memory credentials\n";
      failure_ += HandleError();
      EVP_PKEY_free(key);
      return false;
    }
    EVP_PKEY_free(key);
    X509* cert = cred.GetCert();
    if (SSL_CTX_use_certificate(sslctx, cert) != 1) {
      failure_ = "Can not load certificate from in-memory credentials\n";
      failure_ += HandleError();
      X509_free(cert);
      return false;
    }
    X509_free(cert);
    // Add certificate chain
    STACK_OF(X509)* chain = cred.GetCertChain();
    int res = 1;
    if(chain) {
      for (int id = 0; id < sk_X509_num(chain) && res == 1; ++id) {
        X509* cert = sk_X509_value(chain,id);
        // Differently from SSL_CTX_use_certificate call to SSL_CTX_add_extra_chain_cert does not
        // increase reference counter (calls sk_x509_push inside). So must call it here.
        res = SSL_CTX_add_extra_chain_cert(sslctx, X509_dup(cert));
      }
      sk_X509_pop_free(chain, X509_free);
    }

    if (res != 1) {
      failure_ = "Can not construct certificate chain from in-memory credentials\n";
      failure_ += HandleError();
      return false;
    }
  }
  else {
    if(!cert_file_.empty()) {
      // Try to load proxy then PEM and then ASN1 certificate
      if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file_.c_str()) != 1) &&
         (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
         (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        failure_ = "Can not load certificate file - "+cert_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
    if(!key_file_.empty()) {
      if((SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
         (SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        failure_ = "Can not load key file - "+key_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
    if((!key_file_.empty()) && (!cert_file_.empty())) {
      if(!(SSL_CTX_check_private_key(sslctx))) {
        failure_ = "Private key "+key_file_+" does not match certificate "+cert_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
  };
  if(!dhparam_file_.empty()) {
    logger.msg(VERBOSE, "Using DH parameters from file: %s",dhparam_file_);
    FILE *dhf = fopen(dhparam_file_.c_str(), "r");
    if (!dhf) {
      logger.msg(ERROR, "Failed to open file with DH parameters for reading");
    } else {
      DH *dh = PEM_read_DHparams(dhf, NULL, NULL, NULL);
      fclose(dhf);
      if (dh == NULL) {
        logger.msg(ERROR, "Failed to read file with DH parameters");
      } else {
        if (SSL_CTX_set_tmp_dh(sslctx, dh) != 1) {
          logger.msg(ERROR, "Failed to apply DH parameters");
        } else {
          logger.msg(DEBUG, "DH parameters applied");
        };
        DH_free(dh);
      };
    };
  };
  /*
  if(SSL_CTX_set1_curves_list(sslctx,"P-256:P-384:P-521")) {
    logger.msg(ERROR, "Failed to apply ECDH groups");
  } else {
    logger.msg(DEBUG, "ECDH groups applied");
  };
  */
  if(curve_nid_ != NID_undef) {
    logger.msg(VERBOSE, "Using curve with NID: %u",curve_nid_);
    EC_KEY* ecdh = EC_KEY_new_by_curve_name(curve_nid_);
    if (ecdh == NULL) {
      logger.msg(ERROR, "Failed to generate EC key");
    } else {
      if(SSL_CTX_set_tmp_ecdh(sslctx, ecdh) != 1) {
        logger.msg(ERROR, "Failed to apply ECDH parameters");
      } else {
        logger.msg(DEBUG, "ECDH parameters applied");
      };
      EC_KEY_free(ecdh);
    };
  };
  if(!cipher_list_.empty()) {
    logger.msg(VERBOSE, "Using cipher list: %s",cipher_list_);
    if(!SSL_CTX_set_cipher_list(sslctx,cipher_list_.c_str())) {
      failure_ = "No ciphers found to satisfy requested encryption level. "
                 "Check if OpenSSL supports ciphers '"+cipher_list_+"'\n";
      failure_ += HandleError();
      return false;
    };
  };
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L)
  if(!cipher_suites_.empty()) {
    if(!SSL_CTX_set_ciphersuites(sslctx,cipher_suites_.c_str())) {
      failure_ = "No cipher suites found to satisfy requested encryption level. "
                 "Check if OpenSSL supports cipher suites '"+cipher_suites_+"'\n";
      failure_ += HandleError();
      return false;
    };
  }
#endif
  if(!protocols_.empty()) {
    if(SSL_CTX_set_alpn_protos(sslctx, (unsigned char const *)protocols_.c_str(), (unsigned int)protocols_.length()) != 0) {
      // TODO: add warning message
    };
  };
  if(protocol_options_ != 0) {
    logger.msg(VERBOSE, "Using protocol options: 0x%x",protocol_options_);
    SSL_CTX_set_options(sslctx, protocol_options_);
  };
  return true;
}

std::string ConfigTLSMCC::HandleError(int code) {
  std::string errstr;
  unsigned long e = (code==SSL_ERROR_NONE)?ERR_get_error():code;
  while(e != SSL_ERROR_NONE) {
    if(e == SSL_ERROR_SYSCALL) {
      // Hiding system errors
      // int err = errno;
      // logger.msg(DEBUG, "SSL error: system call failed: %d, %s",err,StrError(err));
    } else {
      const char* lib = ERR_lib_error_string(e);
      const char* func = ERR_func_error_string(e);
      const char* reason = ERR_reason_error_string(e);
      const char* alert = SSL_alert_desc_string_long(e);
      // Ignore unknown errors
      if (reason || func || lib || std::string(alert) != "unknown") {
        if(!errstr.empty()) errstr += "\n";
        errstr += "SSL error";
        if (reason) errstr += ", \"" + std::string(reason) + "\"";
        if (func) errstr += ", in \"" + std::string(func) + "\" function";
        if (lib) errstr += ", at \"" + std::string(lib) + "\" library";
        if (alert) errstr += ", with \"" + std::string(alert) + "\" alert";
        //logger.msg(DEBUG, errstr);
      }
    };  
    e = ERR_get_error();
  }
  return errstr;
}

void ConfigTLSMCC::ClearError(void) {
  int e = ERR_get_error();
  while(e != 0) {
    e = ERR_get_error();
  }
}

static char to_hex(uint8_t v) {
  if (v < 10)
    return '0' + v;
  return 'a' + (v - 10);
}

static ConfigTLSMCC::X509_CRLRef GetCRL(std::string const& path, struct stat& st) {
  constexpr std::size_t CRLCacheByPathThresholdHigh = 1100;
  constexpr std::size_t CRLCacheByPathThresholdLow = 1000;
  struct X509_CRL_CACHE_ITEM {
    std::mutex lock;
    timespec mtime;
    ConfigTLSMCC::X509_CRLRef crl;
    time_t atime;

    bool Matches(struct stat& st) const {
      return (mtime.tv_sec == st.st_mtim.tv_sec) && (mtime.tv_nsec == st.st_mtim.tv_nsec);
    }

    ConfigTLSMCC::X509_CRLRef ShareCRL() {
      auto crl_p = crl.get();
      if (!crl_p) return ConfigTLSMCC::X509_CRLRef();
      if (X509_CRL_up_ref(crl_p) != 1) return ConfigTLSMCC::X509_CRLRef();
      return ConfigTLSMCC::X509_CRLRef(crl_p);
    }
  };
  typedef std::shared_ptr<X509_CRL_CACHE_ITEM> X509_CRL_CACHE_ENTRY;
  typedef std::unordered_map<std::string, X509_CRL_CACHE_ENTRY> X509_CRL_CACHE;
  static X509_CRL_CACHE CRLCacheByPath;
  static std::mutex CRLCacheByPathLock;

  X509_CRL_CACHE_ENTRY entry;
  {
    std::lock_guard<std::mutex> lock(CRLCacheByPathLock);

    // while we have lock acquired trim cache size
    if (CRLCacheByPath.size() > CRLCacheByPathThresholdHigh) {
      // Sort collected CRLs by access time
      std::map<time_t, std::list<X509_CRL_CACHE::iterator> > atimes; // map is ordered by key
      for(auto it = CRLCacheByPath.begin(); it != CRLCacheByPath.end(); ++it) {
        atimes[it->second->atime].push_back(it);
      }
      // Delete them till threshold is met.
      // The entry for 'path' can be deleted here, but probability is rather low.
      for(auto& atime: atimes) {
        for(auto it: atime.second) CRLCacheByPath.erase(it);
        if (CRLCacheByPath.size() <= CRLCacheByPathThresholdLow) break;
      }
    }

    // Get or create cache entry
    auto& entryRef = CRLCacheByPath[path];
    if (!entryRef) entryRef = std::make_shared<X509_CRL_CACHE_ITEM>();
    entry = entryRef;
  } // slight race condition here, but shared_ptr handles it

  {
    std::lock_guard<std::mutex> lock(entry->lock);
    if (entry->crl) { // this will cause previously failed to load CRL to be retried immeditely because we are expecting intermediate error causing previous faiure
      if (entry->Matches(st)) {
        entry->atime = time(nullptr);
        return entry->ShareCRL();
      }
      entry->crl.reset();
    }

    // by here entry is either new one or expired
    entry->mtime = st.st_mtim; // file may have changed since st was obtained, but in worst case it will be reloaded on next call
    entry->atime = time(nullptr);

    auto f = fopen(path.c_str(), "r");
    if (f) {
      X509_CRL* crl = PEM_read_X509_CRL(f, nullptr, nullptr, nullptr);
      fclose(f);
      if (crl) {
        entry->crl = crl;
        return entry->ShareCRL();
      }
    }
  }

  // we failed to load CRL, but we keep it in cache because there is file for it
  return ConfigTLSMCC::X509_CRLRef();
}

static ConfigTLSMCC::X509_CRLRef GetCRL(X509_NAME const* issuer, std::string const& hash, std::string const& dir) {
  for(int suffix = 0; ; ++suffix) {
    std::string path = dir + "/" + hash + ".r" + std::to_string(suffix);

    struct stat st;
    if (0 != stat(path.c_str(), &st))
        break; // no more files with such suffix

    auto crl = GetCRL(path, st);
    if(!crl) continue;

    X509_NAME* name = X509_CRL_get_issuer(crl.get());
    if (!name) continue;
    if(X509_NAME_cmp(name, issuer) != 0) continue;

    return crl;
  }
  return ConfigTLSMCC::X509_CRLRef();
}

ConfigTLSMCC::X509_CRLRef ConfigTLSMCC::GetCRL(X509* cert) const {
  if (!cert) return X509_CRLRef();
  auto hash = X509_issuer_name_hash(cert);
  std::string hash_str;
  for (int n = 28; n >= 0; n-=4) hash_str += to_hex((hash >> n) & 0xf);
  
  if (system_ca_dir_) {
    std::string dir;
    auto dir_env = X509_get_default_cert_dir_env();
    if (dir_env) dir = Arc::GetEnv(dir_env);
    if (dir.empty()) {
      auto def_dir = X509_get_default_cert_dir();
      if (def_dir) dir = def_dir;
    }
    if (!dir.empty()) {
      auto crl = ArcMCCTLS::GetCRL(X509_get_issuer_name(cert), hash_str, dir);
      if (crl) return crl;
    }
  }
  if (!ca_dir_.empty()) {
    auto crl = ArcMCCTLS::GetCRL(X509_get_issuer_name(cert), hash_str, ca_dir_);
    if (crl) return crl;
  }
  return X509_CRLRef();
}

} // namespace Arc

