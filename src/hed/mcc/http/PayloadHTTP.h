#ifndef __ARC_PAYLOADHTTP_H__
#define __ARC_PAYLOADHTTP_H__

#include <string>
#include <map>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <arc/Logger.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>

#define HTTP_OK           (200)
#define HTTP_BAD_REQUEST  (400)
#define HTTP_NOT_FOUND    (404)
#define HTTP_PARTIAL      (206)
#define HTTP_RANGE_NOT_SATISFIABLE (416)
#define HTTP_INTERNAL_ERR (500)
#define HTTP_NOT_IMPLEMENTED (501)

namespace ArcMCCHTTP {

using namespace Arc;

/*
PayloadHTTP
  PayloadHTTPIn 
    Stream - body
    Raw    - body
  PayloadHTTPOut
    Stream - whole
    Raw    - whole
*/



/** These classes implement parsing and generation of HTTP messages.
  They implement only subset of HTTP/1.1 and also provides PayloadRawInterface 
  and PayloadStreamInterface for including as payload into Message passed
  through MCC chains. */

class PayloadHTTP {
 protected:
  static Arc::Logger logger;
  bool valid_;                     /** set to true if object is valid */

  std::string uri_;                /** URI being contacted */
  int version_major_;              /** major number of HTTP version - must be 1 */
  int version_minor_;              /** minor number of HTTP version - must be 0 or 1 */
  std::string method_;             /** HTTP method being used or requested (if request) */
  int code_;                       /** HTTP code being sent or supplied (if response) */
  std::string reason_;             /** HTTP reason being sent or supplied (if response) */
  int64_t length_;                 /** Content-length of HTTP message */
  int64_t offset_;                 /** Logical beginning of content computed from Content-Range */
  int64_t size_;                   /** Logical size of content obtained from Content-Range */
  int64_t end_;                    /** Logical end of content computed from Content-Range */
  bool keep_alive_;                /** true if conection should not be closed after response */
  std::multimap<std::string,std::string> attributes_; /* All HTTP attributes */
  std::string error_;              /** Textual description of error which happened */

 public:
  /** Constructor - creates empty object */
  PayloadHTTP(void);
  /** Constructor - creates HTTP request. */
  PayloadHTTP(const std::string& method,const std::string& url);
  /** Constructor - creates HTTP response. */
  PayloadHTTP(int code,const std::string& reason);
  virtual ~PayloadHTTP(void);

  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };

  /** Returns HTTP header attribute with specified name.
    Empty string if no such attribute. */
  virtual const std::string& Attribute(const std::string& name);
  /** Returns all HTTP header attributes. */
  virtual const std::multimap<std::string,std::string>& Attributes(void);

  /** Returns textual description of last error */
  std::string GetError() { return error_; };
};

class PayloadHTTPIn: public PayloadHTTP, public PayloadRawInterface, public PayloadStreamInterface {
 protected:
  typedef enum {
    CHUNKED_NONE = 0,
    CHUNKED_START,
    CHUNKED_CHUNK,
    CHUNKED_END,
    CHUNKED_EOF,
    CHUNKED_ERROR
  } chunked_t;
  typedef enum {
    MULTIPART_NONE = 0,
    MULTIPART_START,
    MULTIPART_BODY,
    MULTIPART_END,
    MULTIPART_EOF,
    MULTIPART_ERROR
  } multipart_t;
  chunked_t chunked_;              /** chunked encoding parsing state */
  int64_t chunk_size_;
  multipart_t multipart_;
  std::string multipart_tag_;
  std::string multipart_buf_;
  PayloadStreamInterface* stream_; /** stream used to pull HTTP data from */
  uint64_t stream_offset_;         /** amount of data read from stream_ */
  bool stream_own_;                /** if true stream_ is owned by this */
  bool fetched_;                   /** true if whole content of HTTP body 
                                       was fetched and stored in internal
                                       buffers. Otherwise only header and
                                       part of body in tbuf_ was fetched and
                                       rest is to be read through stream_. */
  char tbuf_[1024];                /** intermediate buffer for reading header lines */
  int tbuflen_;                    /** amount of data stored in tbuf */
  char* body_;
  int64_t body_size_;

