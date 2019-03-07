#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <p12.h>
#include <pk11pub.h>
#include <keyhi.h>
#include <nspr.h>
#include <nss.h>
#include <ssl.h>
#include <prerror.h>
#include <base64.h>
#include <pkcs12.h>
#include <p12plcy.h>
#include <cryptohi.h>
#include <secerr.h>
#include <certdb.h>
#include <secder.h>
#include <certt.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <unistd.h>

#include <openssl/pem.h>
#include <openssl/err.h>

#include <arc/Logger.h>

#include "NSSUtil.h"
#include "nssprivkeyinfocodec.h"

/*********************************
 * Structures used in exporting the PKCS 12 blob
 *********************************/

/* A SafeInfo is used for each ContentInfo which makes up the
 * sequence of safes in the AuthenticatedSafe portion of the
 * PFX structure.
 */
struct SEC_PKCS12SafeInfoStr {
    PRArenaPool *arena;

    /* information for setting up password encryption */
    SECItem pwitem;
    SECOidTag algorithm;
    PK11SymKey *encryptionKey;

    /* how many items have been stored in this safe,
     * we will skip any safe which does not contain any
     * items
      */
    unsigned int itemCount;

    /* the content info for the safe */
    SEC_PKCS7ContentInfo *cinfo;

    sec_PKCS12SafeContents *safe;
};


/* An opaque structure which contains information needed for exporting
 * certificates and keys through PKCS 12.
 */
struct SEC_PKCS12ExportContextStr {
    PRArenaPool *arena;
    PK11SlotInfo *slot;
    void *wincx;

    /* integrity information */
    PRBool integrityEnabled;
    PRBool      pwdIntegrity;
    union {
        struct sec_PKCS12PasswordModeInfo pwdInfo;
        struct sec_PKCS12PublicKeyModeInfo pubkeyInfo;
    } integrityInfo;

    /* helper functions */
    /* retrieve the password call back */
    SECKEYGetPasswordKey pwfn;
    void *pwfnarg;

    /* safe contents bags */
    SEC_PKCS12SafeInfo **safeInfos;
    unsigned int safeInfoCount;

    /* the sequence of safes */
    sec_PKCS12AuthenticatedSafe authSafe;

    /* information needing deletion */
    CERTCertificate **certList;
};

typedef struct SEC_PKCS12SafeInfoStr SEC_PKCS12SafeInfo;
typedef struct SEC_PKCS12ExportContextStr SEC_PKCS12ExportContext;

using namespace Arc;
namespace ArcAuthNSS {

  //Logger& NSSUtilLogger = log();
  Arc::Logger NSSUtilLogger(Arc::Logger::rootLogger, "NSSUtil");

  std::string nss_error() {
    int len;
    char* text;
    std::string ret;
    if ((len = PR_GetErrorTextLength()) > 0) {
      text = (char*)malloc(len);
      if (PR_GetErrorText(text) > 0)
        ret.append("error string: ").append(text);
      free(text);
    } 
    else
      ret.append("unknown NSS error");
    return ret;
  }

#define NS_CERT_HEADER "-----BEGIN CERTIFICATE-----"
#define NS_CERT_TRAILER "-----END CERTIFICATE-----"

#define NS_CERTREQ_HEADER "-----BEGIN NEW CERTIFICATE REQUEST-----"
#define NS_CERTREQ_TRAILER "-----END NEW CERTIFICATE REQUEST-----"

  static SECStatus output_cert(const CERTCertificate* cert, PRBool ascii, PRFileDesc *outfile) {
    SECItem data;
    PRInt32 num;
    SECStatus rv = SECFailure;
    data.data = cert->derCert.data;
    data.len = cert->derCert.len;
    if (ascii) {
      PR_fprintf(outfile, "%s\n%s\n%s\n", NS_CERT_HEADER,
                BTOA_DataToAscii(data.data, data.len), NS_CERT_TRAILER);
      rv = SECSuccess;
    } 
    else { //raw data
      num = PR_Write(outfile, data.data, data.len);
      if (num != (PRInt32) data.len) {
        NSSUtilLogger.msg(ERROR, "Error writing raw certificate");
        rv = SECFailure;
      }
      rv = SECSuccess;
    }
    return rv;
  }


  static SECStatus p12u_SwapUnicodeBytes(SECItem *uniItem) {
    unsigned int i;
    unsigned char a;
    if((uniItem == NULL) || (uniItem->len % 2)) {
        return SECFailure;
    }
    for(i = 0; i < uniItem->len; i += 2) {
        a = uniItem->data[i];
        uniItem->data[i] = uniItem->data[i+1];
        uniItem->data[i+1] = a;
    }
    return SECSuccess;
  }

  static PRBool p12u_ucs2_ascii_conversion_function(PRBool toUnicode,
        unsigned char* inBuf, unsigned int inBufLen,
        unsigned char* outBuf, unsigned int maxOutBufLen,
        unsigned int* outBufLen, PRBool swapBytes) {
    SECItem it;
    SECItem *dup = NULL;
    PRBool ret;
    it.data = inBuf;
    it.len = inBufLen;
    dup = SECITEM_DupItem(&it);
    // If converting Unicode to ASCII, swap bytes before conversion as neccessary.
    if (!toUnicode && swapBytes) {
      if (p12u_SwapUnicodeBytes(dup) != SECSuccess) {
        SECITEM_ZfreeItem(dup, PR_TRUE);
        return PR_FALSE;
      }
    }
    // Perform the conversion.
    ret = PORT_UCS2_UTF8Conversion(toUnicode, dup->data, dup->len,
                                   outBuf, maxOutBufLen, outBufLen);
    if (dup)
      SECITEM_ZfreeItem(dup, PR_TRUE);
    return ret;
  }

//RFC 3820 and VOMS AC sequence 
#define OIDT static const unsigned char
  /* RFC 3820 Proxy OID. (1 3 6 1 5 5 7 1 14)*/
  OIDT proxy[] = { 0x2B, 6, 1, 5, 5, 7, 1, 14 };
  OIDT anyLanguage[] = { 0x2B, 6, 1, 5, 5, 7, 21, 0 };//(1.3 .6.1.5.5.7.21.0)
  OIDT inheritAll[] = { 0x2B, 6, 1, 5, 5, 7, 21, 1 }; //(1.3.6.1.5.5.7.21.1)
  OIDT Independent[] = { 0x2B, 6, 1, 5, 5, 7, 21, 2 }; //(1.3.6.1.5.5.7.21.1)
  /* VOMS AC sequence OID. ()*/
  OIDT VOMS_acseq[] = { 0x2B, 6, 1, 4, 1, 0xBE, 0x45, 100, 100, 5 }; //(1.3.6.1.4.1.8005.100.100.5)
  // according to BER "Basic Encoding Ruls", 8005 is encoded as 0xBE 0x45

#define OI(x) { siDEROID, (unsigned char *)x, sizeof x }
#define ODN(oid,desc) { OI(oid), (SECOidTag)0, desc, CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION }
  static const SECOidData oids[] = {
    ODN(proxy,		"RFC 3820 proxy extension"),
    ODN(anyLanguage, 	"Any language"),
    ODN(inheritAll, 	"Inherit all"),
    ODN(Independent, 	"Independent"),
    ODN(VOMS_acseq, 	"acseq"),
  };

  static const unsigned int numOids = (sizeof oids) / (sizeof oids[0]);
  SECOidTag tag_proxy, tag_anylang, tag_inheritall, tag_independent, tag_vomsacseq;
  SECStatus RegisterDynamicOids(void) {

    SECStatus rv = SECSuccess;
    tag_proxy = SECOID_AddEntry(&oids[0]);
    if (tag_proxy == SEC_OID_UNKNOWN) {
      rv = SECFailure;
      NSSUtilLogger.msg(ERROR, "Failed to add RFC proxy OID");
    }
    else {
      NSSUtilLogger.msg(DEBUG, "Succeeded to add RFC proxy OID, tag %d is returned", tag_proxy);
    }

    tag_anylang = SECOID_AddEntry(&oids[1]);
    if (tag_anylang == SEC_OID_UNKNOWN) {
      rv = SECFailure;
      NSSUtilLogger.msg(ERROR, "Failed to add anyLanguage OID");
    }
    else {
      NSSUtilLogger.msg(DEBUG, "Succeeded to add anyLanguage OID, tag %d is returned", tag_anylang);
    }

    tag_inheritall = SECOID_AddEntry(&oids[2]);
    if (tag_inheritall == SEC_OID_UNKNOWN) {
      rv = SECFailure;
      NSSUtilLogger.msg(ERROR, "Failed to add inheritAll OID");
    }
    else {
      NSSUtilLogger.msg(DEBUG, "Succeeded to add inheritAll OID, tag %d is returned", tag_inheritall);
    }

    tag_independent = SECOID_AddEntry(&oids[3]);
    if (tag_independent == SEC_OID_UNKNOWN) {
      rv = SECFailure;
      NSSUtilLogger.msg(ERROR, "Failed to add Independent OID");
    }
    else {
      NSSUtilLogger.msg(DEBUG, "Succeeded to add anyLanguage OID, tag %d is returned", tag_independent);
    }

    tag_vomsacseq = SECOID_AddEntry(&oids[4]);
    if (tag_vomsacseq == SEC_OID_UNKNOWN) {
      rv = SECFailure;
      NSSUtilLogger.msg(ERROR, "Failed to add VOMS AC sequence OID");
    }
    else {
      NSSUtilLogger.msg(DEBUG, "Succeeded to add VOMS AC sequence OID, tag %d is returned", tag_vomsacseq);
    }

    return rv;
  }

  static char* nss_obtain_password(PK11SlotInfo* slot, PRBool retry, void *arg) {
    PasswordSource* source = (PasswordSource*)arg;
    if(!source) return NULL;
    std::string password;
    PasswordSource::Result result = source->Get(password, -1, -1);
    if(result != PasswordSource::PASSWORD) return NULL;
    return PL_strdup(password.c_str());
  }
  
  bool nssInit(const std::string& configdir) {
    SECStatus rv;
    //Initialize NSPR
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 256);
    //Set the PKCS #11 strings for the internal token
    char* db_name = PL_strdup("internal (software)              ");
    PK11_ConfigurePKCS11(NULL,NULL,NULL, db_name, NULL, NULL,NULL,NULL,8,1);
    //Initialize NSS and open the certificate database read-only
    rv = NSS_Initialize(configdir.c_str(), NULL, NULL, "secmod.db", 0);// NSS_INIT_READONLY);
    if (rv != SECSuccess) {
      rv = NSS_NoDB_Init(configdir.c_str());
    }
    if (rv != SECSuccess) { // || NSS_InitTokens() != SECSuccess) {
      NSS_Shutdown();
      NSSUtilLogger.msg(ERROR, "NSS initialization failed on certificate database: %s", configdir.c_str());
      return false;
    }
/*
    if (NSS_SetDomesticPolicy() != SECSuccess ){
      NSS_Shutdown();
      NSSUtilLogger.msg(ERROR, "NSS set domestic policy failed (%s) on certificate database %s", nss_error().c_str(), configdir.c_str());
      return false;
    }
*/
    PK11_SetPasswordFunc(nss_obtain_password);
    NSSUtilLogger.msg(INFO, "Succeeded to initialize NSS");

    PORT_SetUCS2_ASCIIConversionFunction(p12u_ucs2_ascii_conversion_function);
 
