#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "proxy.h"

#include <arc/Utils.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/FileUtils.h>
#include <arc/credential/Credential.h>
#include <arc/credentialstore/CredentialStore.h>


namespace gridftpd {

  char* write_proxy(gss_cred_id_t cred) {
    char* proxy_fname = NULL;
    OM_uint32 major_status = 0;
    OM_uint32 minor_status = 0;
    gss_buffer_desc deleg_proxy_filename;
    if(cred == GSS_C_NO_CREDENTIAL) return NULL;
    major_status = gss_export_cred(&minor_status,
                                   cred,
                                   NULL,
                                   1,
                                   &deleg_proxy_filename);
    if (major_status == GSS_S_COMPLETE) {
      char * cp;
      cp = strchr((char *)deleg_proxy_filename.value, '=');
      if(cp != NULL) {
        cp++;
        proxy_fname=strdup(cp);
      };
      free(deleg_proxy_filename.value);
    };
    return proxy_fname;
  }

  char* write_cert_chain(const gss_ctx_id_t gss_context) {
    /* Globus OID for the remote parties certificate chain */
    gss_OID_desc cert_chain_oid =
          {11, (void*)"\x2b\x06\x01\x04\x01\x9b\x50\x01\x01\x01\x08"};
    gss_buffer_set_t client_cert_chain = NULL;
    OM_uint32 major_status;
    OM_uint32 minor_status;
    int certs_num = 0;
    int n,n_;
    STACK_OF(X509) *cert_chain = NULL;
    BIO* bio = NULL;
    char* fname = NULL;

    major_status = gss_inquire_sec_context_by_oid(&minor_status,
                                                  gss_context,
                                                  &cert_chain_oid,
                                                  &client_cert_chain);
    if(major_status != GSS_S_COMPLETE) {
      return NULL;
    };
    certs_num = client_cert_chain->count;
    if(certs_num <= 0) goto err_exit;
    if((cert_chain = sk_X509_new_null()) == NULL) goto err_exit;
    for(n=0,n_=0;n<certs_num;n++) {
      const unsigned char* value = (unsigned char*)client_cert_chain->elements[n].value;
      int length = (int)client_cert_chain->elements[n].length;
      X509* cert = d2i_X509(NULL,&value,length);
      if(cert) {
        if(cert) sk_X509_insert(cert_chain,cert,n_++);
        /* {
        X509_NAME *name = X509_get_subject_name(cert);
        char buf[256]; buf[0]=0;
        if(name) {
          X509_NAME_oneline(name,buf,sizeof(buf));
          fprintf(stderr,"Name: %s\n",buf);
        } else {
          fprintf(stderr,"Name: none\n");
        };
        }; */
      } else {
        /*
        fprintf(stderr,"No cert\n");
        */
      };
    };
    /* TODO: do not store in file - pass directly to calling function */
    /* Make temporary file */
    {
      std::string tempname = Glib::build_filename(Glib::get_tmp_dir(), "x509.XXXXXX");
      if(!Arc::TmpFileCreate(tempname, "")) goto err_exit;
      fname = strdup(tempname.c_str());
      if((bio=BIO_new_file(fname,"w")) == NULL) goto err_exit;
    };
    for(n=0;n<n_;++n) {
      X509* cert=sk_X509_value(cert_chain,n);
      if(cert) {
        if(!PEM_write_bio_X509(bio,cert))  {
          goto err_exit;
        };
      };
    };
    goto exit;
  err_exit:
    if(fname) {
      unlink(fname); free(fname); fname=NULL;
    };
  exit:
    if(cert_chain) sk_X509_pop_free(cert_chain,X509_free);
    if(bio) BIO_free(bio);
    if(client_cert_chain) gss_release_buffer_set(&minor_status,&client_cert_chain);
    return fname;
  }

} // namespace gridftpd
