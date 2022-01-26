#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

#include <glibmm.h>

#include <arc/FileLock.h>
#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/message/PayloadRaw.h>
#include <arc/data/FileCache.h>
#include "PayloadFile.h"
#include "job.h"

#include "arex.h"


#define MAX_CHUNK_SIZE (10*1024*1024)

namespace ARex {


static Arc::PayloadRaw* newFileInfo(int handle = -1) {
  Arc::PayloadRaw* buf = new Arc::PayloadRaw;
  if(handle != -1) {
    struct stat st;
    if(buf && (::fstat(handle,&st) == 0)) buf->Truncate(st.st_size);
    ::close(handle);
  } else {
    if(buf) buf->Truncate(0);
  }
  return buf;
}


static Arc::PayloadRaw* newFileInfo(Arc::FileAccess& file) {
  struct stat st;
  Arc::PayloadRaw* buf = new Arc::PayloadRaw;
  if(buf && (file.fa_fstat(st))) buf->Truncate(st.st_size);
  return buf;
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

// --------------------------------------------------------------------------------------------------------------

static Arc::MCC_Status GetJobsList(Arc::Message& outmsg,ARexGMConfig& config,Arc::Logger& logger) {
  std::string html;
  html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Jobs list</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
  std::list<std::string> jobs = ARexJob::Jobs(config,logger);
  for(std::list<std::string>::iterator job = jobs.begin();job!=jobs.end();++job) {
    std::string line = "<LI><I>job</I> <A HREF=\"";
    line+=config.Endpoint()+"/"+(*job);
    line+="\">";
    line+=(*job);
    line+="</A>";
    line+=" <A HREF=\"";
    line+=config.Endpoint()+"/"+ARexService::LogsPath+"/"+(*job);
    line+="\">logs</A>\r\n";
    html+=line;
  };
  html+="</UL>\r\n";
  // Service description access
  html+="<A HREF=\""+config.Endpoint()+"/"+ARexService::InfoPath+"\">SERVICE DESCRIPTION</A>";
  html+="</BODY>\r\n</HTML>";
  Arc::PayloadRaw* buf = new Arc::PayloadRaw;
  if(buf) buf->Insert(html.c_str(),0,html.length());
  outmsg.Payload(buf);
  outmsg.Attributes()->set("HTTP:content-type","text/html");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status GetFilesList(Arc::Message& outmsg,ARexGMConfig& config,
                          Arc::FileAccess& dir,std::string const& baseurl,std::string const& basepath,
                          Arc::Logger& logger) {
  std::string html;
  html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Job</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
  for(;;) {
    std::string file;
    if(!dir.fa_readdir(file)) break;
    if(file == ".") continue;
    if(file == "..") continue;
    std::string fpath = basepath+"/"+file;
    struct stat st;
    if(lstat(fpath.c_str(),&st) == 0) {
      if(S_ISREG(st.st_mode)) {
        std::string line = "<LI><I>file</I> <A HREF=\"";
        line+=baseurl+"/"+file;
        line+="\">";
        line+=file;
        line+="</A> - "+Arc::tostring(st.st_size)+" bytes"+"\r\n";
        html+=line;
      } else if(S_ISDIR(st.st_mode)) {
        std::string line = "<LI><I>dir</I> <A HREF=\"";
        line+=baseurl+"/"+file+"/";
        line+="\">";
        line+=file;
        line+="</A>\r\n";
        html+=line;
      };
    } else {
      std::string line = "<LI><I>unknown</I> <A HREF=\"";
      line+=baseurl+"/"+file;
      line+="\">";
      line+=file;
      line+="</A>\r\n";
      html+=line;
    };
  };
  // Add virtual logs folder
  /*
      if((hpath.empty()) && (!joblog.empty())) {
        std::string line = "<LI><I>dir</I> <A HREF=\"";
        line+=furl+"/"+joblog;
        line+="\">";
        line+=joblog;
        line+="</A> - log directory\r\n";
        html+=line;
      };
  */
  html+="</UL>\r\n</BODY>\r\n</HTML>";
  Arc::PayloadRaw* buf = new Arc::PayloadRaw;
  if(buf) buf->Insert(html.c_str(),0,html.length());
  outmsg.Payload(buf);
  outmsg.Attributes()->set("HTTP:content-type","text/html");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

// --------------------------------------------------------------------------------------------------------------

Arc::MCC_Status ARexService::GetJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  if(id.empty()) {
    // Not a specific job - generate page with list of jobs
    return GetJobsList(outmsg,config,logger_);
  }
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Get: there is no job %s - %s", id, job.Failure());
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  };
  std::string hpath = Arc::trim(subpath,"/"); // prepare clean sub-path
  std::string joblog = job.LogDir();
  if(!joblog.empty()) {
    if((strncmp(joblog.c_str(),hpath.c_str(),joblog.length()) == 0)  && 
       ((hpath[joblog.length()] == '/') || (hpath[joblog.length()] == '\0'))) {
      hpath.erase(0,joblog.length()+1);
      return GetLogs(inmsg,outmsg,config,id,hpath);
    };
  };
  // File or folder
  Arc::FileAccess* dir = job.OpenDir(subpath);
  if(dir) {
    // Directory - html with file list
    std::string dirurl = config.Endpoint()+"/"+id;
    if(!hpath.empty()) dirurl+="/"+hpath;
    std::string dirpath = job.GetFilePath(hpath);
    Arc::MCC_Status r = GetFilesList(outmsg,config,*dir,dirurl,dirpath,logger_);
    dir->fa_closedir();
    Arc::FileAccess::Release(dir);
    return r;
  };
  Arc::FileAccess* file = job.OpenFile(hpath,true,false);
  if(file) {
    // File or similar
    off_t range_start;
    off_t range_end;
    ExtractRange(inmsg, range_start, range_end);
    Arc::MessagePayload* h = newFileRead(file,range_start,range_end);
    if(!h) {
      file->fa_close(); Arc::FileAccess::Release(file);
      return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
    };
    outmsg.Payload(h);
    outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  // Can't process this path
  // offset=0; size=0;
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::GetLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  if(id.empty()) {
    // Not a specific job - not expected
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  }
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Get: there is no job %s - %s", id, job.Failure());
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  };
  std::string hpath = Arc::trim(subpath,"/"); // prepare clean sub-path
  if(hpath.empty()) {
    std::list<std::string> logs = job.LogFiles();
    std::string html;
    html="<HTML>\r\n<HEAD>\r\n<TITLE>ARex: Job Logs</TITLE>\r\n</HEAD>\r\n<BODY>\r\n<UL>\r\n";
    for(std::list<std::string>::iterator l = logs.begin();l != logs.end();++l) {
      if(strncmp(l->c_str(),"proxy",5) == 0) continue;
      std::string line = "<LI><I>file</I> <A HREF=\"";
      line+=config.Endpoint()+"/"+ARexService::LogsPath+"/"+id+"/"+(*l);
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
    if(hpath != "proxy") {
      int file = job.OpenLogFile(hpath);
      if(file != -1) {
        off_t range_start;
        off_t range_end;
        ExtractRange(inmsg, range_start, range_end);
        Arc::MessagePayload* h = newFileRead(file,range_start,range_end);
        if(h) {
          outmsg.Payload(h);
          outmsg.Attributes()->set("HTTP:content-type","text/plain");
          return Arc::MCC_Status(Arc::STATUS_OK);
        } else {
          ::close(file);
        };
      };
    };
  };
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::GetInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!subpath.empty()) return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  return GetInfo(inmsg, outmsg);
}

Arc::MCC_Status ARexService::GetInfo(Arc::Message& inmsg,Arc::Message& outmsg) {
  int h = OpenInfoDocument();
  if(h == -1) return Arc::MCC_Status();
  Arc::MessagePayload* payload = newFileRead(h);
  if(!payload) { ::close(h); return Arc::MCC_Status(); };
  outmsg.Payload(payload);
  outmsg.Attributes()->set("HTTP:content-type","text/xml");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::GetNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::GetCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  off_t range_start = 0;
  off_t range_end = (off_t)(-1);
  ExtractRange(inmsg, range_start, range_end);
  return cache_get(outmsg, subpath, range_start, range_end, config, false);
}

// --------------------------------------------------------------------------------------------------------------

Arc::MCC_Status ARexService::HeadLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(id.empty()) {
    // Not a specific job - not expected
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  }
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Get: there is no job %s - %s", id, job.Failure());
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  };
  std::string hpath = Arc::trim(subpath,"/"); // prepare clean sub-path
  if(hpath.empty()) {
    Arc::PayloadRaw* buf = new Arc::PayloadRaw;
    if(buf) buf->Truncate(0);
    outmsg.Payload(buf);
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    return Arc::MCC_Status(Arc::STATUS_OK);
  } else {
    int file = job.OpenLogFile(hpath);
    if(file != -1) {
      struct stat st;
      Arc::PayloadRaw* buf = new Arc::PayloadRaw;
      if(buf && (::fstat(file,&st) == 0)) buf->Truncate(st.st_size);
      ::close(file);
      outmsg.Payload(buf);
      outmsg.Attributes()->set("HTTP:content-type","text/plain");
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::HeadInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!subpath.empty()) return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  return HeadInfo(inmsg, outmsg);
}

Arc::MCC_Status ARexService::HeadInfo(Arc::Message& inmsg,Arc::Message& outmsg) {
  int h = OpenInfoDocument();
  if(h == -1) return Arc::MCC_Status();
  outmsg.Payload(newFileInfo(h));
  outmsg.Attributes()->set("HTTP:content-type","text/xml");
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::HeadNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

Arc::MCC_Status ARexService::HeadCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  off_t range_start = 0;
  off_t range_end = (off_t)(-1);
  return cache_get(outmsg, subpath, range_start, range_end, config, true);
}

Arc::MCC_Status ARexService::HeadJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  if(!&config) {
    return make_http_fault(outmsg, HTTP_ERR_FORBIDDEN, "User is not identified");
  };
  if(id.empty()) {
    // Not a specific job - page with list of jobs
    outmsg.Payload(newFileInfo());
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Head: there is no job %s - %s", id, job.Failure());
    return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
  };
  std::string hpath = Arc::trim(subpath,"/"); // prepare clean sub-path
  std::string joblog = job.LogDir();
  if(!joblog.empty()) {
    if((strncmp(joblog.c_str(),hpath.c_str(),joblog.length()) == 0)  &&
       ((hpath[joblog.length()] == '/') || (hpath[joblog.length()] == '\0'))) {
      hpath.erase(0,joblog.length()+1);
      return HeadLogs(inmsg,outmsg,config,id,hpath);
    };
  };
  // File or folder
  Arc::FileAccess* dir = job.OpenDir(subpath);
  if(dir) {
    // Directory - html with file list
    outmsg.Payload(newFileInfo());
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    dir->fa_closedir();
    Arc::FileAccess::Release(dir);
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  Arc::FileAccess* file = job.OpenFile(hpath,true,false);
  if(file) {
    // File or similar
    outmsg.Payload(newFileInfo(*file));
    file->fa_close(); Arc::FileAccess::Release(file);
    outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  // Can't process this path
  // offset=0; size=0;
  return Arc::MCC_Status(Arc::UNKNOWN_SERVICE_ERROR);
}

// -------------------------------------------------------------------------------------------------

static bool cache_get_allowed(const std::string& url, ARexGMConfig& config, Arc::Logger& logger) {

  // Extract information from credentials
  std::string dn;              // DN of credential
  std::string vo;              // Assuming only one VO
  std::list<std::string> voms; // VOMS attributes

  for (std::list<Arc::MessageAuth*>::const_iterator a = config.beginAuth(); a!=config.endAuth(); ++a) {
    if (*a) {
      Arc::SecAttr* sattr = (*a)->get("TLS");
      if (!sattr) continue;
      dn = sattr->get("IDENTITY");
      vo = sattr->get("VO");
      voms = sattr->getAll("VOMS");
      break;
    }
  }
  // At least DN should be found. VOMS info may not be present.
  if (dn.empty()) {
    logger.msg(Arc::ERROR, "Failed to extract credential information");
    return false;
  }
  logger.msg(Arc::DEBUG, "Checking cache permissions: DN: %s", dn);
  logger.msg(Arc::DEBUG, "Checking cache permissions: VO: %s", vo);
  for (std::list<std::string>::const_iterator att = voms.begin(); att != voms.end(); ++att) {
    logger.msg(Arc::DEBUG, "Checking cache permissions: VOMS attr: %s", *att);
  }

  // Cache configuration specifies URL regexps and a certificate attribute and
  // value. Go through looking for a match.
  for (std::list<struct CacheConfig::CacheAccess>::const_iterator access = config.GmConfig().CacheParams().getCacheAccess().begin();
       access != config.GmConfig().CacheParams().getCacheAccess().end(); ++access) {
    if (access->regexp.match(url)) {
      if (Arc::lower(access->cred_type) == "dn") {
        if (access->cred_value.match(dn)) {
          logger.msg(Arc::VERBOSE, "Cache access allowed to %s by DN %s", url, dn);
          return true;
        }
        logger.msg(Arc::DEBUG, "DN %s doesn't match %s", dn, access->cred_value.getPattern());
      } else if (Arc::lower(access->cred_type) == "voms:vo") {
        if (access->cred_value.match(vo)) {
          logger.msg(Arc::VERBOSE, "Cache access allowed to %s by VO %s", url, vo);
          return true;
        }
        logger.msg(Arc::DEBUG, "VO %s doesn't match %s", vo, access->cred_value.getPattern());
      } else if (Arc::lower(access->cred_type) == "voms:role") {
        // Get the configured allowed role
        std::vector<std::string> role_parts;
        Arc::tokenize(access->cred_value.getPattern(), role_parts, ":");
        if (role_parts.size() != 2) {
          logger.msg(Arc::WARNING, "Bad credential value %s in cache access rules", access->cred_value.getPattern());
          continue;
        }
        std::string cred_vo = role_parts[0];
        std::string cred_role = role_parts[1];
        std::string allowed_role("/VO="+cred_vo+"/Group="+cred_vo+"/Role="+cred_role);
        for (std::list<std::string>::const_iterator attr = voms.begin(); attr != voms.end(); ++attr) {
          if (*attr == allowed_role) {
            logger.msg(Arc::DEBUG, "VOMS attr %s matches %s", *attr, allowed_role);
            logger.msg(Arc::VERBOSE, "Cache access allowed to %s by VO %s and role %s", url, cred_vo, cred_role);
            return true;
          }
          logger.msg(Arc::DEBUG, "VOMS attr %s doesn't match %s", *attr, allowed_role);
        }
      } else if (Arc::lower(access->cred_type) == "voms:group") {
        // Get the configured allowed group
        std::vector<std::string> group_parts;
        Arc::tokenize(access->cred_value.getPattern(), group_parts, ":");
        if (group_parts.size() != 2) {
          logger.msg(Arc::WARNING, "Bad credential value %s in cache access rules", access->cred_value.getPattern());
          continue;
        }
        std::string cred_vo = group_parts[0];
        std::string cred_group = group_parts[1];
        std::string allowed_group("/VO="+cred_vo+"/Group="+cred_vo+"/Group="+cred_group);
        for (std::list<std::string>::const_iterator attr = voms.begin(); attr != voms.end(); ++attr) {
          if (*attr == allowed_group) {
            logger.msg(Arc::DEBUG, "VOMS attr %s matches %s", *attr, allowed_group);
            logger.msg(Arc::VERBOSE, "Cache access allowed to %s by VO %s and group %s", url, cred_vo, cred_group);
            return true;
          }
          logger.msg(Arc::DEBUG, "VOMS attr %s doesn't match %s", *attr, allowed_group);
        }
      } else {
        logger.msg(Arc::WARNING, "Unknown credential type %s for URL pattern %s", access->cred_type, access->regexp.getPattern());
      }
    }
  }

  // If we get to here no match was found
  logger.msg(Arc::VERBOSE, "No match found in cache access rules for %s", url);
  return false;
}

Arc::MCC_Status ARexService::cache_get(Arc::Message& outmsg, const std::string& subpath, off_t range_start, off_t range_end, ARexGMConfig& config, bool no_content) {

  // subpath contains the URL, which can be encoded. Constructing a URL
  // object with encoded=true only decodes the path so have to decode first
  std::string unencoded(Arc::uri_unencode(subpath));
  Arc::URL cacheurl(unencoded);
  logger.msg(Arc::INFO, "Get from cache: Looking in cache for %s", cacheurl.str());

  if (!cacheurl) {
    logger.msg(Arc::ERROR, "Get from cache: Invalid URL %s", subpath);
    return make_http_fault(outmsg, 400, "Bad request: Invalid URL");
  }
  // Security check. The access is configured in arc.conf like
  // cache_access="srm://srm-atlas.cern.ch/grid/atlas* voms:vo atlas"
  // then the url is compared to the certificate attribute specified
  if (!cache_get_allowed(cacheurl.str(), config, logger)) {
    return make_http_fault(outmsg, 403, "Forbidden");
  }

  Arc::FileCache cache(config.GmConfig().CacheParams().getCacheDirs(),
                       config.GmConfig().CacheParams().getDrainingCacheDirs(),
                       config.GmConfig().CacheParams().getReadOnlyCacheDirs(),
                       "0", // Jobid is not used
                       config.User().get_uid(),
                       config.User().get_gid());
  if (!cache) {
    logger.msg(Arc::ERROR, "Get from cache: Error in cache configuration");
    return make_http_fault(outmsg, 500, "Error in cache configuration");
  }
  // Get the cache file corresponding to the URL
  std::string cache_file(cache.File(cacheurl.str()));
  // Check if file exists
  struct stat st;
  if (!Arc::FileStat(cache_file, &st, false)) {
    if (errno == ENOENT) {
      logger.msg(Arc::INFO, "Get from cache: File not in cache");
      return make_http_fault(outmsg, 404, "File not found");
    } else {
      logger.msg(Arc::WARNING, "Get from cache: could not access cached file: %s", Arc::StrError(errno));
      return make_http_fault(outmsg, 500, "Error accessing cached file");
    }
  }
  // Check file size against specified range
  if (range_start > st.st_size) range_start = st.st_size;
  if (range_end > st.st_size) range_end = st.st_size;

  // Check if lockfile exists
  if (Arc::FileStat(cache_file + Arc::FileLock::getLockSuffix(), &st, false)) {
    logger.msg(Arc::INFO, "Get from cache: Cached file is locked");
    return make_http_fault(outmsg, 409, "Cached file is locked");
  }

  // Read the file and fill the payload
  if (!no_content) {
    Arc::MessagePayload* h = newFileRead(cache_file.c_str(), range_start, range_end);
    outmsg.Payload(h);
  } else {
    struct stat st;
    Arc::PayloadRaw* buf = new Arc::PayloadRaw;
    if(buf && Arc::FileStat(cache_file, &st, false)) buf->Truncate(st.st_size);
    outmsg.Payload(buf);
  }
  outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
  return Arc::MCC_Status(Arc::STATUS_OK);
}


} // namespace ARex

