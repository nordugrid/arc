// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <cstdlib>

#include <globus_io.h>
#include <glibmm/fileutils.h>

#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataCallback.h>
#include <arc/data/CheckSum.h>
#include <arc/globusutils/GlobusWorkarounds.h>


#include "DataPointSRM.h"

namespace Arc {

  Logger DataPointSRM::logger(Logger::getRootLogger(), "DataPoint.SRM");

  static bool proxy_initialized = false;

  DataPointSRM::DataPointSRM(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      srm_request(NULL),
      r_handle(NULL),
      reading(false),
      writing(false),
      timeout(false) {
    valid_url_options.push_back("protocol");
    valid_url_options.push_back("spacetoken");
    if (!proxy_initialized) {
      globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE);
      globus_module_activate(GLOBUS_IO_MODULE);
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    }
  }

  DataPointSRM::~DataPointSRM() {
    if(r_handle) delete r_handle;
    if(srm_request) delete srm_request;
    //globus_module_deactivate(GLOBUS_IO_MODULE);
    //globus_module_deactivate(GLOBUS_GSI_GSSAPI_MODULE);
  }

  Plugin* DataPointSRM::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "srm")
      return NULL;
    // Make this code non-unloadable because Globus
    // may have problems with unloading
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(factory && module) factory->makePersistent(module);
    return new DataPointSRM(*dmcarg, *dmcarg);
  }

  DataStatus DataPointSRM::Check() {

    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout);
    if (!client) {
      if (timeout)
        return DataStatus::CheckErrorRetryable;
      return DataStatus::CheckError;
    }

    if (url.HTTPOption("SFN", "") == "")
      srm_request = new SRMClientRequest(url.str());
    else
      srm_request = new SRMClientRequest(url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN"));
    
    if (!srm_request) {
      delete client;
      client = NULL;
      return DataStatus::CheckError;
    }

    logger.msg(VERBOSE, "Check: looking for metadata: %s", CurrentLocation().str());
    srm_request->long_list(true);
    std::list<struct SRMFileMetaData> metadata;

    SRMReturnCode res = client->info(*srm_request, metadata);
    delete srm_request;
    srm_request = NULL;
    delete client;
    client = NULL;

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::CheckErrorRetryable;
      return DataStatus::CheckError;
    }
    
    if (metadata.empty())
      return DataStatus::CheckError;
    if (metadata.front().size > 0) {
      logger.msg(INFO, "Check: obtained size: %lli", metadata.front().size);
      SetSize(metadata.front().size);
    }
    if (metadata.front().checkSumValue.length() > 0 &&
        metadata.front().checkSumType.length() > 0) {
      std::string csum(metadata.front().checkSumType + ":" + metadata.front().checkSumValue);
      logger.msg(INFO, "Check: obtained checksum: %s", csum);
      SetCheckSum(csum);
    }
    if (metadata.front().createdAtTime > 0) {
      logger.msg(INFO, "Check: obtained creation date: %s", Time(metadata.front().createdAtTime).str());
      SetCreated(Time(metadata.front().createdAtTime));
    }
    if (metadata.front().fileLocality == SRM_ONLINE) {
      logger.msg(INFO, "Check: obtained access latency: low (ONLINE)");
      SetAccessLatency(ACCESS_LATENCY_SMALL);
    }
    else if (metadata.front().fileLocality == SRM_NEARLINE) {
      logger.msg(INFO, "Check: obtained access latency: high (NEARLINE)");
      SetAccessLatency(ACCESS_LATENCY_LARGE);
    }

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::Remove() {

    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout);
    if (!client) {
      if (timeout)
        return DataStatus::DeleteErrorRetryable;
      return DataStatus::DeleteError;
    }

    // take out options in srm url
    std::string canonic_url;
    if (!url.HTTPOption("SFN").empty())
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
    else
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

    srm_request = new SRMClientRequest(canonic_url);

    if (!srm_request) {
      delete client;
      client = NULL;
      return DataStatus::DeleteError;
    }
    logger.msg(VERBOSE, "remove_srm: deleting: %s", CurrentLocation().str());

    SRMReturnCode res = client->remove(*srm_request);
    delete client;
    client = NULL;
    delete srm_request;
    srm_request = NULL;

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::DeleteErrorRetryable;              
      return DataStatus::DeleteError;
    }

    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StartReading(DataBuffer& buf) {

    logger.msg(VERBOSE, "StartReading");
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;

    reading = true;
    buffer = &buf;

    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout, buffer->speed.get_max_inactivity_time());
    if (!client) {
      reading = false;
      if (timeout)
        return DataStatus::ReadStartErrorRetryable;
      return DataStatus::ReadStartError;
    }

    // take out options in srm url
    std::string canonic_url;
    if (!url.HTTPOption("SFN").empty())
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
    else
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

    delete srm_request;
    srm_request = new SRMClientRequest(canonic_url);
    
    if (!srm_request) {
      delete client;
      client = NULL;
      return DataStatus::ReadStartError;
    }
    std::list<std::string> turls;
    SRMReturnCode res = client->getTURLs(*srm_request, turls);
    client->disconnect();
    delete client;
    client = NULL;
    
    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::ReadStartErrorRetryable;
      return DataStatus::ReadStartError;
    }

    std::srand(time(NULL));

    // Choose handled URL randomly
    for (;;) {
      if (turls.size() <= 0)
        break;
      int n = (int)((std::rand() * ((double)(turls.size() - 1))) / RAND_MAX + 0.25);
      std::list<std::string>::iterator i = turls.begin();
      for (; n; ++i, ++n) {}
      if (i == turls.end())
        continue;
      // Avoid redirection to SRM
      logger.msg(VERBOSE, "Checking URL returned by SRM: %s", *i);
      if (strncasecmp(i->c_str(), "srm://", 6) == 0) {
        turls.erase(i);
        continue;
      }
      // Try to use this TURL + old options
      r_url = *i;
      {
        std::map<std::string, std::string> options = url.Options();
        if (!options.empty())
          for (std::map<std::string, std::string>::iterator oi = options.begin(); oi != options.end(); ++oi)
            r_url.AddOption((*oi).first, (*oi).second);
      }
      r_handle = new DataHandle(r_url, usercfg);
      // check if url can be handled
      if (!r_handle) {
        turls.erase(i);
        continue;
      }
      if ((*r_handle)->IsIndex()) {
        delete r_handle;
        r_handle = NULL;
        turls.erase(i);
        continue;
      }
      break;
    }

    if (!r_handle) {
      logger.msg(INFO, "SRM returned no useful Transfer URLs: %s", url.str());
      return DataStatus::ReadStartError;
    }

    (*r_handle)->SetAdditionalChecks(false); // checks at higher levels are always done on SRM metadata
    (*r_handle)->SetSecure(force_secure);
    (*r_handle)->Passive(force_passive);

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    if (!(*r_handle)->StartReading(buf)) {
      delete r_handle;
      r_handle = NULL;
      reading = false;
      return DataStatus::ReadStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StopReading() {

    if (!reading) {
      delete srm_request;
      srm_request = NULL;
      return DataStatus::ReadStopError;
    }
    reading = false;

    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopReading();
      delete r_handle;
    }
    
    if (srm_request) {
      SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout, buffer->speed.get_max_inactivity_time());
      if (client) {
        if(buffer->error_read() || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
          client->abort(*srm_request);
        } else if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS) {
          client->releaseGet(*srm_request);
        }
        delete client;
        client = NULL;
      }
      delete srm_request;
    }
    r_handle = NULL;
    srm_request = NULL;
    return r;
  }

  DataStatus DataPointSRM::StartWriting(DataBuffer& buf,
                                        DataCallback* /* space_cb */) {

    logger.msg(VERBOSE, "StartWriting");
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;

    writing = true;
    buffer = &buf;

    SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout, buffer->speed.get_max_inactivity_time());
    if (!client) {
      writing = false;
      if (timeout)
        return DataStatus::WriteStartErrorRetryable;
      return DataStatus::WriteStartError;
    }

    // take out options in srm url
    std::string canonic_url;
    if (!url.HTTPOption("SFN").empty())
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
    else
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

    delete srm_request;
    srm_request = new SRMClientRequest(canonic_url);
    if (!srm_request) {
      delete client;
      client = NULL;
      writing = false;
      return DataStatus::WriteStartError;
    }

    // set space token
    std::string space_token = url.Option("spacetoken");
    if (space_token.empty()) {
      if (client->getVersion().compare("v2.2") == 0)
        // only print message if using v2.2
        logger.msg(VERBOSE, "No space token specified");
    }
    else {
      if (client->getVersion().compare("v2.2") != 0)
        // print warning if not using srm2.2
        logger.msg(WARNING, "Warning: Using SRM protocol v1 which does not support space tokens");
      else {
        logger.msg(VERBOSE, "Using space token description %s", space_token);
        // get token from SRM that matches description
        std::list<std::string> tokens;
        if (client->getSpaceTokens(tokens, space_token) != SRM_OK)
          // not critical so log a warning
          logger.msg(WARNING, "Warning: Error looking up space tokens matching description %s. Will copy without using token", space_token);
        else if (tokens.empty())
          // not critical so log a warning
          logger.msg(WARNING, "Warning: No space tokens found matching description! Will copy without using token");
        else {
          // take the first one in the list
          logger.msg(VERBOSE, "Using space token %s", tokens.front());
          srm_request->space_token(tokens.front());
        }
      }
    }

    std::list<std::string> turls;
    SRMReturnCode res = client->putTURLs(*srm_request, turls);
    client->disconnect();
    delete client;
    client = NULL;
    
    if (res != SRM_OK) {
      writing = false;
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::WriteStartErrorRetryable;
      return DataStatus::WriteStartError;
    }

    std::srand(time(NULL));

    // Choose handled URL randomly
    for (;;) {
      if (turls.size() <= 0)
        break;
      int n = (int)((std::rand() * ((double)(turls.size() - 1))) / RAND_MAX + 0.25);
      std::list<std::string>::iterator i = turls.begin();
      for (; n; ++i, ++n) {}
      if (i == turls.end())
        continue;
      // Avoid redirection to SRM
      logger.msg(VERBOSE, "Checking URL returned by SRM: %s", *i);
      if (strncasecmp(i->c_str(), "srm://", 6) == 0) {
        turls.erase(i);
        continue;
      }
      // Try to use this TURL + old options
      r_url = *i;
      {
        std::map<std::string, std::string> options = url.Options();
        if (!options.empty())
          for (std::map<std::string, std::string>::iterator oi = options.begin(); oi != options.end(); ++oi)
            r_url.AddOption((*oi).first, (*oi).second);
      }
      r_handle = new DataHandle(r_url, usercfg);
      // check if url can be handled
      if (!r_handle) {
        turls.erase(i);
        continue;
      }
      if ((*r_handle)->IsIndex()) {
        delete r_handle;
        r_handle = NULL;
        turls.erase(i);
        continue;
      }
      break;
    }

    if (!r_handle) {
      logger.msg(INFO, "SRM returned no useful Transfer URLs: %s", url.str());
      return DataStatus::WriteStartError;
    }

    logger.msg(INFO, "Redirecting to new URL: %s", (*r_handle)->CurrentLocation().str());
    if (!(*r_handle)->StartWriting(buf)) {
      delete r_handle;
      r_handle = NULL;
      return DataStatus::WriteStartError;
    }
    return DataStatus::Success;
  }

  DataStatus DataPointSRM::StopWriting() {

    if (!writing) {
      delete srm_request;
      srm_request = NULL;
      return DataStatus::WriteStopError;
    }
    writing = false;

    DataStatus r = DataStatus::Success;
    if (r_handle) {
      r = (*r_handle)->StopWriting();
      // disable checksum check at this level if was done at lower level
      // gsiftp turls should provide checksum, but don't, so do checks at srm level
      //if ((*r_handle)->ProvidesMeta())
      //  additional_checks = false;
      delete r_handle;
      r_handle = NULL;
      
      if (!r) {
        SRMClient *client = SRMClient::getInstance(usercfg, url.fullstr(), timeout, buffer->speed.get_max_inactivity_time());
        if(client) {
          // abort() may not delete the file, so call remove() after
          client->abort(*srm_request);
          client->remove(*srm_request);
          delete client;
          client = NULL;
        }
        delete srm_request;
        srm_request = NULL;
        return r;
      }
    }

    SRMClient * client = SRMClient::getInstance(usercfg, url.fullstr(), timeout, buffer->speed.get_max_inactivity_time());
    if(client) {
      // call abort if failure, or releasePut on success
      if(buffer->error() || srm_request->status() == SRM_REQUEST_SHOULD_ABORT) {
        client->abort(*srm_request);
        client->remove(*srm_request);
      }
      else {
        // checksum verification
        if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS && additional_checks) {
          std::string csum;
          // is checksum supplied or calculated?
          if (CheckCheckSum()) {
            csum = checksum;
          } else {
            const CheckSumAny * cs = (CheckSumAny*)buffer->checksum_object();
            if (cs && *cs && buffer->checksum_valid()) {
              char buf[100];
              cs->print(buf,100);
              csum = buf;
            }
          }
          if (!csum.empty() && csum.find(':') != std::string::npos) {
            // get checksum info for checksum verification
            logger.msg(VERBOSE, "StopWriting: looking for metadata: %s", url.str());
            // create a new request
            SRMClientRequest list_request(srm_request->surls());
            list_request.long_list(true);
            std::list<struct SRMFileMetaData> metadata;
            SRMReturnCode res = client->info(list_request,metadata);
            if (res != SRM_OK) {
              client->abort(*srm_request); // if we can't list then we can't remove either
              if (res == SRM_ERROR_TEMPORARY)
                return DataStatus::WriteStopErrorRetryable;              
              return DataStatus::WriteStopError;
            }
            /* provide some metadata */
            if(!metadata.empty()){
              if(metadata.front().checkSumValue.length() > 0 &&
                 metadata.front().checkSumType.length() > 0) {
                std::string servercsum(metadata.front().checkSumType+":"+metadata.front().checkSumValue);
                logger.msg(INFO, "StopWriting: obtained checksum: %s", servercsum);
                if (csum.substr(0, csum.find(':')) == metadata.front().checkSumType) {
                  if (csum.substr(csum.find(':')+1) == metadata.front().checkSumValue) {
                    logger.msg(INFO, "Calculated/supplied transfer checksum %s matches checksum reported by SRM destination %s", csum, servercsum);
                  }
                  else {
                    logger.msg(ERROR, "Checksum mismatch between calculated/supplied checksum (%s) and checksum reported by SRM destination (%s)", csum, servercsum);
                    r = DataStatus::WriteStopErrorRetryable;
                  }
                } else logger.msg(WARNING, "Checksum type of SRM (%s) and calculated/supplied checksum (%s) differ, cannot compare", servercsum, csum);
              } else logger.msg(WARNING, "No checksum information from server");
            } else logger.msg(WARNING, "No checksum information from server");
          } else logger.msg(INFO, "No checksum verification possible");
        }
        if (r) {
          if (srm_request->status() == SRM_REQUEST_FINISHED_SUCCESS)
            client->releasePut(*srm_request);
        } else {
          client->abort(*srm_request);
          client->remove(*srm_request);
        }
      }
    }
    delete srm_request;
    srm_request = NULL;
    delete client;
    client=NULL;
    return r;
  }

  DataStatus DataPointSRM::Stat(FileInfo& file, DataPointInfoType verb) {
    std::list<FileInfo> files;
    DataStatus r = ListFiles(files,verb,-1);
    if(files.size() != 1) return DataStatus::StatError;
    file = *(files.begin());
    if(r == DataStatus::ListErrorRetryable) r = DataStatus::StatErrorRetryable;
    if(r == DataStatus::ListError) r = DataStatus::StatError;
    return r;
  }

  DataStatus DataPointSRM::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    return ListFiles(files,verb,0);
  }

  DataStatus DataPointSRM::ListFiles(std::list<FileInfo>& files, DataPointInfoType verb, int recursion) {

    SRMClient * client = SRMClient::getInstance(usercfg, url.fullstr(), timeout);
    if(!client) {
      if (timeout)
        return DataStatus::ListErrorRetryable;
      return DataStatus::ListError;
    }
    
    std::string canonic_url;
    if (!url.HTTPOption("SFN").empty())
      canonic_url = url.Protocol() + "://" + url.Host() + "/" + url.HTTPOption("SFN");
    else
      canonic_url = url.Protocol() + "://" + url.Host() + url.Path();

    delete srm_request;
    srm_request = new SRMClientRequest(canonic_url);
    if (!srm_request) {
      delete client;
      client = NULL;
      return DataStatus::ListError;
    }
    logger.msg(VERBOSE, "ListFiles: looking for metadata: %s", CurrentLocation().str());
    if ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME) srm_request->long_list(true);
    std::list<struct SRMFileMetaData> srm_metadata;

    // get info from SRM
    SRMReturnCode res = client->info(*srm_request, srm_metadata, recursion);
    delete client;
    client = NULL;
    delete srm_request;
    srm_request = NULL;

    if (res != SRM_OK) {
      if (res == SRM_ERROR_TEMPORARY) return DataStatus::ListErrorRetryable;   
      return DataStatus::ListError;
    }

    if (srm_metadata.empty()) {
      return DataStatus::Success;
    }
    // set URL attributes for surl requested (file or dir)
    if(srm_metadata.front().size > 0)
      SetSize(srm_metadata.front().size);
    if(srm_metadata.front().checkSumType.length() > 0 &&
       srm_metadata.front().checkSumValue.length() > 0) {
      std::string csum(srm_metadata.front().checkSumType+":"+srm_metadata.front().checkSumValue);
      SetCheckSum(csum);
    }
    if(srm_metadata.front().createdAtTime > 0) 
      SetCreated(Time(srm_metadata.front().createdAtTime));

    // set FileInfo attributes for surl requested and any files within a dir
    for (std::list<struct SRMFileMetaData>::iterator i = srm_metadata.begin();
         i != srm_metadata.end();
         ++i) {

      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(i->path));
      f->SetMetaData("path", i->path);
      
      if (i->fileType == SRM_FILE) {
        f->SetType(FileInfo::file_type_file);
        f->SetMetaData("type", "file");
      }
      else if (i->fileType == SRM_DIRECTORY) {
        f->SetType(FileInfo::file_type_dir);
        f->SetMetaData("type", "dir");
      }

      if (i->size >= 0) {
        f->SetSize(i->size);
        f->SetMetaData("size", tostring(i->size));
      }
      if (i->createdAtTime > 0) {
        f->SetCreated(Time(i->createdAtTime));
        f->SetMetaData("ctime", (Time(i->createdAtTime)).str());
      }
      if (i->checkSumType.length() > 0 &&
          i->checkSumValue.length() > 0) {
        std::string csum(i->checkSumType + ":" + i->checkSumValue);
        f->SetCheckSum(csum);
        f->SetMetaData("checksum", csum);
      }
      if (i->fileLocality == SRM_ONLINE) {
        f->SetLatency("ONLINE");
        f->SetMetaData("latency", "ONLINE");
      }
      else if (i->fileLocality == SRM_NEARLINE) {
        f->SetLatency("NEARLINE");
        f->SetMetaData("latency", "NEARLINE");
      }
      if(!i->arrayOfSpaceTokens.empty()) f->SetMetaData("spacetokens", i->arrayOfSpaceTokens);
      if(!i->owner.empty()) f->SetMetaData("owner", i->owner);
      if(!i->group.empty()) f->SetMetaData("group", i->group);
      if(!i->permission.empty()) f->SetMetaData("accessperm", i->permission);
      if(i->lastModificationTime > 0)
        f->SetMetaData("mtime", (Time(i->lastModificationTime)).str());
      if(i->lifetimeLeft != 0) f->SetMetaData("lifetimeleft", tostring(i->lifetimeLeft));
      if(i->lifetimeAssigned != 0) f->SetMetaData("lifetimeassigned", tostring(i->lifetimeAssigned));
  
      if (i->retentionPolicy == SRM_REPLICA) f->SetMetaData("retentionpolicy", "REPLICA");
      else if (i->retentionPolicy == SRM_OUTPUT) f->SetMetaData("retentionpolicy", "OUTPUT");
      else if (i->retentionPolicy == SRM_CUSTODIAL)  f->SetMetaData("retentionpolicy", "CUSTODIAL");

      if (i->fileStorageType == SRM_VOLATILE) f->SetMetaData("filestoragetype", "VOLATILE");
      else if (i->fileStorageType == SRM_DURABLE) f->SetMetaData("filestoragetype", "DURABLE");
      else if (i->fileStorageType == SRM_PERMANENT) f->SetMetaData("filestoragetype", "PERMANENT"); 

    }
    return DataStatus::Success;
  }

  const std::string DataPointSRM::DefaultCheckSum() const {
    return std::string("adler32");
  }

  bool DataPointSRM::ProvidesMeta() {
    return true;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "srm", "HED:DMC", 0, &Arc::DataPointSRM::Instance },
  { NULL, NULL, 0, NULL }
};
