// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <globus_openssl.h>

extern "C" {
/* See http://goc.grid.sinica.edu.tw/gocwiki/UsingLiblfcWithThreads
 * for LFC threading info */

/* use POSIX standard threads library */
#include <pthread.h>
extern int Cthread_init(); /* from <Cthread_api.h> */
#define thread_t      pthread_t
#define thread_create(tid_p, func, arg)  pthread_create(tid_p, NULL, func, arg)
#define thread_join(tid)               pthread_join(tid, NULL)
#define NSTYPE_LFC 1

#include <lfc_api.h>

#ifndef _THREAD_SAFE
#define _THREAD_SAFE
#endif
#include <serrno.h>
}

#include <arc/GUID.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/globusutils/GlobusWorkarounds.h>

#include "DataPointLFC.h"

namespace Arc {

  static bool proxy_initialized = false;
  static bool persistent_initialized = false;

  Logger DataPointLFC::logger(Logger::getRootLogger(), "DataPoint.LFC");

  class LFCEnvLocker: public CertEnvLocker {
  public:
    static Logger logger;
    LFCEnvLocker(const UserConfig& usercfg, const URL& url):CertEnvLocker(usercfg) {
      EnvLockUnwrap(false);
      // if root, we have to set X509_USER_CERT and X509_USER_KEY to
      // X509_USER_PROXY to force LFC to use the proxy. If they are undefined
      // the LFC lib uses the host cert and key.
      if (getuid() == 0 && !GetEnv("X509_USER_PROXY").empty()) {
        SetEnv("X509_USER_KEY", GetEnv("X509_USER_PROXY"), true);
        SetEnv("X509_USER_CERT", GetEnv("X509_USER_PROXY"), true);
      }
      // set retry env variables (don't overwrite if set already)
      // connection timeout
      SetEnv("LFC_CONNTIMEOUT", "30", false);
      // number of retries
      SetEnv("LFC_CONRETRY", "1", false);
      // interval between retries
      SetEnv("LFC_CONRETRYINT", "10", false);

      // set host name env var
      SetEnv("LFC_HOST", url.Host());

      logger.msg(DEBUG, "Using proxy %s", GetEnv("X509_USER_PROXY"));
      logger.msg(DEBUG, "Using key %s", GetEnv("X509_USER_KEY"));
      logger.msg(DEBUG, "Using cert %s", GetEnv("X509_USER_CERT"));
      EnvLockWrap(false);
    };
    ~LFCEnvLocker(void) {
    };
  };

  Logger LFCEnvLocker::logger(Logger::getRootLogger(), "LFCEnvLocker");

