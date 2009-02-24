#ifndef __ARC_PAYLOADHTTP_H__
#define __ARC_PAYLOADHTTP_H__

#include <string>
#include <map>

#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>

#define HTTP_OK           (200)
#define HTTP_BAD_REQUEST  (400)
#define HTTP_NOT_FOUND    (404)
#define HTTP_PARTIAL      (206)
#define HTTP_INTERNAL_ERR (500)

namespace Arc {

/** This class implements parsing and generation of HTTP messages.
  It implements only subset of HTTP/1.1 and also provides an PayloadRawInterface 
  for including as payload into Message passed through MCC chains. */
class PayloadHTTP: virtual public PayloadRaw, virtual public PayloadStreamInterface {
 protected:
  bool valid_;
  bool fetched_;                   /** true if whole content of HTTP body 
                                       was fetched and stored in buffers. 
                                       Otherwise only header was fetched and
                                       part of body in tbuf_ and  rest is to
                                       be read through stream_. */
  PayloadStreamInterface* stream_; /** stream used to comminicate to outside */
  bool stream_own_;                /** if true stream_ is owned by this */
  PayloadRawInterface* body_;      /** associated HTTP Body if any (to avoid copying to own buffer) */
  bool body_own_;                  /** if true body_ is owned by this */
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
  uint64_t stream_offset_;
  int chunked_size_;
  int chunked_offset_;
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
    Supplied stream is associated with object for later use. If own is set to true
    then stream will be deleted in destructor. Because stream can be used by this
    object during whole lifetime it is important not to destroy stream till this 
    object is deleted. */
  PayloadHTTP(PayloadStreamInterface& stream,bool own = false);

  /** Constructor - creates HTTP request to be sent through stream.
    HTTP message is not sent yet. */
  PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream);
  /** Constructor - creates HTTP request to be rendered through Raw interface.  */
  PayloadHTTP(const std::string& method,const std::string& url);

  /** Constructor - creates HTTP response to be sent through stream.
    HTTP message is not sent yet. */
  PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream);
  /** Constructor - creates HTTP response to be rendered through Raw interface. */
  PayloadHTTP(int code,const std::string& reason);

  virtual ~PayloadHTTP(void);

  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };

  /** Returns HTTP header attribute with specified name.
    Empty string if no such attribute. */
  virtual const std::string& Attribute(const std::string& name);
  /** Returns all HTTP header attributes. */
  virtual const std::map<std::string,std::string>& Attributes(void);
  /** Sets HTTP header attribute 'name' to 'value' */
  virtual void Attribute(const std::string& name,const std::string& value);
  /** Send created object through associated stream. If there is no stream associated then
    HTTP specific data is inserted into Raw buffers of this object. In last case 
    this operation should not be repeated till content of buffer is completely
    rewritten. */
  virtual bool Flush(void);
  virtual std::string Method() { return method_; };
  virtual std::string Endpoint() { return uri_; };
  virtual std::string Reason() { return reason_; };
  virtual int Code() { return code_; };
  virtual bool KeepAlive(void) { return keep_alive_; };
  virtual void KeepAlive(bool keep_alive) { keep_alive_=keep_alive; };
  /** Assign HTTP body.
    Assigned object is not copied. Instead it is remembered and made available
    through Raw interface. If 'ownership' is true then passed object
    is treated as being owned by this instance and destroyed in destructor. */
  virtual void Body(PayloadRawInterface& body,bool ownership = true);

  // PayloadRawInterface reimplemented methods
  virtual char operator[](int pos) const;
  virtual char* Content(int pos = -1);
  virtual int Size(void) const;
  virtual char* Insert(int pos = 0,int size = 0);
  virtual char* Insert(const char* s,int pos = 0,int size = -1);
  virtual char* Buffer(unsigned int num = 0);
  virtual int BufferSize(unsigned int num = 0) const;
  virtual int BufferPos(unsigned int num = 0) const;
  virtual bool Truncate(unsigned int size);

  // PayloadStreamInterface implemented methods
  virtual bool Get(char* buf,int& size);
  virtual bool Get(std::string& buf);
  virtual std::string Get(void);
  virtual bool Put(const char* buf,int size);
  virtual bool Put(const std::string& buf);
  virtual bool Put(const char* buf);
  virtual int Timeout(void) const;
  virtual void Timeout(int to);
  virtual int Pos(void) const;

};

} // namespace Arc

#endif /* __ARC_PAYLOADHTTP_H__ */