    SEC_PKCS12EnableCipher(PKCS12_RC4_40, 1);
    SEC_PKCS12EnableCipher(PKCS12_RC4_128, 1);
    SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_40, 1);
    SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_128, 1);
    SEC_PKCS12EnableCipher(PKCS12_DES_56, 1);
    SEC_PKCS12EnableCipher(PKCS12_DES_EDE3_168, 1);
    SEC_PKCS12SetPreferredCipher(PKCS12_DES_EDE3_168, 1);

    RegisterDynamicOids();

    return true;
  }

  static bool ReadPrivKeyAttribute(SECKEYPrivateKey* key, CK_ATTRIBUTE_TYPE type, std::vector<uint8>* output) {
    SECItem item;
    SECStatus rv;
    rv = PK11_ReadRawAttribute(PK11_TypePrivKey, key, type, &item);
    if (rv != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to read attribute %x from private key.", type);
      return false;
    }
    output->assign(item.data, item.data + item.len);
    SECITEM_FreeItem(&item, PR_FALSE);
    return true;
  }

  static bool ExportPrivateKey(SECKEYPrivateKey* key, std::vector<uint8>* output) {
    PrivateKeyInfoCodec private_key_info(true);

    // Manually read the component attributes of the private key and build up
    // the PrivateKeyInfo.
    if (!ReadPrivKeyAttribute(key, CKA_MODULUS, private_key_info.modulus()) ||
      !ReadPrivKeyAttribute(key, CKA_PUBLIC_EXPONENT,
          private_key_info.public_exponent()) ||
      !ReadPrivKeyAttribute(key, CKA_PRIVATE_EXPONENT,
          private_key_info.private_exponent()) ||
      !ReadPrivKeyAttribute(key, CKA_PRIME_1, private_key_info.prime1()) ||
      !ReadPrivKeyAttribute(key, CKA_PRIME_2, private_key_info.prime2()) ||
      !ReadPrivKeyAttribute(key, CKA_EXPONENT_1, private_key_info.exponent1()) ||
      !ReadPrivKeyAttribute(key, CKA_EXPONENT_2, private_key_info.exponent2()) ||
      !ReadPrivKeyAttribute(key, CKA_COEFFICIENT, private_key_info.coefficient())) {
      return false;
    }

    return private_key_info.Export(output);
  }

  bool nssExportCertificate(const std::string& certname, const std::string& certfile) {
    CERTCertList* list;
    CERTCertificate* find_cert = NULL;
    CERTCertListNode* node;

    list = PK11_ListCerts(PK11CertListAll, NULL);
    for (node = CERT_LIST_HEAD(list); !CERT_LIST_END(node,list);
        node = CERT_LIST_NEXT(node)) {
      CERTCertificate* cert = node->cert;
      const char* nickname = (const char*)node->appData;
      if (!nickname) {
        nickname = cert->nickname;
      }
      if(nickname == NULL) continue;
      if (strcmp(certname.c_str(), nickname) == 0) {
        find_cert = CERT_DupCertificate(cert); break;
      }
    }
    if (list) {
      CERT_DestroyCertList(list);
    }

    if(find_cert)
      NSSUtilLogger.msg(INFO, "Succeeded to get credential");
    else { NSSUtilLogger.msg(ERROR, "Failed to get credential"); return false; }

    PRFileDesc* out = NULL;
    out = PR_Open(certfile.c_str(), PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE, 00660);
    output_cert(find_cert, true, out);
    PR_Close(out);

/*
    char* passwd  = "secretpw";
    if (find_cert->slot) {
      SECKEYPrivateKey* privKey = PK11_FindKeyByDERCert(find_cert->slot, find_cert, passwd);
      if (privKey) {
        PRBool isExtractable;
        SECItem value;
        SECStatus rv;
        rv = PK11_ReadRawAttribute(PK11_TypePrivKey, privKey, CKA_EXTRACTABLE, &value);
        if (rv != SECSuccess) {
          NSSUtilLogger.msg(ERROR, "Failed to read CKA_EXTRACTABLE attribute from private key.");
          return false;
        }
        if ((value.len == 1) && (value.data != NULL))
          isExtractable = !!(*(CK_BBOOL*)value.data);
        else
          rv = SECFailure;
        SECITEM_FreeItem(&value, PR_FALSE);
        if (rv == SECSuccess && !isExtractable) {
          NSSUtilLogger.msg(ERROR, "Private key is not extractable.");
          return false;
        }

        std::vector<uint8> output;
        if(!ExportPrivateKey(privKey, &output)) NSSUtilLogger.msg(ERROR, "Failed to export private key"); 

        NSSUtilLogger.msg(INFO, "Succeeded to get credential with name %s.", certname.c_str());
        SECKEYPrivateKey* key = SECKEY_CopyPrivateKey(privKey);
        SECKEY_DestroyPrivateKey(privKey);
        if(key)
          NSSUtilLogger.msg(INFO, "Succeeded to copy private key");
      }
      else
        NSSUtilLogger.msg(ERROR, "The private key is not accessible.");
    }
    else
      NSSUtilLogger.msg(INFO, "The certificate is without slot.");
*/
    return true;
  }


  typedef struct p12uContextStr {
    char        *filename;    /* name of file */
    PRFileDesc  *file;        /* pointer to file */
    PRBool       error;       /* error occurred? */
    SECItem     *data;
  } p12uContext;

  static void p12u_WriteToExportFile(void *arg, const char *buf, unsigned long len) {
    p12uContext* p12cxt = (p12uContext*)arg;
    int writeLen;

    if(!p12cxt || (p12cxt->error == PR_TRUE)) return;
    if(p12cxt->file == NULL) {
      NSSUtilLogger.msg(ERROR, "p12 file is empty");
      p12cxt->error = PR_TRUE;
      return;
    }
    writeLen = PR_Write(p12cxt->file, (unsigned char *)buf, (int32)len);
    if(writeLen != (int)len) {
      PR_Close(p12cxt->file);
      PR_Free(p12cxt->filename);
      p12cxt->filename = NULL;
      p12cxt->file = NULL;
      NSSUtilLogger.msg(ERROR, "Unable to write to p12 file");
      p12cxt->error = PR_TRUE;
    }
  }

  static PRBool p12u_OpenExportFile(p12uContext *p12cxt, PRBool fileRead) {
    if(!p12cxt || !p12cxt->filename) return PR_FALSE;
    if(fileRead) {
      p12cxt->file = PR_Open(p12cxt->filename, PR_RDONLY, 0400);
    } 
    else {
      p12cxt->file = PR_Open(p12cxt->filename, PR_CREATE_FILE | PR_RDWR | PR_TRUNCATE, 0600);
    }

    if(!p12cxt->file) {
      p12cxt->error = PR_TRUE;
      NSSUtilLogger.msg(ERROR, "Failed to open pk12 file");
      return PR_FALSE;
    }

    return PR_TRUE;
  }

  static void p12u_DestroyExportFileInfo(p12uContext **exp_ptr, PRBool removeFile) {
    if(!exp_ptr || !(*exp_ptr)) {
      return;
    }
    if((*exp_ptr)->file != NULL) {
      PR_Close((*exp_ptr)->file);
    }
    if((*exp_ptr)->filename != NULL) {
      if(removeFile) {
        PR_Delete((*exp_ptr)->filename);
      }
      PR_Free((*exp_ptr)->filename);
    }
    PR_Free(*exp_ptr);
    *exp_ptr = NULL;
  }

  static p12uContext * p12u_InitFile(PRBool fileImport, char *filename) {
    p12uContext *p12cxt;
    PRBool fileExist;

    if(fileImport)
      fileExist = PR_TRUE;
    else
      fileExist = PR_FALSE;

    p12cxt = (p12uContext *)PORT_ZAlloc(sizeof(p12uContext));
    if(!p12cxt) {
      NSSUtilLogger.msg(ERROR, "Failed to allocate p12 context");
      return NULL;
    }

    p12cxt->error = PR_FALSE;
    p12cxt->filename = strdup(filename);

    if(!p12u_OpenExportFile(p12cxt, fileImport)) {
      p12u_DestroyExportFileInfo(&p12cxt, PR_FALSE);
      return NULL;
    }

    return p12cxt;
  }


  static CERTCertificate* FindIssuerCert(CERTCertificate* cert) {
    CERTCertificate* issuercert = NULL;
    issuercert = CERT_FindCertByName(cert->dbhandle, &cert->derIssuer);
    return issuercert;
  }

  static CERTCertList* RevertChain(CERTCertList* chain) {
    CERTCertListNode* node;
    CERTCertListNode* newnd;
    CERTCertListNode* head;
    CERTCertList* certlist = NULL;
    certlist = CERT_NewCertList();

    for (node = CERT_LIST_HEAD(chain) ; !CERT_LIST_END(node, chain); node= CERT_LIST_NEXT(node)) {
      head = CERT_LIST_HEAD(certlist);
      newnd = (CERTCertListNode *)PORT_ArenaZAlloc(certlist->arena, sizeof(CERTCertListNode));
      if(newnd == NULL ) return certlist;
      PR_INSERT_BEFORE(&newnd->links, &head->links);
      newnd->cert = node->cert;
    }
    return certlist;
  }

  static CERTCertList* FindCertChain(CERTCertificate* cert) {
    CERTCertificate* issuercert = NULL;
    CERTCertList* certlist = NULL;
    certlist = CERT_NewCertList();
    //certlist = CERT_CertListFromCert(issuercert);
    //cert = CERT_DupCertificate(cert);
    CERT_AddCertToListTail(certlist, cert);
    do {
      issuercert = FindIssuerCert(cert);
      if(issuercert) {
        CERT_AddCertToListTail(certlist, issuercert);
        if(CERT_IsCACert(issuercert, NULL)) break;
        cert = issuercert;
      }
      else break;
    } while(true);

    return certlist;
  }


//The following code is copied from nss source code tree, the file
//mozilla/security/nss/lib/pkcs12/p12e.c, and file
//mozilla/security/nss/lib/pkcs12/p12local.c
//Because we need a new version of SEC_PKCS12AddKeyForCert which can 
//parse the cert chain of proxy 

/*************Start Here************/

/* Creates a new certificate bag and returns a pointer to it.  If an error
 * occurs NULL is returned.
 */
sec_PKCS12CertBag *
sec_PKCS12NewCertBag(PRArenaPool *arena, SECOidTag certType)
{
    sec_PKCS12CertBag *certBag = NULL;
    SECOidData *bagType = NULL;
    SECStatus rv;
    void *mark = NULL;

    if(!arena) {
        return NULL;
    }

    mark = PORT_ArenaMark(arena);
    certBag = (sec_PKCS12CertBag *)PORT_ArenaZAlloc(arena,
                                                    sizeof(sec_PKCS12CertBag));
    if(!certBag) {
        PORT_ArenaRelease(arena, mark);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    bagType = SECOID_FindOIDByTag(certType);
    if(!bagType) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    rv = SECITEM_CopyItem(arena, &certBag->bagID, &bagType->oid);
    if(rv != SECSuccess) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }
 
    PORT_ArenaUnmark(arena, mark);
    return certBag;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}

/* Creates a safeBag of the specified type, and if bagData is specified,
 * the contents are set.  The contents could be set later by the calling
 * routine.
 */
sec_PKCS12SafeBag *
sec_PKCS12CreateSafeBag(SEC_PKCS12ExportContext *p12ctxt, SECOidTag bagType,
                        void *bagData)
{
    sec_PKCS12SafeBag *safeBag;
    PRBool setName = PR_TRUE;
    void *mark = NULL;
    SECStatus rv = SECSuccess;
    SECOidData *oidData = NULL;

    if(!p12ctxt) {
        return NULL;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);
    if(!mark) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    safeBag = (sec_PKCS12SafeBag *)PORT_ArenaZAlloc(p12ctxt->arena,
                                                    sizeof(sec_PKCS12SafeBag));
    if(!safeBag) {
        PORT_ArenaRelease(p12ctxt->arena, mark);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    /* set the bags content based upon bag type */
    switch(bagType) {
        case SEC_OID_PKCS12_V1_KEY_BAG_ID:
            safeBag->safeBagContent.pkcs8KeyBag =
                (SECKEYPrivateKeyInfo *)bagData;
            break;
        case SEC_OID_PKCS12_V1_CERT_BAG_ID:
            safeBag->safeBagContent.certBag = (sec_PKCS12CertBag *)bagData;
            break;
        case SEC_OID_PKCS12_V1_CRL_BAG_ID:
            safeBag->safeBagContent.crlBag = (sec_PKCS12CRLBag *)bagData;
            break;
        case SEC_OID_PKCS12_V1_SECRET_BAG_ID:
            safeBag->safeBagContent.secretBag = (sec_PKCS12SecretBag *)bagData;
            break;
        case SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID:
            safeBag->safeBagContent.pkcs8ShroudedKeyBag =
                (SECKEYEncryptedPrivateKeyInfo *)bagData;
            break;
        case SEC_OID_PKCS12_V1_SAFE_CONTENTS_BAG_ID:
            safeBag->safeBagContent.safeContents =
                (sec_PKCS12SafeContents *)bagData;
            setName = PR_FALSE;
            break;
        default:
            goto loser;
    }

    oidData = SECOID_FindOIDByTag(bagType);
    if(oidData) {
        rv = SECITEM_CopyItem(p12ctxt->arena, &safeBag->safeBagType, &oidData->oid);
        if(rv != SECSuccess) {
            PORT_SetError(SEC_ERROR_NO_MEMORY);
            goto loser;
        }
    } else {
        goto loser;
    }
    safeBag->arena = p12ctxt->arena;
    PORT_ArenaUnmark(p12ctxt->arena, mark);

    return safeBag;

loser:
    if(mark) {
        PORT_ArenaRelease(p12ctxt->arena, mark);
    }

    return NULL;
}

/* this function converts a password to unicode and encures that the
 * required double 0 byte be placed at the end of the string
 */
PRBool
sec_pkcs12_convert_item_to_unicode(PRArenaPool *arena, SECItem *dest,
                                   SECItem *src, PRBool zeroTerm,
                                   PRBool asciiConvert, PRBool toUnicode)
{
    PRBool success = PR_FALSE;
    if(!src || !dest) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return PR_FALSE;
    }

    dest->len = src->len * 3 + 2;
    if(arena) {
        dest->data = (unsigned char*)PORT_ArenaZAlloc(arena, dest->len);
    } else {
        dest->data = (unsigned char*)PORT_ZAlloc(dest->len);
    }

    if(!dest->data) {
        dest->len = 0;
        return PR_FALSE;
    }

    if(!asciiConvert) {
        success = PORT_UCS2_UTF8Conversion(toUnicode, src->data, src->len, dest->data,
                                           dest->len, &dest->len);
    } else {
#ifndef IS_LITTLE_ENDIAN
        PRBool swapUnicode = PR_FALSE;
#else
        PRBool swapUnicode = PR_TRUE;
#endif
        success = PORT_UCS2_ASCIIConversion(toUnicode, src->data, src->len, dest->data,
                                            dest->len, &dest->len, swapUnicode);
    }

    if(!success) {
        if(!arena) {
            PORT_Free(dest->data);
            dest->data = NULL;
            dest->len = 0;
        }
        return PR_FALSE;
    }

    if((dest->data[dest->len-1] || dest->data[dest->len-2]) && zeroTerm) {
        if(dest->len + 2 > 3 * src->len) {
            if(arena) {
                dest->data = (unsigned char*)PORT_ArenaGrow(arena,
                                                     dest->data, dest->len,
                                                     dest->len + 2);
            } else {
                dest->data = (unsigned char*)PORT_Realloc(dest->data,
                                                          dest->len + 2);
            }

            if(!dest->data) {
                return PR_FALSE;
            }
        }
        dest->len += 2;
        dest->data[dest->len-1] = dest->data[dest->len-2] = 0;
    }

    return PR_TRUE;
}

