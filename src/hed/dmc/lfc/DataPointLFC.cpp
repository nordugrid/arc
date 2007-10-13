#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <uuid/uuid.h>
#include <cerrno>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>

#include "DataPointLFC.h"

extern "C" {
#include <lfc_api.h>
#include <serrno.h>
}

static std::string GUID() {
  uuid_t uu;
  uuid_generate(uu);
  char uustr[37];
  uuid_unparse(uu, uustr);
  return uustr;
}

namespace Arc {

  Logger DataPointLFC::logger(DataPoint::logger, "LFC");

  DataPointLFC::DataPointLFC(const URL& url) : DataPointIndex(url) {}

  /* perform resolve operation, which can take long time */
  bool DataPointLFC::meta_resolve(bool source) {
    is_resolved = false;
    is_metaexisting = false;
    if(source) {
      if(url.Path().empty()) {
        logger.msg(INFO, "Source must contain LFN");
        return false;
      }
    }
    else {
      if(url.Path().empty()) {
        logger.msg(INFO, "Destination must contain LFN");
        return false;
      }
      if(url.Locations().size() == 0) {
        logger.msg(INFO, "Locations are missing in destination LFC URL");
        return false;
      }
    }
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    int nbentries = 0;
    struct lfc_filereplica *entries = NULL;
    if(lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries,
                      &entries) != 0) {
      if(source || ((serrno != ENOENT) && (serrno != ENOTDIR))) {
        lfc_endsess();
        return false;
      }
      nbentries = 0;
      entries = NULL;
    }
    else {
      is_metaexisting = true;
    }
    if(url.Locations().size() == 0) {
      for(int n = 0; n < nbentries; n++) {
        locations.push_back(URLLocation(entries[n].sfn, url.ConnectionURL()));
        logger.msg(DEBUG, "Adding location: %s - %s",
                   url.ConnectionURL().c_str(), entries[n].sfn);
      }
    }
    else {
      for(std::list<URLLocation>::const_iterator loc = url.Locations().begin();
          loc != url.Locations().end(); ++loc) {
        for(int n = 0; n < nbentries; n++) {
          if(strncmp(entries[n].sfn, loc->Name().c_str(),
                     loc->Name().length()) == 0) {
            logger.msg(DEBUG, "Adding location: %s - %s",
                       url.ConnectionURL().c_str(), entries[n].sfn);
            if(source) {
              locations.push_back(URLLocation(entries[n].sfn,
                                              url.ConnectionURL()));
            }
            else {
              locations.push_back(URLLocation(*loc, url.ConnectionURL()));
            }
            break;
          }
        }
      }
    }
    if(entries) free(entries);
    struct lfc_filestatg st;
    if(lfc_statg(url.Path().c_str(), NULL, &st) == 0) {
      is_metaexisting = true;
      SetSize(st.filesize);
      SetCreated(st.mtime);
      if(st.csumtype[0] && st.csumvalue[0]) {
        std::string csum = st.csumtype;
        csum += ":";
        csum += st.csumvalue;
        SetCheckSum(csum);
      }
      guid = st.guid;
    }
    lfc_endsess();
    if(!source) {
      if(locations.size() == 0) {
        logger.msg(INFO, "No locations found for destination");
        return false;
      }
      // Make pfns
      for(std::list<URLLocation>::iterator loc = locations.begin();
          loc != locations.end(); loc++) {
        std::string u = loc->str();
        if(url.Protocol() == "se") {
          u += "?";
        }
        else {
          u += "/";
        }
        u += url.Path();
        *loc = URLLocation(u, loc->Name());
        logger.msg(DEBUG, "Using location: %s - %s",
                   loc->Name().c_str(), loc->str().c_str());
      }
    }
    logger.msg(DEBUG, "meta_get_data: checksum: %s", GetCheckSum().c_str());
    logger.msg(DEBUG, "meta_get_data: size: %ull", GetSize());
    logger.msg(DEBUG, "meta_get_data: created: %s",
               GetCreated().str().c_str());
    if(!url.CommonLocOptions().empty()) {
      for(std::list<URLLocation>::iterator loc = locations.begin();
          loc != locations.end(); ++loc) {
        for(std::map<std::string, std::string>::const_iterator i =
              url.CommonLocOptions().begin();
            i != url.CommonLocOptions().end(); i++)
          loc->AddOption(i->first, i->second, false);
      }
    }
    location = locations.begin();
    is_resolved = true;
    return true;
  }

  bool DataPointLFC::meta_preregister(bool replication, bool force) {
    if(replication) {/* replicating inside same lfn */
      if(!is_metaexisting) {/* for replication it must be there */
        logger.msg(ERROR, "LFN is missing in LFC (needed for replication)");
        return false;
      }
      return true;
    }
    if(is_metaexisting) {/* algorithm require this to be new file */
      if(!force) {
        logger.msg(ERROR, "LFN already exists in LFC");
        return false;
      }
      return true;
    }
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    guid = GUID();
    if(lfc_creatg(url.Path().c_str(), guid.c_str(),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
      lfc_endsess();
      return false;
    }
    if(CheckCheckSum()) {
      std::string ckstype;
      std::string cksumvalue = GetCheckSum();
      std::string::size_type p = cksumvalue.find(':');
      if(p == std::string::npos) {
        ckstype = "cksum";
      }
      else {
        ckstype = cksumvalue.substr(0, p);
        cksumvalue = cksumvalue.substr(p + 1);
      }
      if(CheckSize()) {
        lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(),
                      const_cast<char*>(cksumvalue.c_str()));
      }
      else {
        lfc_setfsizeg(guid.c_str(), -1, ckstype.c_str(),
                      const_cast<char*>(cksumvalue.c_str()));
      }
    }
    else if(CheckSize()) {
      lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL);
    }
    //if(CheckCreated()) {
    //}
    lfc_endsess();
    return true;
  }

  bool DataPointLFC::meta_postregister(bool) {
    if(guid.empty()) {
      logger.msg(ERROR,
                 "No GUID defined for LFN - probably not preregistered");
      return false;
    }
    std::string pfn(location->str());
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    if(lfc_addreplica(guid.c_str(), NULL, location->Host().c_str(),
                      pfn.c_str(), '-', 'P', NULL, NULL) != 0) {
      lfc_endsess();
      return false;
    }
    if(CheckCheckSum()) {
      std::string ckstype;
      std::string cksumvalue = GetCheckSum();
      std::string::size_type p = cksumvalue.find(':');
      if(p == std::string::npos) {
        ckstype = "cksum";
      }
      else {
        ckstype = cksumvalue.substr(0, p);
        cksumvalue = cksumvalue.substr(p + 1);
      }
      if(CheckSize()) {
        lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(),
                      const_cast<char*>(cksumvalue.c_str()));
      }
      else {
        lfc_setfsizeg(guid.c_str(), -1, ckstype.c_str(),
                      const_cast<char*>(cksumvalue.c_str()));
      }
    }
    else if(CheckSize()) {
      lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL);
    }
    //if(CheckCreated()) {
    //}
    lfc_endsess();
    return true;
  }

  bool DataPointLFC::meta_preunregister(bool replication) {
    if(replication) return true;
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    if(lfc_unlink(url.Path().c_str()) != 0) {
      if((serrno != ENOENT) && (serrno != ENOTDIR)) {
        logger.msg(ERROR, "Failed to remove LFN in LFC - "
                   "You may need to do that by hand");
        lfc_endsess();
        return false;
      }
    }
    lfc_endsess();
    return true;
  }

  bool DataPointLFC::meta_unregister(bool all) {
    if(!all) {
      if(location == locations.end()) {
        logger.msg(ERROR, "Location is missing");
        return false;
      }
      if(url.Protocol() == "se") {
        logger.msg(DEBUG, "SE location will be unregistered automatically");
        is_metaexisting = false;
        locations.clear();
        location = locations.end();
        return true;
      }
    }
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    if(all) {
      int nbentries = 0;
      struct lfc_filereplica *entries = NULL;
      if(lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries,
                        &entries) != 0) {
        lfc_endsess();
        if((serrno == ENOENT) || (serrno == ENOTDIR)) {
          is_metaexisting = false;
          locations.clear();
          location = locations.end();
          return true;
        }
        return false;
      }
      for(int n = 0; n < nbentries; n++) {
        if(lfc_delreplica(guid.c_str(), NULL, entries[n].sfn) != 0) {
          if(serrno == ENOENT) continue;
          lfc_endsess();
          logger.msg(ERROR, "Failed to remove location from LFC");
          return false;
        }
      }
      if(lfc_unlink(url.Path().c_str()) != 0) {
        if((serrno != ENOENT) && (serrno != ENOTDIR)) {
          logger.msg(ERROR, "Failed to remove LFN in LFC");
          lfc_endsess();
          return false;
        }
      }
    }
    else {
      if(lfc_delreplica(guid.c_str(), NULL, location->str().c_str()) != 0) {
        lfc_endsess();
        logger.msg(ERROR, "Failed to remove location from LFC");
        return false;
      }
    }
    lfc_endsess();
    is_metaexisting = false;
    locations.clear();
    location = locations.end();
    return true;
  }


  bool DataPointLFC::list_files(std::list<FileInfo>& files, bool) {
    if(lfc_startsess(const_cast<char*>((url.Host() + ':' +
                                        tostring(url.Port())).c_str()),
                     "ARC") != 0)
      return false;
    struct lfc_filestatg st;
    if(lfc_statg(url.Path().c_str(), NULL, &st) != 0) {
      lfc_endsess();
      return false;
    }
    std::list<FileInfo>::iterator f = files.insert(files.end(),
                                                   FileInfo(url.Path()));
    f->SetSize(st.filesize);
    if(st.csumvalue[0])
      f->SetCheckSum(std::string(st.csumtype) + ':' + st.csumvalue);
    f->SetCreated(st.mtime);
    f->SetType((st.filemode & S_IFDIR) ? FileInfo::file_type_dir :
               FileInfo::file_type_file);
    int nbentries = 0;
    struct lfc_filereplica *entries = NULL;
    if(lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries,
                      &entries) == 0) {
      for(int n = 0; n < nbentries; n++) {
        f->AddURL(std::string(entries[n].sfn));
      }
    }
    return true;
  }

} // namespace Arc
