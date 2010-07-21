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

  int prepare_proxy(void) {
    int h = -1;
    off_t len;
    char* buf = NULL;
    off_t l,ll;
    int res=-1;

    if(getuid() == 0) { /* create temporary proxy */
      std::string proxy_file=Arc::GetEnv("X509_USER_PROXY");
      if(proxy_file.empty()) goto exit;
      h=Arc::FileOpen(proxy_file,O_RDONLY);
      if(h==-1) goto exit;
      if((len=lseek(h,0,SEEK_END))==-1) goto exit;
      if(lseek(h,0,SEEK_SET) != 0) goto exit;
      buf=(char*)malloc(len);
      if(buf==NULL) goto exit;
      for(l=0;l<len;) {
        ll=read(h,buf+l,len-l);
        if(ll==-1) goto exit;
        if(ll==0) break;
        l+=ll;
      };
      close(h); h=-1; len=l;
      std::string proxy_file_tmp = proxy_file;
      proxy_file_tmp+=".tmp";
      h=Arc::FileOpen(proxy_file_tmp,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
      if(h==-1) goto exit;
      (void)chmod(proxy_file_tmp.c_str(),S_IRUSR | S_IWUSR);
      for(l=0;l<len;) {
        ll=write(h,buf+l,len-l);
        if(ll==1) goto exit;
        l+=ll;
      };
      close(h); h=-1;
      Arc::SetEnv("X509_USER_PROXY",proxy_file_tmp);
    };
    res=0;
   exit:
    if(buf) free(buf);
    if(h!=-1) close(h);
    return res;
  }

  int remove_proxy(void) {
    if(getuid() == 0) {
      std::string proxy_file=Arc::GetEnv("X509_USER_PROXY");
      if(proxy_file.empty()) return 0;
      remove(proxy_file.c_str());
    };
    return 0;
  }

  int renew_proxy(const char* old_proxy,const char* new_proxy) {
    int h = -1;
    off_t len,l,ll;
    char* buf = NULL;
    std::string proxy_file_tmp;
    struct stat st;
    int res = -1;

    h=Arc::FileOpen(new_proxy,O_RDONLY);
    if(h==-1) {
      fprintf(stderr,"Can't open new proxy: %s\n",new_proxy);
      goto exit;
    };
    if((len=lseek(h,0,SEEK_END))==-1) goto exit;
    lseek(h,0,SEEK_SET);
    if((buf=(char*)(malloc(len))) == NULL) {
      fprintf(stderr,"Out of memory\n");
      goto exit;
    };
    for(l=0;l<len;) {
      ll=read(h,buf+l,len-l);
      if(ll==-1) {
        fprintf(stderr,"Can't read new proxy: %s\n",new_proxy);
        goto exit;
      };
      if(ll==0) break;
      l+=ll;
    };
    close(h); h=-1; len=l;
    proxy_file_tmp=old_proxy;
    proxy_file_tmp+=".renew";
    remove(proxy_file_tmp.c_str());
    h=Arc::FileOpen(proxy_file_tmp,O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
    if(h==-1) {
      fprintf(stderr,"Can't create temporary proxy: %s\n",proxy_file_tmp.c_str());
      goto exit;
    };
    (void)chmod(proxy_file_tmp.c_str(),S_IRUSR | S_IWUSR);
    for(l=0;l<len;) {
      ll=write(h,buf+l,len-l);
      if(ll==-1) {
        fprintf(stderr,"Can't write temporary proxy: %s\n",proxy_file_tmp.c_str());
        goto exit;
      };
      l+=ll;
    };
    if(stat(old_proxy,&st) == 0) {
      fchown(h,st.st_uid,st.st_gid);
      if(remove(old_proxy) != 0) {
        fprintf(stderr,"Can't remove proxy: %s\n",old_proxy);
        goto exit;
      };
    };
    close(h); h=-1;
    if(rename(proxy_file_tmp.c_str(),old_proxy) != 0) {
      fprintf(stderr,"Can't rename temporary proxy: %s\n",proxy_file_tmp.c_str());
      goto exit;
    };
    res=0;
   exit:
    if(h!=-1) close(h);
    if(buf) free(buf);
    if(!proxy_file_tmp.empty()) remove(proxy_file_tmp.c_str());
    return res;
  }

  bool myproxy_renew(const char* old_proxy_file,const char* new_proxy_file,const char* myproxy_server) {
    if(!myproxy_server) return false;
    if(!old_proxy_file) return false;
    if(!new_proxy_file) return false;

    Arc::URL url(myproxy_server);
    Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));
    //usercfg.ProxyPath(old_proxy_file);
    usercfg.ProxyPath("");
    usercfg.CertificatePath("");
    usercfg.KeyPath("");
    Arc::CredentialStore cstore(usercfg,url);

    std::map<std::string,std::string> storeopt;
    std::map<std::string,std::string>::const_iterator m;
    m=url.Options().find("username");
    if(m != url.Options().end()) {
      storeopt["username"]=m->second;
    } else {
      Arc::Credential proxy(std::string(old_proxy_file),"","","");
      storeopt["username"]=proxy.GetIdentityName();
    };
    m=url.Options().find("credname");
    if(m != url.Options().end()) {
      storeopt["credname"]=m->second;
    };
    storeopt["lifetime"] = Arc::tostring(60*60*12);
    m=url.Options().find("password");
    if(m != url.Options().end()) {
      storeopt["password"] = m->second;
    };

    /* Get new proxy */
    std::string new_proxy_str;
    if(!cstore.Retrieve(storeopt,new_proxy_str)) {
      fprintf(stderr, "Failed to retrieve a proxy from MyProxy server %s\n", myproxy_server);
      return false;
    };
    std::ofstream h(new_proxy_file,std::ios::trunc);
    h<<new_proxy_str;
    if(h.fail()) {
      fprintf(stderr,"Can't open proxy file: %s\n",new_proxy_file);
      return false;
    };
    h.close();
    if(h.fail()) {
      fprintf(stderr,"Can't write to proxy file: %s\n",new_proxy_file);
      ::unlink(new_proxy_file);
      return false;
    };
    return true;
  }

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

  gss_cred_id_t read_proxy(const char* filename) {
    gss_cred_id_t cred = NULL;
    OM_uint32 major_status;
    OM_uint32 minor_status;
    gss_buffer_desc proxy_filename;

    if(filename == NULL) return NULL;
    proxy_filename.value=malloc(strlen(filename)+32);
    strcpy((char*)proxy_filename.value,"X509_USER_PROXY=");
    strcat((char*)proxy_filename.value,filename);
    proxy_filename.length=strlen((char*)proxy_filename.value);
    major_status = gss_import_cred(&minor_status,
                                   &cred,
                                   NULL,
                                   1,
                                   &proxy_filename,
                                   GSS_C_INDEFINITE,
                                   NULL);
    if (major_status != GSS_S_COMPLETE) {
      cred=NULL;
    };
    free(proxy_filename.value);
    return cred;
  }

  void free_proxy(gss_cred_id_t cred) {
    OM_uint32 minor_status;
    if(cred == NULL) return;
    gss_release_cred(&minor_status,&cred);
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
      int h;
      const char* prefix = "x509.";
      const char* tmp = getenv("TMP");
      if(tmp == NULL) tmp="/tmp";
      fname = (char*)malloc(strlen(tmp)+1+strlen(prefix)+6+1);
      if(fname == NULL) goto err_exit;
      strcpy(fname,tmp);
      strcat(fname,"/"); strcat(fname,prefix); strcat(fname,"XXXXXX");
      if((h=mkstemp(fname)) == -1) {
        free(fname); fname=NULL; goto err_exit;
      };
      fchmod(h,S_IRUSR | S_IWUSR); close(h);
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
