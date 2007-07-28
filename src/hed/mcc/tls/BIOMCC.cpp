#include <openssl/ssl.h>
#include <errno.h>
#include "../../libs/message/PayloadRaw.h"
#include "../../libs/message/PayloadStream.h"
#include "../../libs/message/MCC.h"
#include "../../libs/message/Message.h"

#include "BIOMCC.h"

namespace Arc {

class BIOMCC {
  private:
    //BIO* bio_;
    PayloadStreamInterface* stream_;
    MCCInterface* next_;
  public:
    BIOMCC(MCCInterface* next) { next_=next; stream_=NULL; };
    ~BIOMCC(void) { if(stream_) delete stream_; };
    PayloadStreamInterface* Stream() { return stream_; };
    void Stream(PayloadStreamInterface* stream) { stream_=stream; /*free ??*/ };
    MCCInterface* Next(void) { return next_; };
    void MCC(MCCInterface* next) { next_=next; };
};

static int  mcc_write(BIO *h, const char *buf, int num);
static int  mcc_read(BIO *h, char *buf, int size);
static int  mcc_puts(BIO *h, const char *str);
static long mcc_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int  mcc_new(BIO *h);
static int  mcc_free(BIO *data);


static BIO_METHOD methods_mcc = {
  BIO_TYPE_FD,"Message Chain Component",
  mcc_write,
  mcc_read,
  mcc_puts,
  NULL, /* gets, */
  mcc_ctrl,
  mcc_new,
  mcc_free,
  NULL,
};

BIO_METHOD *BIO_s_MCC(void) {
  return(&methods_mcc);
}

BIO *BIO_new_MCC(MCCInterface* mcc) {
  BIO *ret;
  ret=BIO_new(BIO_s_MCC());
  if (ret == NULL) return(NULL);
  BIO_set_MCC(ret,mcc);
  return(ret);
}

void BIO_set_MCC(BIO* b,MCCInterface* mcc) {
  BIOMCC* biomcc = (BIOMCC*)(b->ptr);
  if(biomcc == NULL) {
    biomcc=new BIOMCC(mcc);
    b->ptr=biomcc;
  };
}

static int mcc_new(BIO *bi) {
  bi->init=1;
  bi->num=0;
  bi->ptr=NULL;
  // bi->flags=BIO_FLAGS_UPLINK; /* essentially redundant */
  return(1);
}

static int mcc_free(BIO *b) {
  if(b == NULL) return(0);
  BIOMCC* biomcc = (BIOMCC*)(b->ptr);
  b->ptr=NULL;
  if(biomcc) delete biomcc;
  return(1);
}
  
static int mcc_read(BIO *b, char *out,int outl) {
  int ret=0;
  if (out == NULL) return(ret);
  if(b == NULL) return(ret);
  BIOMCC* biomcc = (BIOMCC*)(b->ptr);
  if(biomcc == NULL) return(ret);
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream == NULL) return(ret);

  //clear_sys_error();
  bool r = stream->Get(out,outl);
  BIO_clear_retry_flags(b);
  if(r) { ret=outl; } else { ret=-1; };
  return(ret);
}

static int mcc_write(BIO *b, const char *in, int inl) {
  int ret = 0;
  //clear_sys_error();
  if(in == NULL) return(ret);
  if(b == NULL) return(ret);
  if(b->ptr == NULL) return(ret);
  BIOMCC* biomcc = (BIOMCC*)(b->ptr);
  if(biomcc == NULL) return(ret);
  MCCInterface* next = biomcc->Next();
  if(next == NULL) return(ret);
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream != NULL) {
    // If available just use stream directly
    bool r = stream->Put(in,inl);
    BIO_clear_retry_flags(b);
    if(r) { ret=inl; } else { ret=-1; };
    return(ret);
  };

  PayloadRaw nextpayload;
  nextpayload.Insert(in,0,inl); // Not efficient !!!!
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
        if(!stream) {
          biomcc->Stream(retpayload);
        };
      };
      if(stream) delete nextoutmsg.Payload();
    };
    ret=inl;
  } else { ret=-1; };
  return(ret);
}


static long mcc_ctrl(BIO *b __attribute__((unused)), int cmd,
                     long num __attribute__((unused)),
                     void *ptr __attribute__((unused))) {
  long ret=0;

  switch (cmd) {
    case BIO_CTRL_RESET:
    case BIO_CTRL_SET_CLOSE:
    case BIO_CTRL_SET:
    case BIO_CTRL_EOF:
    case BIO_CTRL_FLUSH:
    case BIO_CTRL_DUP:
      ret=1;
      break;
  };
  return(ret);
}

static int mcc_puts(BIO *bp, const char *str) {
  int n,ret;

  n=strlen(str);
  ret=mcc_write(bp,str,n);
  return(ret);
}

} // namespace Arc
