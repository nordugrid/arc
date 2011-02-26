#include <iostream>
#include <globus_gsi_credential.h>

#include <vector>

#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/Logger.h>

#include "../misc/escaped.h"
#include "auth.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserVOMS");

int process_vomsproxy(const char* filename,std::vector<struct voms> &data,bool auto_cert = false);

int AuthUser::process_voms(void) {
  if(!voms_extracted) {
    if(filename.length() > 0) {
      int err = process_vomsproxy(filename.c_str(),voms_data);
      voms_extracted=true;
      logger.msg(Arc::DEBUG, "VOMS proxy processing returns: %i", err);
      if(err != AAA_POSITIVE_MATCH) return err;
    };
  };
  return AAA_POSITIVE_MATCH;
}

int AuthUser::match_voms(const char* line) {
  // parse line
  std::string vo("");
  std::string group("");
  std::string role("");
  std::string capabilities("");
  std::string auto_c("");
  bool auto_cert = false;
  int n;
  n=gridftpd::input_escaped_string(line,vo,' ','"');
  if(n == 0) {
    logger.msg(Arc::ERROR, "Missing VO in configuration");
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,group,' ','"');
  if(n == 0) {
    logger.msg(Arc::ERROR, "Missing group in configuration");
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,role,' ','"');
  if(n == 0) {
    logger.msg(Arc::ERROR, "Missing role in configuration");
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,capabilities,' ','"');
  if(n == 0) {
    logger.msg(Arc::ERROR, "Missing capabilities in configuration");
    return AAA_FAILURE;
  };
  n=gridftpd::input_escaped_string(line,auto_c,' ','"');
  if(n != 0) {
    if(auto_c == "auto") auto_cert=true;
  };
  logger.msg(Arc::VERBOSE, "VOMS config: vo: %s", vo);
  logger.msg(Arc::VERBOSE, "VOMS config: group: %s", group);
  logger.msg(Arc::VERBOSE, "VOMS config: role: %s", role);
  logger.msg(Arc::VERBOSE, "VOMS config: capabilities: %s", capabilities);
  // extract info from voms proxy
  // if(voms_data->size() == 0) {
  process_voms();
  if(voms_data.size() == 0) return AAA_NO_MATCH;
  // analyse permissions
  for(std::vector<struct voms>::iterator v = voms_data.begin();v!=voms_data.end();++v) {
    logger.msg(Arc::DEBUG, "match vo: %s", v->voname);
    if((vo == "*") || (vo == v->voname)) {
      for(std::vector<struct voms_attrs>::iterator d=v->std.begin();d!=v->std.end();++d) {
        logger.msg(Arc::VERBOSE, "match group: %s", d->group);
        logger.msg(Arc::VERBOSE, "match role: %s", d->role);
        logger.msg(Arc::VERBOSE, "match capabilities: %s", d->cap);
        if(((group == "*") || (group == d->group)) &&
           ((role == "*") || (role == d->role)) &&
           ((capabilities == "*") || (capabilities == d->cap))) {
          logger.msg(Arc::VERBOSE, "VOMS matched");
          default_voms_=v->server.c_str();
          default_vo_=v->voname.c_str();
          default_role_=d->role.c_str();
          default_capability_=d->cap.c_str();
          default_vgroup_=d->group.c_str();
          return AAA_POSITIVE_MATCH;
        };
      };
    };
  };
  logger.msg(Arc::VERBOSE, "VOMS matched nothing");
  return AAA_NO_MATCH;
}


int process_vomsproxy(const char* filename,std::vector<struct voms> &data,bool /* auto_cert */) {
  X509 * cert = NULL;
  STACK_OF(X509) * cert_chain = NULL;
  EVP_PKEY * key = NULL;
  int n = 0;
  std::vector<struct voms>::iterator i;
  std::string voms_dir = "/etc/grid-security/vomsdir";
  std::string cert_dir = "/etc/grid-security/certificates";
  {
    char *v;
    if((v = getenv("X509_VOMS_DIR"))) voms_dir = v;
    if((v = getenv("X509_CERT_DIR"))) cert_dir = v;
  };
  BIO *bio = NULL;
  FILE *f = NULL;
  Arc::Credential c(filename, filename, cert_dir, "");
  std::vector<Arc::VOMSACInfo> output;
  std::vector<std::string> output_merged;
  std::string emptystring = "";
  Arc::VOMSTrustList emptylist;

  if((bio = BIO_new_file(filename, "r")) == NULL) {
    logger.msg(Arc::ERROR, "Failed to open file %s", filename);
    goto error_exit;
  };
  if(!PEM_read_bio_X509(bio,&cert, NULL, NULL)) {
    logger.msg(Arc::ERROR, "Failed to read PEM from file %s", filename);
    goto error_exit;
  };
  key=PEM_read_bio_PrivateKey(bio,NULL,NULL,NULL);
  if(!key) {
    logger.msg(Arc::ERROR, "Failed to read private key from file %s - probably no delegation was done", filename);
    // goto error_exit;
  };
  if((cert_chain = sk_X509_new_null()) == NULL) {
    logger.msg(Arc::ERROR, "Failed in SSL (sk_X509_new_null)");
    goto error_exit;
  };
  while(!BIO_eof(bio)) {
    X509 * tmp_cert = NULL;
    if(!PEM_read_bio_X509(bio, &tmp_cert, NULL, NULL)) {
      break;
    };
    if(n == 0) {
      X509_free(cert); cert=tmp_cert;
    } else { 
      if(!sk_X509_insert(cert_chain, tmp_cert, n-1)) {
        logger.msg(Arc::ERROR, "Failed in SSL (sk_X509_insert)");
        goto error_exit;
      };
    };
    ++n;
  };

  if (!parseVOMSAC(c, emptystring, emptystring, emptylist, output, false)) {
    logger.msg(Arc::ERROR, "Error: no VOMS extension found");
    goto error_exit;
  };
  for(int n=0;n<output.size();++n) {
    for(int i=0;i<output[n].attributes.size();++i) {
      output_merged.push_back(output[n].attributes[i]);
    };
  };
  data = AuthUser::arc_to_voms(output_merged);
  X509_free(cert);
  EVP_PKEY_free(key);
  sk_X509_pop_free(cert_chain, X509_free);
  BIO_free(bio);
  ERR_clear_error();
  return AAA_POSITIVE_MATCH;
error_exit:
  if(f) fclose(f);
  if(cert) X509_free(cert);
  if(key) EVP_PKEY_free(key);
  if(cert_chain) sk_X509_pop_free(cert_chain, X509_free);
  if(bio) BIO_free(bio);
  ERR_clear_error();
  return AAA_FAILURE;
}
