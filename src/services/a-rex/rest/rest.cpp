#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/URL.h>
#include <arc/FileUtils.h>
#include <arc/Utils.h>

#include "../job.h"
#include "../PayloadFile.h"
#include "../FileChunks.h"
#include "../delegation/DelegationStores.h"
#include "../grid-manager/files/ControlFileHandling.h"

#include "rest.h"

namespace Arc {
  std::list< std::pair<std::string,int> >::iterator FindFirst(std::list< std::pair<std::string,int> >::iterator first, std::list< std::pair<std::string,int> >::iterator last, std::string const & str) {
    while (first != last) {
      if (first->first == str) return first;
      ++first;
    }
    return last;
  }
}

using namespace ARex;
using namespace Arc;

enum ResponseFormat {
    ResponseFormatHtml,
    ResponseFormatXml,
    ResponseFormatJson
};

static void RenderToJson(Arc::XMLNode xml, std::string& output, char const * array_paths[], int depth = 0) {
    if(xml.Size() == 0) {
	// Either it is a value or it has forced array sub-elements.
	bool has_arrays = false;
        if(array_paths) {
            for(int idx = 0; array_paths[idx]; ++idx) {
	        char const * array_path = array_paths[idx];
	        char const * sep = strchr(array_path, '/');
	        if(sep) continue; // not last element of path
		has_arrays = true;
		break;
	    }
	}
        if(!has_arrays) {
            std::string val = json_encode((std::string)xml);
            if((depth != 0) || (!val.empty())) {
                output += "\"";
                output += val;
                output += "\"";
            }
            return;
	}
    }
    output += "{";
    // Because JSON does not allow for same key we must first
    // group XML elements by names. Using list to preserve order
    // in which elements appear.
    std::list< std::pair<std::string,int> > names;
    for(int n = 0; ; ++n) {
        XMLNode child = xml.Child(n);
        if(!child) break;
        std::string name = child.Name();
        std::list< std::pair<std::string,int> >::iterator nameIt = FindFirst(names.begin(),names.end(),name);
        if(nameIt == names.end())
            names.push_back(std::make_pair(name,1));
        else
            ++(nameIt->second); 
    }
    // Check for forced JSON arrays
    if(array_paths) {
        for(int idx = 0; array_paths[idx]; ++idx) {
	    char const * array_path = array_paths[idx];
	    char const * sep = strchr(array_path, '/');
	    if(sep) continue; // not last element of path
	    // Add fake 2 elements to make them arrays later
            std::list< std::pair<std::string,int> >::iterator nameIt = FindFirst(names.begin(),names.end(),array_path);
            if(nameIt == names.end())
                names.push_back(std::make_pair(array_path,2));
            else
                nameIt->second += 2; 
        }
    }
    bool newElement = true;
    for(std::list< std::pair<std::string,int> >::iterator nameIt = names.begin(); nameIt != names.end(); ++nameIt) {
        if(!newElement) output += ",";
        newElement = false;
        output += "\"";
        output += nameIt->first;
        output += "\"";
        output += ":";
        XMLNode child = xml[nameIt->first.c_str()];
        if(child) {
            std::vector<char const *> new_array_paths;
            if(array_paths) {
                for(int idx = 0; array_paths[idx]; ++idx) {
	            char const * array_path = array_paths[idx];
		    if(strncmp(nameIt->first.c_str(), array_path, nameIt->first.length()) == 0) {
		        if(array_path[nameIt->first.length()] == '/') {
                            new_array_paths.push_back(array_path+(nameIt->first.length()+1));
			}
		    }
                }
            }
            if(nameIt->second == 1) {
                RenderToJson(child, output, new_array_paths.empty() ? NULL : &(new_array_paths[0]), depth+1);
            } else {
                output += "[";
                bool newItem = true;
                while(child) {
                    if(!newItem) output += ",";
                    newItem = false;
                    RenderToJson(child, output, new_array_paths.empty() ? NULL : &(new_array_paths[0]), depth+1);
                    ++child;
                }
                output += "]";
            }
        } else {
	    // Must be forced array element
            output += "[]";
	}
    }
    // Hope no attributes with same name
    if(xml.AttributesSize() > 0) {
        if(!newElement) output += ",";
        output += "\"_attributes\":{";
        for(int n = 0; ; ++n) {
            XMLNode child = xml.Attribute(n);
            if (!child) break;
            if(n != 0) output += ",";
            std::string val = json_encode((std::string)xml);
            output += "\"";
            output += child.Name();
            output += "\":\"";
            output += val;
            output += "\"";
        }
        output += "}";
    } 
    output += "}";
}

static void RenderToHtml(Arc::XMLNode xml, std::string& output, int depth = 0) {
    if(depth == 0) {
        output += "<HTML><HEAD>";
        output += xml.Name();
        output += "</HEAD><BODY>";
    }

    if(xml.Size() == 0) {
        output += (std::string)xml;
    } else {
        output += "<table border=\"1\">";
        for(int n = 0; ; ++n) {
            XMLNode child = xml.Child(n);
            if (!child) break;
            output += "<tr><td>";
            output += child.Name();
            output += "</td><td>";
            RenderToHtml(child, output, depth+1);
            output += "</td></tr>";
        }
        output += "</table>";
    }

    if(depth == 0) {
        output += "</BODY></HTML>";
    }
}

static void RenderToXml(Arc::XMLNode xml, std::string& output, int depth = 0) {
    xml.GetXML(output, "utf-8");
}


static char const * SkipWS(char const * input) {
    while(*input) {
        if(!std::isspace(*input))
            break;
        ++input;
    }
    return input;
}

static char const * SkipTo(char const * input, char tag) {
    while(*input) {
        if(*input == tag)
            break;
        ++input;
    }
    return input;
}

static char const * SkipToEscaped(char const * input, char tag) {
    while(*input) {
        if(*input == '\\') {
            ++input;
            if(!*input) break;
        } else if(*input == tag) {
            break;
        }
        ++input;
    }
    return input;
}

static char const * ParseFromJson(Arc::XMLNode& xml, char const * input, int depth = 0) {
    input = SkipWS(input);
    if(!*input) return input;
    if(*input == '{') {
        // complex item
        ++input;
        char const * nameStart = SkipWS(input);
        if(*nameStart != '}') while(true) {
            if(*nameStart != '"') return NULL;
            ++nameStart;
            char const * nameEnd = SkipToEscaped(nameStart, '"');
            if(*nameEnd != '"') return NULL;
            char const * sep = SkipWS(nameEnd+1);
            if(*sep != ':') return NULL;
            XMLNode item = xml.NewChild(json_unencode(std::string(nameStart, nameEnd-nameStart)));
            input = sep+1;
            input = ParseFromJson(item,input,depth+1);
            if(!input) return NULL;
            input = SkipWS(input);
            if(*input == ',') {
                // next element
                ++input;
            } else if(*input == '}') {
                // last element
                break;
            } else {
                return NULL;
            };
        };
        ++input;
    } else if(*input == '[') {
        ++input;
        // array
        char const * nameStart = SkipWS(input);
        XMLNode item = xml;
        if(*nameStart != ']') while(true) {
            input = ParseFromJson(item,input,depth+1);
            if(!input) return NULL;
            input = SkipWS(input);
            if(*input == ',') {
                // next element
                ++input;
                item = xml.Parent().NewChild(item.Name());
            } else if(*input == ']') {
                // last element
                item = xml.Parent().NewChild(item.Name()); // It will be deleted outside loop
                break;
            } else {
                return NULL;
            };
        };
        item.Destroy();
        ++input;
    } else if(*input == '"') {
        ++input;
        // string
        char const * strStart = input;
        input = SkipToEscaped(strStart, '"');
        if(*input != '"') return NULL;
        xml = json_unencode(std::string(strStart, input-strStart));
        ++input;
    // } else if((*input >= '0') && (*input <= '9')) {
    // } else if(*input == 't') {
    // } else if(*input == 'f') {
    // } else if(*input == 'n') {
    } else {
        ++input;
        // true, false, null, number
        char const * strStart = input;
        while(*input) {
            if((*input == ',') || (*input == '}') || (*input == ']') || (std::isspace(*input)))
                break;
            ++input;
        }
        xml = std::string(strStart, input-strStart);
    };
    return input;
}

static void RenderResponse(Arc::XMLNode xml, ResponseFormat format, std::string& output, char const * json_arrays[]) {
    switch(format) {
        case ResponseFormatXml:
            RenderToXml(xml, output);
            break;
        case ResponseFormatHtml:
            RenderToHtml(xml, output);
            break;
        case ResponseFormatJson:
            RenderToJson(xml, output, json_arrays);
            break;
        default:
            break;
    }
}

