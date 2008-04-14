#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#include "cert_util.h"
#include "Credential.h"

  X509_EXTENSION* CreateExtension(std::string& name, std::string& data, bool crit) {
    X509_EXTENSION*   ext = NULL;
    ASN1_OBJECT*      ext_obj = NULL;
    ASN1_OCTET_STRING*  ext_oct = NULL;

    if(!(ext_obj = OBJ_nid2obj(OBJ_txt2nid((char *)(name.c_str()))))) {
      std::cerr<<"Can not convert string into ASN1_OBJECT"<<std::endl;
      return NULL;
    }

    ext_oct = ASN1_OCTET_STRING_new();

    ext_oct->data = (unsigned char*) malloc(data.size());
    memcpy(ext_oct->data, data.c_str(), data.size());
    ext_oct->length = data.size();

    if (!(ext = X509_EXTENSION_create_by_OBJ(NULL, ext_obj, crit, ext_oct))) {
      std::cerr<<"Can not create extension for proxy certificate"<<std::endl;
      if(ext_oct) ASN1_OCTET_STRING_free(ext_oct);
      if(ext_obj) ASN1_OBJECT_free(ext_obj);
      return NULL;
    }

    ext_oct = NULL;
    return ext;
  }


int main(void) {
   BIO* certbio;
   FILE* file;
   certbio = BIO_new(BIO_s_file());
   file = fopen("./out.pem", "r");
   BIO_set_fp(certbio, file, BIO_NOCLOSE);

   X509* cert;
   if(!(cert = PEM_read_bio_X509(certbio, NULL, NULL, NULL))) {
      std::cerr<<"PEM_read_bio_X509 failed"<<std::endl;
    }

    //if(!(d2i_X509_REQ_bio(reqbio, &req_))) {
    //  credentialLogger.msg(ERROR, "Can't convert X509_REQ struct from DER encoded to internal form");
    //  LogError(); return false;
    //}

   ArcLib::Credential::InitProxyCertInfo();

   STACK_OF(X509_EXTENSION)* extensions = NULL;
   X509_EXTENSION* ext;
   ArcLib::PROXYPOLICY*  policy = NULL;
   ASN1_OBJECT*  policy_lang = NULL;
   ASN1_OBJECT*  extension_oid = NULL;
   int certinfo_NID, certinfo_old_NID, certinfo_v3_NID, certinfo_v4_NID, nid;
   int i;

   ArcLib::PROXYCERTINFO * cert_info;
   //Get the PROXYCERTINFO from cert' extension
   certinfo_v3_NID = OBJ_sn2nid("PROXYCERTINFO_V3");
   certinfo_v4_NID = OBJ_sn2nid("PROXYCERTINFO_V4");

   int res = X509_get_ext_by_NID(cert, certinfo_v3_NID, -1);
   if (res == -1) X509_get_ext_by_NID(cert, certinfo_v4_NID, -1);

   if (res != -1) ext = X509_get_ext(cert,res);
   
   if (ext) cert_info = (ArcLib::PROXYCERTINFO*) X509V3_EXT_d2i(ext);

   //X509V3_EXT_METHOD*  ext_method = X509V3_EXT_get_nid(certinfo_v3_NID);
   //unsigned char* data = ext->value->data;
   //cert_info = (ArcLib::PROXYCERTINFO*)ext_method->d2i(NULL, (unsigned char **) &data, ext->value->length);

   if (cert_info == NULL) std::cerr<<"1. Can not convert DER encode PROXYCERTINFO extension to internal format"<<std::endl; 

   FILE* fp = fopen("./proxycertinfo1", "a");
   PROXYCERTINFO_print_fp(fp, cert_info);



   X509V3_EXT_METHOD*  ext_method1 = X509V3_EXT_get_nid(certinfo_v3_NID);
   int length = ext_method1->i2d(cert_info, NULL);
   std::cout<<"Length of proxy cert info: "<<length<<std::endl;
   unsigned char* data1 = NULL;
   data1 = (unsigned char*) malloc(length);

   unsigned char* derdata;
   derdata = data1;
   length = ext_method1->i2d(cert_info, &derdata);
   std::cout<<"Length of proxy cert info: "<<length<<" Data: "; for(int j =0; j< length; j++)std::cout<<data1[j];  std::cout<<std::endl;
 

   std::cout<<"Original cert info: ";
   for(int i = 0; i<length; i++) std::cout<<data1[i]; std::cout<<std::endl;
   std::string ext_data((char*)data1, length); free(data1);
   std::cout<<"Proxy cert info:" <<ext_data<<std::endl;
   std::string cert_sn = "PROXYCERTINFO_V3";
   X509_EXTENSION* ext2 = CreateExtension(cert_sn, ext_data, 1);
  

   //ASN1_OCTET_STRING* ext_data = ASN1_OCTET_STRING_new();
   //if(!ASN1_OCTET_STRING_set(ext_data, data1, length)) std::cerr<<"Error when set ext data"<<std::endl;
   //free(data1);
   //X509_EXTENSION* ext2 = X509_EXTENSION_create_by_NID(NULL, certinfo_v3_NID, 1, ext_data);
   //ASN1_OCTET_STRING_free(ext_data);


   ArcLib::PROXYCERTINFO * cert_info2;
   X509V3_EXT_METHOD*  ext_method2 = X509V3_EXT_get_nid(certinfo_v3_NID);
   unsigned char* data2 = ext2->value->data;
#if(OPENSSL_VERSION_NUMBER >= 0x0090800fL)
   cert_info2 = (ArcLib::PROXYCERTINFO*)ext_method2->d2i(NULL, (const unsigned char**) &data2, ext2->value->length);
#else 
   cert_info2 = (ArcLib::PROXYCERTINFO*)ext_method2->d2i(NULL, (unsigned char**) &data2, ext2->value->length);
#endif
   //cert_info2 = (ArcLib::PROXYCERTINFO*)X509V3_EXT_d2i(ext2);
   
   if (cert_info2 == NULL) std::cerr<<"2. Can not convert DER encode PROXYCERTINFO extension to internal format"<<std::endl;


   FILE* fp1 = fopen("./proxycertinfo3", "a");
   PROXYCERTINFO_print_fp(fp1, cert_info);


}

