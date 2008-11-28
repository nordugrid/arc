#ifndef __ARC_PAYLOADGSISTREAM_H__
#define __ARC_PAYLOADGSISTREAM_H__

#include <unistd.h>
#include <string>

#include <arc/message/PayloadStream.h>
#include <arc/Logger.h>

#include <gssapi.h>

namespace Arc {

  class PayloadGSIStream
    : public PayloadStreamInterface {
  public:
    PayloadGSIStream(PayloadStreamInterface *stream,
		     gss_ctx_id_t& ctx,
		     Logger& logger,
		     bool client);
    virtual ~PayloadGSIStream();

    virtual bool Get(char *buf, int& size);

    virtual bool Get(std::string& buf);

    virtual std::string Get() {
      std::string buf;
      Get(buf);
      return buf;
    }

    virtual bool Put(const char *buf, int size) {}

    virtual bool Put(const std::string& buf) {
      return Put(buf.c_str(), buf.length());
    }

    virtual bool Put(const char *buf) {
      return Put(buf, buf ? strlen(buf) : 0);
    }

    virtual operator bool() {
      return (ctx != GSS_C_NO_CONTEXT);
    }

    virtual bool operator!() {
      return (ctx == GSS_C_NO_CONTEXT);
    }

    virtual int Timeout() const {
      return timeout;
    }

    virtual void Timeout(int to) {
      timeout = to;
    }

    virtual int Pos() const {
      return 0;
    }

  protected:
    int timeout;
    PayloadStreamInterface *stream;
    char *buffer;
    int bufferpos;
    int bufferlen;
    gss_ctx_id_t& ctx;
    Logger& logger;
    bool client;
  };

} // namespace Arc

#endif /* __ARC_PAYLOADGSISTREAM_H__ */
