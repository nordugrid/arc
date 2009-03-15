// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/globusutils/GSSCredential.h>

#include "PayloadGSIStream.h"

namespace Arc {

  PayloadGSIStream::PayloadGSIStream(PayloadStreamInterface *stream,
                                     gss_ctx_id_t& ctx,
                                     Logger& logger,
                                     bool client)
    : timeout(60),
      stream(stream),
      buffer(NULL),
      bufferpos(0),
      bufferlen(0),
      ctx(ctx),
      logger(logger),
      client(client) {}

  PayloadGSIStream::~PayloadGSIStream() {}

  bool PayloadGSIStream::Get(char *buf, int& size) {

    if (!buffer) {
      gss_buffer_desc input_tok = GSS_C_EMPTY_BUFFER;
      gss_buffer_desc output_tok = GSS_C_EMPTY_BUFFER;

      int len = 5;
      char readbuf[5];
      stream->Get(&readbuf[0], len);

      input_tok.length = (unsigned char)readbuf[3] * 256 +
                         (unsigned char)readbuf[4] + 5;
      input_tok.value = malloc(input_tok.length);
      memcpy(input_tok.value, readbuf, 5);

      logger.msg(DEBUG, "input token length: %i", input_tok.length);

      int pos = len;
      while (input_tok.length > pos) {
        len = input_tok.length - pos;
        stream->Get(&((char*)input_tok.value)[pos], len);
        pos += len;
      }

      OM_uint32 majstat, minstat;

      if (client) {
        majstat = gss_unwrap(&minstat,
                             ctx,
                             &input_tok,
                             &output_tok,
                             NULL,
                             GSS_C_QOP_DEFAULT);

        logger.msg(INFO, "GSS unwrap: %i/%i", majstat, minstat);
      }
      else {
        majstat = gss_wrap(&minstat,
                           ctx,
                           0,
                           GSS_C_QOP_DEFAULT,
                           &input_tok,
                           NULL,
                           &output_tok);
        logger.msg(INFO, "GSS wrap: %i/%i", majstat, minstat);
      }
      if (GSS_ERROR(majstat)) {
        logger.msg(ERROR, "GSS wrap/unwrap failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
        return false;
      }

      logger.msg(DEBUG, "Output token length: %i", output_tok.length);

      bufferlen = output_tok.length;
      bufferpos = 0;
      buffer = new char[bufferlen];
      memcpy(buffer, output_tok.value, bufferlen);

      majstat = gss_release_buffer(&minstat, &input_tok);
      majstat = gss_release_buffer(&minstat, &output_tok);
    }

    if (size > bufferlen - bufferpos)
      size = bufferlen - bufferpos;

    memcpy(buf, &buffer[bufferpos], size);
    bufferpos += size;
    if (bufferpos == bufferlen) {
      delete[] buffer;
      buffer = NULL;
    }

    return true;
  }

  bool PayloadGSIStream::Get(std::string& buf) {
    char tbuf[1024];
    int l = sizeof(tbuf);
    bool result = Get(tbuf, l);
    buf.assign(tbuf, l);
    return result;
  }

} // namespace Arc
