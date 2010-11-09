#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/message/PayloadRaw.h>
#include "PayloadFile.h"
#include "job.h"

#include "arex.h"

#define MAX_CHUNK_SIZE (10*1024*1024)

namespace ARex {

static Arc::MCC_Status http_get(Arc::Message& outmsg,const std::string& burl,ARexJob& job,std::string hpath,size_t start,size_t end);

Arc::MCC_Status ARexService::Get(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,const std::string& id,const std::string& subpath) {
  size_t range_start = 0;
  size_t range_end = (size_t)(-1);
  {
    std::string val;
    val=inmsg.Attributes()->get("HTTP:RANGESTART");
    if(!val.empty()) { // Negative ranges not supported
      if(!Arc::stringto<size_t>(val,range_start)) {
        range_start=0;
      } else {
        val=inmsg.Attributes()->get("HTTP:RANGEEND");
        if(!val.empty()) {
          if(!Arc::stringto<size_t>(val,range_end)) {
            range_end=(size_t)(-1);
          };
        };
      };
    };
  };
  if(id.empty()) {
    // Make list of jobs
    std::string html;
    html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Jobs list</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
    std::list<std::string> jobs = ARexJob::Jobs(config,logger_);
    for(std::list<std::string>::iterator job = jobs.begin();job!=jobs.end();++job) {
      std::string line = "<LI><I>job</I> <A HREF=\"";
      line+=config.Endpoint()+"/"+(*job);
      line+="\">";
      line+=(*job);
      line+="</A>\r\n";
      html+=line;
    };
    html+="</UL>\r\n";
    // Service description access
    html+="<A HREF=\""+config.Endpoint()+"/?info>SERVICE DESCRIPTION</A>";
    html+="</BODY>\r\n</HTML>";
    Arc::PayloadRaw* buf = new Arc::PayloadRaw;
    if(buf) buf->Insert(html.c_str(),0,html.length());
    outmsg.Payload(buf);
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  if(id == "?info") {
    int h = infodoc_.OpenDocument();
    if(h == -1) return Arc::MCC_Status();
    Arc::MessagePayload* payload = newFileRead(h);
    if(!payload) {
      ::close(h);
      return Arc::MCC_Status();
    };
    outmsg.Payload(payload);
    outmsg.Attributes()->set("HTTP:content-type","text/xml");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Get: there is no job %s - %s", id, job.Failure());
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  if(!http_get(outmsg,config.Endpoint()+"/"+id,job,subpath,range_start,range_end)) {
    // Can't get file
    logger.msg(Arc::ERROR, "Get: can't process file %s", subpath);
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
} 

Arc::MCC_Status ARexService::Head(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,const std::string& id,const std::string& subpath) {
  if(id.empty()) {
    Arc::PayloadRaw* buf = new Arc::PayloadRaw;
    if(buf) buf->Truncate(0);
    outmsg.Payload(buf);
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  return Arc::MCC_Status();
}

// burl - base URL
// bpath - base path
// hpath - path relative to base path and base URL
// start - chunk start
// start - chunk end
static Arc::MCC_Status http_get(Arc::Message& outmsg,const std::string& burl,ARexJob& job,std::string hpath,size_t start,size_t end) {
Arc::Logger::rootLogger.msg(Arc::VERBOSE, "http_get: start=%u, end=%u, burl=%s, hpath=%s", (unsigned int)start, (unsigned int)end, burl, hpath);
  if(!hpath.empty()) if(hpath[0] == '/') hpath=hpath.substr(1);
  if(!hpath.empty()) if(hpath[hpath.length()-1] == '/') hpath.resize(hpath.length()-1);
  std::string joblog = job.LogDir();
  if(!joblog.empty()) {
    if((strncmp(joblog.c_str(),hpath.c_str(),joblog.length()) == 0)  && 
       ((hpath[joblog.length()] == '/') || (hpath[joblog.length()] == 0))) {
      hpath.erase(0,joblog.length()+1);
      if(hpath.empty()) {
        std::list<std::string> logs = job.LogFiles();
        std::string html;
        html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Job Logs</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
        for(std::list<std::string>::iterator l = logs.begin();l != logs.end();++l) {
          if(strncmp(l->c_str(),"proxy",5) == 0) continue;
          std::string line = "<LI><I>file</I> <A HREF=\"";
          line+=burl+"/"+joblog+"/"+(*l);
          line+="\">";
          line+=*l;
          line+="</A> - log file\r\n";
          html+=line;
        };
        html+="</UL>\r\n</BODY>\r\n</HTML>";
        Arc::PayloadRaw* buf = new Arc::PayloadRaw;
        if(buf) buf->Insert(html.c_str(),0,html.length());
        outmsg.Payload(buf);
        outmsg.Attributes()->set("HTTP:content-type","text/html");
        return Arc::MCC_Status(Arc::STATUS_OK);
      } else {
        int file = job.OpenLogFile(hpath);
        if(file != -1) {
          Arc::MessagePayload* h = newFileRead(file,start,end);
          outmsg.Payload(h);
          outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
          return Arc::MCC_Status(Arc::STATUS_OK);
        };
        return Arc::MCC_Status();
      };
    };
  };
  Glib::Dir* dir = job.OpenDir(hpath);
  if(dir) {
    // Directory - html with file list
    std::string file;
    std::string html;
    html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Job</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
    std::string furl = burl;
    if(!hpath.empty()) furl+="/"+hpath;
    std::string path = job.GetFilePath(hpath);
    for(;;) {
      file=dir->read_name();
      if(file.empty()) break;
      if(file == ".") continue;
      if(file == "..") continue;
      std::string fpath = path+"/"+file;
      struct stat st;
      if(lstat(fpath.c_str(),&st) == 0) {
        if(S_ISREG(st.st_mode)) {
          std::string line = "<LI><I>file</I> <A HREF=\"";
          line+=furl+"/"+file;
          line+="\">";
          line+=file;
          line+="</A> - "+Arc::tostring(st.st_size)+" bytes"+"\r\n";
          html+=line;
        } else if(S_ISDIR(st.st_mode)) {
          std::string line = "<LI><I>dir</I> <A HREF=\"";
          line+=furl+"/"+file+"/";
          line+="\">";
          line+=file;
          line+="</A>\r\n";
          html+=line;
        };
      } else {
        std::string line = "<LI><I>unknown</I> <A HREF=\"";
        line+=furl+"/"+file;
        line+="\">";
        line+=file;
        line+="</A>\r\n";
        html+=line;
      };
    };
    if((hpath.empty()) && (!joblog.empty())) {
      std::string line = "<LI><I>dir</I> <A HREF=\"";
      line+=furl+"/"+joblog;
      line+="\">";
      line+=joblog;
      line+="</A> - log directory\r\n";
      html+=line;
    };
    html+="</UL>\r\n</BODY>\r\n</HTML>";
    Arc::PayloadRaw* buf = new Arc::PayloadRaw;
    if(buf) buf->Insert(html.c_str(),0,html.length());
    outmsg.Payload(buf);
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    delete dir;
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  int file = job.OpenFile(hpath,true,false);
  if(file != -1) {
    // File 
    Arc::MessagePayload* h = newFileRead(file,start,end);
    outmsg.Payload(h);
    outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  // Can't process this path
  // offset=0; size=0;
  return Arc::MCC_Status();
}

} // namespace ARex