static void ExtractRange(Arc::Message& inmsg, off_t& range_start, off_t& range_end) {
  range_start = 0;
  range_end = (off_t)(-1);
  {
    std::string val;
    val=inmsg.Attributes()->get("HTTP:RANGESTART");
    if(!val.empty()) {
      // Negative ranges not supported
      if(!Arc::stringto<off_t>(val,range_start)) {
        range_start=0;
      } else {
        val=inmsg.Attributes()->get("HTTP:RANGEEND");
        if(!val.empty()) {
          if(!Arc::stringto<off_t>(val,range_end)) {
            range_end=(off_t)(-1);
          } else {
            // Rest of code here treats end of range as exclusive
            // While HTTP ranges are inclusive
            ++range_end;
          };
        };
      };
    };
  };
}


std::string ARexRest::ProcessingContext::operator[](char const * key) const {
  if(!key)
    return "";
  std::multimap<std::string,std::string>::const_iterator it = query.find(key);
  if(it == query.end())
    return "";
  return it->second;
}

static Arc::MCC_Status extract_content(Arc::Message& inmsg,std::string& content,uint32_t size_limit) {
  // Identify payload
  Arc::MessagePayload* payload = inmsg.Payload();
  if(!payload) {
    return Arc::MCC_Status(Arc::GENERIC_ERROR,"","Missing payload");
  };
  Arc::PayloadStreamInterface* stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
  Arc::PayloadRawInterface* buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
  if((!stream) && (!buf)) {
    return Arc::MCC_Status(Arc::GENERIC_ERROR,"","Error processing payload");
  }
  // Fetch content
  content.clear();
  if(stream) {
    std::string add_str;
    while(stream->Get(add_str)) {
      content.append(add_str);
      if((size_limit != 0) && (content.size() >= size_limit)) break;
    }
  } else {
    for(unsigned int n = 0;buf->Buffer(n);++n) {
      content.append(buf->Buffer(n),buf->BufferSize(n));
      if((size_limit != 0) && (content.size() >= size_limit)) break;
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

// Strip first token from path delimited by /.
static bool GetPathToken(std::string& subpath, std::string& token) {
  std::string::size_type token_start = 0;
  while(subpath[token_start] == '/') ++token_start;
  std::string::size_type token_end = token_start;
  while((token_end < subpath.length()) && (subpath[token_end] != '/')) ++token_end;
  if (token_start == token_end)
    return false;
  token = subpath.substr(token_start, token_end-token_start);
  while(subpath[token_end] == '/') ++token_end;
  subpath.erase(0, token_end);
  return true;
}

static std::string StripNewLine(char const * str) {
  std::string res(str);
  for(std::string::size_type pos = res.find_first_of("\r\n"); pos != std::string::npos; pos = res.find_first_of("\r\n",pos)) {
    res[pos] = ' ';
  }
  return res;
}

// Insert generic (error) HTTP response into outmsg.
static Arc::MCC_Status HTTPFault(Arc::Message& inmsg, Arc::Message& outmsg,int code,const char* resp,const char* msg = NULL) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  if(msg && *msg) {
    outpayload->Insert(msg);
    outmsg.Attributes()->set("HTTP:Content-Type","text/plain");
  }
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE",Arc::tostring(code));
  if(resp) outmsg.Attributes()->set("HTTP:REASON",StripNewLine(resp));
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPResponse(Arc::Message& inmsg, Arc::Message& outmsg) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE","200");
  outmsg.Attributes()->set("HTTP:REASON","OK");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPResponse(Arc::Message& inmsg, Arc::Message& outmsg,
                                   std::string const & content, std::string const& mime) {
  if(inmsg.Attributes()->get("HTTP:METHOD") == "HEAD") {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    if(outpayload) outpayload->Truncate(content.length());
    delete outmsg.Payload(outpayload);
  } else {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    if(outpayload) outpayload->Insert(content.c_str(),0,content.length());
    delete outmsg.Payload(outpayload);
  }
  outmsg.Attributes()->set("HTTP:CODE","200");
  outmsg.Attributes()->set("HTTP:REASON","OK");
  outmsg.Attributes()->set("HTTP:content-type",mime);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPResponseFile(Arc::Message& inmsg, Arc::Message& outmsg,
                                        int& fileHandle, std::string const& mime) {
  if(inmsg.Attributes()->get("HTTP:METHOD") == "HEAD") {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    struct stat st;
    if(outpayload && (::fstat(fileHandle,&st) == 0)) outpayload->Truncate(st.st_size);
    delete outmsg.Payload(outpayload);
  } else {
    off_t range_start = 0;
    off_t range_end = 0;
    ExtractRange(inmsg, range_start, range_end);
    Arc::MessagePayload* outpayload = newFileRead(fileHandle,range_start,range_end);
    delete outmsg.Payload(outpayload);
    fileHandle = -1;
  }
  outmsg.Attributes()->set("HTTP:CODE","200");
  outmsg.Attributes()->set("HTTP:REASON","OK");
  outmsg.Attributes()->set("HTTP:content-type",mime);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPResponseFile(Arc::Message& inmsg, Arc::Message& outmsg,
                                        Arc::FileAccess*& fileHandle, std::string const& mime) {
  if(inmsg.Attributes()->get("HTTP:METHOD") == "HEAD") {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    struct stat st;
    if(outpayload && fileHandle->fa_fstat(st)) outpayload->Truncate(st.st_size);
    delete outmsg.Payload(outpayload);
  } else {
    off_t range_start;
    off_t range_end;
    ExtractRange(inmsg, range_start, range_end);
    Arc::MessagePayload* outpayload = newFileRead(fileHandle,range_start,range_end);
    delete outmsg.Payload(outpayload);
    fileHandle = NULL;
  }
  outmsg.Attributes()->set("HTTP:CODE","200");
  outmsg.Attributes()->set("HTTP:REASON","OK");
  outmsg.Attributes()->set("HTTP:content-type",mime);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPDELETEResponse(Arc::Message& inmsg, Arc::Message& outmsg, bool queued = false) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  if(queued) {
    outmsg.Attributes()->set("HTTP:CODE","204");
    outmsg.Attributes()->set("HTTP:REASON","No Content");
  } else {
    outmsg.Attributes()->set("HTTP:CODE","202");
    outmsg.Attributes()->set("HTTP:REASON","Accepted");
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPPOSTDelayedResponse(Arc::Message& inmsg, Arc::Message& outmsg) {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE","202");
  outmsg.Attributes()->set("HTTP:REASON","Queued");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPPOSTResponse(Arc::Message& inmsg, Arc::Message& outmsg,
                                   std::string const & redir = "") {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE","201");
  outmsg.Attributes()->set("HTTP:REASON","Created");
  if(!redir.empty()) outmsg.Attributes()->set("HTTP:location",redir);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPPOSTResponse(Arc::Message& inmsg, Arc::Message& outmsg,
                                   std::string const & content, std::string const& mime,
                                   std::string const & redir = "") {
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  if(outpayload) outpayload->Insert(content.c_str(),0,content.length());
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE","201");
  outmsg.Attributes()->set("HTTP:REASON","Created");
  outmsg.Attributes()->set("HTTP:content-type",mime);
  if(!redir.empty()) outmsg.Attributes()->set("HTTP:location",redir);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static ResponseFormat ProcessAcceptedFormat(Arc::Message& inmsg, Arc::Message& outmsg) {
  // text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, */*;q=0.8
  std::list<std::string> accepts;
  for(Arc::AttributeIterator attrIt = inmsg.Attributes()->getAll("HTTP:accept"); attrIt.hasMore(); ++attrIt) tokenize(*attrIt, accepts, ",");
  for(std::list<std::string>::iterator acc = accepts.begin(); acc != accepts.end(); ++acc) {
    *acc = Arc::trim(*acc, " ");
    std::string::size_type pos = acc->find_first_of(';');
    if(pos != std::string::npos) acc->erase(pos);
  }
  ResponseFormat outFormat = ResponseFormatHtml;
  for(std::list<std::string>::iterator acc = accepts.begin(); acc != accepts.end(); ++acc) {
    if(*acc == "application/json") {
      outFormat = ResponseFormatJson;
      outmsg.Attributes()->set("HTTP:content-type","application/json");
      break;
    } else if((*acc == "text/xml") || (*acc == "application/xml")) {
      outFormat = ResponseFormatXml;
      outmsg.Attributes()->set("HTTP:content-type","application/xml");
      break;
    } else if(*acc == "text/html") {
      outFormat = ResponseFormatHtml;
      outmsg.Attributes()->set("HTTP:content-type","text/html");
      break;
    }
  }
  return outFormat;
}

// Insert structured positive response into outmsg.
static Arc::MCC_Status HTTPResponse(Arc::Message& inmsg, Arc::Message& outmsg, Arc::XMLNode& resp, char const * json_arrays[]) {
  ResponseFormat outFormat = ProcessAcceptedFormat(inmsg,outmsg);
  std::string respStr;
  RenderResponse(resp, outFormat, respStr, json_arrays);
  if(inmsg.Attributes()->get("HTTP:METHOD") == "HEAD") {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    if(outpayload) outpayload->Truncate(respStr.length());
    delete outmsg.Payload(outpayload);
  } else {
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    if(outpayload) outpayload->Insert(respStr.c_str(),0,respStr.length());
    delete outmsg.Payload(outpayload);
  }
  outmsg.Attributes()->set("HTTP:CODE","200");
  outmsg.Attributes()->set("HTTP:REASON","OK");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status HTTPPOSTResponse(Arc::Message& inmsg, Arc::Message& outmsg,
                                    Arc::XMLNode& resp, char const * json_arrays[], std::string const & redir = "") {
  ResponseFormat outFormat = ProcessAcceptedFormat(inmsg,outmsg);
  std::string respStr;
  RenderResponse(resp, outFormat, respStr, json_arrays);
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  if(outpayload) outpayload->Insert(respStr.c_str(),0,respStr.length());
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE","201");
  outmsg.Attributes()->set("HTTP:REASON","Created");
  if(!redir.empty()) outmsg.Attributes()->set("HTTP:location",redir);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static std::string GetPath(Arc::Message &inmsg,std::string &base,std::multimap<std::string,std::string>& query) {
  base = inmsg.Attributes()->get("HTTP:ENDPOINT");
  Arc::AttributeIterator iterator = inmsg.Attributes()->getAll("PLEXER:EXTENSION");
  std::string path;
  if(iterator.hasMore()) {
    // Service is behind plexer
    path = *iterator;
    if(base.length() > path.length()) base.resize(base.length()-path.length());
  } else {
    // Standalone service
    path=Arc::URL(base).Path();
    base.resize(0);
  };
  std::string::size_type queryPos = path.find('?');
  if(queryPos == std::string::npos) queryPos = path.find(';');
  if(queryPos != std::string::npos) {
    std::list<std::string> queryItems;
    Arc::tokenize(path.substr(queryPos+1), queryItems, "&");
    for(std::list<std::string>::iterator queryItem = queryItems.begin(); queryItem != queryItems.end(); ++queryItem) {
      std::string::size_type valuePos = queryItem->find('=');
      std::string value;
      if(valuePos != std::string::npos) {
        value = queryItem->substr(valuePos+1);
        queryItem->resize(valuePos);
      };
      query.insert(std::pair<std::string,std::string>(Arc::uri_unencode(*queryItem),Arc::uri_unencode(value)));
    };
    path.resize(queryPos);
  };
  // Path is encoded in HTTP URLs too
  path = Arc::uri_unencode(path);
  return path;
}

static void ParseIds(std::multimap<std::string,std::string> const & query, std::list<std::string>& ids) {
  typedef std::multimap<std::string,std::string>::const_iterator iter;
  std::pair<iter,iter> range = query.equal_range("id");
  for(iter id = range.first; id != range.second; ++id) {
    ids.push_back(id->second);
  };
}

static void ParseJobIds(Arc::Message& inmsg, Arc::Message& outmsg, std::list<std::string>& ids) {
  std::string content;
  Arc::MCC_Status status = extract_content(inmsg,content,1024*1024);
  std::string contentType = inmsg.Attributes()->get("HTTP:content-type");
  Arc::XMLNode listXml;
  if(contentType == "application/json") {
    Arc::XMLNode("<jobs/>").Move(listXml);    
    (void)ParseFromJson(listXml, content.c_str());
  } else if((contentType == "application/xml") || contentType.empty()) {
    Arc::XMLNode(content).Move(listXml);    
  }
  // jobs
  //   job
  //     id
  for(Arc::XMLNode jobXml = listXml["job"];(bool)jobXml;++jobXml) {
    std::string id = jobXml["id"];
    if(!id.empty()) ids.push_back(id);
  }
}

//   REST State   A-REX State
// * ACCEPTING    ACCEPTED
// * ACCEPTED     PENDING:ACCEPTED
// * PREPARING    PREPARING
// * PREPARED     PENDING:PREPARING
// * SUBMITTING   SUBMIT
// - QUEUING      INLRMS + LRMS queued
// - RUNNING      INLRMS + LRMS running
// - HELD         INLRMS + LRMS on hold
// - EXITINGLRMS  INLRMS + LRMS finished
// - OTHER        INLRMS + LRMS other
// * EXECUTED     PENDING:INLRMS
// * FINISHING    FINISHING
// * KILLING      CANCELLING | PREPARING + DTR cancel | FINISHING + DTR cancel
// * FINISHED     FINISHED + no errors & no cancel
// * FAILED       FINISHED + errors
// * KILLED       FINISHED + cancel
// * WIPED        DELETED
static void convertActivityStatusREST(const std::string& gm_state,std::string& rest_state,
                                      bool failed,bool pending,const std::string& /*failedstate*/,const std::string& failedcause) {
  rest_state.clear();
  if(gm_state == "ACCEPTED") {
    if(!pending)
      rest_state="ACCEPTING";
    else
      rest_state="ACCEPTED";
  } else if(gm_state == "PREPARING") {
    if(!pending)
      rest_state="PREPARING";
    else
      rest_state="PREPARED";
  } else if(gm_state == "SUBMIT") {
      rest_state="SUBMITTING";
  } else if(gm_state == "INLRMS") {
    if(!pending) {
      // Talking to LRMS would be too heavy. Just choose something innocent enough.
      rest_state="RUNNING";
    } else {
      rest_state="EXECUTED";
    }
  } else if(gm_state == "FINISHING") {
    rest_state="FINISHING";
  } else if(gm_state == "CANCELING") {
    rest_state="KILLING";
  } else if(gm_state == "FINISHED") {
    if(!pending) {
      if(failed) {
        // TODO: hack
        if(failedcause.find("Job is canceled by external request") != std::string::npos) {
          rest_state = "KILLED";
        } else {
          rest_state = "FAILED";
        }
      } else {
        rest_state="FINISHED";
      }
    } else {
      rest_state="EXECUTED";
    }
  } else if(gm_state == "DELETED") {
    rest_state="WIPED";
  } else {
    rest_state="None";
  }
}

ARexRest::ARexRest(Arc::Config *cfg, Arc::PluginArgument *parg, GMConfig& config,
                   ARex::DelegationStores& delegation_stores,unsigned int& all_jobs_count):
       logger_(Arc::Logger::rootLogger, "A-REX REST"),
       config_(config),delegation_stores_(delegation_stores),all_jobs_count_(all_jobs_count) {
  endpoint_=(std::string)((*cfg)["endpoint"]);
  uname_=(std::string)((*cfg)["usermap"]["defaultLocalName"]);
}

ARexRest::~ARexRest(void)  {
}

// Main request processor of REST interface
Arc::MCC_Status ARexRest::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Split request path into parts: service, jobs, files, etc. 
  // TODO: make it HTTP independent
  std::string endpoint;
  ProcessingContext context;
  context.method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string clientid = (inmsg.Attributes()->get("TCP:REMOTEHOST"))+":"+(inmsg.Attributes()->get("TCP:REMOTEPORT"));
  logger_.msg(Arc::INFO, "Connection from %s: %s", inmsg.Attributes()->get("TCP:REMOTEHOST"), inmsg.Attributes()->get("TLS:IDENTITYDN"));
  context.subpath = GetPath(inmsg,endpoint,context.query);
  context.processed = "/";
  if((inmsg.Attributes()->get("PLEXER:PATTERN").empty()) && context.subpath.empty()) context.subpath=endpoint;
  logger_.msg(Arc::VERBOSE, "process: method: %s", context.method);
  logger_.msg(Arc::VERBOSE, "process: endpoint: %s", endpoint);

  // {<service endpoint URL>/rest}/<version>/<functionality>

  logger_.msg(Arc::VERBOSE, "REST: process %s at %s",context.method,context.subpath);

  std::string apiVersion;
  GetPathToken(context.subpath, apiVersion); // drop /rest
  if ((!GetPathToken(context.subpath, apiVersion)) || apiVersion.empty()) {
    // {<service endpoint URL>/rest
    return processVersions(inmsg, outmsg, context);
  }
  context.processed += apiVersion;
  context.processed += "/";
  if (apiVersion == "1.0") {
    context.version = ProcessingContext::Version_1_0;
  } else if (apiVersion == "1.1") {
    context.version = ProcessingContext::Version_1_1;
  } else {
    return HTTPFault(inmsg,outmsg,404,"Version Not Supported");
  }

  std::string functionality;
  if(!GetPathToken(context.subpath, functionality) || functionality.empty()) {
    // {<service endpoint URL>/rest}/<version>
    return processGeneral(inmsg, outmsg, context);
  }
  context.processed += functionality;
  context.processed += "/";
  if (functionality == "info") {
    // {<service endpoint URL>/rest}/<version>/info[?schema=glue2]
    return processInfo(inmsg, outmsg, context);
  } else if (functionality == "delegations") {
    // {<service endpoint URL>/rest}/delegations/[<delegation id>[?action=get,renew,delete]]
    return processDelegations(inmsg, outmsg, context);
  } else if (functionality == "jobs") {
    // {<service endpoint URL>/rest}/<version>/jobs[?state=<state1>[&state=<state2>[...]]]
    // {<service endpoint URL>/rest}/<version>/jobs?action={new|info|status|kill|clean|restart}
    return processJobs(inmsg, outmsg, context);
  }

  return HTTPFault(inmsg,outmsg,404,"Functionality Not Supported");
}

// ---------------------------- GENERAL INFO ---------------------------------

Arc::MCC_Status ARexRest::processVersions(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context) {
  if((context.method == "GET") || (context.method == "HEAD")) {
    XMLNode versions("<versions><version>1.0</version><version>1.1</version></versions>"); // only supported versions are 1.0 and 1.1
    char const * json_arrays[] = { "version", NULL };
    return HTTPResponse(inmsg, outmsg, versions, json_arrays);
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

Arc::MCC_Status ARexRest::processGeneral(Arc::Message& inmsg,Arc::Message& outmsg, ProcessingContext& context) {
  return HTTPFault(inmsg,outmsg,404,"Not Found");
}

Arc::MCC_Status ARexRest::processInfo(Arc::Message& inmsg,Arc::Message& outmsg, ProcessingContext& context) {
  if(!context.subpath.empty())
    return HTTPFault(inmsg,outmsg,404,"Not Found");

  // GET <base URL>/info[?schema=glue2] - retrieve generic information about cluster.
  // HEAD - supported.
  // PUT,POST,DELETE - not supported.
  if((context.method != "GET") && (context.method != "HEAD")) {
    logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
    return HTTPFault(inmsg,outmsg,501,"Not Implemented");
  }

  std::string schema = context["schema"];
  if (!schema.empty() && (schema != "glue2")) {
    logger_.msg(Arc::VERBOSE, "process: schema %s is not supported for subpath %s",schema,context.processed);
    return HTTPFault(inmsg,outmsg,501,"Schema not implemented");
  }

  std::string infoStr;
  Arc::FileRead(config_.InformationFile(), infoStr);
  XMLNode infoXml(infoStr);
  return HTTPResponse(inmsg, outmsg, infoXml, NULL);
}

// ---------------------------- DELEGATIONS ---------------------------------

Arc::MCC_Status ARexRest::processDelegations(Arc::Message& inmsg,Arc::Message& outmsg, ProcessingContext& context) {
  // GET <base URL>/delegations[&type={x509|jwt}] - retrieves list of delegations belonging to authenticated user
  // HEAD - supported.
  // POST <base URL>/delegations?action=new starts a new delegation process (1st step).
  // PUT <base URL>/delegations/<delegation id> stores public part (2nd step).
  // POST <base URL>/delegations/<delegation id>?action=get,renew,delete used to manage delegation.
  std::string delegationId;
  if(GetPathToken(context.subpath, delegationId)) {
    context.processed += delegationId;
    context.processed += "/";
    return processDelegation(inmsg,outmsg,context,delegationId);
  }

  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config) {
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
  }
  if((context.method == "GET") || (context.method == "HEAD")) {
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    std::string requestedType;
    if(context.version >= ProcessingContext::Version_1_1) {
      requestedType = context["type"];
    }
    XMLNode listXml("<delegations/>");
    std::list<std::pair<std::string,std::list<std::string> > > ids = delegation_stores_[config_.DelegationDir()].ListCredInfos(config->GridName());
    for(std::list<std::pair<std::string,std::list<std::string> > >::iterator itId = ids.begin(); itId != ids.end(); ++itId) {
      char const * delegType = "x509";
      if(itId->second.size() > 0) delegType = itId->second.front().c_str();
      if (!requestedType.empty()) {
         if(requestedType != delegType) continue;
      }
      XMLNode delegXml = listXml.NewChild("delegation");
      delegXml.NewChild("id") = itId->first;
      delegXml.NewChild("type") = delegType;
    }
    char const * json_arrays[] = { "delegation", NULL };
    return HTTPResponse(inmsg, outmsg, listXml, json_arrays);
  } else if(context.method == "POST") {
    std::string action = context["action"];
    if(action != "new") 
      return HTTPFault(inmsg,outmsg,501,"Action not implemented");
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCreate, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    std::string requestedType;
    if(context.version >= ProcessingContext::Version_1_1) {
      requestedType = context["type"];
    }
    std::string delegationId;
    std::string delegationRequest;
    if(requestedType.empty() || (requestedType == "x509")) {
      // TODO: explicitely put x509 into meta
      if(!delegation_stores_.GetRequest(config_.DelegationDir(),delegationId,config->GridName(),delegationRequest)) {
        return HTTPFault(inmsg,outmsg,500,"Failed generating delegation request");
      }
      Arc::URL base(inmsg.Attributes()->get("HTTP:ENDPOINT"));
      return HTTPPOSTResponse(inmsg,outmsg,delegationRequest,"application/x-pem-file",base.Path()+"/"+delegationId);
    } else if(requestedType == "jwt") {
      Arc::AttributeIterator tokenIt = inmsg.Attributes()->getAll("HTTP:x-token-delegation");
      if(!tokenIt.hasMore()) 
        return HTTPFault(inmsg,outmsg,501,"Missing X-Token-Delegation header in delegation request");
      std::list<std::string> meta;
      meta.push_back("jwt");
      if(!delegation_stores_.PutCred(config_.DelegationDir(),delegationId,config->GridName(),*tokenIt,meta)) {
        return HTTPFault(inmsg,outmsg,500,"Failed storing delegation token");
      }
      Arc::URL base(inmsg.Attributes()->get("HTTP:ENDPOINT"));
      return HTTPPOSTResponse(inmsg,outmsg,base.Path()+"/"+delegationId);
    }
    return HTTPFault(inmsg,outmsg,501,"Unknown delegation type specified");
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

void UpdateProxyFile(ARex::DelegationStores& delegation_stores, ARexConfigContext& config, std::string const& id) {
#if 1
  // In case of update for compatibility during intermediate period store delegations in
  // per-job proxy file too.
  DelegationStore& delegation_store(delegation_stores[config.GmConfig().DelegationDir()]);
  std::list<std::string> job_ids;
  if(delegation_store.GetLocks(id,config.GridName(),job_ids)) {
    for(std::list<std::string>::iterator job_id = job_ids.begin(); job_id != job_ids.end(); ++job_id) {
      // check if that is main delegation for this job
      std::string delegationid;
      if(job_local_read_delegationid(*job_id,config.GmConfig(),delegationid)) {
        if(id == delegationid) {
          std::string credentials;
          if(delegation_store.GetCred(id,config.GridName(),credentials)) {
            if(!credentials.empty()) {
              GMJob job(*job_id,Arc::User(config.User().get_uid()));
              (void)job_proxy_write_file(job,config.GmConfig(),credentials);
            };
          };
        };
      };
    };
  };
#endif
}

Arc::MCC_Status ARexRest::processDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context,std::string const & id) {
  // GET,HEAD,DELETE - not supported.
  // PUT <base URL>/delegations/<delegation id> stores public part (2nd step) to finish delegation procedure or to re-new delegation.
  // POST <base URL>/delegations/<delegation id>?action=get,renew,delete used to manage delegation.
  if(!context.subpath.empty())
    return HTTPFault(inmsg,outmsg,404,"Not Found"); // no more sub-resources
  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config)
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");

  // POST - manages delegation.
  if(context.method == "PUT") {
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCreate, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    // Fetch HTTP content to pass it as delegation
    std::string content;
    Arc::MCC_Status res = extract_content(inmsg,content,1024*1024); // 1mb size limit is sane enough
    if(!res)
      return HTTPFault(inmsg,outmsg,500,res.getExplanation().c_str());
    if(content.empty())
      return HTTPFault(inmsg,outmsg,500,"Missing payload");
    if(!delegation_stores_.PutDeleg(config_.DelegationDir(),id,config->GridName(),content))
      return HTTPFault(inmsg,outmsg,500,"Failed accepting delegation");
    UpdateProxyFile(delegation_stores_, *config, id);
    return HTTPResponse(inmsg,outmsg);
  } else if(context.method == "POST") {
    std::string action = context["action"];
    if(action == "get") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::string credentials;
      if(!delegation_stores_[config_.DelegationDir()].GetDeleg(id, config->GridName(), credentials)) {
        return HTTPFault(inmsg,outmsg,404,"No delegation found");
      }
      return HTTPResponse(inmsg, outmsg, credentials, "application/x-pem-file"); // ??
    } else if(action == "renew") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCreate, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::string delegationId = id;
      std::string delegationRequest;
      if(!delegation_stores_.GetRequest(config_.DelegationDir(),delegationId,config->GridName(),delegationRequest))
        return HTTPFault(inmsg,outmsg,500,"Failed generating delegation request");
      return HTTPPOSTResponse(inmsg,outmsg,delegationRequest,"application/x-pem-file","");
    } else if(action == "delete") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobDelete, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      Arc::DelegationConsumerSOAP* deleg = 
                delegation_stores_[config_.DelegationDir()].FindConsumer(id, config->GridName());
      if(!deleg)
        return HTTPFault(inmsg,outmsg,404,"No such delegation");
      if(!(delegation_stores_[config_.DelegationDir()].RemoveConsumer(deleg)))
        return HTTPFault(inmsg,outmsg,500,"Failed deleting delegation");
      return HTTPDELETEResponse(inmsg, outmsg); // ??
    }
    logger_.msg(Arc::VERBOSE, "process: action %s is not supported for subpath %s",action,context.processed);
    return HTTPFault(inmsg,outmsg,501,"Action not implemented");
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

// ---------------------------- JOBS ---------------------------------

static bool processJobInfo(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml);
static bool processJobStatus(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml);
static bool processJobKill(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml);
static bool processJobClean(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml);
static bool processJobRestart(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml);
static bool processJobDelegations(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml, ARex::DelegationStores& delegation_stores);

Arc::MCC_Status ARexRest::processJobs(Arc::Message& inmsg,Arc::Message& outmsg,ProcessingContext& context) {
  // GET <base URL>/jobs[?state=<state1[,state2[...]]>]
  // HEAD - supported.
  // POST <base URL>/jobs?action=new initiates creation of a new job instance or multiple jobs.
  // POST <base URL>/jobs?action={info|status|kill|clean|restart|delegations} - job management operations supporting arrays of jobs.
  // PUT - not supported.

  std::string jobId;
  if(GetPathToken(context.subpath, jobId)) {
    // <base URL>/jobs/<job id>/...
    context.processed += jobId;
    context.processed += "/";
    return processJob(inmsg,outmsg,context,jobId);
  }

  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config) {
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
  }
  if((context.method == "GET") || (context.method == "HEAD")) {
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    std::list<std::string> states;
    tokenize(context["state"], states, ",");
    XMLNode listXml("<jobs/>");
    std::list<std::string> ids = ARexJob::Jobs(*config,logger_);
    for(std::list<std::string>::iterator itId = ids.begin(); itId != ids.end(); ++itId) {
      std::string rest_state;
      if(!states.empty()) {
        ARexJob job(*itId,*config,logger_);
        if(!job) continue; // There is no such job
        bool job_pending = false;
        std::string gm_state = job.State(job_pending);
        bool job_failed = job.Failed();
        std::string failed_cause;
        std::string failed_state = job.FailedState(failed_cause);
        convertActivityStatusREST(gm_state,rest_state,job_failed,job_pending,failed_state,failed_cause);
        bool state_found = false;
        for(std::list<std::string>::iterator itState = states.begin(); itState != states.end(); ++itState) {
          if(rest_state == *itState) {
            state_found = true;
            break;
          }
        }
        if(!state_found) continue;
      } // states filter
      XMLNode jobXml = listXml.NewChild("job");
      jobXml.NewChild("id") = *itId;
      if(!rest_state.empty())
        jobXml.NewChild("state") = rest_state;
    }
    char const * json_arrays[] = { "job", NULL };
    return HTTPResponse(inmsg, outmsg, listXml, json_arrays);
  } else if(context.method == "POST") {
    std::string action = context["action"];
    if(action == "new") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCreate, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      if((config->GmConfig().MaxTotal() > 0) && (all_jobs_count_ >= config->GmConfig().MaxTotal()))
        return HTTPFault(inmsg,outmsg,500,"No more jobs allowed");
      // Fetch HTTP content to pass it as job description
      std::string desc_str;
      Arc::MCC_Status res = extract_content(inmsg,desc_str,100*1024*1024);
      if(!res)
        return HTTPFault(inmsg,outmsg,500,res.getExplanation().c_str());
      if(desc_str.empty())
        return HTTPFault(inmsg,outmsg,500,"Missing payload");
      JobIDGeneratorREST idgenerator(config->Endpoint());
      std::string clientid = (inmsg.Attributes()->get("TCP:REMOTEHOST"))+":"+(inmsg.Attributes()->get("TCP:REMOTEPORT"));
      // TODO: Make ARexJob accept JobDescription directly to avoid reparsing jobs and use Arc::JobDescription::Parse here.
      // Quck and dirty check for job type
      std::string::size_type start_pos = desc_str.find_first_not_of(" \t\r\n");
      if(start_pos == std::string::npos)
        return HTTPFault(inmsg,outmsg,500,"Payload is empty");

      std::string default_queue;
      std::string default_delegation_id;
      if(context.version >= ProcessingContext::Version_1_1) {
        default_queue = context["queue"];
        default_delegation_id = context["delegation_id"];
      }
      XMLNode listXml("<jobs/>");
      // TODO: Split to separate functions
      switch(desc_str[start_pos]) {
        case '<': { // XML (multi- or single-ADL)
          Arc::XMLNode jobs_desc_xml(desc_str);
          if (jobs_desc_xml.Name() == "ActivityDescriptions") {
            // multi
            for(int idx = 0;;++idx) {
              Arc::XMLNode job_desc_xml = jobs_desc_xml.Child(idx);
              if(!job_desc_xml)
                break;
              XMLNode jobXml = listXml.NewChild("job");
              ARexJob job(job_desc_xml,*config,default_delegation_id,default_queue,clientid,logger_,idgenerator);
              if(!job) {
                jobXml.NewChild("status-code") = "500";
                jobXml.NewChild("reason") = job.Failure();
              } else {
                jobXml.NewChild("status-code") = "201";
                jobXml.NewChild("reason") = "Created";
                jobXml.NewChild("id") = job.ID();
                jobXml.NewChild("state") = "ACCEPTING";
              } 
            }
          } else {
            // maybe single
            XMLNode jobXml = listXml.NewChild("job");
            ARexJob job(jobs_desc_xml,*config,default_delegation_id,default_queue,clientid,logger_,idgenerator);
            if(!job) {
              jobXml.NewChild("status-code") = "500";
              jobXml.NewChild("reason") = job.Failure();
            } else {
              jobXml.NewChild("status-code") = "201";
              jobXml.NewChild("reason") = "Created";
              jobXml.NewChild("id") = job.ID();
              jobXml.NewChild("state") = "ACCEPTING";
            } 
          }
        }; break;

        case '&': { // single-xRSL
          XMLNode jobXml = listXml.NewChild("job");
          ARexJob job(desc_str,*config,default_delegation_id,default_queue,clientid,logger_,idgenerator);
          if(!job) {
            jobXml.NewChild("status-code") = "500";
            jobXml.NewChild("reason") = job.Failure();
          } else {
            jobXml.NewChild("status-code") = "201";
            jobXml.NewChild("reason") = "Created";
            jobXml.NewChild("id") = job.ID();
            jobXml.NewChild("state") = "ACCEPTING";
          } 
        }; break;

        case '+': { // multi-xRSL
          std::list<JobDescription> jobdescs;
          Arc::JobDescriptionResult result = Arc::JobDescription::Parse(desc_str, jobdescs, "nordugrid:xrsl", "GRIDMANAGER");
          if (!result) {
            return HTTPFault(inmsg,outmsg,500,result.str().c_str());
          } else {
            for(std::list<JobDescription>::iterator jobdesc = jobdescs.begin(); jobdesc != jobdescs.end(); ++jobdesc) {
              XMLNode jobXml = listXml.NewChild("job");
              std::string jobdesc_str;
              result = jobdesc->UnParse(jobdesc_str, "nordugrid:xrsl", "GRIDMANAGER");
              if (!result) {
                jobXml.NewChild("status-code") = "500";
                jobXml.NewChild("reason") = result.str();
              } else {
                ARexJob job(jobdesc_str,*config,default_delegation_id,default_queue,clientid,logger_,idgenerator);
                if(!job) {
                  jobXml.NewChild("status-code") = "500";
                  jobXml.NewChild("reason") = job.Failure();
                } else {
                  jobXml.NewChild("status-code") = "201";
                  jobXml.NewChild("reason") = "Created";
                  jobXml.NewChild("id") = job.ID();
                  jobXml.NewChild("state") = "ACCEPTING";
                } 
              }
            }
          }
        }; break;

        default:
          return HTTPFault(inmsg,outmsg,500,"Payload is not recognized");
          break;
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "info") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobInfo(inmsg,*config,logger_,*id,jobXml);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "status") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobStatus(inmsg,*config,logger_,*id,jobXml);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "kill") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCancel, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobKill(inmsg,*config,logger_,*id,jobXml);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "clean") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobDelete, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobClean(inmsg,*config,logger_,*id,jobXml);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "restart") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobCreate, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobRestart(inmsg,*config,logger_,*id,jobXml);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);
    } else if(action == "delegations") {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      std::list<std::string> ids;
      ParseJobIds(inmsg,outmsg,ids);
      XMLNode listXml("<jobs/>");
      for(std::list<std::string>::iterator id = ids.begin(); id != ids.end(); ++id) {
        XMLNode jobXml = listXml.NewChild("job");
        (void)processJobDelegations(inmsg,*config,logger_,*id,jobXml,delegation_stores_);
      }
      char const * json_arrays[] = { "job", NULL };
      return HTTPPOSTResponse(inmsg, outmsg, listXml, json_arrays);      
    }
    logger_.msg(Arc::VERBOSE, "process: action %s is not supported for subpath %s",action,context.processed);
    return HTTPFault(inmsg,outmsg,501,"Action not implemented");
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

static bool processJobInfo(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:GET job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    jobXml.NewChild("info_document");
    return false;
  }
  std::string glue_s;
  Arc::XMLNode glue_xml(job_xml_read_file(id,config.GmConfig(),glue_s)?glue_s:"");
  if(!glue_xml) {
    // Fallback: create something minimal
    static const char* job_xml_template =
    "<ComputingActivity xmlns=\"http://schemas.ogf.org/glue/2009/03/spec_2.0_r1\"\n"
    "                   BaseType=\"Activity\" CreationTime=\"\" Validity=\"60\">\n"
    "  <ID></ID>\n"
    "  <OtherInfo>SubmittedVia=org.ogf.glue.emies.activitycreation</OtherInfo>\n"
    "  <Type>single</Type>\n"
    "  <IDFromEndpoint></IDFromEndpoint>\n"
    "  <JobDescription>emies:adl</JobDescription>\n"
    "  <State></State>\n"
    "  <Owner></Owner>\n"
    "  <Associations>\n"
    "    <ComputingShareID></ComputingShareID>\n"
    "  </Associations>\n"
    "</ComputingActivity>";
    Arc::XMLNode(job_xml_template).New(glue_xml);
    Arc::URL headnode(config.GmConfig().HeadNode());
    glue_xml["ID"] = std::string("urn:caid:")+headnode.Host()+":org.ogf.glue.emies.activitycreation:"+id;
    glue_xml["IDFromEndpoint"] = "urn:idfe:"+id;
    {
      // Collecting job state
      bool job_pending = false;
      std::string gm_state = job.State(job_pending);
      bool job_failed = job.Failed();
      std::string failed_cause;
      std::string failed_state = job.FailedState(failed_cause);

      std::string primary_state;
      std::list<std::string> state_attributes;
      convertActivityStatusES(gm_state,primary_state,state_attributes,
                              job_failed,job_pending,failed_state,failed_cause);
      glue_xml["State"] = "emies:"+primary_state;
      std::string prefix = glue_xml["State"].Prefix();
      for(std::list<std::string>::iterator attr = state_attributes.begin();
                  attr != state_attributes.end(); ++attr) {
        glue_xml.NewChild(prefix+":State") = "emiesattr:"+(*attr);
      };

      std::string rest_state;
      convertActivityStatusREST(gm_state,rest_state,
                              job_failed,job_pending,failed_state,failed_cause);
      glue_xml["State"] = "arcrest:"+rest_state;
    };
    glue_xml["Owner"] = config.GridName();
    glue_xml.Attribute("CreationTime") = job.Created().str(Arc::ISOTime);
  };
  // Delegation ids?
  jobXml.NewChild("status-code") = "200";
  jobXml.NewChild("reason") = "OK";
  jobXml.NewChild("id") = id;
  jobXml.NewChild("info_document").NewChild("ComputingActivity").Exchange(glue_xml);
  return true;
}

static bool processJobStatus(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:GET job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    jobXml.NewChild("state") = "None";
    return false;
  }
  // Collecting job state
  // Most detailed state is obtianable from XML info
  std::string rest_state;
  {
    std::string glue_s;
    if(job_xml_read_file(id,config.GmConfig(),glue_s)) {
      Arc::XMLNode glue_xml(glue_s);
      if((bool)glue_xml) {
        for(Arc::XMLNode snode = glue_xml["State"]; (bool)snode ; ++snode) {
          std::string state_str = snode;
          if(state_str.compare(0, 8, "arcrest:") == 0) {
            rest_state = state_str.substr(8);
            break;
          }
        }
      }
    }
  }
  if (rest_state.empty()) {
    // Faster but less detailed state can be computed from GM state
    bool job_pending = false;
    std::string gm_state = job.State(job_pending);
    bool job_failed = job.Failed();
    std::string failed_cause;
    std::string failed_state = job.FailedState(failed_cause);
    convertActivityStatusREST(gm_state,rest_state,
                              job_failed,job_pending,failed_state,failed_cause);
  }
  jobXml.NewChild("status-code") = "200";
  jobXml.NewChild("reason") = "OK";
  jobXml.NewChild("id") = id;
  jobXml.NewChild("state") = rest_state;
  return true;
}

static bool processJobKill(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:KILL job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    return false;
  }
  if(!job.Cancel()) {
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:KILL job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "505";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job could not be canceled";
    jobXml.NewChild("id") = id;
    return false;
  }
  jobXml.NewChild("status-code") = "202";
  jobXml.NewChild("reason") = "Queued for killing";
  jobXml.NewChild("id") = id;
  return true;
}

static bool processJobClean(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:CLEAN job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    return false;
  }
  if(!job.Clean()) {
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:CLEAN job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "505";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job could not be cleaned";
    jobXml.NewChild("id") = id;
    return false;
  }
  jobXml.NewChild("status-code") = "202";
  jobXml.NewChild("reason") = "Queued for cleaning";
  jobXml.NewChild("id") = id;
  return true;
}

static bool processJobRestart(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:RESTART job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    return false;
  }
  if(!job.Resume()) {
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:RESTART job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "505";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job could not be resumed";
    jobXml.NewChild("id") = id;
    return false;
  }
  jobXml.NewChild("status-code") = "202";
  jobXml.NewChild("reason") = "Queued for restarting";
  jobXml.NewChild("id") = id;
  return true;
}

static bool processJobDelegations(Arc::Message& inmsg,ARexConfigContext& config, Arc::Logger& logger, std::string const & id, XMLNode jobXml, ARex::DelegationStores& delegation_stores) {
  ARexJob job(id,config,logger);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger.msg(Arc::ERROR, "REST:RESTART job %s - %s", id, failure);
    jobXml.NewChild("status-code") = "404";
    jobXml.NewChild("reason") = (!failure.empty()) ? failure : "Job not found";
    jobXml.NewChild("id") = id;
    return false;
  }
  jobXml.NewChild("status-code") = "200";
  jobXml.NewChild("reason") = "OK";
  jobXml.NewChild("id") = id;
  std::list<std::string> ids = delegation_stores[config.GmConfig().DelegationDir()].ListLockedCredIDs(id,config.GridName());
  for(std::list<std::string>::iterator itId = ids.begin(); itId != ids.end(); ++itId) {
    jobXml.NewChild("delegation_id") = *itId;
  }
  return true;
}

