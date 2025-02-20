#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>

#include <openssl/ssl.h>

#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/MCC.h>
#include <arc/message/Message.h>

#include "BIOMCC.h"

namespace ArcMCCTLS {

using namespace Arc;


class BIOMCC {
  private:
    PayloadStreamInterface* stream_;
    MCCInterface* next_;
    MCC_Status result_;
    BIO_METHOD* biom_;
    BIO* bio_; // not owned by this object
  public:
    BIOMCC(MCCInterface* next):result_(STATUS_OK) {
      next_=NULL; stream_=NULL;
      bio_ = NULL;
      if(MakeMethod()) {
        bio_ = BIO_new(biom_);
        if(bio_) {
          next_=next;
          BIO_set_data(bio_,this);
        }
      }
    };
    BIOMCC(PayloadStreamInterface* stream):result_(STATUS_OK) {
      next_=NULL; stream_=NULL;
      bio_ = NULL;
      if(MakeMethod()) {
        bio_ = BIO_new(biom_);
        if(bio_) {
          stream_=stream;
          BIO_set_data(bio_,this);
        }
      }
    };
    ~BIOMCC(void) {
      if(stream_ && next_) delete stream_;
      if(biom_) BIO_meth_free(biom_);
    };
    BIO* GetBIO() const { return bio_; };
    PayloadStreamInterface* Stream() const { return stream_; };
    void Stream(PayloadStreamInterface* stream) { stream_=stream; /*free ??*/ };
    MCCInterface* Next(void) const { return next_; };
    void MCC(MCCInterface* next) { next_=next; };
    const MCC_Status& Result(void) { return result_; }; 
  private:
    static int  mcc_write(BIO *h, const char *buf, int num);
    static int  mcc_read(BIO *h, char *buf, int size);
    static int  mcc_puts(BIO *h, const char *str);
    static long mcc_ctrl(BIO *h, int cmd, long arg1, void *arg2);
    static int  mcc_new(BIO *h);
    static int  mcc_free(BIO *data);
    bool MakeMethod(void) {
      biom_ = BIO_meth_new(BIO_TYPE_FD,"Message Chain Component");
      if(biom_) {
        (void)BIO_meth_set_write(biom_,&BIOMCC::mcc_write);
        (void)BIO_meth_set_read(biom_,&BIOMCC::mcc_read);
        (void)BIO_meth_set_puts(biom_,&BIOMCC::mcc_puts);
        (void)BIO_meth_set_ctrl(biom_,&BIOMCC::mcc_ctrl);
        (void)BIO_meth_set_create(biom_,&BIOMCC::mcc_new);
        (void)BIO_meth_set_destroy(biom_,&BIOMCC::mcc_free);
      }
      return (biom_ != NULL);
    };
};


BIO *BIO_new_MCC(MCCInterface* mcc) {
  BIOMCC* biomcc = new BIOMCC(mcc);
  BIO* bio = biomcc->GetBIO();
  if(!bio) delete biomcc;
  return bio;
}

BIO* BIO_new_MCC(PayloadStreamInterface* stream) {
  BIOMCC* biomcc = new BIOMCC(stream);
  BIO* bio = biomcc->GetBIO();
  if(!bio) delete biomcc;
  return bio;
}

int BIOMCC::mcc_new(BIO *bi) {
  BIO_set_data(bi,NULL);
  BIO_set_init(bi,1);
  return(1);
}

int BIOMCC::mcc_free(BIO *b) {
  if(b == NULL) return(0);
  BIOMCC* biomcc = (BIOMCC*)(BIO_get_data(b));
  BIO_set_data(b,NULL);
  if(biomcc) delete biomcc;
  return(1);
}
  
int BIOMCC::mcc_read(BIO *b, char *out,int outl) {
  int ret=0;
  if (out == NULL) return(ret);
  if(b == NULL) return(ret);
  BIOMCC* biomcc = (BIOMCC*)(BIO_get_data(b));
  if(biomcc == NULL) return(ret);
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream == NULL) return ret;

  //clear_sys_error();
  bool r = stream->Get(out,outl);
  BIO_clear_retry_flags(b);
  if(r) {
    ret=outl;
  } else {
    ret=-1;
    biomcc->result_ = stream->Failure();
  };
  return ret;
}

int BIOMCC::mcc_write(BIO *b, const char *in, int inl) {
  int ret = 0;
  //clear_sys_error();
  if(in == NULL) return(ret);
  if(b == NULL) return(ret);
  if(BIO_get_data(b) == NULL) return(ret);
  BIOMCC* biomcc = (BIOMCC*)(BIO_get_data(b));
  if(biomcc == NULL) return(ret);
  PayloadStreamInterface* stream = biomcc->Stream();
  if(stream != NULL) {
    // If available just use stream directly
    // TODO: check if stream has changed ???
    bool r = stream->Put(in,inl);
    BIO_clear_retry_flags(b);
    if(r) {
      ret=inl;
    } else {
      ret=-1;
      biomcc->result_ = stream->Failure();
    };
    return ret;
  };

  MCCInterface* next = biomcc->Next();
  if(next == NULL) return(ret);
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
        biomcc->Stream(retpayload);
      } else {
        delete nextoutmsg.Payload();
      };
    };
    ret=inl;
  } else {
    biomcc->result_ = mccret;
    if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
    ret=-1;
  };
  return(ret);
}


long BIOMCC::mcc_ctrl(BIO*, int cmd, long, void*) {
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

int BIOMCC::mcc_puts(BIO *bp, const char *str) {
  int n,ret;

  n=strlen(str);
  ret=mcc_write(bp,str,n);
  return(ret);
}

bool BIO_MCC_failure(BIO* bio, MCC_Status& s) {
  if(!bio) return false;
  BIOMCC* b = (BIOMCC*)(BIO_get_data(bio));
  if(!b || b->Result()) return false;
  s = b->Result();
  return true;
}

} // namespace ArcMCCTLS
