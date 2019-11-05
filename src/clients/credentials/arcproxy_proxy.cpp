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

using namespace ArcCredential;

void create_tmp_proxy(std::string& proxy, Arc::Credential& signer) {
  int keybits = 2048;
  Arc::Time now;
  Arc::Period period = 3600 * 12 + 300;
  std::string req_str;
  Arc::Credential tmp_proxyreq(now-Arc::Period(300), period, keybits);
  tmp_proxyreq.GenerateRequest(req_str);

  std::string proxy_private_key;
  std::string signing_cert;
  std::string signing_cert_chain;

  tmp_proxyreq.OutputPrivatekey(proxy_private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);

  if (!signer.SignRequest(&tmp_proxyreq, proxy)) throw std::runtime_error("Failed to sign proxy");
  proxy.append(proxy_private_key).append(signing_cert).append(signing_cert_chain);
}

void create_proxy(std::string& proxy,
    Arc::Credential& signer, 
    const std::string& proxy_policy,
    const Arc::Time& proxy_start, const Arc::Period& proxy_period, 
    const std::string& vomsacseq,
    int keybits,
    const std::string& signing_algorithm) {

  std::string private_key, signing_cert, signing_cert_chain;
  std::string req_str;
  if(keybits < 0) keybits = signer.GetKeybits();

  Arc::Credential cred_request(proxy_start, proxy_period, keybits);
  cred_request.SetSigningAlgorithm(signer.GetSigningAlgorithm());

  if(!signing_algorithm.empty() && signing_algorithm != "inherit") {
    if(signing_algorithm == "sha1") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA1);
    } else if(signing_algorithm == "sha2") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA256);
    } else if(signing_algorithm == "sha224") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA224);
    } else if(signing_algorithm == "sha256") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA256);
    } else if(signing_algorithm == "sha384") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA384);
    } else if(signing_algorithm == "sha512") {
      cred_request.SetSigningAlgorithm(Arc::SIGN_SHA512);
    } else {
      throw std::runtime_error("Unknown signing algorithm specified: "+signing_algorithm);
    }
  }
  cred_request.GenerateRequest(req_str);
  cred_request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);

  //Put the voms attribute certificate into proxy certificate
  if (!vomsacseq.empty()) {
    bool r = cred_request.AddExtension("acseq", (char**)(vomsacseq.c_str()));
    if (!r) std::cout << Arc::IString("Failed to add VOMS AC extension. Your proxy may be incomplete.") << std::endl;
  }

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

  if (!signer.SignRequest(&cred_request, proxy)) throw std::runtime_error("Failed to sign proxy");
  proxy.append(private_key).append(signing_cert).append(signing_cert_chain);
}

void write_proxy_file(const std::string& path, const std::string& content) {
  std::string::size_type off = 0;
  if((!Arc::FileDelete(path)) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove proxy file " + path);
  }
  if(!Arc::FileCreate(path, content, 0, 0, S_IRUSR | S_IWUSR)) {
    throw std::runtime_error("Failed to create proxy file " + path);
  }
}

void remove_proxy_file(const std::string& path) {
  if((!Arc::FileDelete(path)) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove proxy file " + path);
  }
}

void remove_cert_file(const std::string& path) {
  if((!Arc::FileDelete(path)) && (errno != ENOENT)) {
    throw std::runtime_error("Failed to remove certificate file " + path);
  }
}

