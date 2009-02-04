#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <fstream>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

static char trans2[128] = { 0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            0,   0,  0,  0,  0,  0,  0,  0,
                            52, 53, 54, 55, 56, 57, 58, 59,
                            60, 61,  0,  0,  0,  0,  0,  0,
                            0,  26, 27, 28, 29, 30, 31, 32,
                            33, 34, 35, 36, 37, 38, 39, 40,
                            41, 42, 43, 44, 45, 46, 47, 48,
                            49, 50, 51, 62,  0, 63,  0,  0,
                            0,   0,  1,  2,  3,  4,  5,  6,
                            7,   8,  9, 10, 11, 12, 13, 14,
                            15, 16, 17, 18, 19, 20, 21, 22,
                            23, 24, 25,  0,  0,  0,  0,  0};

static char *base64Decode(const char *data, int size, int *j)
{
  BIO *b64 = NULL;
  BIO *in = NULL;

  char *buffer = (char *)malloc(size);
  if (!buffer)
    return NULL;

  memset(buffer, 0, size);

  b64 = BIO_new(BIO_f_base64());
  in = BIO_new_mem_buf((void*)data, size);
  in = BIO_push(b64, in);

  *j = BIO_read(in, buffer, size);

  BIO_free_all(in);

  return buffer;
}

static char *MyDecode(const char *data, int size, int *n)
{
  int bit = 0;
  int i = 0;
  char *res;

  if (!data || !size) return NULL;

  if ((res = (char *)calloc(1, (size*3)/4 + 2))) {
    *n = 0;

    while (i < size) {
      char c  = trans2[(int)data[i]];
      char c2 = (((i+1) < size) ? trans2[(int)data[i+1]] : 0);

      switch(bit) {
      case 0:
        res[*n] = ((c & 0x3f) << 2) | ((c2 & 0x30) >> 4);
        if ((i+1) < size)
          (*n)++;
        bit=4;
        i++;
        break;
      case 4:
        res[*n] = ((c & 0x0f) << 4) | ((c2 & 0x3c) >> 2);
        if ((i+1) < size)
          (*n)++;
        bit=2;
        i++;
        break;
      case 2:
        res[*n] = ((c & 0x03) << 6) | (c2 & 0x3f);
        if ((i+1) < size)
          (*n)++;

        i += 2;
        bit = 0;
        break;
      }
    }

    return res;
  }
  return NULL;
}

char *Decode(const char *data, int size, int *j)
{
  int i = 0;

  while (i < size)
    if (data[i++] == '\n')
      return base64Decode(data, size, j);

  return MyDecode(data, size, j);
}

int main(void) {
  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "Test2VOMSServer");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  // The message which will be sent to voms server
  //std::string send_msg("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>G/playground.knowarc.eu</command><lifetime>43200</lifetime></voms>");
  std::string send_msg("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>G/knowarc.eu</command><lifetime>43200</lifetime></voms>");


  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  //std::string voms_url("httpg://arthur.hep.lu.se:15001");
  //Arc::URL voms_service(voms_url);

  Arc::MCCConfig cfg;
  cfg.AddProxy("/tmp/x509up_u1001");
  //cfg.AddCertificate("/home/wzqiang/arc-0.9/src/tests/echo/usercert.pem");
  //cfg.AddPrivateKey("/home/wzqiang/arc-0.9/src/tests/echo/userkey-nopass.pem");
  cfg.AddCADir("/home/qiangwz/.globus/certificates/");

  //Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15002, Arc::GSISec);
  Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15001, Arc::GSISec);
  //Arc::ClientTCP client(cfg, "squark.uio.no", 15011, Arc::GSISec);

  Arc::PayloadRaw request;
  request.Insert(send_msg.c_str(),0,send_msg.length());
  //Arc::PayloadRawInterface& buffer = dynamic_cast<Arc::PayloadRawInterface&>(request);
  //std::cout<<"Message in PayloadRaw:  "<<((Arc::PayloadRawInterface&)buffer).Content()<<std::endl;

  Arc::PayloadStreamInterface *response = NULL;
  Arc::MCC_Status status = client.process(&request, &response,true);
  if (!status) {
    logger.msg(Arc::ERROR, (std::string)status);
    if (response)
      delete response;
    return 1;
  }
  if (!response) {
    logger.msg(Arc::ERROR, "No stream response");
    return 1;
  }
 
  std::string ret_str;
  int length;
  char ret_buf[1024];
  memset(ret_buf,0,1024);
  int len;
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    ret_str.append(ret_buf);
    memset(ret_buf,0,1024);
  }while(len == 1024);

  logger.msg(Arc::INFO, "Returned msg from voms server: %s ", ret_str.c_str());

  Arc::Time t;
  int keybits = 1024;
  int proxydepth = 10;
  std::string req_file_ac("./request_withac.pem");
  std::string out_file_ac("./out_withac.pem");
  Arc::Credential cred_request(t, Arc::Period(12*3600), keybits, "rfc","inheritAll", "", -1);
  cred_request.GenerateRequest(req_file_ac.c_str());


  //Put the return attribute certificate into proxy certificate as the extension part
  Arc::XMLNode node(ret_str);
  std::string codedac;
  codedac = (std::string)(node["ac"]);
  std::cout<<"Coded AC: "<<codedac<<std::endl;
  std::string decodedac;
  int size;
  char* dec = NULL;
  dec = Decode((char *)(codedac.c_str()), codedac.length(), &size);
  decodedac.append(dec,size);
  if(dec!=NULL) { delete []dec;dec = NULL; }
  //std::cout<<"Decoded AC: "<<decodedac<<std::endl<<" Size: "<<size<<std::endl;

  ArcCredential::AC** aclist = NULL;
  Arc::addVOMSAC(aclist, decodedac);
  Arc::Credential proxy;
  proxy.InquireRequest(req_file_ac.c_str());
  //Add AC extension to proxy certificat before signing it
  proxy.AddExtension("acseq", (char**) aclist);
  
  std::string cert("../../tests/echo/usercert.pem");
  std::string key("../../tests/echo/userkey.pem");
  std::string cadir("../../tests/echo/certificates/");
  Arc::Credential signer(cert, key, cadir, "");
  signer.SignRequest(&proxy, out_file_ac.c_str());

  std::string private_key, signing_cert, signing_cert_chain;
  cred_request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);
  std::ofstream out_f(out_file_ac.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();

  if(response) delete response;
  return 0;
}