/* sec_PKCS12AddAttributeToBag
 * adds an attribute to a safeBag.  currently, the only attributes supported
 * are those which are specified within PKCS 12.
 *
 *      p12ctxt - the export context
 *      safeBag - the safeBag to which attributes are appended
 *      attrType - the attribute type
 *      attrData - the attribute data
 */
SECStatus
sec_PKCS12AddAttributeToBag(SEC_PKCS12ExportContext *p12ctxt,
                            sec_PKCS12SafeBag *safeBag, SECOidTag attrType,
                            SECItem *attrData)
{
    sec_PKCS12Attribute *attribute;
    void *mark = NULL, *dummy = NULL;
    SECOidData *oiddata = NULL;
    SECItem unicodeName = { siBuffer, NULL, 0};
    void *src = NULL;
    unsigned int nItems = 0;
    SECStatus rv;

    if(!safeBag || !p12ctxt) {
        return SECFailure;
    }

    mark = PORT_ArenaMark(safeBag->arena);

    /* allocate the attribute */
    attribute = (sec_PKCS12Attribute *)PORT_ArenaZAlloc(safeBag->arena,
                                                sizeof(sec_PKCS12Attribute));
    if(!attribute) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }
    /* set up the attribute */
    oiddata = SECOID_FindOIDByTag(attrType);
    if(!oiddata) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }
    if(SECITEM_CopyItem(p12ctxt->arena, &attribute->attrType, &oiddata->oid) !=
                SECSuccess) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    nItems = 1;
    switch(attrType) {
        case SEC_OID_PKCS9_LOCAL_KEY_ID:
            {
                src = attrData;
                break;
            }
        case SEC_OID_PKCS9_FRIENDLY_NAME:
            {
                if(!sec_pkcs12_convert_item_to_unicode(p12ctxt->arena,
                                        &unicodeName, attrData, PR_FALSE,
                                        PR_FALSE, PR_TRUE)) {
                    goto loser;
                }
                src = &unicodeName;
                break;
            }
        default:
            goto loser;
    }
    /* append the attribute to the attribute value list  */
    attribute->attrValue = (SECItem **)PORT_ArenaZAlloc(p12ctxt->arena,
                                            ((nItems + 1) * sizeof(SECItem *)));
    if(!attribute->attrValue) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    /* XXX this will need to be changed if attributes requiring more than
     * one element are ever used.
     */
    attribute->attrValue[0] = (SECItem *)PORT_ArenaZAlloc(p12ctxt->arena,
                                                          sizeof(SECItem));
    if(!attribute->attrValue[0]) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }
    attribute->attrValue[1] = NULL;

    rv = SECITEM_CopyItem(p12ctxt->arena, attribute->attrValue[0],
                          (SECItem*)src);
    if(rv != SECSuccess) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    /* append the attribute to the safeBag attributes */
    if(safeBag->nAttribs) {
        dummy = PORT_ArenaGrow(p12ctxt->arena, safeBag->attribs,
                        ((safeBag->nAttribs + 1) * sizeof(sec_PKCS12Attribute *)),
                        ((safeBag->nAttribs + 2) * sizeof(sec_PKCS12Attribute *)));
        safeBag->attribs = (sec_PKCS12Attribute **)dummy;
    } else {
        safeBag->attribs = (sec_PKCS12Attribute **)PORT_ArenaZAlloc(p12ctxt->arena,
                                                2 * sizeof(sec_PKCS12Attribute *));
        dummy = safeBag->attribs;
    }
    if(!dummy) {
        goto loser;
    }

    safeBag->attribs[safeBag->nAttribs] = attribute;
    safeBag->attribs[++safeBag->nAttribs] = NULL;

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    if(mark) {
        PORT_ArenaRelease(p12ctxt->arena, mark);
    }

    return SECFailure;
}

/*********************************
 * Routines to handle the exporting of the keys and certificates
 *********************************/

/* creates a safe contents which safeBags will be appended to */
sec_PKCS12SafeContents *
sec_PKCS12CreateSafeContents(PRArenaPool *arena)
{
    sec_PKCS12SafeContents *safeContents;

    if(arena == NULL) {
        return NULL;
    }

    /* create the safe contents */
    safeContents = (sec_PKCS12SafeContents *)PORT_ArenaZAlloc(arena,
                                            sizeof(sec_PKCS12SafeContents));
    if(!safeContents) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    /* set up the internal contents info */
    safeContents->safeBags = NULL;
    safeContents->arena = arena;
    safeContents->bagCount = 0;

    return safeContents;

loser:
    return NULL;
}  

/* appends a safe bag to a safeContents using the specified arena.
 */
SECStatus
sec_pkcs12_append_bag_to_safe_contents(PRArenaPool *arena,
                                       sec_PKCS12SafeContents *safeContents,
                                       sec_PKCS12SafeBag *safeBag)
{
    void *mark = NULL, *dummy = NULL;

    if(!arena || !safeBag || !safeContents) {
        return SECFailure;
    }

    mark = PORT_ArenaMark(arena);
    if(!mark) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }

    /* allocate space for the list, or reallocate to increase space */
    if(!safeContents->safeBags) {
        safeContents->safeBags = (sec_PKCS12SafeBag **)PORT_ArenaZAlloc(arena,
                                                (2 * sizeof(sec_PKCS12SafeBag *)));
        dummy = safeContents->safeBags;
        safeContents->bagCount = 0;
    } else {
        dummy = PORT_ArenaGrow(arena, safeContents->safeBags,
                        (safeContents->bagCount + 1) * sizeof(sec_PKCS12SafeBag *),
                        (safeContents->bagCount + 2) * sizeof(sec_PKCS12SafeBag *));
        safeContents->safeBags = (sec_PKCS12SafeBag **)dummy;
    }

    if(!dummy) {
        PORT_ArenaRelease(arena, mark);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }

    /* append the bag at the end and null terminate the list */
    safeContents->safeBags[safeContents->bagCount++] = safeBag;
    safeContents->safeBags[safeContents->bagCount] = NULL;

    PORT_ArenaUnmark(arena, mark);

    return SECSuccess;
}

/* appends a safeBag to a specific safeInfo.
 */
SECStatus
sec_pkcs12_append_bag(SEC_PKCS12ExportContext *p12ctxt,
                      SEC_PKCS12SafeInfo *safeInfo, sec_PKCS12SafeBag *safeBag)
{
    sec_PKCS12SafeContents *dest;
    SECStatus rv = SECFailure;

    if(!p12ctxt || !safeBag || !safeInfo) {
        return SECFailure;
    }

    if(!safeInfo->safe) {
        safeInfo->safe = sec_PKCS12CreateSafeContents(p12ctxt->arena);
        if(!safeInfo->safe) {
            return SECFailure;
        }
    }

    dest = safeInfo->safe;
    rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena, dest, safeBag);
    if(rv == SECSuccess) {
        safeInfo->itemCount++;
    }

    return rv;
}

/* compute the thumbprint of the DER cert and create a digest info
 * to store it in and return the digest info.
 * a return of NULL indicates an error.
 */
SGNDigestInfo *
sec_pkcs12_compute_thumbprint(SECItem *der_cert)
{
    SGNDigestInfo *thumb = NULL;
    SECItem digest;
    PRArenaPool *temparena = NULL;
    SECStatus rv = SECFailure;

    if(der_cert == NULL)
        return NULL;

    temparena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(temparena == NULL) {
        return NULL;
    }

    digest.data = (unsigned char *)PORT_ArenaZAlloc(temparena,
                                                    sizeof(unsigned char) *
                                                    SHA1_LENGTH);
    /* digest data and create digest info */
    if(digest.data != NULL) {
        digest.len = SHA1_LENGTH;
        rv = PK11_HashBuf(SEC_OID_SHA1, digest.data, der_cert->data,
                          der_cert->len);
        if(rv == SECSuccess) {
            thumb = SGN_CreateDigestInfo(SEC_OID_SHA1,
                                         digest.data,
                                         digest.len);
        } else {
            PORT_SetError(SEC_ERROR_NO_MEMORY);
        }
    } else {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena(temparena, PR_TRUE);

    return thumb;
}

/* SEC_PKCS12AddKeyForCert
 *      Extracts the key associated with a particular certificate and exports
 *      it.
 *
 *      p12ctxt - the export context
 *      safe - the safeInfo to place the key in
 *      nestedDest - the nested safeContents to place a key
 *      cert - the certificate which the key belongs to
 *      shroudKey - encrypt the private key for export.  This value should
 *              always be true.  lower level code will not allow the export
 *              of unencrypted private keys.
 *      algorithm - the algorithm with which to encrypt the private key
 *      pwitem - the password to encrypt the private key with
 *      keyId - the keyID attribute
 *      nickName - the nickname attribute
 */
SECStatus
my_SEC_PKCS12AddKeyForCert(SEC_PKCS12ExportContext *p12ctxt, SEC_PKCS12SafeInfo *safe,
                        void *nestedDest, CERTCertificate *cert,
                        PRBool shroudKey, SECOidTag algorithm, SECItem *pwitem,
                        SECItem *keyId, SECItem *nickName)
{
    void *mark;
    void *keyItem;
    SECOidTag keyType;
    SECStatus rv = SECFailure;
    SECItem nickname = {siBuffer,NULL,0}, uniPwitem = {siBuffer, NULL, 0};
    sec_PKCS12SafeBag *returnBag;

    if(!p12ctxt || !cert || !safe) {
        return SECFailure;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);
    /* retrieve the key based upon the type that it is and
     * specify the type of safeBag to store the key in
     */
    if(!shroudKey) {

        /* extract the key unencrypted.  this will most likely go away */
        SECKEYPrivateKeyInfo *pki = PK11_ExportPrivateKeyInfo(cert,
                                                              p12ctxt->wincx);
        if(!pki) {
            PORT_ArenaRelease(p12ctxt->arena, mark);
            PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
            return SECFailure;
        }
        keyItem = PORT_ArenaZAlloc(p12ctxt->arena, sizeof(SECKEYPrivateKeyInfo));
        if(!keyItem) {
            PORT_SetError(SEC_ERROR_NO_MEMORY);
            goto loser;
        }
        rv = SECKEY_CopyPrivateKeyInfo(p12ctxt->arena,
                                       (SECKEYPrivateKeyInfo *)keyItem, pki);
        keyType = SEC_OID_PKCS12_V1_KEY_BAG_ID;
        SECKEY_DestroyPrivateKeyInfo(pki, PR_TRUE);
    } else {

        /* extract the key encrypted */
        SECKEYEncryptedPrivateKeyInfo *epki = NULL;
        PK11SlotInfo *slot = NULL;

        if(!sec_pkcs12_convert_item_to_unicode(p12ctxt->arena, &uniPwitem,
                                 pwitem, PR_TRUE, PR_TRUE, PR_TRUE)) {
            PORT_SetError(SEC_ERROR_NO_MEMORY);
            goto loser;
        }
        /* we want to make sure to take the key out of the key slot */
        if(PK11_IsInternal(p12ctxt->slot)) {
            slot = PK11_GetInternalKeySlot();
        } else {
            slot = PK11_ReferenceSlot(p12ctxt->slot);
        }

        epki = PK11_ExportEncryptedPrivateKeyInfo(slot, algorithm,
                                                  &uniPwitem, cert, 1,
                                                  p12ctxt->wincx);
        PK11_FreeSlot(slot);
        if(!epki) {
            PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
            goto loser;
        }

        keyItem = PORT_ArenaZAlloc(p12ctxt->arena,
                                  sizeof(SECKEYEncryptedPrivateKeyInfo));
        if(!keyItem) {
            PORT_SetError(SEC_ERROR_NO_MEMORY);
            goto loser;
        }
        rv = SECKEY_CopyEncryptedPrivateKeyInfo(p12ctxt->arena,
                                        (SECKEYEncryptedPrivateKeyInfo *)keyItem,
                                        epki);
        keyType = SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID;
        SECKEY_DestroyEncryptedPrivateKeyInfo(epki, PR_TRUE);
    }

    if(rv != SECSuccess) {
        goto loser;
    }
    /* if no nickname specified, let's see if the certificate has a
     * nickname.
     */
    if(!nickName) {
        if(cert->nickname) {
            nickname.data = (unsigned char *)cert->nickname;
            nickname.len = PORT_Strlen(cert->nickname);
            nickName = &nickname;
        }
    }

    /* create the safe bag and set any attributes */
    returnBag = sec_PKCS12CreateSafeBag(p12ctxt, keyType, keyItem);
    if(!returnBag) {
        rv = SECFailure;
        goto loser;
    }

    if(nickName) {
        if(sec_PKCS12AddAttributeToBag(p12ctxt, returnBag,
                                       SEC_OID_PKCS9_FRIENDLY_NAME, nickName)
                                       != SECSuccess) {
            goto loser;
        }
    }

    if(keyId) {
        if(sec_PKCS12AddAttributeToBag(p12ctxt, returnBag, SEC_OID_PKCS9_LOCAL_KEY_ID,
                                       keyId) != SECSuccess) {
            goto loser;
        }
    }

    if(nestedDest) {
        rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena,
                                          (sec_PKCS12SafeContents*)nestedDest,
                                          returnBag);
    } else {
        rv = sec_pkcs12_append_bag(p12ctxt, safe, returnBag);
    }

loser:

    if (rv != SECSuccess) {
        PORT_ArenaRelease(p12ctxt->arena, mark);
    } else {
        PORT_ArenaUnmark(p12ctxt->arena, mark);
    }

    return rv;
}

