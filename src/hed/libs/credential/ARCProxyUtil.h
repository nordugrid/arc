#ifndef __ARC_PROXYUTIL_H__
#define __ARC_PROXYUTIL_H__


namespace Arc {

/// Utility class for generating proxies
/**
 * \since Added in 3.0.1.
 **/
class ARCProxyUtil {
  enum { SHA1,
#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
         SHA224,
         SHA256,
         SHA384,
         SHA512,
#endif
       } SHA_OPTION;

  public:
    ARCProxyUtil();

    virtual ~ARCProxyUtil();

    bool Contact_VOMS_Server(bool use_nssdb, const std::list<std::string>& vomslist,
      const std::list<std::string>& orderlist, bool use_gsi_comm, bool use_http_comm,
      const std::string& voms_period, std::string vomsacseq);

    bool Contact_MyProxy_Server();

    bool Create_Proxy(bool use_nssdb, const std::string& proxy_policy,
            Arc::Time& proxy_start, Arc::Period& proxy_period,
            const std::string& vomsacseq);

    void Set_ProxyPath(const std::string& proxypath) { proxy_path = proxypath; };
    std::string Get_ProxyPath() { return proxy_path; };

    void Set_CertPath(const std::string& certpath) { cert_path = certpath; };
    std::string Get_CertPath() { return cert_path; };

    void Set_KeyPath(const std::string& keypath) { key_path = keypath; };
    std::string Get_KeyPath() { return key_path; };

    void Set_CACertDir(const std::string& cadir) { ca_dir = cadir; };
    std::string Get_CACertDir() { return ca_dir; };

    void Set_Keybits(int bits) { keybits = bits; };
    int Get_Keybits() { return keybits; };

    void Set_Digest(SHA_OPTION sha_opt) { sha = sha_opt; };
    SHA_OPTION Get_Digest() { return sha; };

    void Set_VOMSDir(const std::string& vomsdir) { voms_dir = vomsdir; };
    std::string Get_VOMSDir() { return voms_dir; };

    void Set_VOMSESPath(const std::string& vomsespath) { vomses_path = vomsespath; };
    std::string Get_VOMSESPath() { return vomses_path; };

    void Set_NSSDBPath(const std::string& nssdbpath) { nssdb_path = nssdbpath; };
    std::string Get_NSSDBPath() { return nssdb_path; };

  private:
    std::string proxy_path;
    std::string cert_path;
    std::string key_path;
    std::string ca_dir;
    std::string voms_dir;
    std::string vomses_path;
    std::string nssdb_path;

    int keybits;
    SHA_OPTION sha;

    std::string debug_level;



};

}

#endif /* __ARC_PROXYUTIL_H__ */
