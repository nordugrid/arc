#include <string>
#include <map>

#include "PayloadRaw.h"
#include "PayloadStream.h"

#define HTTP_OK          (200)
#define HTTP_NOT_FOUND   (404)

class PayloadHTTP: public PayloadRaw {
 protected:
  PayloadStreamInterface& stream_;
  std::string uri_;
  int version_major_;
  int version_minor_;
  std::string method_;
  int code_;
  std::string reason_;
  int length_;
  bool chunked_;
  std::map<std::string,std::string> attributes_;
  char tbuf_[1024];
  int tbuflen_;
  bool readline(std::string& line);
  bool read(char* buf,int& size);
  bool parse_header(void);
  bool get_body(void);
 public:
  // Creates object by parsing information from stream
  PayloadHTTP(PayloadStreamInterface& stream);
  // Creates HTTP request to be sent through stream
  PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream);
  // Creates HTTP response to be sent through stream
  PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream);
  virtual ~PayloadHTTP(void);
//  virtual char operator[](int pos) const;
//  virtual char* Content(int pos = -1);
//  virtual int Size(void) const;
//  virtual char* Insert(int pos = 0,int size = 0);
//  virtual char* Insert(const char* s,int pos = 0,int size = 0);
//  virtual char* Buffer(int num);
//  virtual int BufferSize(int num) const;
  virtual const std::string& Attribute(const std::string& name);
  virtual void Attribute(const std::string& name,const std::string& value);
  // Send object through associated stream
  virtual bool Flush(void);
};

/*
class PayloadHTTPRequest: public PayloadHTTP {
 public:
  PayloadHTTPRequest(PayloadStreamInterface& stream);
  virtual ~PayloadHTTPRequest(void);
};

class PayloadHTTPResponse: public PayloadHTTP {
 public:
  PayloadHTTPResponse(PayloadHTTPRequest& request);
  virtual ~PayloadHTTPResponse(void);
  virtual Flush(void);
};
*/