/*************End Here************/

  //Our version of SEC_PKCS12AddCert
  static SECStatus my_SEC_PKCS12AddCert(SEC_PKCS12ExportContext* p12ctxt, SEC_PKCS12SafeInfo* safe,
                  void* nestedDest, CERTCertificate* cert,
                  CERTCertDBHandle* certDb, SECItem* keyId) {
    sec_PKCS12CertBag *certBag;
    sec_PKCS12SafeBag *safeBag;
    void *mark = NULL;
    CERTCertListNode* node;
    CERTCertList* certlist_tmp = NULL;
    CERTCertList* certlist = NULL;
    SECStatus rv;
    SECItem nick;

    if(!p12ctxt) return SECFailure;

    certlist_tmp = FindCertChain(cert);
    if(!certlist_tmp) {
      NSSUtilLogger.msg(ERROR, "Failed to find issuer certificate for proxy certificate"); goto err;
    }
    certlist = RevertChain(certlist_tmp);
/*
    for (i=chain->len-1; i>=0; i--) {
        CERTCertificate *c;
        c = CERT_FindCertByDERCert(handle, &chain->certs[i]);
        for (j=i; j<chain->len-1; j++) printf("  ");
        printf("\"%s\" [%s]\n\n", c->nickname, c->subjectName);
        CERT_DestroyCertificate(c);
    }
*/
    for (node = CERT_LIST_HEAD(certlist); !CERT_LIST_END(node,certlist); node= CERT_LIST_NEXT(node)) {
      CERTCertificate* cert = node->cert;
      nick.type = siBuffer;
      nick.data = NULL;
      nick.len  = 0;
      if(!cert) return SECFailure;
      mark = PORT_ArenaMark(p12ctxt->arena);
      certBag = sec_PKCS12NewCertBag(p12ctxt->arena, SEC_OID_PKCS9_X509_CERT);
      if(!certBag) goto err;

      if(SECITEM_CopyItem(p12ctxt->arena, &certBag->value.x509Cert, &cert->derCert) != SECSuccess) {
        goto err;
      }

      //If the certificate has a nickname, set the friendly name to that.
      if(cert->nickname) {
        nick.data = (unsigned char *)cert->nickname;
        nick.len = PORT_Strlen(cert->nickname);
      }
      safeBag = sec_PKCS12CreateSafeBag(p12ctxt, SEC_OID_PKCS12_V1_CERT_BAG_ID,
                                      certBag);
      if(!safeBag) goto err;

      // Add the friendly name and keyId attributes, if necessary
      if(nick.data) {
        if(sec_PKCS12AddAttributeToBag(p12ctxt, safeBag, SEC_OID_PKCS9_FRIENDLY_NAME, &nick) != SECSuccess) {
          goto err;
        }
      }
      if(keyId) {
        if(sec_PKCS12AddAttributeToBag(p12ctxt, safeBag, SEC_OID_PKCS9_LOCAL_KEY_ID, keyId) != SECSuccess) {
          goto err;
        }
      }

      // Append the cert safeBag 
      if(nestedDest) {
        rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena,
             (sec_PKCS12SafeContents*)nestedDest, safeBag);
      }
      else {
        rv = sec_pkcs12_append_bag(p12ctxt, safe, safeBag);
      }
      if(rv != SECSuccess) goto err;
      PORT_ArenaUnmark(p12ctxt->arena, mark);
    }

    return SECSuccess;
err:
    if(mark) PORT_ArenaRelease(p12ctxt->arena, mark);
    if(certlist_tmp) CERT_DestroyCertList(certlist_tmp);
    if(certlist) CERT_DestroyCertList(certlist);
    return SECFailure;
  }

  //Our version of SEC_PKCS12AddCertAndKey
  SECStatus my_SEC_PKCS12AddCertAndKey(SEC_PKCS12ExportContext *p12ctxt,
                               void *certSafe, void *certNestedDest,
                               CERTCertificate *cert, CERTCertDBHandle *certDb,
                               void *keySafe, void *keyNestedDest,
                               PRBool shroudKey, SECItem *pwitem,
                               SECOidTag algorithm) {
    SECStatus rv = SECFailure;
    SGNDigestInfo *digest = NULL;
    void *mark = NULL;

    if(!p12ctxt || !certSafe || !keySafe || !cert) {
        return SECFailure;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);

    //Generate the thumbprint of the cert to use as a keyId
    digest = sec_pkcs12_compute_thumbprint(&cert->derCert);
    if(!digest) {
      PORT_ArenaRelease(p12ctxt->arena, mark);
      return SECFailure;
    }

    // add the certificate 
    rv = my_SEC_PKCS12AddCert(p12ctxt, (SEC_PKCS12SafeInfo*)certSafe,
                           (SEC_PKCS12SafeInfo*)certNestedDest, cert, certDb,
                           &digest->digest);
    if(rv != SECSuccess) {
        goto loser;
    }

    /* add the key */
    rv = my_SEC_PKCS12AddKeyForCert(p12ctxt, (SEC_PKCS12SafeInfo*)keySafe,
                                 keyNestedDest, cert,
                                 shroudKey, algorithm, pwitem,
                                 &digest->digest, NULL );
    if(rv != SECSuccess) {
        goto loser;
    }

    SGN_DestroyDigestInfo(digest);

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    SGN_DestroyDigestInfo(digest);
    PORT_ArenaRelease(p12ctxt->arena, mark);

    return SECFailure;
  }


  bool nssOutputPKCS12(const std::string certname, char* outfile, char* slotpw, char* p12pw) {
    PasswordSource* passphrase = NULL;
    if(slotpw) {
      passphrase = new PasswordSourceString(slotpw);
    } else {
      passphrase = new PasswordSourceInteractive("TODO: prompt here",false);
    }
    PasswordSource* p12passphrase = NULL;
    if(p12pw) {
      p12passphrase = new PasswordSourceString(p12pw);
    } else {
      p12passphrase = new PasswordSourceNone();
    }
    bool r = nssOutputPKCS12(certname, outfile, *passphrase, *p12passphrase);
    delete passphrase;
    delete p12passphrase;
    return r;
  }

  bool nssOutputPKCS12(const std::string certname, char* outfile, PasswordSource& passphrase, PasswordSource& p12passphrase) {
    SEC_PKCS12ExportContext *p12ecx = NULL;
    SEC_PKCS12SafeInfo *keySafe = NULL, *certSafe = NULL;
    SECItem *pwitem = NULL;
    PK11SlotInfo *slot = NULL;
    p12uContext *p12cxt = NULL;
    CERTCertList* certlist = NULL;
    CERTCertListNode* node = NULL;

    slot = PK11_GetInternalKeySlot();
    if (PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to authenticate to PKCS11 slot %s", PK11_GetSlotName(slot));
      goto err;
    }

    certlist = PK11_FindCertsFromNickname((char*)(certname.c_str()), (void*)&passphrase);
    if(!certlist) {
      NSSUtilLogger.msg(ERROR, "Failed to find certificates by nickname: %s", certname.c_str());
      return false;
    }

    if((SECSuccess != CERT_FilterCertListForUserCerts(certlist)) || CERT_LIST_EMPTY(certlist)) {
      NSSUtilLogger.msg(ERROR, "No user certificate by nickname %s found", certname.c_str());
      return false;
    }

    if(certlist) {
      CERTCertificate* cert = NULL;
      node = CERT_LIST_HEAD(certlist);
      if(node) cert = node->cert;
      if(cert) slot = cert->slot; 
      //use the slot from the first matching
      //certificate to create the context. This is for keygen
    }
    if(!slot) {
      NSSUtilLogger.msg(ERROR, "Certificate does not have a slot");
      goto err;
    }

    p12ecx = SEC_PKCS12CreateExportContext(NULL, NULL, slot, (void*)&passphrase);
    if(!p12ecx) {
      NSSUtilLogger.msg(ERROR, "Failed to create export context");
      goto err;
    }

    //Password for the output PKCS12 file.
    {
      std::string p12pw;
      PasswordSource::Result p12res = p12passphrase.Get(p12pw,-1,-1);
      if(p12res == PasswordSource::PASSWORD) {
        pwitem = SECITEM_AllocItem(NULL, NULL, p12pw.length() + 1);
        if(pwitem) {
          memset(pwitem->data, 0, pwitem->len); // ??
          memcpy(pwitem->data, p12pw.c_str(), pwitem->len);
        }
      } else if(p12res == PasswordSource::CANCEL) {
        NSSUtilLogger.msg(ERROR, "PKCS12 output password not provided");
        goto err;
      }
    }

    if(pwitem != NULL) {
      if(SEC_PKCS12AddPasswordIntegrity(p12ecx, pwitem, SEC_OID_SHA1) != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "PKCS12 add password integrity failed");
        goto err;
      }
    }

    for(node = CERT_LIST_HEAD(certlist); !CERT_LIST_END(node,certlist); node = CERT_LIST_NEXT(node)) {
      CERTCertificate* cert = node->cert;
      if(!cert->slot) {
        NSSUtilLogger.msg(ERROR, "Certificate does not have a slot");
        goto err;
      }

      keySafe = SEC_PKCS12CreateUnencryptedSafe(p12ecx);
      if(!SEC_PKCS12IsEncryptionAllowed() || PK11_IsFIPS() || !pwitem) {
        certSafe = keySafe;
      } else {
        certSafe = SEC_PKCS12CreatePasswordPrivSafe(p12ecx, pwitem,
            SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC);
      }

      if(!certSafe || !keySafe) {
        NSSUtilLogger.msg(ERROR, "Failed to create key or certificate safe");
        goto err;
      }

      //issuercert = FindIssuerCert(cert);
      /*
      if(my_SEC_PKCS12AddCert(p12ecx, certSafe, NULL, cert,
                CERT_GetDefaultCertDB(), NULL) != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "Failed to add cert and key");
        goto err;
      }
      */
      if(my_SEC_PKCS12AddCertAndKey(p12ecx, certSafe, NULL, cert,
                CERT_GetDefaultCertDB(), keySafe, NULL, !pwitem ? PR_FALSE : PR_TRUE, pwitem,
                SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC)
                != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "Failed to add certificate and key");
        goto err;
      }
    }
    CERT_DestroyCertList(certlist);
    certlist = NULL;

    p12cxt = p12u_InitFile(PR_FALSE, outfile);
    if(!p12cxt) {
      NSSUtilLogger.msg(ERROR, "Failed to initialize PKCS12 file: %s", outfile);
      goto err;
    }
    if(SEC_PKCS12Encode(p12ecx, p12u_WriteToExportFile, p12cxt)
               != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to encode PKCS12");
      goto err;
    }
    NSSUtilLogger.msg(INFO, "Succeeded to export PKCS12");

    p12u_DestroyExportFileInfo(&p12cxt, PR_FALSE);
    if(pwitem) SECITEM_ZfreeItem(pwitem, PR_TRUE);
    SEC_PKCS12DestroyExportContext(p12ecx);
    return true;

