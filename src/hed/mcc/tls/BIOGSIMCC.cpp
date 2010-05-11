#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <openssl/ssl.h>

#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/MCC.h>
#include <arc/message/Message.h>

#include "BIOGSIMCC.h"

namespace Arc {

class BIOGSIMCC {
  private:
    PayloadStreamInterface* stream_;
    MCCInterface* next_;
    unsigned int header_;
    unsigned int token_;
  public:
    BIOGSIMCC(MCCInterface* next) { next_=next; stream_=NULL; header_=4; token_=0; };
    BIOGSIMCC(PayloadStreamInterface* stream) { next_=NULL; stream_=stream; header_=4; token_=0; };
    ~BIOGSIMCC(void) { if(stream_ && next_) delete stream_; };
    PayloadStreamInterface* Stream() { return stream_; };
    void Stream(PayloadStreamInterface* stream) { stream_=stream; /*free ??*/ };
    MCCInterface* Next(void) { return next_; };
    void MCC(MCCInterface* next) { next_=next; };
    int Header(void) { return header_; };
    void Header(int v) { header_=v; };
    int Token(void) { return token_; };
    void Token(int v) { token_=v; };
};

static int  mcc_write(BIO *h, const char *buf, int num);
static int  mcc_read(BIO *h, char *buf, int size);
static int  mcc_puts(BIO *h, const char *str);
static long mcc_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int  mcc_new(BIO *h);
static int  mcc_free(BIO *data);

static void BIO_set_GSIMCC(BIO* b,MCCInterface* mcc);
static void BIO_set_GSIMCC(BIO* b,PayloadStreamInterface* stream);

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

BIO_METHOD *BIO_s_GSIMCC(void) {
  return(&methods_mcc);
}

BIO *BIO_new_GSIMCC(MCCInterface* mcc) {
  BIO *ret;
  ret=BIO_new(BIO_s_GSIMCC());
  if (ret == NULL) return(NULL);
  BIO_set_GSIMCC(ret,mcc);
  return(ret);
}

BIO* BIO_new_GSIMCC(PayloadStreamInterface* stream) {
  BIO *ret;
  ret=BIO_new(BIO_s_GSIMCC());
  if (ret == NULL) return(NULL);
  BIO_set_GSIMCC(ret,stream);
  return(ret);
}

static void BIO_set_GSIMCC(BIO* b,MCCInterface* mcc) {
  BIOGSIMCC* biomcc = (BIOGSIMCC*)(b->ptr);
  if(biomcc == NULL) {
    biomcc=new BIOGSIMCC(mcc);
    b->ptr=biomcc;
  };
}

static void BIO_set_GSIMCC(BIO* b,PayloadStreamInterface* stream) {
  BIOGSIMCC* biomcc = (BIOGSIMCC*)(b->ptr);
  if(biomcc == NULL) {
    biomcc=new BIOGSIMCC(stream);
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
  BIOGSIMCC* biomcc = (BIOGSIMCC*)(b->ptr);
  b->ptr=NULL;
  if(biomcc) delete biomcc;
  return(1);
}
  
static int mcc_read(BIO *b, char *out,int outl) {
  int ret=0;
  if (out == NULL) return(ret);
  if(b == NULL) return(ret);
  BIOGSIMCC* biomcc = (BIOGSIMCC*)(b->ptr);
  if(biomcc == NULL) return(ret);
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream == NULL) return ret;
  bool r = true;
  if(biomcc->Header()) {
    unsigned char header[4];
    int l = biomcc->Header();
    r = stream->Get((char*)(header+(4-l)),l);
    if(r) {
      for(int n = (4-biomcc->Header());n<(4-biomcc->Header()+l);++n) {
        biomcc->Token(biomcc->Token() | (header[n] << ((3-n)*8)));
      };
      biomcc->Header(biomcc->Header()-l);
    };
  };
  if(r) {
    if(biomcc->Header() == 0) {
      if(biomcc->Token()) {
        int l = biomcc->Token();
        if(l > outl) l=outl;
        r = stream->Get(out,l);
        if(r) {
          biomcc->Token(biomcc->Token() - l);
          outl = l;
        };
      } else {
        outl=0;
      };
      if(biomcc->Token() == 0) biomcc->Header(4);
    };
  };
  //clear_sys_error();
  BIO_clear_retry_flags(b);
  if(r) { ret=outl; } else { ret=-1; };
  return ret;
}

static int mcc_write(BIO *b, const char *in, int inl) {
  int ret = 0;
  //clear_sys_error();
  if(in == NULL) return(ret);
  if(b == NULL) return(ret);
  if(b->ptr == NULL) return(ret);
  BIOGSIMCC* biomcc = (BIOGSIMCC*)(b->ptr);
  if(biomcc == NULL) return(ret);
  unsigned char header[4];
  header[0]=(inl>>24)&0xff;
  header[1]=(inl>>16)&0xff;
  header[2]=(inl>>8)&0xff;
  header[3]=(inl>>0)&0xff;
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream != NULL) {
    // If available just use stream directly
    // TODO: check if stream has changed ???
    bool r;
    r = stream->Put((const char*)header,4);
    if(r) r = stream->Put(in,inl);
    BIO_clear_retry_flags(b);
    if(r) { ret=inl; } else { ret=-1; };
    return ret;
  };

  MCCInterface* next = biomcc->Next();
  if(next == NULL) return(ret);
  PayloadRaw nextpayload;
  nextpayload.Insert((const char*)header,0,4);
  nextpayload.Insert(in,4,inl); // Not efficient !!!!
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
        biomcc->Stream(retpayload);
      } else {
        delete nextoutmsg.Payload();
      };
    };
    ret=inl;
  } else {
    if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
    ret=-1;
  };
  return(ret);
}


static long mcc_ctrl(BIO*, int cmd, long, void*) {
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
