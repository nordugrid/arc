#include <string>
#include <map>

#include "PayloadRaw.h"
#include "PayloadStream.h"

#define HTTP_OK          (200)
#define HTTP_NOT_FOUND   (404)

class DataPayloadHTTP: public DataPayloadRaw {
 protected:
  DataPayloadStreamInterface& stream_;
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
  DataPayloadHTTP(DataPayloadStreamInterface& stream);
  // Creates HTTP request to be sent through stream
  DataPayloadHTTP(const std::string& method,const std::string& url,DataPayloadStreamInterface& stream);
  // Creates HTTP response to be sent through stream
  DataPayloadHTTP(int code,const std::string& reason,DataPayloadStreamInterface& stream);
  virtual ~DataPayloadHTTP(void);
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
class DataPayloadHTTPRequest: public DataPayloadHTTP {
 public:
  DataPayloadHTTPRequest(DataPayloadStreamInterface& stream);
  virtual ~DataPayloadHTTPRequest(void);
};

class DataPayloadHTTPResponse: public DataPayloadHTTP {
 public:
  DataPayloadHTTPResponse(DataPayloadHTTPRequest& request);
  virtual ~DataPayloadHTTPResponse(void);
  virtual Flush(void);
};
*/

