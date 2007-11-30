#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/misc/ClientInterface.h>
#include <openssl/bio.h>

#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/MCC.h>
#include <arc/message/Message.h>
#include "../hed/mcc/tls/BIOMCC.h"   //should move BIOMCO.h to "include"?

#include "vomsutil.h"

//The callback method below is to adapt the voms implementation, which requires client to send the length of encrypted data package (token)
//to the sever side. 
//The implementation is based on the callback mechanism of BIO structure, 
//i.e. implement:  bio->callback(BIO *b, int oper, const char *argp, int argi, long argl, long retvalue) 
//the BIO_vomshead_callback method will catch the length of the data package and send the lengh to the other side, before the 
//data package is sent (BIO_write).

static long  BIO_vomshead_callback(BIO *bio, int cmd, const char *argp,
	     int argi, long argl, long ret) {
  BIO *b;
  size_t length;
  unsigned char length_buffer[4];

  b=(BIO *)bio->cb_arg;
  if(b == NULL || b->ptr == NULL) return(ret);

  swith(cmd) {
    case BIO_CB_WRITE:
      // get the socket stream attached to the BIOMCC
      BIOMCC* biomcc = (BIOMCC*)(b->ptr);
      if(biomcc == NULL) return(ret);
      PayloadStreamInterface* stream = biomcc->Stream();

      //get the length of the data package that will be sent
      length = (size_t)(b->num_write);

      // encode the data package length in network byte order: 4 byte
      length_buffer[0] = (unsigned char) ((length >> 24) & 0xffffffff);
      length_buffer[1] = (unsigned char) ((length >> 16) & 0xffffffff);
      length_buffer[2] = (unsigned char) ((length >>  8) & 0xffffffff);
      length_buffer[3] = (unsigned char) ((length      ) & 0xffffffff);

      // send the length of the data package, the same as the code in mcc_write()
      if(stream != NULL) {
        bool r = stream->Put(length_buffer,4);
        BIO_clear_retry_flags(b); 
        if(r) { ret=4; } else { ret=-1; };
        return(ret);
      };  

      MCCInterface* next = biomcc->Next();
      if(next == NULL) return(ret);
      PayloadRaw nextpayload;
      nextpayload.Insert(length_buffer,0,4);
      Message nextinmsg;
      nextinmsg.Payload(&nextpayload);
      Message nextoutmsg;
  
      MCC_Status mccret = next->process(nextinmsg,nextoutmsg);
      BIO_clear_retry_flags(b);
      if(mccret) { 
        if(nextoutmsg.Payload()) {
          PayloadStreamInterface* retpayload = NULL;
          try {
            retpayload=dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());
          } catch(std::exception& e) { };
          if(retpayload) {
            if(!stream)
              biomcc->Stream(retpayload);
          }
          if(stream) delete nextoutmsg.Payload();
          ret= 4;
          } 
          else { ret=-1; };
        }

      break;

    default:
      break;
    }
    
  //BIO_dump(b, length_buffer, 4 );
  
  return ret;
}

namespace Arc{

  Logger VOMSUtil::logger(Logger::rootLogger, "VOMSUtil");

  bool VOMSUtil::contact(const std::string &hostname, int port, const std::string &contact,
        const std::string &command, std::string &buffer, std::string &username,
        std::string &ca) {
    struct hostent *hp; 
    if (!(hp = gethostbyname(hostname.c_str()))) {
      logger.msg(Arc::ERROR, "Host name unknown to DNS");
      return false;
    }
    std::string host(inet_ntoa(*(struct in_addr *)(hp->h_addr)));    

    MCCConfig cfg;

    ClientTCP client(cfg, host, port, 1);
   
    //Add the callback method into BIO   
 
    PayloadRaw request;
    PayloadStreamInterface* response;

    //Compose the request 
    

    client.process(&request, &response);

}

} // namespace Arc

