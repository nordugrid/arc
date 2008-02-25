#include <sys/stat.h>
#include <sys/types.h>
#include <list>
#include <fstream>
#include <arc/URL.h>
#include <arc/misc/ClientInterface.h>

static bool html_to_list(const char* html,const Arc::URL base,std::list<Arc::URL>& urls) {
  for(const char* pos = html;;) {
    // Looking for tag
    const char* tag_start = strchr(pos,'<');
    if(!tag_start) break; // No more tags
    // Looking for end of tag
    const char* tag_end = strchr(tag_start+1,'>');
    if(!tag_end) return false; // Broken html?
    // 'A' tag?
    if(strcasecmp(tag_start,"<A ") == 0) {
      // Lookig for HREF
      const char* href = strstr(tag_start+3,"href=");
      if(!href) href=strstr(tag_start+3,"HREF=");
      if(href) {
        const char* url_start = href+5;
        const char* url_end = NULL;
        if((*url_start) == '"') {
          ++url_start;
          url_end=strchr(url_start,'"');
          if((!url_end) || (url_end > tag_end)) url_end=NULL;
        } else if((*url_start) == '\'') {
          url_end=strchr(url_start,'\'');
          if((!url_end) || (url_end > tag_end)) url_end=NULL;
        } else {
          url_end=strchr(url_start,' ');
          if((!url_end) || (url_end > tag_end)) url_end=tag_end;
        };
        if(!url_end) return false; // Broken HTML
        std::string url_str(url_start,url_end-url_start);
        Arc::URL url(url_str);
        if(!url) return false; // Bad URL
        urls.push_back(url);
      };
    };
    pos=tag_end+1;
  };
  return true;
}

static bool PayloadRaw_to_string(Arc::PayloadRawInterface& buf,std::string& str,uint64_t& end) {
  end=0;
  for(int n = 0;;++n) {
    const char* content = buf.Buffer(n);
    if(!content) break;
    int size = buf.BufferSize(n);
    if(size > 0) {
      str.append(content,size);
    };
    end=buf.BufferPos(n)+size;
  };
  return true;
}

static bool PayloadRaw_to_stream(Arc::PayloadRawInterface& buf,std::ostream& o,uint64_t& end) {
  end=0;
  for(int n = 0;;++n) {
    const char* content = buf.Buffer(n);
    if(!content) break;
    uint64_t size = buf.BufferSize(n);
    uint64_t pos = buf.BufferPos(n);
    if(size > 0) {
      o.seekp(pos);
      o.write(content,size);
      if(!o) return false;
    };
    end=pos+size;
  };
  return true;
}

// TODO: limit recursion depth
static bool get_file(Arc::ClientHTTP& client,const Arc::URL& url,const std::string& dir) {
  // Read file in chunks. Use first chunk to determine type of file.
  Arc::PayloadRaw req; // Empty request body
  Arc::PayloadRawInterface* resp;
  Arc::HTTPClientInfo info;
  const uint64_t chunk_len = 1024*1024*1024; // Some reasonable size;
  uint64_t chunk_start = 0;
  uint64_t chunk_end = chunk_len;
  // First chunk
  Arc::MCC_Status r = client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
  if(!r) return false;
  if(!resp) return false;
  if(strcasecmp(info.type.c_str(),"text/html") == 0) {
    std::string html;
    uint64_t file_size = resp->Size();
    PayloadRaw_to_string(*resp,html,chunk_end);
    delete resp;
    // Fetch whole html
    for(;chunk_end<file_size;) {
      chunk_start=chunk_end;
      chunk_end=chunk_start+chunk_len;
      r=client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
      if(!r) return false;
      if(!resp) return false;
      if(resp->Size() <= 0) break;
      PayloadRaw_to_string(*resp,html,chunk_end);
      delete resp; resp=NULL;
    };
    if(resp) delete resp;
    // Make directory
    if(::mkdir(dir.c_str(),S_IRWXU) != 0) return false;
    // Fetch files
    std::list<Arc::URL> urls;
    if(!html_to_list(html.c_str(),url,urls)) return false;
    for(std::list<Arc::URL>::iterator u = urls.begin();u!=urls.end();++u) {
      if(!get_file(client,*u,dir+(u->Path().substr(url.Path().length())))) return false;
    };
    return true;
  };
  // Start storing file
  uint64_t file_size = resp->Size();
  std::ofstream f(dir.c_str(),std::ios::trunc);
  if(!f) { delete resp; return false; };
  if(!PayloadRaw_to_stream(*resp,f,chunk_end)) { delete resp; return false; };
  delete resp; resp=NULL;
  // Continue fetching file
  for(;chunk_end<file_size;) {
    chunk_start=chunk_end;
    chunk_end=chunk_start+chunk_len;
    r=client.process("GET",url.Path(),chunk_start,chunk_end,&req,&info,&resp);
    if(!r) return false;
    if(!resp) return false;
    if(resp->Size() <= 0) break;
    if(!PayloadRaw_to_stream(*resp,f,chunk_end)) { delete resp; return false; };
    delete resp; resp=NULL;
  };
  if(resp) delete resp;
  return true;
}

int main(void) {
  return 0;
}