err:
    SEC_PKCS12DestroyExportContext(p12ecx);
    if (certlist) {
        CERT_DestroyCertList(certlist);
        certlist = NULL;
    }   
    p12u_DestroyExportFileInfo(&p12cxt, PR_TRUE);
    if(pwitem) {
      SECITEM_ZfreeItem(pwitem, PR_TRUE);
    }
    return false;
  }

  static SECStatus DeleteCertOnly(const char* certname) {
    SECStatus rv;
    CERTCertificate *cert;
    CERTCertDBHandle* handle;

    handle = CERT_GetDefaultCertDB();
    cert = CERT_FindCertByNicknameOrEmailAddr(handle, (char*)certname);
    if(!cert) {
      NSSUtilLogger.msg(INFO, "There is no certificate named %s found, the certificate could be removed when generating CSR", certname);
      return SECSuccess;
    }
    rv = SEC_DeletePermCertificate(cert);
    CERT_DestroyCertificate(cert);
    if(rv) {
      NSSUtilLogger.msg(ERROR, "Failed to delete certificate");
    }
    return rv;
  }

  static SECStatus deleteKeyAndCert(const char* privkeyname, PasswordSource& passphrase, bool delete_cert) {
    SECKEYPrivateKeyList* list;
    SECKEYPrivateKeyListNode* node;
    int count = 0;
    SECStatus rv;
    PK11SlotInfo* slot;

    slot = PK11_GetInternalKeySlot();

    if(!privkeyname) NSSUtilLogger.msg(WARNING, "The name of the private key to delete is empty");

    if(PK11_NeedLogin(slot)) {
      rv = PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase);
      if(rv != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "Failed to authenticate to token %s.", PK11_GetTokenName(slot));
        return SECFailure;
      }
    }
 
    list = PK11_ListPrivKeysInSlot(slot, (char *)privkeyname, (void*)&passphrase);
    if(list == NULL) {
      NSSUtilLogger.msg(INFO, "No private key with nickname %s exist in NSS database", privkeyname);
      return SECFailure;
    }

    for(node=PRIVKEY_LIST_HEAD(list); !PRIVKEY_LIST_END(node,list); node=PRIVKEY_LIST_NEXT(node)) {
      char * keyname;
      static const char orphan[] = { "(orphan)" };
      CERTCertificate* cert;
      keyname = PK11_GetPrivateKeyNickname(node->key);
      if(!keyname || !keyname[0]) {
        PORT_Free((void *)keyname);
        keyname = NULL;
        cert = PK11_GetCertFromPrivateKey(node->key);
        if(cert) {
          if(cert->nickname && cert->nickname[0]) {
            keyname = PORT_Strdup(cert->nickname);
          } 
          else if(cert->emailAddr && cert->emailAddr[0]) {
            keyname = PORT_Strdup(cert->emailAddr);
          }
          CERT_DestroyCertificate(cert);
        }
      }
      if(!keyname || PL_strcmp(keyname, privkeyname)) {
        /* PKCS#11 module returned unwanted keys */
        PORT_Free((void *)keyname);
        continue;
      }
      cert = PK11_GetCertFromPrivateKey(node->key);
      if(cert && delete_cert){
        //Delete the private key and the cert related
        rv = PK11_DeleteTokenCertAndKey(cert, (void*)&passphrase);
        if(rv != SECSuccess) {
          NSSUtilLogger.msg(ERROR, "Failed to delete private key and certificate");
          CERT_DestroyCertificate(cert); continue;
        }
        CERT_DestroyCertificate(cert);
      }
      else{
        //Delete the private key without deleting the cert related
        //rv = PK11_DeleteTokenPrivateKey(node->key, PR_FALSE);
        rv = PK11_DestroyTokenObject(node->key->pkcs11Slot, node->key->pkcs11ID);
        if(rv != SECSuccess) {
          NSSUtilLogger.msg(ERROR, "Failed to delete private key");
          continue;
        }
      }
      if(!keyname) keyname = (char *)orphan;
      if(keyname != (char *)orphan) PORT_Free((void *)keyname);
      count++;
    }
    SECKEY_DestroyPrivateKeyList(list);

    if(count == 0) {
      NSSUtilLogger.msg(WARNING, "Can not find key with name: %s", privkeyname);
    }
    if(slot) PK11_FreeSlot(slot);
    return SECSuccess;
  }

  static SECStatus DeleteKeyOnly(const char* privkeyname, PasswordSource& passphrase) {
    return deleteKeyAndCert(privkeyname, passphrase, false);
  }

  static SECStatus DeleteKeyAndCert(const char* privkeyname, PasswordSource& passphrase) {
    return deleteKeyAndCert(privkeyname, passphrase, true);
  }

  static SECStatus DeleteCertAndKey(const char* certname, PasswordSource& passphrase) {
    SECStatus rv;
    CERTCertificate* cert;
    PK11SlotInfo* slot;

    slot = PK11_GetInternalKeySlot();
    if(PK11_NeedLogin(slot)) {
      SECStatus rv = PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase);
      if(rv != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "Failed to authenticate to token %s.", PK11_GetTokenName(slot));
        return SECFailure;
      }
    }
    cert = PK11_FindCertFromNickname((char*)certname, (void*)&passphrase);
    if(!cert) {
      PK11_FreeSlot(slot);
      return SECFailure;
    }
    rv = PK11_DeleteTokenCertAndKey(cert, (void*)&passphrase);
    if(rv != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to delete private key that attaches to certificate: %s", certname);
    }
    CERT_DestroyCertificate(cert);
    PK11_FreeSlot(slot);
    return rv;
  }

  static bool InputPrivateKey(std::vector<uint8>& output, const std::string& privk_in) {
    EVP_PKEY* pkey=NULL;

    BIO* in = NULL;
    in = BIO_new(BIO_s_mem());
    BIO_write(in, privk_in.c_str(), privk_in.length());

    //PW_CB_DATA cb_data;
    //cb_data.password = (passphrase.empty()) ? NULL : (void*)(passphrase.c_str());
    //cb_data.prompt_info = prompt_info.empty() ? NULL : prompt_info.c_str();
    //if(!(pkey = PEM_read_bio_PrivateKey(keybio, NULL, passwordcb, &cb_data))) {
    if(!(pkey = PEM_read_bio_PrivateKey(in, NULL, NULL, NULL))) {
      int reason = ERR_GET_REASON(ERR_peek_error());
      if(reason == PEM_R_BAD_BASE64_DECODE)
        NSSUtilLogger.msg(ERROR, "Can not read PEM private key: probably bad password");
      if(reason == PEM_R_BAD_DECRYPT)
        NSSUtilLogger.msg(ERROR, "Can not read PEM private key: failed to decrypt");
      if(reason == PEM_R_BAD_PASSWORD_READ)
        NSSUtilLogger.msg(ERROR, "Can not read PEM private key: failed to obtain password");
      if(reason == PEM_R_PROBLEMS_GETTING_PASSWORD)
        NSSUtilLogger.msg(ERROR,"Can not read PEM private key: failed to obtain password");
      NSSUtilLogger.msg(ERROR, "Can not read PEM private key");
    }
    BIO_free(in);

    PKCS8_PRIV_KEY_INFO *p8inf = NULL;
    if (pkey) {
      if (!(p8inf = EVP_PKEY2PKCS8(pkey))) {
        NSSUtilLogger.msg(ERROR, "Failed to convert EVP_PKEY to PKCS8");
        EVP_PKEY_free(pkey);
        return false;
      }
      BIO* key = NULL; 
      key = BIO_new(BIO_s_mem());
      i2d_PKCS8_PRIV_KEY_INFO_bio(key, p8inf);
      std::string privk_out;
      for(;;) {
        char s[256];
        int l = BIO_read(key,s,sizeof(s));
        if(l <= 0) break;
        privk_out.append(s,l);
      }
      std::string::iterator it;
      for(it = privk_out.begin() ; it < privk_out.end(); it++) output.push_back(*it);
      BIO_free(key);
    }

    EVP_PKEY_free(pkey);
    PKCS8_PRIV_KEY_INFO_free(p8inf);
    return true;
  }

  static bool OutputPrivateKey(const std::vector<uint8>& input, std::string& privk_out) {
    std::stringstream strstream;
    std::vector<uint8>::const_iterator it;
    for(it = input.begin(); it != input.end(); it++) strstream<<(*it);

    BIO* key= NULL;
    PKCS8_PRIV_KEY_INFO* p8info = NULL;
    EVP_PKEY* pkey=NULL;
    key = BIO_new(BIO_s_mem());
    std::string str = strstream.str();
    BIO_write(key, str.c_str(), str.length());

    p8info = d2i_PKCS8_PRIV_KEY_INFO_bio(key, NULL);
    if(p8info == NULL) { NSSUtilLogger.msg(ERROR, "Failed to load private key"); return false; }
    else NSSUtilLogger.msg(INFO, "Succeeded to load PrivateKeyInfo");
    if(p8info) {
      pkey = EVP_PKCS82PKEY(p8info);
      if(pkey == NULL) { NSSUtilLogger.msg(ERROR, "Failed to convert PrivateKeyInfo to EVP_PKEY"); BIO_free(key); return false; }
      else NSSUtilLogger.msg(INFO, "Succeeded to convert PrivateKeyInfo to EVP_PKEY");
    }
    BIO* out = NULL;
    //out = BIO_new_file (outfile, "wb");
    out = BIO_new(BIO_s_mem());
    //char* passout = "secretpw";
    //PEM_write_bio_PrivateKey(out, pkey, EVP_des_ede3_cbc(), NULL, 0, NULL, passout);
    PEM_write_bio_PrivateKey(out, pkey, NULL, NULL, 0, NULL, NULL);
    privk_out.clear();
    for(;;) {
      char s[256];
      int l = BIO_read(out,s,sizeof(s));
      if(l <= 0) break;
      privk_out.append(s,l);
    }
    BIO_free(key);
    BIO_free(out);
    PKCS8_PRIV_KEY_INFO_free(p8info);
    return true;
  }

  static bool ImportDERPrivateKey(PK11SlotInfo* slot, const std::vector<uint8>& input, const std::string& name) {
    SECItem der_private_key_info;
    SECStatus rv;
    der_private_key_info.data = const_cast<unsigned char*>(&input.front());
    der_private_key_info.len = input.size();
    //The private key is set to be used for 
    //key unwrapping, data decryption, and signature generation.
    SECItem nickname;
    nickname.data = (unsigned char*)(name.c_str());
    nickname.len = name.size();
    const unsigned int key_usage = KU_KEY_ENCIPHERMENT | KU_DATA_ENCIPHERMENT |
                                 KU_DIGITAL_SIGNATURE;
    rv =  PK11_ImportDERPrivateKeyInfoAndReturnKey(
      slot, &der_private_key_info, &nickname, NULL, PR_TRUE, PR_FALSE,
      key_usage, NULL, NULL);//&privKey, NULL);
    if(rv != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to import private key");
      return false;
    }
    else NSSUtilLogger.msg(INFO, "Succeeded to import private key");
    
    return true;
  }

  static bool GenerateKeyPair(PasswordSource& passphrase, SECKEYPublicKey **pubk, SECKEYPrivateKey **privk, std::string& privk_str, int keysize, const std::string& nick_str) {
    PK11RSAGenParams rsaParams;
    rsaParams.keySizeInBits = keysize;
    rsaParams.pe = 0x10001;

    PK11SlotInfo* slot = NULL;
    slot = PK11_GetInternalKeySlot();
    if(PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to authenticate to key database");
      if(slot) PK11_FreeSlot(slot);
      return false;
    }

    *privk = PK11_GenerateKeyPair(slot, CKM_RSA_PKCS_KEY_PAIR_GEN, &rsaParams,
        pubk, PR_FALSE, PR_FALSE, NULL); 
    //Only by setting the "isPerm" parameter to be false, the private key can be export
    if(*privk != NULL && *pubk != NULL)
      NSSUtilLogger.msg(DEBUG, "Succeeded to generate public/private key pair");
    else {
      NSSUtilLogger.msg(ERROR, "Failed to generate public/private key pair");
      if(slot) PK11_FreeSlot(slot);
      return false;
    }
    std::vector<uint8> output;
    if(!ExportPrivateKey(*privk, &output)) NSSUtilLogger.msg(ERROR, "Failed to export private key");
    OutputPrivateKey(output, privk_str);

    ImportDERPrivateKey(slot, output, nick_str);

    if(slot) PK11_FreeSlot(slot);
    return true;
  }
 
  static bool ImportPrivateKey(PasswordSource& passphrase, const std::string& keyfile, const std::string& nick_str) {
    BIO* key = NULL;
    key = BIO_new_file(keyfile.c_str(), "r");
    std::string key_str;
    for(;;) {
      char s[256];
      int l = BIO_read(key,s,sizeof(s));
      if(l <= 0) break;
      key_str.append(s,l);
    }
    BIO_free_all(key);
    std::vector<uint8> input;
    InputPrivateKey(input, key_str);

    PK11SlotInfo* slot = NULL;
    slot = PK11_GetInternalKeySlot();
    if(PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to authenticate to key database");
      if(slot) PK11_FreeSlot(slot);
      return false;
    }
    DeleteKeyOnly((nick_str.c_str()), passphrase);

    ImportDERPrivateKey(slot, input, nick_str);

    if(slot) PK11_FreeSlot(slot);
    return true;
  }

  bool nssGenerateCSR(const std::string& privkey_name, const std::string& dn, const char* slotpw, const std::string& outfile, std::string& privk_str, bool ascii) {
    PasswordSource* passphrase = NULL;
    if(slotpw) {
      passphrase = new PasswordSourceString(slotpw);
    } else {
      passphrase = new PasswordSourceInteractive("TODO: prompt here",false);
    }
    bool r = nssGenerateCSR(privkey_name, dn, *passphrase, outfile, privk_str, ascii);
    delete passphrase;
    return r;
  }

  bool nssGenerateCSR(const std::string& privkey_name, const std::string& dn, Arc::PasswordSource& passphrase, const std::string& outfile, std::string& privk_str, bool ascii) {
    CERTCertificateRequest* req = NULL;
    CERTSubjectPublicKeyInfo* spki;
    SECKEYPrivateKey* privkey = NULL;
    SECKEYPublicKey* pubkey = NULL;
    CERTName* name = NULL;
    int keybits = 1024;
    PRArenaPool* arena;
    SECItem* encoding;
    SECOidTag signAlgTag;
    SECStatus rv;
    SECItem result;
    PRFileDesc* out = NULL;

    if(!dn.empty()) {
      name = CERT_AsciiToName((char*)(dn.c_str()));
      if(name == NULL) {
        NSSUtilLogger.msg(ERROR, "Failed to create subject name");
        return false;
      }
    }

    //Remove the existing private key and related cert in nss db
    rv = DeleteKeyAndCert((privkey_name.c_str()), passphrase);

    if(!GenerateKeyPair(passphrase, &pubkey, &privkey, privk_str, keybits, privkey_name)) return false;
    //PK11_SetPrivateKeyNickname(privkey, privkey_name.c_str());    

    //privkey = SECKEY_CreateRSAPrivateKey(keybits, &pubkey, NULL);
    spki = SECKEY_CreateSubjectPublicKeyInfo(pubkey);

    req = CERT_CreateCertificateRequest(name, spki, NULL);
    if(req == NULL) {
      NSSUtilLogger.msg(ERROR, "Failed to create certificate request");
    }
    if(pubkey != NULL) {
      SECKEY_DestroyPublicKey(pubkey);
    }
    if(spki != NULL) {
      SECKEY_DestroySubjectPublicKeyInfo(spki);
    }
    if(name) CERT_DestroyName(name);

    //Output the cert request
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if(!arena ) {
      NSSUtilLogger.msg(ERROR, "Failed to call PORT_NewArena");
      return false;
    }
    //Encode the cert request 
    encoding = SEC_ASN1EncodeItem(arena, NULL, req, SEC_ASN1_GET(CERT_CertificateRequestTemplate));
    CERT_DestroyCertificateRequest(req);
    if (encoding == NULL){
      PORT_FreeArena (arena, PR_FALSE);
      NSSUtilLogger.msg(ERROR, "Failed to encode the certificate request with DER format");
      return false;
    }
    //Sign the cert request
    signAlgTag = SEC_GetSignatureAlgorithmOidTag(privkey->keyType, SEC_OID_UNKNOWN);
    if (signAlgTag == SEC_OID_UNKNOWN) {
      PORT_FreeArena (arena, PR_FALSE);
      NSSUtilLogger.msg(ERROR, "Unknown key or hash type");
      return false;
    }
    rv = SEC_DerSignData(arena, &result, encoding->data, encoding->len, privkey, signAlgTag);
    if(rv) {
      PORT_FreeArena (arena, PR_FALSE);
      NSSUtilLogger.msg(ERROR, "Failed to sign the certificate request");
      return false;
    }
  
    out = PR_Open(outfile.c_str(), PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE, 00660); 
    // Encode cert request with specified format
    if (ascii) {
      char* buf;
      int len, num;
      buf = BTOA_ConvertItemToAscii(&result);
      len = PL_strlen(buf);
      PR_fprintf(out, "%s\n", NS_CERTREQ_HEADER);
      num = PR_Write(out, buf, len);
      PORT_Free(buf);
      if(num != len) {
        PORT_FreeArena (arena, PR_FALSE);
        NSSUtilLogger.msg(ERROR, "Failed to output the certificate request as ASCII format");
        return false;
      }
      PR_fprintf(out, "\n%s\n", NS_CERTREQ_TRAILER);
    } 
    else {
      int num = PR_Write(out, result.data, result.len);
      if(num != (int)result.len) {
        PORT_FreeArena (arena, PR_FALSE);
        NSSUtilLogger.msg(ERROR,"Failed to output the certificate request as DER format");
        return false;
      }
    }
    PORT_FreeArena (arena, PR_FALSE);
    PR_Close(out);
    if (privkey) {
        SECKEY_DestroyPrivateKey(privkey);
    }
    NSSUtilLogger.msg(INFO, "Succeeded to output the certificate request into %s", outfile.c_str());
    return true;
  }

  SECStatus SECU_FileToItem(SECItem *dst, PRFileDesc *src) {
    PRFileInfo info;
    PRStatus status;
    status = PR_GetOpenFileInfo(src, &info);
    if(status != PR_SUCCESS) {
      return SECFailure;
    }

    dst->data = 0;
    if(!SECITEM_AllocItem(NULL, dst, info.size)) {
      SECITEM_FreeItem(dst, PR_FALSE);
      dst->data = NULL;
      return SECFailure;
    }

    PRInt32 num = PR_Read(src, dst->data, info.size);
    if (num != info.size) {
      SECITEM_FreeItem(dst, PR_FALSE);
      dst->data = NULL;
      return SECFailure;
    }
    return SECSuccess;
  }

  SECStatus SECU_ReadDERFromFile(SECItem* der, PRFileDesc* infile, bool ascii) {
    SECStatus rv;
    if (ascii) {
      //Convert ascii to binary/
      SECItem filedata;
      char* data;
      char* body;
      // Read ascii data
      rv = SECU_FileToItem(&filedata, infile);
      data = (char *)filedata.data;
      if (!data) {
        NSSUtilLogger.msg(ERROR, "Failed to read data from input file");
        return SECFailure;
      }
      //Remove headers and trailers from data
      if((body = strstr(data, "-----BEGIN")) != NULL) {
        char* trailer = NULL;
        data = body;
        body = PORT_Strchr(data, '\n');
        if (!body)
          body = PORT_Strchr(data, '\r');
        if (body)
          trailer = strstr(++body, "-----END");
        if (trailer != NULL) {
          *trailer = '\0';
        }
        else {
          NSSUtilLogger.msg(ERROR, "Input is without trailer\n");
          PORT_Free(filedata.data);
          return SECFailure;
        }
      }
      else {
        body = data;
      }
      // Convert to binary
      rv = ATOB_ConvertAsciiToItem(der, body);
      if(rv) {
        NSSUtilLogger.msg(ERROR, "Failed to convert ASCII to DER");
        PORT_Free(filedata.data);
        return SECFailure;
      }
      PORT_Free(filedata.data);
    }
    else{
      // Read binary data
      rv = SECU_FileToItem(der, infile);
      if(rv) {
        NSSUtilLogger.msg(ERROR, "Failed to read data from input file");
        return SECFailure;
      }
    }
    return SECSuccess;
  }

  static CERTCertificateRequest* getCertRequest(const std::string& infile, bool ascii) {
    CERTCertificateRequest* req = NULL;
    CERTSignedData signed_data;
    PRArenaPool* arena = NULL;
    SECItem req_der;
    PRFileDesc* in;
    SECStatus rv;

    in = PR_Open(infile.c_str(), PR_RDONLY, 0);
    req_der.data = NULL;
    do {
      arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
      if (arena == NULL) { rv = SECFailure; break; }
      rv = SECU_ReadDERFromFile(&req_der, in, ascii);
      if (rv) break;
      req = (CERTCertificateRequest*) PORT_ArenaZAlloc
                (arena, sizeof(CERTCertificateRequest));
      if (!req) { rv = SECFailure; break; }
      req->arena = arena;
      PORT_Memset(&signed_data, 0, sizeof(signed_data));
      rv = SEC_ASN1DecodeItem(arena, &signed_data,
               SEC_ASN1_GET(CERT_SignedDataTemplate), &req_der);
      if (rv) break;
      rv = SEC_ASN1DecodeItem(arena, req,
               SEC_ASN1_GET(CERT_CertificateRequestTemplate), &signed_data.data);
      if (rv) break;
      rv = CERT_VerifySignedDataWithPublicKeyInfo(&signed_data,
                &req->subjectPublicKeyInfo, NULL);
    } while (0);

    if (req_der.data)
      SECITEM_FreeItem(&req_der, PR_FALSE);

    if (rv) {
      NSSUtilLogger.msg(ERROR, "Certificate request is invalid");
      if (arena) {
        PORT_FreeArena(arena, PR_FALSE);
      }
      req = NULL;
    }
    return req;
  }

  //With pathlen and with policy
  struct ProxyPolicy1 {
    SECItem policylanguage;
    SECItem policy;
  };
  struct ProxyCertInfo1 {
    PLArenaPool *arena;
    SECItem pathlength;
    ProxyPolicy1 proxypolicy;
  };
  const SEC_ASN1Template ProxyPolicyTemplate1[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyPolicy1) },
    { SEC_ASN1_OBJECT_ID,
          offsetof(ProxyPolicy1, policylanguage), NULL, 0 },
    { SEC_ASN1_OCTET_STRING,
          offsetof(ProxyPolicy1, policy), NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyPolicyTemplate1)
  const SEC_ASN1Template ProxyCertInfoTemplate1[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyCertInfo1) },
    { SEC_ASN1_INTEGER,
          offsetof(ProxyCertInfo1, pathlength), NULL, 0 },
    { SEC_ASN1_INLINE,
          offsetof(ProxyCertInfo1, proxypolicy),
          ProxyPolicyTemplate1, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyCertInfoTemplate1)

  //With pathlen and without policy
  struct ProxyPolicy2 {
    SECItem policylanguage;
  };
  struct ProxyCertInfo2 {
    PLArenaPool *arena;
    SECItem pathlength;
    ProxyPolicy2 proxypolicy;
  };
  const SEC_ASN1Template ProxyPolicyTemplate2[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyPolicy2) },
    { SEC_ASN1_OBJECT_ID,
          offsetof(ProxyPolicy2, policylanguage), NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyPolicyTemplate2)
  const SEC_ASN1Template ProxyCertInfoTemplate2[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyCertInfo2) },
    { SEC_ASN1_INTEGER,
          offsetof(ProxyCertInfo2, pathlength), NULL, 0 },
    { SEC_ASN1_INLINE,
          offsetof(ProxyCertInfo2, proxypolicy),
          ProxyPolicyTemplate2, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyCertInfoTemplate2)

  //Without pathlen and with policy
  struct ProxyPolicy3 {
    SECItem policylanguage;
    SECItem policy;
  };
  struct ProxyCertInfo3 {
    PLArenaPool *arena;
    ProxyPolicy3 proxypolicy;
  };
  const SEC_ASN1Template ProxyPolicyTemplate3[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyPolicy3) },
    { SEC_ASN1_OBJECT_ID,
          offsetof(ProxyPolicy3, policylanguage), NULL, 0 },
    { SEC_ASN1_OCTET_STRING,
          offsetof(ProxyPolicy3, policy), NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyPolicyTemplate3)
  const SEC_ASN1Template ProxyCertInfoTemplate3[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyCertInfo3) },
    { SEC_ASN1_INLINE,
          offsetof(ProxyCertInfo3, proxypolicy),
          ProxyPolicyTemplate3, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyCertInfoTemplate3)

  //Without pathlen and without policy
  struct ProxyPolicy4 {
    SECItem policylanguage;
  };
  struct ProxyCertInfo4 {
    PLArenaPool *arena;
    ProxyPolicy4 proxypolicy;
  };
  const SEC_ASN1Template ProxyPolicyTemplate4[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyPolicy4) },
    { SEC_ASN1_OBJECT_ID,
          offsetof(ProxyPolicy4, policylanguage), NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyPolicyTemplate4)
  const SEC_ASN1Template ProxyCertInfoTemplate4[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ProxyCertInfo4) },
    { SEC_ASN1_INLINE,
          offsetof(ProxyCertInfo4, proxypolicy),
          ProxyPolicyTemplate4, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(ProxyCertInfoTemplate4)

  SECStatus EncodeProxyCertInfoExtension1(PRArenaPool *arena, 
                ProxyCertInfo1* info, SECItem* dest) {
    SECStatus rv = SECSuccess;
    PORT_Assert(info != NULL && dest != NULL);
    if(info == NULL || dest == NULL) {
      return SECFailure;
    }
    if(SEC_ASN1EncodeItem(arena, dest, info, SEC_ASN1_GET(ProxyCertInfoTemplate1)) == NULL) {
      rv = SECFailure;
    }
    return(rv);
  }

  SECStatus EncodeProxyCertInfoExtension2(PRArenaPool *arena,
                ProxyCertInfo2* info, SECItem* dest) {
    SECStatus rv = SECSuccess;
    PORT_Assert(info != NULL && dest != NULL);
    if(info == NULL || dest == NULL) {
      return SECFailure;
    }
    if(SEC_ASN1EncodeItem(arena, dest, info, SEC_ASN1_GET(ProxyCertInfoTemplate2)) == NULL) {
      rv = SECFailure;
    }
    return(rv);
  }

  SECStatus EncodeProxyCertInfoExtension3(PRArenaPool *arena,
                ProxyCertInfo3* info, SECItem* dest) {
    SECStatus rv = SECSuccess;
    PORT_Assert(info != NULL && dest != NULL);
    if(info == NULL || dest == NULL) {
      return SECFailure;
    }
    if(SEC_ASN1EncodeItem(arena, dest, info, SEC_ASN1_GET(ProxyCertInfoTemplate3)) == NULL) {
      rv = SECFailure;
    }
    return(rv);
  }

  SECStatus EncodeProxyCertInfoExtension4(PRArenaPool *arena,
                ProxyCertInfo4* info, SECItem* dest) {
    SECStatus rv = SECSuccess;
    PORT_Assert(info != NULL && dest != NULL);
    if(info == NULL || dest == NULL) {
      return SECFailure;
    }
    if(SEC_ASN1EncodeItem(arena, dest, info, SEC_ASN1_GET(ProxyCertInfoTemplate4)) == NULL) {
      rv = SECFailure;
    }
    return(rv);
  }

  typedef SECStatus (* EXTEN_EXT_VALUE_ENCODER) (PRArenaPool *extHandleArena,
                                               void *value, SECItem *encodedValue);

  SECStatus SECU_EncodeAndAddExtensionValue(PRArenaPool *arena, void *extHandle,
                                void *value, PRBool criticality, int extenType,
                                EXTEN_EXT_VALUE_ENCODER EncodeValueFn) {
    SECItem encodedValue;
    SECStatus rv;

    encodedValue.data = NULL;
    encodedValue.len = 0;
    do {
      rv = (*EncodeValueFn)(arena, value, &encodedValue);
      if (rv != SECSuccess)
        break;
      rv = CERT_AddExtension(extHandle, extenType, &encodedValue,
                               criticality, PR_TRUE);
      if (rv != SECSuccess)
        break;
    } while (0);

    return (rv);
  }

  template <class T>
  static std::string to_string (const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
  }

  static SECStatus AddProxyCertInfoExtension(void* extHandle, int pathlen, 
    const char* policylang, const char* policy) {
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    SECOidData* oid = NULL;
    SECItem policy_item;
    std::string pl_lang;
    SECOidTag tag;
    void* mark;

    pl_lang = policylang;
    if(pl_lang == "Any language") tag = tag_anylang;
    else if(pl_lang == "Inherit all") tag = tag_inheritall;
    else if(pl_lang == "Independent") tag = tag_independent;
    else { NSSUtilLogger.msg(ERROR, "The policy language: %s is not supported", policylang); goto error; }
    oid = SECOID_FindOIDByTag(tag);

    if((policy != NULL) && (pathlen != -1)) {
      ProxyCertInfo1* proxy_certinfo = NULL;
    
      arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
      if (!arena ) {
        NSSUtilLogger.msg(ERROR, "Failed to new arena");
        return SECFailure;
      }
      proxy_certinfo = PORT_ArenaZNew(arena, ProxyCertInfo1);
      if ( proxy_certinfo== NULL) {
       return SECFailure;
      }
      proxy_certinfo->arena = arena;
      if((pathlen != 0) && (SEC_ASN1EncodeInteger(arena, &proxy_certinfo->pathlength, pathlen) == NULL)) {
        NSSUtilLogger.msg(ERROR, "Failed to create path length"); goto error;
      }
      if (oid == NULL || SECITEM_CopyItem(arena, &proxy_certinfo->proxypolicy.policylanguage, &oid->oid) == SECFailure) {
        NSSUtilLogger.msg(ERROR, "Failed to create policy language"); goto error;       
      }
      proxy_certinfo->proxypolicy.policy.len = PORT_Strlen(policy);
      proxy_certinfo->proxypolicy.policy.data = (unsigned char*)PORT_ArenaStrdup(arena, policy);

      rv = SECU_EncodeAndAddExtensionValue(arena, extHandle, proxy_certinfo, PR_TRUE, tag_proxy,
               (EXTEN_EXT_VALUE_ENCODER)EncodeProxyCertInfoExtension1);
    }
    else if((policy == NULL) && (pathlen != -1)) {
      ProxyCertInfo2* proxy_certinfo = NULL;

      arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
      if (!arena ) {
        NSSUtilLogger.msg(ERROR, "Failed to new arena");
        return SECFailure;
      }
      proxy_certinfo = PORT_ArenaZNew(arena, ProxyCertInfo2);
      if ( proxy_certinfo== NULL) {
       return SECFailure;
      }
      proxy_certinfo->arena = arena;
      if((pathlen != -1) && (SEC_ASN1EncodeInteger(arena, &proxy_certinfo->pathlength, pathlen) == NULL)) {
        NSSUtilLogger.msg(ERROR, "Failed to create path length"); goto error;
      }
      if (oid == NULL || SECITEM_CopyItem(arena, &proxy_certinfo->proxypolicy.policylanguage, &oid->oid) == SECFailure) {
        NSSUtilLogger.msg(ERROR, "Failed to create policy language"); goto error;
      }

      rv = SECU_EncodeAndAddExtensionValue(arena, extHandle, proxy_certinfo, PR_TRUE, tag_proxy,
               (EXTEN_EXT_VALUE_ENCODER)EncodeProxyCertInfoExtension2);
    }
    else if((policy != NULL) && (pathlen == -1)) {
      ProxyCertInfo3* proxy_certinfo = NULL;

      arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
      if (!arena ) {
        NSSUtilLogger.msg(ERROR, "Failed to new arena");
        return SECFailure;
      }
      proxy_certinfo = PORT_ArenaZNew(arena, ProxyCertInfo3);
      if ( proxy_certinfo== NULL) {
       return SECFailure;
      }
      proxy_certinfo->arena = arena;
      if (oid == NULL || SECITEM_CopyItem(arena, &proxy_certinfo->proxypolicy.policylanguage, &oid->oid) == SECFailure) {
        NSSUtilLogger.msg(ERROR, "Failed to create policy language"); goto error;
      }
      proxy_certinfo->proxypolicy.policy.len = PORT_Strlen(policy);
      proxy_certinfo->proxypolicy.policy.data = (unsigned char*)PORT_ArenaStrdup(arena, policy);

      rv = SECU_EncodeAndAddExtensionValue(arena, extHandle, proxy_certinfo, PR_TRUE, tag_proxy,
               (EXTEN_EXT_VALUE_ENCODER)EncodeProxyCertInfoExtension3);
    }
    else {
      ProxyCertInfo4* proxy_certinfo = NULL;

      arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
      if (!arena ) {
        NSSUtilLogger.msg(ERROR, "Failed to new arena");
        return SECFailure;
      }
      proxy_certinfo = PORT_ArenaZNew(arena, ProxyCertInfo4);
      if ( proxy_certinfo== NULL) {
       return SECFailure;
      }
      proxy_certinfo->arena = arena;
      if (oid == NULL || SECITEM_CopyItem(arena, &proxy_certinfo->proxypolicy.policylanguage, &oid->oid) == SECFailure) {
        NSSUtilLogger.msg(ERROR, "Failed to create policy language"); goto error;
      }

      rv = SECU_EncodeAndAddExtensionValue(arena, extHandle, proxy_certinfo, PR_TRUE, tag_proxy,
               (EXTEN_EXT_VALUE_ENCODER)EncodeProxyCertInfoExtension4);
    }

error:
    if (arena)
	PORT_FreeArena(arena, PR_FALSE);
    return (rv);
  }

  // Add a binary into extension, specific for the VOMS AC sequence
  static SECStatus AddVOMSACSeqExtension(void* extHandle, char* vomsacseq, int length) {
    SECStatus rv = SECFailure;
    SECOidData* oid = NULL;
    SECOidTag tag;

    tag = tag_vomsacseq;
    oid = SECOID_FindOIDByTag(tag);

    if(vomsacseq != NULL) {
      SECItem encodedValue;

      encodedValue.data = (unsigned char*)vomsacseq;
      encodedValue.len = length;
      rv = CERT_AddExtension(extHandle, tag, &encodedValue,
                               PR_FALSE, PR_TRUE);
    }

    return (rv);
  }

#if !defined(MACOS)
// CERT_NameFromDERCert 
// CERT_IssuerNameFromDERCert 
// CERT_SerialNumberFromDERCert
// The above nss methods which we need to use have not been exposed by
// nss.def (the situation does not apply to MACOS, only to Win and Linux,
// strange, maybe *.def is not used by MACOS when packaging).
// Therefore we have two different solutions for Linux and Win.
// For Linux, the three methods are duplicated here.
// For Win, the code duplication does not work ( arcproxy always crashes
// when goes to these three method, the crash is from 
// nssutil3.dll!SECOID_GetAlgorithmTag_Util, which I don't know how 
// to solve it for now). So the other working solution is to change the
// nss.def file under nss source tree to add these three methods, so that
// these methods can be exposed to external.

  const SEC_ASN1Template SEC_SkipTemplate[] = {
      { SEC_ASN1_SKIP, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SkipTemplate)

  //Find the subjectName in a DER encoded certificate
  const SEC_ASN1Template SEC_CertSubjectTemplate[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED |
          SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
          0, SEC_SkipTemplate, 0 },  /* version */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* serial number */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* signature algorithm */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* issuer */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* validity */
    { SEC_ASN1_ANY, 0, NULL, 0 },          /* subject */
    { SEC_ASN1_SKIP_REST, 0, NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(SEC_CertSubjectTemplate)

  //Find the issuerName in a DER encoded certificate
  const SEC_ASN1Template SEC_CertIssuerTemplate[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED |
          SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
          0, SEC_SkipTemplate, 0 },  /* version */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* serial number */
    { SEC_ASN1_SKIP, 0, NULL, 0 },          /* signature algorithm */
    { SEC_ASN1_ANY, 0, NULL, 0 },          /* issuer */
    { SEC_ASN1_SKIP_REST, 0, NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(SEC_CertIssuerTemplate)

  //Find the serialNumber in a DER encoded certificate
  const SEC_ASN1Template SEC_CertSerialNumberTemplate[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED |
          SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
          0, SEC_SkipTemplate, 0 },  /* version */
    { SEC_ASN1_ANY, 0, NULL, 0 }, /* serial number */
    { SEC_ASN1_SKIP_REST, 0, NULL, 0 },
    { 0, 0, NULL, 0 }
  };
  SEC_ASN1_CHOOSER_IMPLEMENT(SEC_CertSerialNumberTemplate)

/* Extract the subject name from a DER certificate
   This is a copy from nss code, due to the "undefined reference to" compiling issue
 */
SECStatus
my_CERT_NameFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);

    if ( ! arena ) {
        return(SECFailure);
    }

    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, SEC_ASN1_GET(CERT_SignedDataTemplate), derCert);

    if ( rv ) {
        goto loser;
    }

    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_ASN1_GET(SEC_CertSubjectTemplate), &sd.data);

    if ( rv ) {
        goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
        goto loser;
    }

    PORT_Memcpy(derName->data, tmpptr, derName->len);

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}

