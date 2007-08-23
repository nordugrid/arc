#ifndef __ARC_PAYLOADHTTP_H__
#define __ARC_PAYLOADHTTP_H__

#include <string>
#include <map>

#include "../../libs/message/PayloadRaw.h"
#include "../../libs/message/PayloadStream.h"

#define HTTP_OK          (200)
#define HTTP_NOT_FOUND   (404)

namespace Arc {

/** This class implements parsing and generation of HTTP messages.
  It implements only subset of HTTP/1.1 and also provides an PayloadRawInterface 
  for including as payload into Message passed through MCC chains. */
class PayloadHTTP: public PayloadRaw {
 protected:
  bool valid_;
  PayloadStreamInterface& stream_; /** stream used to comminicate to outside */
  std::string uri_;                /** URI being contacted */
  int version_major_;              /** major number of HTTP version - must be 1 */
  int version_minor_;              /** minor number of HTTP version - must be 0 or 1 */
  std::string method_;             /** HTTP method being used or requested */
  int code_;                       /** HTTP code being sent or supplied */
  std::string reason_;             /** HTTP reason being sent or supplied */
  int length_;                     /** Content-length of HTTP message */
  //int offset_;                   /** Logical beginning of content computed from Content-Range */
  //int size_;                     /** Logical size of content obtained from Content-Range */
  bool chunked_;                   /** true if content is chunked */
  bool keep_alive_;                /** true if conection should not be closed after response */
  std::map<std::string,std::string> attributes_; /* All HTTP attributes */
  char tbuf_[1024];
  int tbuflen_;
  /** Read from stream till \r\n */
  bool readline(std::string& line);
  /** Read up to 'size' bytes from stream_ */
  bool read(char* buf,int& size);
  /** Read HTTP header and fill internal variables */
  bool parse_header(void);
  /** Read Body of HTTP message and attach it to inherited PayloadRaw object */
  bool get_body(void);
 public:
  /** Constructor - creates object by parsing HTTP request or response from stream.
    Supplied stream is associated with object for later use. */
  PayloadHTTP(PayloadStreamInterface& stream);
  /** Constructor - creates HTTP request to be sent through stream.
    HTTP message is not sent yet. */
  PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream);
  /** Constructor - creates HTTP response to be sent through stream.
    HTTP message is not sent yet. */
  PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream);
  /** Constructor - creates HTTP request to be rendered through Raw interface.  */
  PayloadHTTP(const std::string& method,const std::string& url);
  /** Constructor - creates HTTP response to be rendered through Raw interface. */
  PayloadHTTP(int code,const std::string& reason);
  virtual ~PayloadHTTP(void);
  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };
//  virtual char operator[](int pos) const;
//  virtual char* Content(int pos = -1);
//  virtual int Size(void) const;
//  virtual char* Insert(int pos = 0,int size = 0);
//  virtual char* Insert(const char* s,int pos = 0,int size = 0);
//  virtual char* Buffer(int num);
//  virtual int BufferSize(int num) const;
  /** Returns HTTP header attribute with specified name.
    Empty string if no such attribute. */
  virtual const std::string& Attribute(const std::string& name);
  /** Returns HTTP all header attributes. */
  virtual const std::map<std::string,std::string>& Attributes(void);
  /** Sets HTTP header attribute 'name' to 'value' */
  virtual void Attribute(const std::string& name,const std::string& value);
  /** Send created object through associated stream. If there is no stream associated then
    HTTP specific data is inserted into Raw buffers of this object. */
  virtual bool Flush(void);
  virtual std::string Method() { return method_; };
  virtual std::string Endpoint() { return uri_; };
  virtual std::string Reason() { return reason_; };
  virtual int Code() { return code_; };
};

} // namespace Arc

#endif /* __ARC_PAYLOADHTTP_H__ */