  #define LFCLOCKINT(result,func,url,err) { \
    LFCEnvLocker lfc_lock(usercfg,url); \
    result = func; \
    err = serrno; \
  }

  #define LFCLOCKVOID(func,url,err) { \
    LFCEnvLocker lfc_lock(usercfg,url); \
    func; \
    err = serrno; \
  }

  DataPointLFC::DataPointLFC(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointIndex(url, usercfg, parg),
      guid(""),
      path_for_guid(url.Path()),
      error_no(0) {
    /*
    // set retry env variables (don't overwrite if set already)
    // connection timeout
    SetEnv("LFC_CONNTIMEOUT", "30", false);
    // number of retries
    SetEnv("LFC_CONRETRY", "1", false);
    // interval between retries
    SetEnv("LFC_CONRETRYINT", "10", false);
    */
  }

  DataPointLFC::~DataPointLFC() {}

  Plugin* DataPointLFC::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "lfc")
      return NULL;
    // Make this code non-unloadable because Globus
    // may have problems with unloading
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus "
                        "in non-persistent mode - LFC code is disabled. Report to developers.");
      return NULL;
    }
    if(!persistent_initialized) {
      factory->makePersistent(module);
      persistent_initialized = true;
    };
    OpenSSLInit();
    if (!proxy_initialized) {
#ifndef WITH_CTHREAD
      /* Initialize Cthread library - should be called before any LFC-API function */
      if (0 != Cthread_init()) {
        logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
        return NULL;
      }
#endif
      // LFC uses GSI plugin which uses Globus GSI which 
      // causes a lot of mess in OpenSSL. Trying to intercept.
#ifdef HAVE_GLOBUS_THREAD_SET_MODEL
      globus_thread_set_model("pthread");
#endif
      GlobusPrepareGSSAPI();
      GlobusModuleActivate(GLOBUS_OPENSSL_MODULE);
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    }
    return new DataPointLFC(*dmcarg, *dmcarg, arg);
  }

  DataStatus DataPointLFC::Check(bool check_meta) {
    // simply check that the file can be resolved
    DataStatus r = Resolve(true);
    if (r) return r;
    return DataStatus(DataStatus::CheckError, r.GetErrno(), r.GetDesc());
  }

  /// Class for passing resolve arguments to separate thread function
  class ResolveArgs {
  public:
    ResolveArgs(const char** lfns, const char** guids, int size, int* nbentries, struct lfc_filereplicas** entries) :
      lfns(lfns), guids(guids), size(size), nbentries(nbentries), entries(entries), result(0), serrno_(0) {}
    ~ResolveArgs() { delete[] lfns; delete[] guids; }
    const char** lfns;
    const char** guids;
    int size; // size of the lfn or guid list
    int* nbentries;
    struct lfc_filereplicas** entries;
    int result;
    int serrno_;
    SimpleCounter count;
  };

  /// Function for resolving in a separate thread
  void do_resolve(void* arg) {
    ResolveArgs* args = (ResolveArgs*)arg;
    // If guid is supplied it is preferred over LFN
    if (args->guids && *(args->guids) && **(args->guids)) {
      args->result = lfc_getreplicas(args->size, args->guids, NULL, args->nbentries, args->entries);
      args->serrno_ = serrno;
    } else if (args->lfns && *(args->lfns) && **(args->lfns)) {
      args->result = lfc_getreplicasl(args->size, args->lfns, NULL, args->nbentries, args->entries);
      args->serrno_ = serrno;
    } else {
      // this case should never happen, but just in case...
      args->result = -1;
      args->serrno_ = EINVAL;
    }
  }

  DataStatus DataPointLFC::Resolve(bool source) {

    // NOTE: Sessions are not used in Resolve(), because under heavy load in A-REX
    // it can end up using all the LFC server threads. See bug 2576 for more info.
    //
    // Resolving is also carried out using a separate thread wrapped in a timeout
    // in order to avoid hanging caused by misbehaving DNS, which LFC client does
    // not (and will not) handle. See bug 2762.

    int lfc_r = 0;
    std::string path = url.Path();

    if (source) {
      // check for guid in the attributes
      if (!url.MetaDataOption("guid").empty()) guid = url.MetaDataOption("guid");
    }
    else if (path.empty() || path == "/") {
      path = ResolveGUIDToLFN();
      if (path.empty()) {
        if (source) return DataStatus(DataStatus::ReadResolveError, lfc2errno());
        return DataStatus(DataStatus::WriteResolveError, lfc2errno());
      }
    }
    if (!source && url.Locations().size() == 0 && !HaveLocations()) {
      logger.msg(ERROR, "Locations are missing in destination LFC URL");
      return DataStatus(DataStatus::WriteResolveError, EINVAL, "No locations specified");
    }

    resolved = false;
    registered = false;
    int* nbentries = new int(0);
    struct lfc_filereplicas **lfc_entries = new struct lfc_filereplicas*;
    const char** lfns = new const char*[1];
    lfns[0] = path.c_str();
    const char** guids = new const char*[1];
    guids[0] = guid.c_str();
    ResolveArgs* args = new ResolveArgs(lfns, guids, 1, nbentries, lfc_entries);
    bool res;
    {
      LFCEnvLocker lfc_env(usercfg, url);
      res = CreateThreadFunction(&do_resolve, args, &args->count);
      if (res) {
        res = args->count.wait(300*1000);
      }
    }
    if (!res) {
      // error or timeout. Timeout will leave the thread hanging, and create
      // a memory leak since the ResolveArgs object is not deleted.
      logger.msg(WARNING, "LFC resolve timed out");
      if (source) return DataStatus(DataStatus::ReadResolveError, ETIMEDOUT);
      return DataStatus(DataStatus::WriteResolveError, ETIMEDOUT);
    }
    lfc_r = args->result;
    serrno = args->serrno_;
    delete args;
    int nentries = *nbentries;
    delete nbentries;
    struct lfc_filereplicas *entries = NULL;
    if (lfc_entries) entries = *lfc_entries;
    delete lfc_entries;

    if (lfc_r != 0) {
      logger.msg(ERROR, "Error finding replicas: %s", sstrerror(serrno));
      if (source) return DataStatus(DataStatus::ReadResolveError, lfc2errno());
      return DataStatus(DataStatus::WriteResolveError, lfc2errno());
    }
    if (nentries == 0 || !entries) {
      // Even if file doesn't exist in LFC an entry should be returned
      logger.msg(ERROR, "LFC resolve returned no entries");
      if (entries) free(entries);
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "No results returned");
    }
    if (entries[0].sfn[0] == '\0') {
      if (source) {
        logger.msg(ERROR, "File does not exist in LFC");
        free(entries);
        return DataStatus(DataStatus::ReadResolveError, ENOENT);
      }
    }
    else {
      registered = true;
    }

    if (source) { // add locations resolved above
      for (int n = 0; n < nentries; n++) {
        URL uloc(entries[n].sfn);
        if (!uloc) {
          logger.msg(WARNING, "Skipping invalid location: %s - %s", url.ConnectionURL(), entries[n].sfn);
          continue;
        }
        for (std::map<std::string, std::string>::const_iterator i = url.CommonLocOptions().begin();
             i != url.CommonLocOptions().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        for (std::map<std::string, std::string>::const_iterator i = url.Options().begin();
             i != url.Options().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        if (AddLocation(uloc, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError)
          logger.msg(WARNING, "Duplicate replica found in LFC: %s", uloc.plainstr());
        else
          logger.msg(VERBOSE, "Adding location: %s - %s", url.ConnectionURL(), entries[n].sfn);
      }
    }
    else { // add given locations, checking they don't already exist
      for (std::list<URLLocation>::const_iterator loc = url.Locations().begin(); loc != url.Locations().end(); ++loc) {

        URL uloc(loc->fullstr());
        if (!uloc) {
          logger.msg(WARNING, "Skipping invalid location: %s - %s", url.ConnectionURL(), loc->str());
          continue;
        }

        // check that this replica doesn't exist already
        for (int n = 0; n < nentries; n++) {
          if (std::string(entries[n].sfn) == uloc.plainstr()) {
            logger.msg(ERROR, "Replica %s already exists for LFN %s", entries[n].sfn, url.plainstr());
            free(entries);
            return DataStatus(DataStatus::WriteResolveError, EEXIST);
          }
        }

        for (std::map<std::string, std::string>::const_iterator i = url.CommonLocOptions().begin();
             i != url.CommonLocOptions().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        for (std::map<std::string, std::string>::const_iterator i = url.Options().begin();
             i != url.Options().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        for (std::map<std::string, std::string>::const_iterator i = url.MetaDataOptions().begin();
             i != url.MetaDataOptions().end(); i++)
          uloc.AddMetaDataOption(i->first, i->second, false);

       if (AddLocation(uloc, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError)
          logger.msg(WARNING, "Duplicate replica location: %s", uloc.plainstr());
        else
          logger.msg(VERBOSE, "Adding location: %s - %s", url.ConnectionURL(), uloc.plainstr());
      }
    }

    if (!HaveLocations()) {
      logger.msg(ERROR, "No locations found for %s", url.str());
      free(entries);
      if (source) return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "No valid locations found");
      return DataStatus(DataStatus::WriteResolveError, EINVAL, "No valid locations found");
    }

    // Set meta-attributes
    if (registered) {
      SetSize(entries[0].filesize);
      SetCreated(entries[0].ctime);
      if (entries[0].csumtype[0] && entries[0].csumvalue[0]) {
        std::string csum = entries[0].csumtype;
        if (csum == "MD")
          csum = "md5";
        else if (csum == "AD") 
          csum = "adler32";
        csum += ":";
        csum += entries[0].csumvalue;
        SetCheckSum(csum);
      }
      guid = entries[0].guid;
    }
    free(entries);
    if (CheckCheckSum()) logger.msg(VERBOSE, "meta_get_data: checksum: %s", GetCheckSum());
    if (CheckSize()) logger.msg(VERBOSE, "meta_get_data: size: %llu", GetSize());
    if (CheckCreated()) logger.msg(VERBOSE, "meta_get_data: created: %s", GetCreated().str());

    resolved = true;
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreRegister(bool replication, bool force) {
    int lfc_r;

    if (replication) { /* replicating inside same lfn */
      if (!registered) {
        logger.msg(ERROR, "LFN is missing in LFC (needed for replication)");
        return DataStatus(DataStatus::PreRegisterError, ENOENT);
      }
      return DataStatus::Success;
    }
    if (registered) {
      if (!force) {
        logger.msg(ERROR, "LFN already exists in LFC");
        return DataStatus(DataStatus::PreRegisterError, EEXIST);
      }
      return DataStatus::Success;
    }
    LFCLOCKINT(lfc_r,lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus(DataStatus::PreRegisterError, lfc2errno());
    }
    if (!url.MetaDataOption("guid").empty()) {
      guid = url.MetaDataOption("guid");
      logger.msg(VERBOSE, "Using supplied guid %s", guid);
    }
    else if (guid.empty())
      guid = UUID();

    LFCLOCKINT(lfc_r,lfc_creatg(url.Path().c_str(), guid.c_str(),
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP), url, error_no);
    if(lfc_r != 0) {

      // check error - if it is no such file or directory, try to create the
      // required dirs
      if (serrno == ENOENT) {
        DataStatus r = CreateDirectory(true);
        if (!r) return DataStatus(DataStatus::PreRegisterError, r.GetErrno());

        LFCLOCKINT(lfc_r,lfc_creatg(url.Path().c_str(), guid.c_str(),
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP), url, error_no);
        if(lfc_r != 0 && serrno != EEXIST) {
          logger.msg(ERROR, "Error creating LFC entry: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus(DataStatus::PreRegisterError, lfc2errno());
        }
      }
      // LFN may already exist, caused by 2 uploaders simultaneously uploading to the same LFN
      // ignore the error if force is true and get the previously registered guid 
      else if (serrno == EEXIST && force) {
        struct lfc_filestatg st;
        LFCLOCKINT(lfc_r,lfc_statg(url.Path().c_str(), NULL, &st), url, error_no);
        if(lfc_r == 0) {
          registered = true;
          guid = st.guid;
          lfc_endsess();
          return DataStatus::Success;
        }
        else {
          logger.msg(ERROR, "Error finding info on LFC entry %s which should exist: %s", url.Path(), sstrerror(serrno));
          lfc_endsess();
          return DataStatus(DataStatus::PreRegisterError, lfc2errno());
        }
      }
      else {
        logger.msg(ERROR, "Error creating LFC entry %s, guid %s: %s", url.Path(), guid, sstrerror(serrno));
        lfc_endsess();
        return DataStatus(DataStatus::PreRegisterError, lfc2errno());
      }
    }
    if (CheckCheckSum()) {
      std::string cksum = GetCheckSum();
      std::string::size_type p = cksum.find(':');
      if (p == std::string::npos) {
        std::string ckstype = cksum.substr(0,p);
        if (ckstype=="md5")
          ckstype = "MD";
        if (ckstype=="adler32")
          ckstype = "AD";
        // only md5 and adler32 are supported by LFC
        if (ckstype == "MD" || ckstype == "AD") {
          std::string cksumvalue = cksum.substr(p+1);
          if (CheckSize()) {
            LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(),
                                           const_cast<char*>(cksumvalue.c_str())), url, error_no);
            if(lfc_r != 0)
              logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
          }
          else {
            LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(),
                                           const_cast<char*>(cksumvalue.c_str())), url, error_no);
            if(lfc_r != 0)
              logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
          }
        }
        else
          logger.msg(WARNING, "Warning: only md5 and adler32 checksums are supported by LFC");
      }
    }
    else if (CheckSize()) {
      LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL), url, error_no);
      if(lfc_r != 0)
        logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
    }

    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PostRegister(bool replication) {
    int lfc_r;

    if (guid.empty()) {
      logger.msg(ERROR, "No GUID defined for LFN - probably not preregistered");
      return DataStatus(DataStatus::PostRegisterError, EARCLOGIC, "No GUID defined");
    }
    LFCLOCKINT(lfc_r,lfc_startsess(const_cast<char*>(url.Host().c_str()),
               const_cast<char*>("ARC")), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus(DataStatus::PostRegisterError, lfc2errno());
    }
    LFCLOCKINT(lfc_r,lfc_addreplica(guid.c_str(), NULL, CurrentLocation().Host().c_str(),
                                    CurrentLocation().plainstr().c_str(), '-', 'P', NULL, NULL), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error adding replica: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus(DataStatus::PostRegisterError, lfc2errno());
    }
    if (!replication && !registered) {
      if (CheckCheckSum()) {
        std::string cksum = GetCheckSum();
        std::string::size_type p = cksum.find(':');
        if(p != std::string::npos) {
          std::string ckstype = cksum.substr(0,p);
          if (ckstype=="md5")
            ckstype = "MD";
          if (ckstype=="adler32")
            ckstype = "AD";
          // only md5 and adler32 are supported by LFC
          if (ckstype == "MD" || ckstype == "AD") {
            std::string cksumvalue = cksum.substr(p+1);
            if (CheckSize()) {
              logger.msg(VERBOSE, "Entering checksum type %s, value %s, file size %llu", ckstype, cksumvalue, GetSize());
              LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(),
                                             const_cast<char*>(cksumvalue.c_str())), url, error_no);
              if(lfc_r != 0)
                logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
            }
            else {
              LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(),
                                             const_cast<char*>(cksumvalue.c_str())), url, error_no);
              if(lfc_r != 0)
                logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
            }
          }
          else
            logger.msg(WARNING, "Warning: only md5 and adler32 checksums are supported by LFC");
        }
      }
      else if (CheckSize()) {
        LFCLOCKINT(lfc_r,lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL), url, error_no);
        if(lfc_r != 0)
          logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
      }
    }
    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreUnregister(bool replication) {
    int lfc_r;

    if (replication || registered)
      return DataStatus::Success;
    LFCLOCKINT(lfc_r,lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus(DataStatus::UnregisterError, lfc2errno());
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus::UnregisterError;
    }
    LFCLOCKINT(lfc_r,lfc_unlink(path.c_str()), url, error_no);
    if(lfc_r != 0)
      if ((serrno != ENOENT) && (serrno != ENOTDIR)) {
        logger.msg(ERROR, "Failed to remove LFN in LFC - You may need to do it by hand");
        lfc_endsess();
        return DataStatus(DataStatus::UnregisterError, lfc2errno());
      }
    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::Unregister(bool all) {
    int lfc_r;

    if (!all && (!LocationValid())) {
      logger.msg(ERROR, "Location is missing");
      return DataStatus(DataStatus::UnregisterError, EINVAL, "No location");
    }
    LFCLOCKINT(lfc_r,lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus(DataStatus::UnregisterError, lfc2errno());
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus(DataStatus::UnregisterError, lfc2errno());
    }
    if (all) {
      int nbentries = 0;
      struct lfc_filereplica *entries = NULL;
      LFCLOCKINT(lfc_r,lfc_getreplica(path.c_str(), NULL, NULL, &nbentries, &entries), url, error_no);
      if(lfc_r != 0) {
        lfc_endsess();
        if (error_no == ENOENT) {
          registered = false;
          ClearLocations();
          return DataStatus::Success;
        }
        logger.msg(ERROR, "Error getting replicas: %s", sstrerror(error_no));
        return DataStatus(DataStatus::UnregisterError, lfc2errno());
      }
      for (int n = 0; n < nbentries; n++) {
        LFCLOCKINT(lfc_r,lfc_delreplica(guid.c_str(), NULL, entries[n].sfn), url, error_no);
        if(lfc_r != 0) {
          if (serrno == ENOENT)
            continue;
          lfc_endsess();
          logger.msg(ERROR, "Failed to remove location from LFC: %s", sstrerror(error_no));
          return DataStatus(DataStatus::UnregisterError, lfc2errno());
        }
      }
      LFCLOCKINT(lfc_r,lfc_unlink(path.c_str()), url, error_no);
      if(lfc_r != 0) {
        if (serrno == EPERM) { // we have a directory
          LFCLOCKINT(lfc_r,lfc_rmdir(path.c_str()), url, error_no);
          if(lfc_r != 0) {
            if (serrno == EEXIST) { // still files in the directory
              logger.msg(ERROR, "Failed to remove LFC directory: directory is not empty");
              lfc_endsess();
              return DataStatus(DataStatus::UnregisterError, ENOTEMPTY);
            }
            logger.msg(ERROR, "Failed to remove LFC directory: %s", sstrerror(serrno));
            lfc_endsess();
            return DataStatus(DataStatus::UnregisterError, lfc2errno());
          }
        }
        else if ((serrno != ENOENT) && (serrno != ENOTDIR)) {
          logger.msg(ERROR, "Failed to remove LFN in LFC: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus(DataStatus::UnregisterError, lfc2errno());
        }
      }
      registered = false;
    }
    else {
      LFCLOCKINT(lfc_r,lfc_delreplica(guid.c_str(), NULL, CurrentLocation().plainstr().c_str()), url, error_no);
      if(lfc_r != 0) {
        lfc_endsess();
        logger.msg(ERROR, "Failed to remove location from LFC: %s", sstrerror(serrno));
        return DataStatus(DataStatus::UnregisterError, lfc2errno());
      }
    }
    lfc_endsess();
    return DataStatus::Success;
  }


  DataStatus DataPointLFC::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    std::list<FileInfo> files;
    DataStatus r = ListFiles(files,verb,false);
    if(!r) {
      return DataStatus(DataStatus::StatError, r.GetErrno(), r.GetDesc());
    }
    if(files.size() < 1) {
      return DataStatus(DataStatus::StatError, EARCRESINVAL, "No results returned");
    }
    file = files.front();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::Stat(std::list<FileInfo>& files,
                                const std::list<DataPoint*>& urls,
                                DataPointInfoType verb) {
    // Bulk operations not implemented yet
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      FileInfo f;
      DataStatus res = (*i)->Stat(f, verb);
      if (!res.Passed()) return res;
      else files.push_back(f);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    return ListFiles(files,verb,true);
  }
 
  DataStatus DataPointLFC::ListFiles(std::list<FileInfo>& files, DataPointInfoType verb, bool listdir) {
    int lfc_r;

    LFCLOCKINT(lfc_r,lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus(DataStatus::ListError, lfc2errno());
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus(DataStatus::ListError, lfc2errno());
    }
    // first stat the url and see if it is a file or directory
    struct lfc_filestatg st;

    LFCLOCKINT(lfc_r,lfc_statg(path.c_str(), NULL, &st), url, error_no);
    if(lfc_r != 0) {
      logger.msg(ERROR, "Error listing file or directory: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus(DataStatus::ListError, lfc2errno());
    }

    if (listdir) {
      if(!(st.filemode & S_IFDIR)) {
        logger.msg(ERROR, "Not a directory");
        lfc_endsess();
        return DataStatus(DataStatus::ListNonDirError, ENOTDIR);
      }
      lfc_DIR *dir = NULL;
      {
        LFCEnvLocker lfc_lock(usercfg, url);
        dir = lfc_opendirxg(const_cast<char*>(url.Host().c_str()), path.c_str(), NULL);
      }
      if (dir == NULL) {
        logger.msg(ERROR, "Error opening directory: %s", sstrerror(serrno));
        error_no = serrno;
        lfc_endsess();
        return DataStatus(DataStatus::ListError, lfc2errno());
      }

      // list entries. If not long list use readdir
      // does funny things with pointers, so always use long listing for now
      //if (false) { // if (!long_list) { // causes compiler warnings so just comment
      //  struct dirent *direntry;
      //  while ((direntry = lfc_readdir(dir)))
      //    std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(direntry->d_name));
      //}
      //else { // for long list use readdirg
      {
        struct lfc_direnstatg *direntry;
        {
          LFCEnvLocker lfc_lock(usercfg, url);
          direntry = lfc_readdirg(dir);
        }
        while (direntry) {
          std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(direntry->d_name));

          f->SetSize(direntry->filesize);
          if (strcmp(direntry->csumtype, "") != 0 && strcmp(direntry->csumvalue, "") != 0) {
            std::string csum = direntry->csumtype;
            csum += ":";
            csum += direntry->csumvalue;
            f->SetCheckSum(csum);
          }
          f->SetCreated(direntry->ctime);
          f->SetType((direntry->filemode & S_IFDIR) ? FileInfo::file_type_dir : FileInfo::file_type_file);
          {
            LFCEnvLocker lfc_lock(usercfg, url);
            direntry = lfc_readdirg(dir);
          }
        }
      }
      if (serrno) {
        logger.msg(ERROR, "Error listing directory: %s", sstrerror(serrno));
        int err = lfc2errno();
        LFCLOCKINT(lfc_r,lfc_closedir(dir), url, error_no);
        lfc_endsess();
        return DataStatus(DataStatus::ListError, err);
      }

      // if we want to resolve replicas call readdirxr
      if (verb & INFO_TYPE_STRUCT) {
        LFCLOCKVOID(lfc_rewinddir(dir), url, error_no);
        struct lfc_direnrep *direntry;

        {
          LFCEnvLocker lfc_lock(usercfg, url);
          direntry = lfc_readdirxr(dir, NULL);
        }
        while (direntry) {
          for (std::list<FileInfo>::iterator f = files.begin();
               f != files.end();
               f++) {
            if (f->GetName() == direntry->d_name) {
              for (int n = 0; n < direntry->nbreplicas; n++) {
                f->AddURL(URL(std::string(direntry->rep[n].sfn)));
              }
              break;
            }
          }
          // break if found ?
          {
            LFCEnvLocker lfc_lock(usercfg, url);
            direntry = lfc_readdirxr(dir, NULL);
          }
        }
        if (serrno) {
          logger.msg(ERROR, "Error listing directory: %s", sstrerror(serrno));
          int err = lfc2errno();
          LFCLOCKINT(lfc_r,lfc_closedir(dir), url, error_no);
          lfc_endsess();
          return DataStatus(DataStatus::ListError, err);
        }
      }
      LFCLOCKINT(lfc_r,lfc_closedir(dir), url, error_no);
    } // if (listdir)
    else {
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(path.c_str()));
      f->SetMetaData("path", path);
      f->SetSize(st.filesize);
      f->SetMetaData("size", tostring(st.filesize));
      SetSize(st.filesize);
      if (st.csumtype[0] && st.csumvalue[0]) {
        std::string csum = st.csumtype;
        if (csum == "MD")
          csum = "md5";
        else if (csum == "AD")
          csum = "adler32";
        csum += ":";
        csum += st.csumvalue;
        f->SetCheckSum(csum);
        f->SetMetaData("checksum", csum);
        SetCheckSum(csum);
      }
      f->SetCreated(st.mtime);
      SetCreated(Arc::Time(st.mtime));
      f->SetMetaData("mtime", f->GetCreated().str());
      f->SetType((st.filemode & S_IFDIR) ? FileInfo::file_type_dir : FileInfo::file_type_file);
      f->SetMetaData("type", (st.filemode & S_IFDIR) ? "dir" : "file");
      if (verb & INFO_TYPE_STRUCT) {
        int nbentries = 0;
        struct lfc_filereplica *entries = NULL;

        LFCLOCKINT(lfc_r,lfc_getreplica(path.c_str(), NULL, NULL, &nbentries, &entries), url, error_no);
        if(lfc_r != 0) {
          logger.msg(ERROR, "Error listing replicas: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus(DataStatus::ListError, lfc2errno());
        }
        for (int n = 0; n < nbentries; n++)
          f->AddURL(URL(std::string(entries[n].sfn)));
      }
      // fill some more metadata
      if (st.guid[0] != '\0') f->SetMetaData("guid", std::string(st.guid));
      if (verb & INFO_TYPE_ACCESS) {
        if (st.uid != 0) {
          char username[256];
          LFCLOCKINT(lfc_r,lfc_getusrbyuid(st.uid, username), url, error_no);
          if(lfc_r == 0) f->SetMetaData("owner", username);
        }
        if (st.gid != 0) {
          char groupname[256];
          LFCLOCKINT(lfc_r,lfc_getgrpbygid(st.gid, groupname), url, error_no);
          if(lfc_r == 0) f->SetMetaData("group", groupname);
        }
      }
      mode_t mode = st.filemode;
      std::string perms;
      if (mode & S_IRUSR) perms += 'r'; else perms += '-';
      if (mode & S_IWUSR) perms += 'w'; else perms += '-';
      if (mode & S_IXUSR) perms += 'x'; else perms += '-';
      if (mode & S_IRGRP) perms += 'r'; else perms += '-';
      if (mode & S_IWGRP) perms += 'w'; else perms += '-';
      if (mode & S_IXGRP) perms += 'x'; else perms += '-';
      if (mode & S_IROTH) perms += 'r'; else perms += '-';
      if (mode & S_IWOTH) perms += 'w'; else perms += '-';
      if (mode & S_IXOTH) perms += 'x'; else perms += '-';
      f->SetMetaData("accessperm", perms);
      f->SetMetaData("ctime", (Time(st.ctime)).str());
      f->SetMetaData("atime", (Time(st.atime)).str());
    }
    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::CreateDirectory(bool with_parents) {

    std::string::size_type slashpos = url.Path().find("/", 1); // don't create root dir
    int lfc_r;

    if (!with_parents) {
      std::string dirname = url.Path().substr(0, url.Path().rfind("/"));
      if (dirname.empty() || dirname == url.Path()) return DataStatus::Success;

      logger.msg(VERBOSE, "Creating LFC directory %s", dirname);
      LFCLOCKINT(lfc_r,lfc_mkdir(dirname.c_str(), 0775), url, error_no);
      if (lfc_r == 0 || serrno == EEXIST) return DataStatus::Success;

      logger.msg(ERROR, "Error creating required LFC dirs: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus(DataStatus::CreateDirectoryError, lfc2errno());
    }
    while (slashpos != std::string::npos) {
      std::string dirname = url.Path().substr(0, slashpos);
      // list dir to see if it exists
      struct lfc_filestat statbuf;
      LFCLOCKINT(lfc_r,lfc_stat(dirname.c_str(), &statbuf), url, error_no);
      if(lfc_r == 0) {
        slashpos = url.Path().find("/", slashpos + 1);
        continue;
      }

      logger.msg(VERBOSE, "Creating LFC directory %s", dirname);
      LFCLOCKINT(lfc_r,lfc_mkdir(dirname.c_str(), 0775), url, error_no);
      if(lfc_r != 0)
        if (serrno != EEXIST) {
          logger.msg(ERROR, "Error creating required LFC dirs: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus(DataStatus::CreateDirectoryError, lfc2errno());
        }
      slashpos = url.Path().find("/", slashpos + 1);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::Rename(const URL& newurl) {

    // source can be specified by GUID, but destination must have path
    std::string path = url.Path();
    if (path.empty() || path == "/") path = ResolveGUIDToLFN();
    if (path.empty()) return DataStatus(DataStatus::RenameError, lfc2errno());

    if (newurl.Path().empty() || newurl.Path() == "/") {
      logger.msg(ERROR, "Cannot rename to root directory");
      return DataStatus(DataStatus::RenameError, EINVAL);
    }
    logger.msg(VERBOSE, "Renaming %s to %s", path, newurl.Path());

    int lfc_r;
    LFCLOCKINT(lfc_r, lfc_rename(path.c_str(), newurl.Path().c_str()), url, error_no);
    if (lfc_r != 0) {
      logger.msg(ERROR, "Error renaming %s to %s: %s", path, newurl.Path(), sstrerror(serrno));
      return DataStatus(DataStatus::RenameError, lfc2errno());
    }
    return DataStatus::Success;
  }

  std::string DataPointLFC::ResolveGUIDToLFN() {

    // check if guid is already defined
    if (!guid.empty()) {
      if (path_for_guid.empty()) return "/";
      return path_for_guid;
    }

    // check for guid in the attributes
    if (url.MetaDataOption("guid").empty()) {
      if (url.Path().empty()) return "/";
      return url.Path();
    }

    guid = url.MetaDataOption("guid");

    lfc_list listp;
    struct lfc_linkinfo *info = NULL;
    {
      LFCEnvLocker lfc_lock(usercfg, url);
      info = lfc_listlinks(NULL, (char*)guid.c_str(),
                           CNS_LIST_BEGIN, &listp);
    }
    if (!info) {
      logger.msg(ERROR, "Error finding LFN from guid %s: %s",
                 guid, sstrerror(serrno));
      error_no = serrno;
      guid.clear();
      return "";
    }

    logger.msg(VERBOSE, "guid %s resolved to LFN %s", guid, info[0].path);
    path_for_guid = info[0].path;
    {
      LFCEnvLocker lfc_lock(usercfg, url);
      lfc_listlinks(NULL, (char*)guid.c_str(), CNS_LIST_END, &listp);
    }

    if (path_for_guid.empty()) return "/";
    return path_for_guid;
  }

  const std::string DataPointLFC::DefaultCheckSum() const {
    return std::string("adler32");
  }

  std::string DataPointLFC::str() const {
    std::string tmp = url.plainstr(); // url with no options or meta-options
    // add guid if supplied
    if (!url.MetaDataOption("guid").empty())
      tmp += ":guid=" + url.MetaDataOption("guid");
    return tmp;
  }

  DataStatus DataPointLFC::Resolve(bool source, const std::list<DataPoint*>& urls) {

    if (urls.empty()) return DataStatus::Success;

    // sanity check
    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      if ((*i)->GetURL().Protocol() != url.Protocol() ||
          (*i)->GetURL().Host() != url.Host()) {
        logger.msg(ERROR, "Mismatching protocol/host in bulk resolve!");
        if (source) return DataStatus(DataStatus::ReadResolveError, EINVAL);
        return DataStatus(DataStatus::WriteResolveError, EINVAL);
      }
    }

    // TODO: Destination lookup in bulk
    if (!source) {
      for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
        DataStatus res = (*i)->Resolve(source);
        if (!res.Passed()) return res;
      }
      return DataStatus::Success;
    }

    // NOTE: Sessions are not used in Resolve(), because under heavy load in A-REX
    // it can end up using all the LFC server threads. See bug 2576 for more info.

    // Figure out whether to use guids or LFNs in bulk request
    bool use_guids = !urls.front()->GetURL().MetaDataOption("guid").empty();

    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      if ((!use_guids && !(*i)->GetURL().MetaDataOption("guid").empty()) ||
          (use_guids && (*i)->GetURL().MetaDataOption("guid").empty())) {
        // cannot have a mixture of guids and LFNs
        logger.msg(ERROR, "Cannot use a mixture of GUIDs and LFNs in bulk resolve");
        return DataStatus(DataStatus::ReadResolveError, EINVAL);
      }
    }

    int* nbentries = new int(0);
    struct lfc_filereplicas **lfc_entries = new struct lfc_filereplicas*;
    int lfc_r;

    const char ** guids = new const char*[urls.size()];
    const char ** paths = new const char*[urls.size()];
    if (use_guids) {
      unsigned int n = 0;
      for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i, ++n) {
        guids[n] = (*i)->GetURL().MetaDataOption("guid").c_str();
      }
      paths[0] = NULL;
    }
    else {
      unsigned int n = 0;
      for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i, ++n) {
        paths[n] = (*i)->GetURL().Path().c_str();
      }
      guids[0] = NULL;
    }

    // See Resolve(bool) for explanation
    ResolveArgs* args = new ResolveArgs(paths, guids, urls.size(), nbentries, lfc_entries);
    bool res;
    {
      LFCEnvLocker lfc_env(usercfg, url);
      res = CreateThreadFunction(&do_resolve, args, &args->count);
      if (res) {
        res = args->count.wait(300*1000);
      }
    }
    if (!res) {
      // error or timeout. Timeout will leave the thread hanging, and create
      // a memory leak since the ResolveArgs object is not deleted.
      logger.msg(WARNING, "LFC resolve timed out");
      return DataStatus(DataStatus::ReadResolveError, ETIMEDOUT);
    }
    lfc_r = args->result;
    serrno = args->serrno_;
    delete args;
    int nentries = *nbentries;
    delete nbentries;
    struct lfc_filereplicas *entries = NULL;
    if (lfc_entries) entries = *lfc_entries;
    delete lfc_entries;

    if(lfc_r != 0) {
      logger.msg(ERROR, "Error finding replicas: %s", sstrerror(serrno));
      return DataStatus(DataStatus::ReadResolveError, lfc2errno());
    }
    if (nentries == 0 || !entries) {
      logger.msg(ERROR, "Bulk resolve returned no entries");
      return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "No results returned");
    }

    // We assume that the order of returned GUIDs is the same as the order of
    // urls, then there is no need to resolve LFNs to GUIDs if we only have LFNs.
    // Entries has at least one entry per GUID, even if there are no replicas
    // and has an entry per replica if there are multiple.
    std::string previous_guid(entries[0].guid);
    unsigned int guid_count = 0;
    std::list<DataPoint*>::const_iterator datapoint = urls.begin();
    DataPoint* dp = *datapoint;

    for (int n = 0; n < nentries; n++) {
      logger.msg(DEBUG, "GUID %s, SFN %s", entries[n].guid, entries[n].sfn);

      if (previous_guid != entries[n].guid) {
        if (++guid_count >= urls.size() || datapoint == urls.end()) {
          logger.msg(ERROR, "LFC returned more results than we asked for!");
          free(entries);
          return DataStatus(DataStatus::ReadResolveError, EARCRESINVAL, "More results than expected");
        }
        ++datapoint;
        dp = *datapoint;
        previous_guid = entries[n].guid;
      }

      if (entries[n].sfn[0] == '\0') {
        logger.msg(WARNING, "No locations found for %s", dp->GetURL().str());
        continue;
      }

      URL uloc(entries[n].sfn);
      if (!uloc) {
        logger.msg(WARNING, "Skipping invalid location: %s - %s", url.ConnectionURL(), entries[n].sfn);
        continue;
      }
      // Add URL options to replicas
      for (std::map<std::string, std::string>::const_iterator i = dp->GetURL().CommonLocOptions().begin();
           i != dp->GetURL().CommonLocOptions().end(); i++)
        uloc.AddOption(i->first, i->second, false);

      if (dp->AddLocation(uloc, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError)
        logger.msg(WARNING, "Duplicate replica found in LFC: %s", uloc.plainstr());
      else
        logger.msg(VERBOSE, "Adding location: %s - %s", dp->GetURL().ConnectionURL(), entries[n].sfn);

      // Add metadata if available
      dp->SetSize(entries[n].filesize);
      dp->SetCreated(entries[n].ctime);
      if (entries[n].csumtype[0] && entries[n].csumvalue[0]) {
        std::string csum = entries[n].csumtype;
        if (csum == "MD")
          csum = "md5";
        else if (csum == "AD")
          csum = "adler32";
        csum += ":";
        csum += entries[n].csumvalue;
        dp->SetCheckSum(csum);
      }
    }
    if (entries)
      free(entries);

    for (std::list<DataPoint*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
      DataPoint * dp = *i;
      if (dp->CheckCheckSum()) logger.msg(VERBOSE, "meta_get_data: checksum: %s", dp->GetCheckSum());
      if (dp->CheckSize()) logger.msg(VERBOSE, "meta_get_data: size: %llu", dp->GetSize());
      if (dp->CheckCreated()) logger.msg(VERBOSE, "meta_get_data: created: %s", dp->GetCreated().str());
      // dp->resolved = true; ??
    }
    return DataStatus::Success;
  }

  int DataPointLFC::lfc2errno() const {
    // serrno codes in includepath/lfc/serrno.h
    // Not all codes are handled here

    // Regular errno
    if (error_no < SEBASEOFF) return error_no;

    switch (error_no) {
      case SECOMERR:     // communication error
      case ENSNACT:      // name server not active
      case SEINTERNAL:   // internal error
      case SECONNDROP:   // connection dropped by server
      case SEWOULDBLOCK: // temporarily unavailable
      case SESYSERR:     // system error
        return EARCSVCTMP;

      case SETIMEDOUT:   // timeout
        return ETIMEDOUT;

      case SENOMAPFND:   // no user mapping
        return EACCES;

      default:
        return EARCSVCPERM;
    }
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "lfc", "HED:DMC", "LCG File Catalog", 0, &Arc::DataPointLFC::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