Arc::MCC_Status ARexRest::processJob(Arc::Message& inmsg,Arc::Message& outmsg,
                                     ProcessingContext& context,std::string const & id) {
  std::string subResource;
  if(GetPathToken(context.subpath, subResource)) {
    context.processed += subResource;
    context.processed += "/";
    if(subResource == "session") {
      return processJobSessionDir(inmsg,outmsg,context,id);
    } else if(subResource == "diagnose") {
      return processJobControlDir(inmsg,outmsg,context,id);
    }
    return HTTPFault(inmsg,outmsg,404,"Wrong job sub-resource requested");
  }
  return HTTPFault(inmsg,outmsg,404,"Missing job sub-resource");
}

// -------------------------------  PER-JOB SESSION DIR -------------------------------------

static bool write_file(Arc::FileAccess& h,char* buf,size_t size) {
  for(;size>0;) {
    ssize_t l = h.fa_write(buf,size);
    if(l == -1) return false;
    size-=l; buf+=l;
  };
  return true;
}

static Arc::MCC_Status PutJobFile(Arc::Message& inmsg, Arc::Message& outmsg, Arc::FileAccess& file, std::string& errstr,
                                  Arc::PayloadStreamInterface& stream, FileChunks& fc, bool& complete) {
  complete = false;
  // TODO: Use memory mapped file to minimize number of in memory copies
  const int bufsize = 1024*1024;
  if(!fc.Size()) fc.Size(stream.Size());
  off_t pos = stream.Pos(); 
  if(file.fa_lseek(pos,SEEK_SET) != pos) {
    std::string err = Arc::StrError();
    errstr = "failed to set position of file to "+Arc::tostring(pos)+" - "+err;
    return HTTPFault(inmsg, outmsg, 500, "Error seeking to specified position in file");
  };
  char* buf = new char[bufsize];
  if(!buf) {
    errstr = "failed to allocate memory";
    return HTTPFault(inmsg, outmsg, 500, "Error allocating memory");
  };
  bool got_something = false;
  for(;;) {
    int size = bufsize;
    if(!stream.Get(buf,size)) break;
    if(size > 0) got_something = true;
    if(!write_file(file,buf,size)) {
      std::string err = Arc::StrError();
      delete[] buf;
      errstr = "failed to write to file - "+err;
      return HTTPFault(inmsg, outmsg, 500, "Error writing to file");
    };
    if(size) fc.Add(pos,size);
    pos+=size;
  };
  delete[] buf;
  // Due to limitation of PayloadStreamInterface it is not possible to
  // directly distingush between zero sized file and file with undefined
  // size. But by applying some dynamic heuristics it is possible.
  // TODO: extend/modify PayloadStreamInterface.
  if((stream.Size() == 0) && (stream.Pos() == 0) && (!got_something)) {
    complete = true;
  }
  return HTTPResponse(inmsg,outmsg);
}