  bool readtbuf(void);
  /** Read from stream_ till \r\n */
  bool readline(std::string& line);
  bool readline_chunked(std::string& line);
  bool readline_multipart(std::string& line);

  /** Read up to 'size' bytes from stream_ */
  bool read(char* buf,int64_t& size);

  bool read_chunked(char* buf,int64_t& size);
  bool flush_chunked(void);

  char* find_multipart(char* buf,int64_t size);
  bool read_multipart(char* buf,int64_t& size);
  bool flush_multipart(void);

  /** Read HTTP header and fill internal variables */
  bool read_header(void);
  bool parse_header(void);

  /** Read Body of HTTP message and store it into internal buffer.
     Avoid using it unless really needed because it can read a lot of data.
     TODO: maybe using on disk buffer can help with GB sized bodies. */
  bool get_body(void);

 public:
  /** Constructor - creates object by parsing HTTP request or response from 'stream'.
    Supplied stream is associated with object for later use. If 'own' is set to true
    then stream will be deleted in destructor. Because stream can be used by this
    object during whole lifetime it is important not to destroy stream till this 
    object is deleted. */
  PayloadHTTPIn(PayloadStreamInterface& stream,bool own = false);

  virtual ~PayloadHTTPIn(void);

  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };

  virtual std::string Method(void) { return method_; };
  virtual std::string Endpoint(void) { return uri_; };
  virtual std::string Reason(void) { return reason_; };
  virtual int Code(void) { return code_; };
  virtual bool KeepAlive(void) { return keep_alive_; };

  // PayloadRawInterface implemented methods
  virtual char operator[](PayloadRawInterface::Size_t pos) const;
  virtual char* Content(PayloadRawInterface::Size_t pos = -1);
  virtual PayloadRawInterface::Size_t Size(void) const;
  virtual char* Insert(PayloadRawInterface::Size_t pos = 0,PayloadRawInterface::Size_t size = 0);
  virtual char* Insert(const char* s,PayloadRawInterface::Size_t pos = 0,PayloadRawInterface::Size_t size = -1);
  virtual char* Buffer(unsigned int num = 0);
  virtual PayloadRawInterface::Size_t BufferSize(unsigned int num = 0) const;
  virtual PayloadRawInterface::Size_t BufferPos(unsigned int num = 0) const;
  virtual bool Truncate(PayloadRawInterface::Size_t size);

  // PayloadStreamInterface implemented methods
  virtual bool Get(char* buf,int& size);
  virtual bool Put(const char* buf,PayloadStreamInterface::Size_t size);
  virtual int Timeout(void) const;
  virtual void Timeout(int to);
  virtual PayloadStreamInterface::Size_t Pos(void) const;
  virtual PayloadStreamInterface::Size_t Limit(void) const;

};

class PayloadHTTPOut: public PayloadHTTP {
 protected:
  bool head_response_;             /** true if HTTP response for HEAD request to be generated */

  PayloadRawInterface* rbody_;     /** associated HTTP Body buffer if any (to avoid copying to own buffer) */
  PayloadStreamInterface* sbody_;  /** associated HTTP Body stream if any (to avoid copying to own buffer) */
  PayloadStreamInterface::Size_t sbody_size_;
  bool body_own_;                  /** if true rbody_ and sbody_ is owned by this */

  std::string header_;             /** Header to be prepended to body */
  bool to_stream_;                 /** Header was generated for streaming data */

  bool use_chunked_transfer_;      /** Chunked transfer to be used */

  uint64_t stream_offset_;         /** Amount of data read read through Stream interface */

  bool make_header(bool to_stream);
  bool remake_header(bool to_stream);
  PayloadRawInterface::Size_t body_size(void) const;
  PayloadRawInterface::Size_t data_size(void) const;