SECStatus
my_CERT_IssuerNameFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);

    if ( ! arena ) {
        return(SECFailure);
    }

    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, SEC_ASN1_GET(CERT_SignedDataTemplate), derCert);

    if ( rv ) {
        goto loser;
    }

    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_ASN1_GET(SEC_CertIssuerTemplate), &sd.data);

    if ( rv ) {
        goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
        goto loser;
    }

    PORT_Memcpy(derName->data, tmpptr, derName->len);

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}

SECStatus
my_CERT_SerialNumberFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);

    if ( ! arena ) {
        return(SECFailure);
    }

    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, SEC_ASN1_GET(CERT_SignedDataTemplate), derCert);

    if ( rv ) {
        goto loser;
    }

    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_ASN1_GET(SEC_CertSerialNumberTemplate), &sd.data);

    if ( rv ) {
        goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
        goto loser;
    }

    PORT_Memcpy(derName->data, tmpptr, derName->len);

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}
#endif // #if !defined(MACOS)

  void nssListUserCertificatesInfo(std::list<certInfo>& certInfolist) {
    CERTCertList* list;
    CERTCertificate* find_cert = NULL;
    CERTCertListNode* node;

    list = PK11_ListCerts(PK11CertListAll, NULL);
    for (node = CERT_LIST_HEAD(list); !CERT_LIST_END(node,list);
        node = CERT_LIST_NEXT(node)) {
      CERTCertificate* cert = node->cert;
      const char* nickname = (const char*)node->appData;
      if (!nickname) {
        nickname = cert->nickname;
      }
      if(nickname == NULL) continue;
      PRBool isUser = CERT_IsUserCert(cert);
      if(!isUser) continue;

      certInfo cert_info;
      cert_info.certname = nickname;

      SECStatus rv;
      std::string subject_dn;
      SECItem derSubject;

#if !defined(MACOS)
      rv = my_CERT_NameFromDERCert(&cert->derCert, &derSubject);
#else
      rv = CERT_NameFromDERCert(&cert->derCert, &derSubject);
#endif

      if(rv == SECSuccess) {
        char* subjectName = CERT_DerNameToAscii(&derSubject);
        subject_dn = subjectName;
        if(subjectName) PORT_Free(subjectName);
        cert_info.subject_dn = subject_dn;
      }

      std::string issuer_dn;
      SECItem derIssuer;

#if !defined(MACOS)
      rv = my_CERT_IssuerNameFromDERCert(&cert->derCert, &derIssuer);
#else
      rv = CERT_IssuerNameFromDERCert(&cert->derCert, &derIssuer);
#endif
      if(rv == SECSuccess) {
        char* issuerName = CERT_DerNameToAscii(&derIssuer);
        issuer_dn = issuerName;
        if(issuerName) PORT_Free(issuerName);
        cert_info.issuer_dn = issuer_dn;
      }

      cert_info.serial = 0;
      std::string serial;
      SECItem derSerial;

#if !defined(MACOS)
      rv = my_CERT_SerialNumberFromDERCert (&cert->derCert, &derSerial);
#else
      rv = CERT_SerialNumberFromDERCert (&cert->derCert, &derSerial);
#endif

      if(rv == SECSuccess) {
        SECItem decodedValue;
        decodedValue.data = NULL;
        rv = SEC_ASN1DecodeItem (NULL, &decodedValue,
                                SEC_ASN1_GET(SEC_IntegerTemplate),
                                &derSerial);
        if (rv == SECSuccess) {
          unsigned long res;
          rv = SEC_ASN1DecodeInteger(&decodedValue, &res); 
          if(rv == SECSuccess) cert_info.serial = res;
        }
      }

      PRTime notBefore, notAfter;
      rv = CERT_GetCertTimes(cert, &notBefore, &notAfter);
      if(rv == SECSuccess) {
        cert_info.start = Arc::Time(notBefore/1000/1000);
        cert_info.end = Arc::Time(notAfter/1000/1000); 
        certInfolist.push_back(cert_info);
      }
    }
    if (list) {
      CERT_DestroyCertList(list);
    }
  }

  static SECStatus copy_CERTName(CERTName* destName, CERTName* srcName) {
    PRArenaPool* poolp = NULL;
    SECStatus rv;
    if (destName->arena != NULL) {
      poolp = destName->arena;
    } else {
      poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    }
    if (poolp == NULL) {
      return SECFailure;
    }
    destName->arena = NULL;
    rv = CERT_CopyName(poolp, destName, srcName);
    destName->arena = poolp;
    return rv;
  }

  static SECStatus AddKeyUsageExtension (void* extHandle, CERTCertificate* issuercert) {
    SECStatus rv;
    SECItem bitStringValue;
    PRBool isCriticalExt = PR_TRUE;
    SECItem keyUsageValue = {siBuffer, NULL, 0};
    unsigned char ku_value;

    rv = CERT_FindKeyUsageExtension(issuercert, &keyUsageValue);
    if(rv == SECFailure) {
      rv = (PORT_GetError () == SEC_ERROR_EXTENSION_NOT_FOUND) ?
        SECSuccess : SECFailure;
      return rv;
    } 
    else {
      ku_value = keyUsageValue.data[0];
      // mask off the key usage that should not be allowed for proxy
      if(ku_value & KU_NON_REPUDIATION) ku_value &=(~KU_NON_REPUDIATION);
      if(ku_value & KU_KEY_CERT_SIGN) ku_value &=(~KU_KEY_CERT_SIGN);
      if(ku_value & KU_CRL_SIGN) ku_value &=(~KU_CRL_SIGN);
    }
    PORT_Free (keyUsageValue.data);

    bitStringValue.data = &ku_value;
    bitStringValue.len = 1;

    return (CERT_EncodeAndAddBitStrExtension
            (extHandle, SEC_OID_X509_KEY_USAGE, &bitStringValue,
             isCriticalExt));
  }

  bool nssCreateCert(const std::string& csrfile, const std::string& issuername, const char* passwd, const int duration, const std::string& vomsacseq, std::string& outfile, bool ascii) {
    CERTCertDBHandle* certhandle;
    CERTCertificate* issuercert = NULL;
    SECKEYPrivateKey* issuerkey = NULL;
    CERTValidity* validity;
    CERTCertificate* cert = NULL;
    PRExplodedTime extime;
    PRTime now, start, end;
    int serialnum;
    CERTCertificateRequest* req = NULL;
    void* ext_handle; 
    PRArenaPool* arena;
    SECOidTag tag_sigalg;
    SECOidTag tag_hashalg;
    int pathlen = -1;
    const char* policylang = "Inherit all"; //TODO
    const char* policy = NULL;//"test policy"; //TODO
    CERTCertExtension** exts;
    SECItem cert_der;
    SECItem derSubject;
    void* dummy;
    SECItem* signed_cert = NULL;
    PRFileDesc* out = NULL;
    SECStatus rv = SECSuccess;
    bool ret = false;

    req = getCertRequest(csrfile, ascii);
    if(!req) {
      NSSUtilLogger.msg(ERROR, "Failed to parse certificate request from CSR file %s", csrfile.c_str());
      return false;
    }
    
    certhandle = CERT_GetDefaultCertDB();
    issuercert = CERT_FindCertByNicknameOrEmailAddr(certhandle, (char*)(issuername.c_str()));
    if(!issuercert) {
      NSSUtilLogger.msg(ERROR, "Can not find certificate with name %s", issuername.c_str());
      return false;
    }

    now = PR_Now();
    PR_ExplodeTime(now, PR_GMTParameters, &extime);
    extime.tm_min  -= 5;
    start = PR_ImplodeTime(&extime);
    extime.tm_min +=5;
    extime.tm_hour += duration;
    end = PR_ImplodeTime (&extime);
    validity = CERT_CreateValidity(start, end);

    //Subject
#if !defined(MACOS)
    my_CERT_NameFromDERCert(&issuercert->derCert, &derSubject);
#else
    CERT_NameFromDERCert(&issuercert->derCert, &derSubject);
#endif

    char* subjectName = CERT_DerNameToAscii(&derSubject);
    std::string subname_str = subjectName;

    srand(time(NULL));
    unsigned long random_cn;
    random_cn = rand();
    char* CN_name = NULL;
    CN_name = (char*)malloc(sizeof(long)*4 + 1);
    snprintf(CN_name, sizeof(long)*4 + 1, "%lu", random_cn);
    std::string str = "CN="; str.append(CN_name); str.append(",");
    subname_str.insert(0, str.c_str(), str.length());
    NSSUtilLogger.msg(DEBUG, "Proxy subject: %s", subname_str.c_str());
    if(CN_name) free(CN_name);
    CERTName* subName = NULL;
    if(!subname_str.empty()) subName = CERT_AsciiToName((char*)(subname_str.c_str()));
    if(subjectName) PORT_Free(subjectName);

    if (validity) {
      if(subName != NULL) {
        rv = copy_CERTName(&req->subject, subName);
      }
      cert = CERT_CreateCertificate(rand(), &issuercert->subject, validity, req);

      CERT_DestroyValidity(validity);
      if(subName) CERT_DestroyName(subName);
    }

    //Extensions
    ext_handle = CERT_StartCertExtensions (cert);
    if (ext_handle == NULL) {
      NSSUtilLogger.msg(ERROR, "Failed to start certificate extension");
      goto error;
    }

    if(AddKeyUsageExtension(ext_handle, issuercert) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to add key usage extension");
      goto error;
    }

    if(AddProxyCertInfoExtension(ext_handle, pathlen, policylang, policy) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to add proxy certificate information extension");
      goto error;
    }
    if((!vomsacseq.empty()) && (AddVOMSACSeqExtension(ext_handle, (char*)(vomsacseq.c_str()), vomsacseq.length()) != SECSuccess)) {
      NSSUtilLogger.msg(ERROR, "Failed to add voms AC extension");
      goto error;
    }

    if(req->attributes != NULL &&
       req->attributes[0] != NULL &&
       req->attributes[0]->attrType.data != NULL &&
       req->attributes[0]->attrType.len   > 0    &&
      SECOID_FindOIDTag(&req->attributes[0]->attrType)
            == SEC_OID_PKCS9_EXTENSION_REQUEST) {
      rv = CERT_GetCertificateRequestExtensions(req, &exts);
      if(rv != SECSuccess) goto error;
      rv = CERT_MergeExtensions(ext_handle, exts);
      if (rv != SECSuccess) goto error;
    }
    CERT_FinishExtensions(ext_handle);

    //Sign the certificate
    issuerkey = PK11_FindKeyByAnyCert(issuercert, (char*)passwd);
    if(issuerkey == NULL) {
      NSSUtilLogger.msg(ERROR, "Failed to retrieve private key for issuer");
      goto error;
    }
    arena = cert->arena;
    tag_hashalg = SEC_OID_SHA1;
    tag_sigalg = SEC_GetSignatureAlgorithmOidTag(issuerkey->keyType, tag_hashalg);
    if (tag_sigalg == SEC_OID_UNKNOWN) {
      NSSUtilLogger.msg(ERROR, "Unknown key or hash type of issuer");
      goto error;
    }

    rv = SECOID_SetAlgorithmID(arena, &cert->signature, tag_sigalg, 0);
    if(rv != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to set signature algorithm ID");
      goto error;
    }

    *(cert->version.data) = 2;
    cert->version.len = 1;

    cert_der.len = 0;
    cert_der.data = NULL;
    dummy = SEC_ASN1EncodeItem (arena, &cert_der, cert,
                                SEC_ASN1_GET(CERT_CertificateTemplate));
    if (!dummy) {
      NSSUtilLogger.msg(ERROR, "Failed to encode certificate");
      goto error;
    }

    signed_cert = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
    if(signed_cert == NULL) {
      NSSUtilLogger.msg(ERROR, "Failed to allocate item for certificate data");
      goto error;
    }

    rv = SEC_DerSignData(arena, signed_cert, cert_der.data, cert_der.len, issuerkey, tag_sigalg);
    if(rv != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to sign encoded certificate data");
      //signed_cert will be freed when arena is freed.
      signed_cert = NULL;
      goto error;
    }
    cert->derCert = *signed_cert;

    out = PR_Open(outfile.c_str(), PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE, 00660);
    if(!out) {
      NSSUtilLogger.msg(ERROR, "Failed to open file %s", outfile.c_str());
      goto error;
    }
    if(ascii) {
      PR_fprintf(out, "%s\n%s\n%s\n", NS_CERT_HEADER,
             BTOA_DataToAscii(signed_cert->data, signed_cert->len), NS_CERT_TRAILER);
    } 
    else {
      PR_Write(out, signed_cert->data, signed_cert->len);
    }
    if(out) PR_Close(out);
    NSSUtilLogger.msg(INFO, "Succeeded to output certificate to %s", outfile.c_str());
    ret = true;

error:
    if(issuerkey) SECKEY_DestroyPrivateKey(issuerkey);
    if(issuercert) CERT_DestroyCertificate(issuercert);
    if(req) CERT_DestroyCertificateRequest(req);

    return ret;
  } 

  bool nssImportCert(char* slotpw, const std::string& certfile, const std::string& name, const char* trusts, bool ascii) {
    PasswordSource* passphrase = NULL;
    if(slotpw) {
      passphrase = new PasswordSourceString(slotpw);
    } else {
      passphrase = new PasswordSourceInteractive("TODO: prompt here",false);
    }
    bool r = nssImportCert(*passphrase, certfile, name, trusts, ascii);
    delete passphrase;
    return r;
  }

  bool nssImportCert(PasswordSource& passphrase, const std::string& certfile, const std::string& name, const char* trusts, bool ascii) {
    PK11SlotInfo* slot = NULL;
    CERTCertDBHandle* certhandle;
    CERTCertTrust* trust = NULL;
    CERTCertificate* cert = NULL;
    PRFileDesc* in = NULL;
    SECItem certder;
    SECStatus rv = SECSuccess;

    slot = PK11_GetInternalKeySlot();
    if(PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
      NSSUtilLogger.msg(ERROR, "Failed to authenticate to key database");
      if(slot) PK11_FreeSlot(slot);
      return false;
    }

    in = PR_Open(certfile.c_str(), PR_RDONLY, 0);
    if(in == NULL) { 
      NSSUtilLogger.msg(ERROR, "Failed to open input certificate file %s", certfile.c_str());
      if(slot) PK11_FreeSlot(slot);
      return false;
    }

    certhandle = CERT_GetDefaultCertDB();

    rv = DeleteCertOnly(name.c_str());
    //rv = DeleteCertAndKey(name.c_str(), slotpw);
    if(rv == SECFailure) {
      PR_Close(in);
      PK11_FreeSlot(slot);
    }

    certder.data = NULL;
    do {
      rv = SECU_ReadDERFromFile(&certder, in, ascii);
      if(rv != SECSuccess) {
        NSSUtilLogger.msg(ERROR, "Failed to read input certificate file");
        break;
      }
      cert = CERT_DecodeCertFromPackage((char *)certder.data, certder.len);
      if(!cert) {
        NSSUtilLogger.msg(ERROR, "Failed to get certificate from certificate file");
        rv = SECFailure; break;
      }

      //Create a cert trust
      trust = (CERTCertTrust *)PORT_ZAlloc(sizeof(CERTCertTrust));
      if(!trust) {
        NSSUtilLogger.msg(ERROR, "Failed to allocate certificate trust");
        rv = SECFailure; break;
      }
      rv = CERT_DecodeTrustString(trust, (char*)trusts);
      if(rv) {
        NSSUtilLogger.msg(ERROR, "Failed to decode trust string");
        rv = SECFailure; break;
      }
    
      //Import the certificate
      rv = PK11_ImportCert(slot, cert, CK_INVALID_HANDLE, (char*)(name.c_str()), PR_FALSE);
      if(rv != SECSuccess) {
        if(PORT_GetError() == SEC_ERROR_TOKEN_NOT_LOGGED_IN) {
          if(PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
            NSSUtilLogger.msg(ERROR, "Failed to authenticate to token %s", PK11_GetTokenName(slot));
            rv = SECFailure; break;
          }
          rv = PK11_ImportCert(slot, cert, CK_INVALID_HANDLE, (char*)(name.c_str()), PR_FALSE);
          if(rv != SECSuccess) {
            NSSUtilLogger.msg(ERROR, "Failed to add certificate to token or database");
            break;
          }
          else NSSUtilLogger.msg(INFO, "Succeeded to import certificate");
        }
      }
      else NSSUtilLogger.msg(INFO, "Succeeded to import certificate");

      rv = CERT_ChangeCertTrust(certhandle, cert, trust);
      if(rv != SECSuccess) {
        if (PORT_GetError() == SEC_ERROR_TOKEN_NOT_LOGGED_IN) {
          if(PK11_Authenticate(slot, PR_TRUE, (void*)&passphrase) != SECSuccess) {
            NSSUtilLogger.msg(ERROR, "Failed to authenticate to token %s", PK11_GetTokenName(slot));
            rv = SECFailure; break;
          }
          rv = CERT_ChangeCertTrust(certhandle, cert, trust);
          if(rv != SECSuccess) {
            NSSUtilLogger.msg(ERROR, "Failed to add certificate to token or database");
            break;
          }
          else NSSUtilLogger.msg(INFO, "Succeeded to change trusts to: %s", trusts);
        }
      }
      else NSSUtilLogger.msg(INFO, "Succeeded to change trusts to: %s", trusts);
    } while (0);

    PR_Close(in);
    PK11_FreeSlot(slot);
    CERT_DestroyCertificate (cert);
    PORT_Free(trust);
    PORT_Free(certder.data);
    if(rv == SECSuccess) return true;
    else return false;
  }

  bool nssImportCertAndPrivateKey(char* slotpw, const std::string& keyfile, const std::string& keyname, const std::string& certfile, const std::string& certname, const char* trusts, bool ascii) { 
    PasswordSource* passphrase = NULL;
    if(slotpw) {
      passphrase = new PasswordSourceString(slotpw);
    } else {
      passphrase = new PasswordSourceInteractive("TODO: prompt here",false);
    }
    bool r = nssImportCertAndPrivateKey(*passphrase, keyfile, keyname, certfile, certname, trusts, ascii);
    delete passphrase;
    return r;
  }

  bool nssImportCertAndPrivateKey(PasswordSource& passphrase, const std::string& keyfile, const std::string& keyname, const std::string& certfile, const std::string& certname, const char* trusts, bool ascii) { 
    bool res;
    res = ImportPrivateKey(passphrase, keyfile, keyname);
    if(!res) { NSSUtilLogger.msg(ERROR, "Failed to import private key from file: %s", keyfile.c_str()); return false; }
    res = nssImportCert(passphrase, certfile, certname, trusts, ascii);
    if(!res) { NSSUtilLogger.msg(ERROR, "Failed to import certificate from file: %s", certfile.c_str()); return false; }
    return true;
  }

}