static Arc::MCC_Status PutJobFile(Arc::Message& inmsg, Arc::Message& outmsg, Arc::FileAccess& file, std::string& errstr,
                                  Arc::PayloadRawInterface& buf, FileChunks& fc, bool& complete) {
  complete = false;
  bool got_something = false;
  if(!fc.Size()) fc.Size(buf.Size());
  for(int n = 0;;++n) {
    char* sbuf = buf.Buffer(n);
    if(sbuf == NULL) break;
    off_t offset = buf.BufferPos(n);
    off_t size = buf.BufferSize(n);
    if(size > 0) {
      got_something = true;
      off_t o = file.fa_lseek(offset,SEEK_SET);
      if(o != offset) {
        std::string err = Arc::StrError();
        errstr = "failed to set position of file to "+Arc::tostring(offset)+" - "+err;
        return HTTPFault(inmsg, outmsg, 500, "Error seeking to specified position");
      };
      if(!write_file(file,sbuf,size)) {
        std::string err = Arc::StrError();
        errstr = "failed to write to file - "+err;
        return HTTPFault(inmsg, outmsg, 500, "Error writing file");
      };
      if(size) fc.Add(offset,size);
    };
  };
  if((buf.Size() == 0) && (!got_something)) {
    complete = true;
  }
  return HTTPResponse(inmsg,outmsg);
}