 public:
  /** Constructor - creates HTTP request to be sent. */
  PayloadHTTPOut(const std::string& method,const std::string& url);
  /** Constructor - creates HTTP response to be sent.
     If 'head_response' is set to true then response is generated
     as if it is result of HEAD request. */
  PayloadHTTPOut(int code,const std::string& reason,bool head_response = false);

  virtual ~PayloadHTTPOut(void);

  /** Adds HTTP header attribute 'name' = 'value' */
  virtual void Attribute(const std::string& name,const std::string& value);

  virtual void KeepAlive(bool keep_alive) { keep_alive_=keep_alive; };

  /** Send created object through provided stream. */
  /* After this call associated stream body object usually is
    positioned at its end of data. */
  virtual bool Flush(PayloadStreamInterface& stream);
};

class PayloadHTTPOutStream: public PayloadHTTPOut, public PayloadStreamInterface {
 protected:
  //int chunk_size_get(char* buf,int size,int l,uint64_t chunk_size);
  //std::string chunk_size_str_;     /** Buffer to store chunk size */
  //std::string::size_type chunk_size_offset_; /** How much of chunk_size_str_ is sent */
  bool stream_finished_;

 public:
  PayloadHTTPOutStream(const std::string& method,const std::string& url);
  PayloadHTTPOutStream(int code,const std::string& reason,bool head_response = false);
  virtual ~PayloadHTTPOutStream(void);
  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };
  /** Assign HTTP body.
    Assigned object is not copied. Instead it is remembered and made available
    through Raw and Stream interfaces. Previously attached body is discarded.
    If 'ownership' is true then passed object is treated as being owned by
    this instance and destroyed in destructor or when discarded. */
  virtual void Body(PayloadStreamInterface& body,bool ownership = true);
  // PayloadStreamInterface implemented methods
  virtual PayloadStreamInterface::Size_t Size(void) const;
  virtual bool Get(char* buf,int& size);
  virtual bool Get(PayloadStreamInterface& dest,int& size);
  virtual bool Put(const char* buf,PayloadStreamInterface::Size_t size);
  virtual int Timeout(void) const;
  virtual void Timeout(int to);
  virtual PayloadStreamInterface::Size_t Pos(void) const;
  virtual PayloadStreamInterface::Size_t Limit(void) const;
};

class PayloadHTTPOutRaw: public PayloadHTTPOut, public PayloadRawInterface {
 public:
  PayloadHTTPOutRaw(const std::string& method,const std::string& url);
  PayloadHTTPOutRaw(int code,const std::string& reason,bool head_response = false);
  virtual ~PayloadHTTPOutRaw(void);
  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };
  /** Assign HTTP body.
    Assigned object is not copied. Instead it is remembered and made available
    through Raw and Stream interfaces. Previously attached body is discarded.
    If 'ownership' is true then passed object is treated as being owned by
    this instance and destroyed in destructor or when discarded. */
  virtual void Body(PayloadRawInterface& body,bool ownership = true);
  // PayloadRawInterface implemented methods
  virtual char operator[](PayloadRawInterface::Size_t pos) const;
  virtual char* Content(PayloadRawInterface::Size_t pos = -1);
  virtual PayloadRawInterface::Size_t Size(void) const;
  virtual char* Insert(PayloadRawInterface::Size_t pos = 0,PayloadRawInterface::Size_t size = 0);
  virtual char* Insert(const char* s,PayloadRawInterface::Size_t pos = 0,PayloadRawInterface::Size_t size = -1);
  virtual char* Buffer(unsigned int num = 0);
  virtual PayloadRawInterface::Size_t BufferSize(unsigned int num = 0) const;
  virtual PayloadRawInterface::Size_t BufferPos(unsigned int num = 0) const;
  virtual bool Truncate(PayloadRawInterface::Size_t size);
};

} // namespace ArcMCCHTTP

#endif /* __ARC_PAYLOADHTTP_H__ */
