#include <iostream>
#include <globus_gsi_credential.h>

#include <vector>

#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/credential/VOMSUtil.h>

#include "../misc/escaped.h"
#include "auth.h"

#define odlog(int) std::cerr

int process_vomsproxy(const char* filename,std::vector<struct voms> &data,bool auto_cert = false);

int AuthUser::process_voms(void) {
  if(!voms_extracted) {
    if(filename.length() > 0) {
      int err = process_vomsproxy(filename.c_str(),voms_data);
      voms_extracted=true;
      odlog(DEBUG)<<"VOMS proxy processing returns: "<<err<<std::endl;
      if(err != AAA_POSITIVE_MATCH) return err;
    };
  };
  return AAA_POSITIVE_MATCH;
};

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
    odlog(ERROR)<<"Missing VO in configuration"<<std::endl; 
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,group,' ','"');
  if(n == 0) {
    odlog(ERROR)<<"Missing group in configuration"<<std::endl; 
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,role,' ','"');
  if(n == 0) {
    odlog(ERROR)<<"Missing role in configuration"<<std::endl; 
    return AAA_FAILURE;
  };
  line+=n;
  n=gridftpd::input_escaped_string(line,capabilities,' ','"');
  if(n == 0) {
    odlog(ERROR)<<"Missing capabilities in configuration"<<std::endl; 
    return AAA_FAILURE;
  };
  n=gridftpd::input_escaped_string(line,auto_c,' ','"');
  if(n != 0) {
    if(auto_c == "auto") auto_cert=true;
  };
  odlog(VERBOSE)<<"VOMS config: vo: "<<vo<<std::endl;
  odlog(VERBOSE)<<"VOMS config: group: "<<group<<std::endl;
  odlog(VERBOSE)<<"VOMS config: role: "<<role<<std::endl;
  odlog(VERBOSE)<<"VOMS config: capabilities: "<<capabilities<<std::endl;
  // extract info from voms proxy
  // if(voms_data->size() == 0) {
  process_voms();
  if(voms_data.size() == 0) return AAA_NO_MATCH;
  // analyse permissions
  for(std::vector<struct voms>::iterator v = voms_data.begin();v!=voms_data.end();++v) {
    odlog(DEBUG)<<"match vo: "<<v->voname<<std::endl;
    if((vo == "*") || (vo == v->voname)) {
      for(std::vector<struct voms_attrs>::iterator d=v->std.begin();d!=v->std.end();++d) {
        odlog(VERBOSE)<<"match group: "<<d->group<<std::endl;
        odlog(VERBOSE)<<"match role: "<<d->role<<std::endl;
        odlog(VERBOSE)<<"match capabilities: "<<d->cap<<std::endl;
        if(((group == "*") || (group == d->group)) &&
           ((role == "*") || (role == d->role)) &&
           ((capabilities == "*") || (capabilities == d->cap))) {
          odlog(VERBOSE)<<"VOMS matched"<<std::endl;
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
  odlog(VERBOSE)<<"VOMS matched nothing"<<std::endl;
  return AAA_NO_MATCH;
}


int process_vomsproxy(const char* filename,std::vector<struct voms> &data,bool auto_cert) {
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
  std::vector<std::string> output;
  std::string emptystring = "";
  Arc::VOMSTrustList emptylist;

  if((bio = BIO_new_file(filename, "r")) == NULL) {
    odlog(ERROR)<<"Failed to open file "<<filename<<std::endl;
    goto error_exit;
  };
  if(!PEM_read_bio_X509(bio,&cert, NULL, NULL)) {
    odlog(ERROR)<<"Failed to read PEM from file "<<filename<<std::endl;
    goto error_exit;
  };
  key=PEM_read_bio_PrivateKey(bio,NULL,NULL,NULL);
  if(!key) {
    odlog(ERROR)<<"Failed to read private key from file "<<filename<<" - probably no delegation was done"<<std::endl;
    // goto error_exit;
  };
  if((cert_chain = sk_X509_new_null()) == NULL) {
    odlog(ERROR)<<"Failed in SSL (sk_X509_new_null)"<<std::endl;
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
        odlog(ERROR)<<"failed in SSL (sk_X509_insert)"<<std::endl;
        goto error_exit;
      };
    };
    ++n;
  };

  if (!parseVOMSAC(c, emptystring, emptystring, emptylist, output, false)) {
    odlog(ERROR)<<"Error: no VOMS extension found"<<std::endl;
    goto error_exit;
  }
  data = AuthUser::arc_to_voms(output);
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