static void STATtoPROP(std::string const& name, struct stat& st,
                       std::list<std::string> requestProps, XMLNode& response) {
  XMLNode propstat = response.NewChild("d:propstat");
  XMLNode prop = propstat.NewChild("d:prop");
  propstat.NewChild("d:status") = "HTTP/1.1 200 OK";
  prop.NewChild("d:displayname") = name;
  if(S_ISDIR(st.st_mode)) {
    prop.NewChild("d:resourcetype").NewChild("d:collection");
  } else {
    prop.NewChild("d:resourcetype");
    prop.NewChild("d:getcontentlength") = Arc::tostring(st.st_size);
  };
  prop.NewChild("d:getlastmodified") = Arc::Time(st.st_mtime).str(Arc::ISOTime);
  prop.NewChild("d:creationdate") = Arc::Time(st.st_ctime).str(Arc::ISOTime);
}

static void ProcessPROPFIND(Arc::FileAccess* fa, Arc::XMLNode& multistatus,URL const& url,std::string const& path,uid_t uid,gid_t gid,int depth) {
  std::string name;
  std::size_t pos = path.rfind('/');
  if(pos == std::string::npos)
    name = path;
  else
    name = path.substr(pos+1);

  XMLNode response = multistatus.NewChild("d:response");
  std::string hrefStr = url.fullstr();
  struct stat st;
  if(!fa->fa_stat(path,st)) {
    // Not found
    response.NewChild("d:href") = hrefStr;
    response.NewChild("d:status") = "HTTP/1.1 404 Not Found";
  } else if(S_ISREG(st.st_mode)) {
    while(!hrefStr.empty() && hrefStr[hrefStr.length()-1] == '/') hrefStr.resize(hrefStr.length()-1);
    response.NewChild("d:href") = hrefStr;
    STATtoPROP(name, st, std::list<std::string>(), response);
  } else if(S_ISDIR(st.st_mode)) {
    if(!hrefStr.empty() && hrefStr[hrefStr.length()-1] != '/') hrefStr += '/';
    response.NewChild("d:href") = hrefStr;
    STATtoPROP(name, st, std::list<std::string>(), response);
    if(depth > 0) {
      if (fa->fa_opendir(path)) {
        std::list<std::string> names;
        std::string name;
        while(fa->fa_readdir(name)) {
          if(name == ".") continue;
          if(name == "..") continue;
          names.push_back(name);
        }
        fa->fa_closedir();
        for(std::list<std::string>::iterator name = names.begin(); name != names.end(); ++name) {
          URL subUrl(url);
          subUrl.ChangePath(subUrl.Path() + "/" + *name);
          std::string subPath = path + "/" + *name;
          ProcessPROPFIND(fa,multistatus,subUrl,subPath,uid,gid,depth-1);
        }
      }
    }
  } else {
    // Not for this interface
    response.NewChild("d:href") = hrefStr;
    response.NewChild("d:status") = "HTTP/1.1 404 Not Found";
  }
}

Arc::MCC_Status ARexRest::processJobSessionDir(Arc::Message& inmsg,Arc::Message& outmsg,
                                            ProcessingContext& context,std::string const & id) {
  class FileAccessRef {
   public:
    FileAccessRef(Arc::FileAccess* obj):obj_(obj) {
    }
  
    ~FileAccessRef() {
      if(obj_) {
        obj_->fa_close();
        obj_->fa_closedir();
        Arc::FileAccess::Release(obj_);
      }  
    }
  
    operator bool() const {
      return (obj_ == NULL);
    }
  
    bool operator !() const {
      return (obj_ == NULL);
    }
  
    Arc::FileAccess& operator*() {
      return *obj_;
    }
  
    Arc::FileAccess* operator->() {
      return obj_;
    }
  
    operator Arc::FileAccess*() {
      return obj_;
    }
  
    Arc::FileAccess*& get() {
      return obj_;
    }
  
  protected:
    Arc::FileAccess* obj_;
  };

  // GET,HEAD,PUT,DELETE - supported for files stored in job's session directory and perform usual actions.
  // GET,HEAD - for directories retrieves list of stored files (consider WebDAV for format).
  // DELETE - for directories removes whole directory.
  // PUT - for directory not supported.
  // POST - not supported.
  // PATCH - for files modifies part of files (body format need to be defined, all files treated as binary, currently support non-standard PUT with ranges).
  // PROPFIND - list diectories, stat files.
  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config) {
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
  }
  ARexJob job(id,*config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "REST:GET job %s - %s", id, job.Failure());
    return HTTPFault(inmsg,outmsg,404,job.Failure().c_str());
  }
  // Make sure path is correct while working with files
  if(!CanonicalDir(context.subpath, false, false))
    return HTTPFault(inmsg,outmsg,404,"Wrong path");

  if((context.method == "GET") || (context.method == "HEAD")) {
    // File or folder
    FileAccessRef dir(job.OpenDir(context.subpath));
    if(dir) {
      std::string errmsg;
      if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationDataInfo, config, errmsg))
        return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
      XMLNode listXml("<list/>");
      std::string dirpath = job.GetFilePath(context.subpath);
      for(;;) {
        std::string fileName;
        if(!dir->fa_readdir(fileName)) break;
        if(fileName == ".") continue;
        if(fileName == "..") continue;
        std::string fpath = dirpath+"/"+fileName;
        struct stat st;
        if(dir->fa_lstat(fpath.c_str(),st)) {
          if(S_ISREG(st.st_mode)) {
            XMLNode itemXml = listXml.NewChild("file");
            itemXml = fileName;
            itemXml.NewAttribute("size") = Arc::tostring(st.st_size);
          } else if(S_ISDIR(st.st_mode)) {
            XMLNode itemXml = listXml.NewChild("dir");
            itemXml = fileName;
          };
        };
      };
      char const * json_arrays[] = { "file", "dir", NULL };
      return HTTPResponse(inmsg,outmsg,listXml,json_arrays);
    };
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationDataRead, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    FileAccessRef file(job.OpenFile(context.subpath,true,false));
    if(file) {
      // File or similar
      Arc::MCC_Status r = HTTPResponseFile(inmsg,outmsg,file.get(),"application/octet-stream");
      return r;
    }
    return HTTPFault(inmsg,outmsg,404,"Not found");
  } else if(context.method == "PUT") {
    // Check for proper payload
    Arc::MessagePayload* payload = inmsg.Payload();
    Arc::PayloadStreamInterface* stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
    Arc::PayloadRawInterface* buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
    if((!stream) && (!buf)) {
      logger_.msg(Arc::ERROR, "REST:PUT job %s: file %s: there is no payload", id, context.subpath);
      return HTTPFault(inmsg, outmsg, 500, "Missing payload");
    };
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationDataWrite, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    // Prepare access to file 
    FileAccessRef file(job.CreateFile(context.subpath));
    if(!file) {
      // TODO: report something
      logger_.msg(Arc::ERROR, "%s: put file %s: failed to create file: %s", job.ID(), context.subpath, job.Failure());
      return HTTPFault(inmsg, outmsg, 500, "Error creating file");
    };
    FileChunksRef fc(files_chunks_.Get(job.GetFilePath(context.subpath)));
    Arc::MCC_Status r;
    std::string err;
    bool complete(false);
    if(stream) {
      r = PutJobFile(inmsg,outmsg,*file,err,*stream,*fc,complete);
    } else {
      r = PutJobFile(inmsg,outmsg,*file,err,*buf,*fc,complete);
    }
    if(!r) {
      logger_.msg(Arc::ERROR, "HTTP:PUT %s: put file %s: %s", job.ID(), context.subpath, err);
    } else {
      if(complete || fc->Complete()) job.ReportFileComplete(context.subpath);
    }
    return r;
  } else if(context.method == "DELETE") {
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationDataWrite, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    std::string fpath = job.GetFilePath(context.subpath);
    if(!fpath.empty()) {
      if((!FileDelete(fpath,job.UID(),job.GID())) &&
         (!DirDelete(fpath,true,job.UID(),job.GID()))) {
        return HTTPFault(inmsg,outmsg,500,"Failed to delete");
      }
    }
    return HTTPDELETEResponse(inmsg,outmsg);
  } else if(context.method == "PROPFIND") {
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationDataInfo, config, errmsg))
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    int depth = 10; // infinite with common sense
    std::string depthStr = inmsg.Attributes()->get("HTTP:depth");
    if(depthStr == "0")
      depth = 0;
    else if(depthStr == "1")
      depth = 1;
    std::string fpath = job.GetFilePath(context.subpath);
    URL url(inmsg.Attributes()->get("HTTP:ENDPOINT"));
    Arc::XMLNode multistatus("<d:multistatus xmlns:d=\"DAV:\"/>");
    FileAccessRef fa(Arc::FileAccess::Acquire());
    if(fa) ProcessPROPFIND(fa,multistatus,url,fpath,job.UID(),job.GID(),depth);
    std::string payload;
    multistatus.GetDoc(payload);
    return HTTPResponse(inmsg,outmsg,payload,"application/xml");
  };
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

// ------------------------------- PER-JOB CNTROL DIR ----------------------------

Arc::MCC_Status ARexRest::processJobControlDir(Arc::Message& inmsg,Arc::Message& outmsg,
                                            ProcessingContext& context,std::string const & id) {
  // GET - return the content of file in A-REX control directory for requested jobID
  // HEAD - supported.
  // PUT, POST, DELETE - not supported.
  char const * const mimeText = "text/plain";
  char const * const mimeXml = "application/xml";
  struct resourceDef { char const * const name; char const * const mime; };
  resourceDef const allowedSubResources[] = {
    { "failed", mimeText },
    { "local", mimeText },
    { "errors", mimeText },
    { "description", mimeText },
    { "diag", mimeText },
    { "comment", mimeText },
    { "status", mimeText },
    { "acl", mimeText },
    { "xml", mimeXml },
    { "input", mimeText },
    { "output", mimeText },
    { "input_status", mimeText },
    { "output_status", mimeText },
    { "statistics", mimeText },
    { NULL, NULL }
  };
  std::string subResource = context.subpath;
  resourceDef const * allowedSubResource = allowedSubResources;
  for(; allowedSubResource->name; ++allowedSubResource) {
    if(subResource == allowedSubResource->name)
      break;
  }
  if(!(allowedSubResource->name))
    return HTTPFault(inmsg,outmsg,404,"Diagnostic item not found");

  if((context.method == "GET") || (context.method == "HEAD")) {
    ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
    if(!config) {
      return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
    }
    std::string errmsg;
    if(!ARexConfigContext::CheckOperationAllowed(ARexConfigContext::OperationJobInfo, config, errmsg)) {
      return HTTPFault(inmsg,outmsg,HTTP_ERR_FORBIDDEN,"Operation is not allowed",errmsg.c_str());
    }
    ARexJob job(id,*config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "REST:GET job %s - %s", id, job.Failure());
      return HTTPFault(inmsg,outmsg,404,job.Failure().c_str());
    }
    int file = job.OpenLogFile(subResource);
    if(file == -1) 
      return HTTPFault(inmsg,outmsg,404,"Not found");
    Arc::MCC_Status r = HTTPResponseFile(inmsg,outmsg,file,allowedSubResource->mime);
    if(file != -1) 
      ::close(file);
    return r;
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

/*
Arc::MCC_Status ARexRest::processJobDelegations(Arc::Message& inmsg,Arc::Message& outmsg,
                        ProcessingContext& context,std::string const & id) {

  std::string delegationId;
  if(GetPathToken(context.subpath, delegationId)) {
    context.processed += delegationId;
    context.processed += "/";
    return processJobDelegation(inmsg,outmsg,context,id,delegationId);
  }

  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config) {
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
  }
  ARexJob job(id,*config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "REST:GET job %s - %s", id, job.Failure());
    return HTTPFault(inmsg,outmsg,404,job.Failure().c_str());
  }

  // GET - retrieves list of delegations belonging to specified job
  // HEAD - supported.
  if((context.method == "GET") || (context.method == "HEAD")) {
    XMLNode listXml("<delegations/>");
    std::list<std::string> ids = delegation_stores_[config_.DelegationDir()].ListLockedCredIDs(job.ID(),config->GridName());
    for(std::list<std::string>::iterator itId = ids.begin(); itId != ids.end(); ++itId) {
      listXml.NewChild("delegation").NewChild("id") = *itId;
    }
    char const * json_arrays[] = { "delegation", NULL };
    return HTTPResponse(inmsg, outmsg, listXml, json_arrays);
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}

Arc::MCC_Status ARexRest::processJobDelegation(Arc::Message& inmsg,Arc::Message& outmsg,
                        ProcessingContext& context,std::string const & jobId,std::string const & delegId) {
  if(!context.subpath.empty())
    return HTTPFault(inmsg,outmsg,404,"Not Found"); // no more sub-resources
  ARexConfigContext* config = ARexConfigContext::GetRutimeConfiguration(inmsg,config_,uname_,endpoint_);
  if(!config) {
    return HTTPFault(inmsg,outmsg,500,"User can't be assigned configuration");
  }
  // GET - returns public part of the stored delegation as application/x-pem-file.
  // HEAD - supported.
  if((context.method == "GET") || (context.method == "HEAD")) {
    std::string credentials;
    if(!delegation_stores_[config_.DelegationDir()].GetDeleg(delegId, config->GridName(), credentials)) {
      return HTTPFault(inmsg,outmsg,404,"No delegation found");
    }
    return HTTPResponse(inmsg, outmsg, credentials, "application/x-pem-file");
  }
  logger_.msg(Arc::VERBOSE, "process: method %s is not supported for subpath %s",context.method,context.processed);
  return HTTPFault(inmsg,outmsg,501,"Not Implemented");
}
*/
