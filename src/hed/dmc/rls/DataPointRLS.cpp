#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <uuid/uuid.h>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "DataPointRLS.h"
#include "RLS.h"

#define globus_rls_free_result(err) \
  globus_rls_client_error_info(err, NULL, NULL, 0, GLOBUS_FALSE)

static std::string GUID() {
  uuid_t uu;
  uuid_generate(uu);
  char uustr[37];
  uuid_unparse(uu, uustr);
  return uustr;
}

namespace Arc {

  Logger DataPointRLS::logger(DataPoint::logger, "RLS");

  DataPointRLS::DataPointRLS(const URL& url) : DataPointIndex(url),
                                               guid_enabled(false) {
    std::string guidopt = url.Option("guid", "no");
    if((guidopt == "yes") || (guidopt == ""))
      guid_enabled = true;
  }

  static globus_result_t
  globus_rls_client_lrc_attr_put (globus_rls_handle_t *h, char *key,
                                  globus_rls_attribute_t *attr,
                                  int overwrite) {
    globus_result_t err;
    int errcode;
    err = globus_rls_client_lrc_attr_add(h, key, attr);
    if(err != GLOBUS_SUCCESS) {
      err = globus_rls_client_error_info(err, &errcode, NULL, 0, GLOBUS_TRUE);
      if((overwrite) && (errcode == GLOBUS_RLS_DBERROR)) {
        /* guess this can mean duplicate entry */
        globus_result_t err_ = globus_rls_client_lrc_attr_remove(h, key, attr);
        globus_rls_free_result(err_);
        if(err_ != GLOBUS_SUCCESS) return err;
        return globus_rls_client_lrc_attr_put(h, key, attr, 0);
      }
      if(errcode != GLOBUS_RLS_ATTR_NEXIST) return err;
      globus_rls_free_result(err);
      err = globus_rls_client_lrc_attr_create(h, attr->name,
                                              attr->objtype, attr->type);
      if(err != GLOBUS_SUCCESS) return err;
      err = globus_rls_client_lrc_attr_add(h, key, attr);
    }
    return err;
  }

  typedef class meta_resolve_rls_t {
   public:
    DataPointRLS& dprls;
    bool source;
    bool success;
    bool obtained_info;
    std::string guid;
    meta_resolve_rls_t(DataPointRLS& d, bool s) : dprls(d), source(s),
                                                  success(false),
                                                  obtained_info(false) {};
  };

  static bool meta_resolve_callback(globus_rls_handle_t *h,
                                    const URL& rlsurl, void *arg) {
    ((meta_resolve_rls_t*)arg)->dprls.ResolveCallback(h, rlsurl, arg);
  }

  bool DataPointRLS::ResolveCallback(globus_rls_handle_t *h,
                                     const URL& rlsurl, void *arg) {
    bool& source(((meta_resolve_rls_t*)arg)->source);
    bool& success(((meta_resolve_rls_t*)arg)->success);
    bool& obtained_info(((meta_resolve_rls_t*)arg)->obtained_info);
    std::string& guid(((meta_resolve_rls_t*)arg)->guid);

    char errmsg[MAXERRMSG + 32];
    globus_result_t err;
    int errcode;

    // Ask LRC if it contains file of interest

    if(guid_enabled && source && guid.empty()) {
      // map lfn->guid (only once)
      globus_rls_attribute_t opr;
      opr.type = globus_rls_attr_type_str;
      opr.val.s = const_cast<char*>(url.Path().c_str());
      int off = 0;
      globus_list_t *guids = NULL;
      err = globus_rls_client_lrc_attr_search(h, "lfn", globus_rls_obj_lrc_lfn,
                                              globus_rls_attr_op_eq, &opr,
                                              NULL, &off, 1, &guids);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        logger.msg(INFO, "Failed to find GUID for specified LFN in %s: %s",
                   rlsurl.str().c_str(), errmsg);
        return true;
      }
      if(!guids) {
        logger.msg(INFO, "There is no GUID for specified LFN in %s",
                   rlsurl.str().c_str());
        return true;
      }
      globus_rls_attribute_object_t *obattr =
        (globus_rls_attribute_object_t*)globus_list_first(guids);
      guid = obattr->key;
      globus_rls_client_free_list(guids);
    }
    globus_list_t *pfns_list = NULL;
    if(source) {
      if(!guid.empty()) {
        err = globus_rls_client_lrc_get_pfn
          (h, const_cast<char*>(guid.c_str()), 0, 0, &pfns_list);
      }
      else {
        err = globus_rls_client_lrc_get_pfn
          (h, const_cast<char*>(url.Path().c_str()), 0, 0, &pfns_list);
      }
    }
    else {
      err = globus_rls_client_lrc_get_pfn
        (h, "__storage_service__", 0, 0, &pfns_list);
    }
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      if(errcode == GLOBUS_RLS_INVSERVER) {
        return true;
      }
      else if(errcode == GLOBUS_RLS_LFN_NEXIST) {
        return true;
      }
      else { // do not know
        logger.msg(INFO, "Warning: can't get PFNs from server %s: %s",
                   rlsurl.str().c_str(), errmsg);
        return true;
      }
    }
    if(!success) {
      success = true; // got something
      if(source)
        is_metaexisting = true;
    }
    if(url.Locations().size() == 0) {
      for(globus_list_t *lp = pfns_list; lp; lp = globus_list_rest(lp)) {
        globus_rls_string2_t *str2 =
          (globus_rls_string2_t*)globus_list_first(lp);
        URL pfn(str2->s2);
        locations.push_back(URLLocation(pfn, rlsurl.str()));
        logger.msg(DEBUG, "Adding location: %s - %s",
                   rlsurl.str().c_str(), pfn.str().c_str());
      }
    }
    else {
      for(std::list<URLLocation>::const_iterator loc = url.Locations().begin();
          loc != url.Locations().end(); loc++) {
        for(globus_list_t *lp = pfns_list; lp; lp = globus_list_rest(lp)) {
          globus_rls_string2_t *str2 =
            (globus_rls_string2_t*)globus_list_first(lp);
          URL pfn(str2->s2);
          // for RLS URLs are used instead of metanames
          if(pfn == *loc) {
            logger.msg(DEBUG, "Adding location: %s - %s",
                       rlsurl.str().c_str(), pfn.str().c_str());
            if(source) {
              locations.push_back(URLLocation(pfn, rlsurl.str()));
            }
            else {
              locations.push_back(URLLocation(*loc, rlsurl.str()));
            }
            break;
          }
        }
      }
    }
    globus_rls_client_free_list(pfns_list);
    if(!obtained_info) {
      /* obtain metadata - assume it is same everywhere */
      globus_list_t *attr_list;
      if(!guid.empty()) {
        err = globus_rls_client_lrc_attr_value_get
          (h, const_cast<char*>(guid.c_str()),
           NULL, globus_rls_obj_lrc_lfn, &attr_list);
      }
      else {
        err = globus_rls_client_lrc_attr_value_get
          (h, const_cast<char*>(url.Path().c_str()),
           NULL, globus_rls_obj_lrc_lfn, &attr_list);
      }
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if(errcode == GLOBUS_RLS_ATTR_NEXIST) {
          return true;
        }
        logger.msg(INFO, "Warning: Failed to obtain attributes from %s: %s",
                   rlsurl.str().c_str(), errmsg);
        return true;
      }
      is_metaexisting = true; // even for destination
      for(globus_list_t *lpa = attr_list; lpa; lpa = globus_list_rest(lpa)) {
        globus_rls_attribute_t *attr =
          (globus_rls_attribute_t*)globus_list_first(lpa);
        if(attr->type != globus_rls_attr_type_str) continue;
        logger.msg(DEBUG, "Attribute: %s - %s", attr->name, attr->val.s);
        if(strcmp(attr->name, "filechecksum") == 0) {
          if(!CheckCheckSum())
            SetCheckSum(attr->val.s);
        }
        else if(strcmp(attr->name, "size") == 0) {
          if(!CheckSize())
            SetSize(stringtoull(attr->val.s));
        }
        else if(strcmp(attr->name, "modifytime") == 0) {
          if(!CheckCreated()) {
            Time created(attr->val.s);
            if(created == -1)
              created.SetTime(stringtoull(attr->val.s));
            SetCreated(created);
          }
        }
        else if(strcmp(attr->name, "created") == 0) {
          if(!CheckCreated()) {
            Time created(attr->val.s);
            if(created == -1)
              created.SetTime(stringtoull(attr->val.s));
            SetCreated(created);
          }
        }
      }
      globus_rls_client_free_list(attr_list);
      obtained_info = true;
    }
    return true;
  }

  /* perform resolve operation, which can take long time */
  bool DataPointRLS::meta_resolve(bool source) {
    is_resolved = false;
    is_metaexisting = false;
    if(source) {
      if(url.Path().empty()) {
        logger.msg(INFO, "Source must contain LFN");
        return false;
      }
      std::list<URL> rlis;
      std::list<URL> lrcs;
      rlis.push_back(url.ConnectionURL());
      lrcs.push_back(url.ConnectionURL());
      meta_resolve_rls_t arg(*this, source);
      rls_find_lrcs(rlis, lrcs, true, false,
                    &meta_resolve_callback, (void*)&arg);
      if(!arg.success) return false;
    }
    else {
      if(url.Path().empty()) {
        logger.msg(INFO, "Destination must contain LFN");
        return false;
      }
      std::list<URL> rlis;
      std::list<URL> lrcs;
      rlis.push_back(url.ConnectionURL());
      lrcs.push_back(url.ConnectionURL());
      if(url.Locations().size() == 0) {
        logger.msg(INFO, "Locations are missing in destination RLS url - "
                   "will use those registered with special name");
      }
      meta_resolve_rls_t arg(*this, source);
      rls_find_lrcs(rlis, lrcs, true, false,
                    &meta_resolve_callback, (void*)&arg);
      if(!arg.success) return false;
      if(locations.size() == 0) {
        logger.msg(INFO, "No locations found for destination");
        return false;
      }
      // Make pfns
      std::list<URL>::iterator lrc_p = lrcs.begin();
      for(std::list<URLLocation>::iterator loc = locations.begin();
          loc != locations.end();) {
        bool se_uses_lfn = false;
        std::string u = loc->str();
        if(loc->Protocol() == "se") {
          u += "?";
          se_uses_lfn = true;
        }
        else {
          u += "/";
        }
        if(guid_enabled) {
          std::string guid = GUID();
          if((!se_uses_lfn) && (!pfn_path.empty()))
            u += pfn_path;
          else
            u += guid;
        }
        else {
          if((!se_uses_lfn) && (!pfn_path.empty()))
            u += pfn_path;
          else
            u += url.Path();
        }
        *loc = URLLocation(u, loc->Name());
        if(!loc->Name().empty()) {
          logger.msg(DEBUG, "Using location: %s - %s",
                     loc->Name().c_str(), loc->str().c_str());
          ++loc;
        }
        else { // Use arbitrary lrc
          if(lrc_p == lrcs.end()) { // no LRC
            logger.msg(DEBUG, "Removing location: %s - %s",
                       loc->Name().c_str(), loc->str().c_str());
            loc = locations.erase(loc);
          }
          else {
            *loc = URLLocation(*loc, lrc_p->str());
            ++lrc_p;
            if(lrc_p == lrcs.end())
              lrc_p = lrcs.begin();
            logger.msg(DEBUG, "Using location: %s - %s",
                       loc->Name().c_str(), loc->str().c_str());
            ++loc;
          }
        }
      }
    }
    logger.msg(DEBUG, "meta_get_data: checksum: %s", GetCheckSum().c_str());
    logger.msg(DEBUG, "meta_get_data: size: %ull", GetSize());
    logger.msg(DEBUG, "meta_get_data: created: %s",
               GetCreated().str().c_str());
    if(!url.CommonLocOptions().empty()) {
      for(std::list<URLLocation>::iterator loc = locations.begin();
          loc != locations.end(); loc++) {
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

  bool DataPointRLS::meta_preregister(bool replication, bool force) {
    if(replication) {/* replicating inside same lfn */
      if(!is_metaexisting) {/* for replication it must be there */
        logger.msg(ERROR, "LFN is missing in RLS (needed for replication)");
        return false;
      }
      return true;
    }
    if(is_metaexisting) {/* algorithm require this to be new file */
      if(!force) {
        logger.msg(ERROR, "LFN already exists in replica");
        return false;
      }
    }
    /* RLS does not support LFN only in database - hence doing nothing here */
    return true;
  }

  bool DataPointRLS::meta_postregister(bool replication) {
    globus_rls_handle_t *h;
    char errmsg[MAXERRMSG + 32];
    globus_result_t err;
    int errcode;

    err = globus_rls_client_connect
      (const_cast<char*>(url.ConnectionURL().c_str()), &h);
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, NULL, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      logger.msg(INFO, "Failed to connect to RLS server: %s", errmsg);
      return false;
    }
    // assume that is RLI and try to resolve for special/any name

    std::string pfn;
    std::string guid;
    pfn = location->str();
    // it is always better to register pure url
    std::string rls_lfn = url.Path();
    if(!replication) {
      if(guid_enabled) {
        for(;;) {
          // generate guid
          guid = GUID();
          // store in LRC
          if((err = globus_rls_client_lrc_create
              (h, const_cast<char*>(guid.c_str()),
               const_cast<char*>(pfn.c_str()))) != GLOBUS_SUCCESS) {
            err = globus_rls_client_error_info(err, &errcode, NULL, 0,
                                               GLOBUS_TRUE);
            if(errcode == GLOBUS_RLS_LFN_EXIST) {
              globus_rls_free_result(err);
              continue;
            }
          }
          rls_lfn = guid;
          break;
        }
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                       GLOBUS_FALSE);
          logger.msg(INFO, "Failed to create GUID in RLS: %s", errmsg);
          globus_rls_client_close(h);
          return false;
        }
        // Check if there is no same LFN
        globus_rls_attribute_t opr;
        opr.type = globus_rls_attr_type_str;
        opr.val.s = const_cast<char*>(url.Path().c_str());
        int off = 0;
        globus_list_t *guids = NULL;
        err = globus_rls_client_lrc_attr_search(h, "lfn",
                                                globus_rls_obj_lrc_lfn,
                                                globus_rls_attr_op_eq,
                                                &opr, NULL, &off, 1, &guids);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                       GLOBUS_FALSE);
          if((errcode != GLOBUS_RLS_LFN_NEXIST) &&
             (errcode != GLOBUS_RLS_ATTR_NEXIST) &&
             (errcode != GLOBUS_RLS_ATTR_VALUE_NEXIST)) {
            logger.msg(INFO, "Failed to check for existing LFN in %s: %s",
                       url.str().c_str(), errmsg);
            globus_rls_client_close(h);
            return false;
          }
        }
        if(guids) {
          globus_rls_client_free_list(guids);
          logger.msg(INFO, "There is same LFN in %s", url.str().c_str());
          globus_rls_client_close(h);
          return false;
        }
        // add LFN
        globus_rls_attribute_t attr;
        attr.objtype = globus_rls_obj_lrc_lfn;
        attr.type = globus_rls_attr_type_str;
        attr.name = "lfn";
        attr.val.s = const_cast<char*>(url.Path().c_str());
        err = globus_rls_client_lrc_attr_put
          (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                       GLOBUS_FALSE);
          logger.msg(INFO, "Failed to add LFN-GUID to RLS: %s", errmsg);
          globus_rls_client_close(h);
          return false;
        }
      }
      else {
        if((err = globus_rls_client_lrc_create
            (h, const_cast<char*>(url.Path().c_str()),
             const_cast<char*>(pfn.c_str()))) != GLOBUS_SUCCESS) {
          err = globus_rls_client_error_info(err, &errcode, NULL, 0,
                                             GLOBUS_TRUE);
          if(errcode == GLOBUS_RLS_LFN_EXIST) {
            globus_rls_free_result(err);
            err = globus_rls_client_lrc_add
              (h, const_cast<char*>(url.Path().c_str()),
               const_cast<char*>(pfn.c_str()));
          }
        }
      }
    }
    else {
      if(guid_enabled) {
        // get guid
        globus_rls_attribute_t opr;
        opr.type = globus_rls_attr_type_str;
        opr.val.s = const_cast<char*>(url.Path().c_str());
        int off = 0;
        globus_list_t *guids = NULL;
        err = globus_rls_client_lrc_attr_search(h, "lfn",
                                                globus_rls_obj_lrc_lfn,
                                                globus_rls_attr_op_eq,
                                                &opr, NULL, &off, 1, &guids);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                       GLOBUS_FALSE);
          logger.msg(INFO, "Failed to find GUID for specified LFN in %s: %s",
                     url.str().c_str(), errmsg);
          globus_rls_client_close(h);
          return false;
        }
        if(!guids) {
          logger.msg(INFO, "There is no GUID for specified LFN in %s",
                     url.str().c_str());
          globus_rls_client_close(h);
          return false;
        }
        globus_rls_attribute_object_t *obattr =
          (globus_rls_attribute_object_t*)globus_list_first(guids);
        guid = obattr->key;
        globus_rls_client_free_list(guids);
        rls_lfn = guid;
      }
      err = globus_rls_client_lrc_add
        (h, const_cast<char*>(rls_lfn.c_str()),
         const_cast<char*>(pfn.c_str()));
    }
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      if(errcode != GLOBUS_RLS_MAPPING_EXIST) {
        logger.msg(INFO, "Failed to create/add LFN-PFN mapping: %s", errmsg);
        globus_rls_client_close(h);
        return false;
      }
    }
    globus_rls_attribute_t attr;
    std::string attr_val;
    attr.objtype = globus_rls_obj_lrc_lfn;
    attr.type = globus_rls_attr_type_str;
    attr.name = "filetype";
    attr.val.s = "file";
    err = globus_rls_client_lrc_attr_put
      (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      if(errcode != GLOBUS_RLS_ATTR_EXIST) {
        logger.msg(INFO, "Warning: failed to add attribute to RLS: %s",
                   errmsg);
      }
    }
    if(CheckSize()) {
      attr.name = "size";
      attr_val = tostring(GetSize());
      attr.val.s = const_cast<char*>(attr_val.c_str());
      err = globus_rls_client_lrc_attr_put
        (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if(errcode != GLOBUS_RLS_ATTR_EXIST) {
          logger.msg(INFO, "Warning: failed to add attribute to RLS: %s",
                     errmsg);
        }
      }
    }
    if(CheckCheckSum()) {
      attr.name = "filechecksum";
      attr_val = GetCheckSum();
      attr.val.s = const_cast<char*>(attr_val.c_str());
      err = globus_rls_client_lrc_attr_put
        (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if(errcode != GLOBUS_RLS_ATTR_EXIST) {
          logger.msg(INFO, "Warning: failed to add attribute to RLS: %s",
                     errmsg);
        }
      }
    }
    if(CheckCreated()) {
      attr.name = "modifytime";
      attr_val = GetCreated();
      attr.val.s = const_cast<char*>(attr_val.c_str());
      err = globus_rls_client_lrc_attr_put
        (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if(errcode != GLOBUS_RLS_ATTR_EXIST) {
          logger.msg(INFO, "Warning: failed to add attribute to RLS: %s",
                     errmsg);
        }
      }
    }
    if(url.Options().size() > 0) {
      for(std::map<std::string, std::string>::const_iterator pos =
            url.Options().begin(); pos != url.Options().end(); pos++) {
        attr.name = const_cast<char*>(pos->first.c_str());
        attr.val.s = const_cast<char*>(pos->second.c_str());
        err = globus_rls_client_lrc_attr_put
          (h, const_cast<char*>(rls_lfn.c_str()), &attr, 0);
        if(err != GLOBUS_SUCCESS) {
          globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                       GLOBUS_FALSE);
          if(errcode != GLOBUS_RLS_ATTR_EXIST) {
            logger.msg(INFO, "Warning: failed to add attribute to RLS: %s",
                       errmsg);
          }
        }
      }
    }
    globus_rls_client_close(h);
    return true;
  }

  bool DataPointRLS::meta_preunregister(bool replication) {
    if(replication) return true;
    return true;
  }

  typedef class meta_unregister_rls_t {
  public:
    DataPointRLS& dprls;
    bool all;
    bool failure;
    std::string guid;
    meta_unregister_rls_t(DataPointRLS& d, bool a) : dprls(d), all(a),
                                                     failure(false) {};
  };

  static bool meta_unregister_callback(globus_rls_handle_t *h,
                                       const URL& rlsurl, void *arg) {
    ((meta_unregister_rls_t*)arg)->dprls.UnregisterCallback(h, rlsurl, arg);
  }

  bool DataPointRLS::UnregisterCallback(globus_rls_handle_t *h,
                                        const URL& rlsurl, void *arg) {
    bool& all(((meta_unregister_rls_t*)arg)->all);
    bool& failure(((meta_unregister_rls_t*)arg)->failure);
    std::string& guid(((meta_unregister_rls_t*)arg)->guid);

    int lrc_offset = 0;
    int lrc_limit = 0;
    globus_result_t err;
    int errcode;
    char errmsg[MAXERRMSG + 32];
    globus_list_t *pfns_list;
    if(guid_enabled && guid.empty()) {
      // map lfn->guid (only once)
      globus_rls_attribute_t opr;
      opr.type = globus_rls_attr_type_str;
      opr.val.s = const_cast<char*>(url.Path().c_str());
      int off = 0;
      globus_list_t *guids = NULL;
      err = globus_rls_client_lrc_attr_search(h, "lfn",
                                              globus_rls_obj_lrc_lfn,
                                              globus_rls_attr_op_eq,
                                              &opr, NULL, &off, 1, &guids);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        logger.msg(VERBOSE, "Failed to find GUID for specified LFN in %s: %s",
                   rlsurl.str().c_str(), errmsg);
        return true;
      }
      if(!guids) {
        logger.msg(VERBOSE, "There is no GUID for specified LFN in %s",
                   rlsurl.str().c_str());
        return true;
      }
      globus_rls_attribute_object_t *obattr =
        (globus_rls_attribute_object_t*)globus_list_first(guids);
      guid = obattr->key;
      globus_rls_client_free_list(guids);
    }
    if(all) {
      if(!guid.empty()) {
        err = globus_rls_client_lrc_get_pfn
          (h, const_cast<char*>(guid.c_str()),
           &lrc_offset, lrc_limit, &pfns_list);
      }
      else {
        err = globus_rls_client_lrc_get_pfn
          (h, const_cast<char*>(url.Path().c_str()),
           &lrc_offset, lrc_limit, &pfns_list);
      }
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        logger.msg(INFO, "Warning: Failed to retrieve LFN/PFNs from %s: %s",
                   rlsurl.str().c_str(), errmsg);
        failure = true;
        return true;
      }
      for(globus_list_t *lp = pfns_list; lp; lp = globus_list_rest(lp)) {
        globus_rls_string2_t *str2 =
          (globus_rls_string2_t*)globus_list_first(lp);
        URL pfn(str2->s2);
        if(pfn.Protocol() == "se") {
          logger.msg(DEBUG, "SE location will be unregistered automatically");
        }
        else {
          err = globus_rls_client_lrc_delete(h, str2->s1, str2->s2);
          if(err != GLOBUS_SUCCESS) {
            globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                         GLOBUS_FALSE);
            if((errcode != GLOBUS_RLS_MAPPING_NEXIST) &&
               (errcode != GLOBUS_RLS_LFN_NEXIST) &&
               (errcode != GLOBUS_RLS_PFN_NEXIST)) {
              logger.msg(INFO, "Warning: Failed to delete LFN/PFN from %s: %s",
                         rlsurl.str().c_str(), errmsg);
              failure = true;
              continue;
            }
          }
        }
      }
      globus_rls_client_free_list(pfns_list);
    }
    else { // ! all
      err = globus_rls_client_lrc_delete
        (h, const_cast<char*>(url.Path().c_str()),
         const_cast<char*>(location->str().c_str()));
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if((errcode != GLOBUS_RLS_MAPPING_NEXIST) &&
           (errcode != GLOBUS_RLS_LFN_NEXIST) &&
           (errcode != GLOBUS_RLS_PFN_NEXIST)) {
          logger.msg(INFO, "Warning: Failed to delete LFN/PFN from %s: %s",
                     rlsurl.str().c_str(), errmsg);
          failure = true;
        }
      }
    }
    return true;
  }

  bool DataPointRLS::meta_unregister(bool all) {
    if(!all) {
      if(location == locations.end()) {
        logger.msg(ERROR, "Location is missing");
        return false;
      }
      if(location->Protocol() == "se") {
        logger.msg(DEBUG, "SE location will be unregistered automatically");
        return true;
      }
    }
    if(!guid_enabled) {
      globus_rls_handle_t *h;
      char errmsg[MAXERRMSG + 32];
      globus_result_t err;
      int errcode;
      globus_list_t *pfns_list;

      err = globus_rls_client_connect
        (const_cast<char*>(url.ConnectionURL().c_str()), &h);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, NULL, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        logger.msg(INFO, "Failed to connect to RLS server: %s", errmsg);
        return false;
      }
      // first find all LRC servers storing required information
      globus_list_t *lrcs = NULL;
      globus_rls_string2_t lrc_direct;
      globus_bool_t free_lrcs = GLOBUS_FALSE;
      lrc_direct.s1 = const_cast<char*>(url.Path().c_str());
      lrc_direct.s2 = NULL; // for current connection
      int lrc_offset = 0;
      int lrc_limit = 0;
      err = globus_rls_client_rli_get_lrc
        (h, const_cast<char*>(url.Path().c_str()),
         &lrc_offset, lrc_limit, &lrcs);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        if(errcode == GLOBUS_RLS_LFN_NEXIST) {
          logger.msg(INFO, "LFN must be already deleted, try LRC anyway");
          lrcs = NULL;
        }
        else if(errcode != GLOBUS_RLS_INVSERVER) {
          logger.msg(INFO, "Failed to retrieve LFN/LRC: %s", errmsg);
          globus_rls_client_close(h);
          return false;
        }
        // Probably that is LRC server only.
        globus_list_insert(&lrcs, &lrc_direct);
      }
      else {
        free_lrcs = GLOBUS_TRUE;
      }
      err = GLOBUS_SUCCESS;
      // TODO: sort by lrc and cache connections
      bool failure = false;
      for(globus_list_t *p = lrcs; p; p = globus_list_rest(p)) {
        globus_rls_string2_t *str2 =
          (globus_rls_string2_t*)globus_list_first(p);
        char *lrc = str2->s2;
        globus_rls_handle_t *h_;
        if(lrc) {
          err = globus_rls_client_connect(lrc, &h_);
          if(err != GLOBUS_SUCCESS) {
            globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                         GLOBUS_FALSE);
            logger.msg(INFO, "Warning: Failed to connect to LRC at %s: %s",
                       lrc, errmsg);
            failure = true;
            continue;
          }
        }
        else {
          h_ = h; // This server is already connected
        }
        if(all) {
          err = globus_rls_client_lrc_get_pfn
            (h_, const_cast<char*>(url.Path().c_str()),
             &lrc_offset, lrc_limit, &pfns_list);
          if(err != GLOBUS_SUCCESS) {
            globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                         GLOBUS_FALSE);
            if((errcode != GLOBUS_RLS_MAPPING_NEXIST) &&
               (errcode != GLOBUS_RLS_LFN_NEXIST) &&
               (errcode != GLOBUS_RLS_PFN_NEXIST)) {
              logger.msg(INFO,
                         "Warning: Failed to retrieve LFN/PFNs from %s: %s",
                         lrc ? lrc : url.ConnectionURL().c_str(), errmsg);
              if(lrc) globus_rls_client_close(h_);
              failure = true;
              continue;
            }
            // Probably no such LFN - good, less work to do
            pfns_list = NULL;
          }
          for(globus_list_t *lp = pfns_list; lp; lp = globus_list_rest(lp)) {
            globus_rls_string2_t *str2 =
              (globus_rls_string2_t*)globus_list_first(lp);
            URL pfn(str2->s1);
            if(pfn.Protocol() == "se") {
              logger.msg(DEBUG,
                         "SE location will be unregistered automatically");
            }
            else {
              err = globus_rls_client_lrc_delete
                (h_, const_cast<char*>(url.Path().c_str()), str2->s1);
              if(err != GLOBUS_SUCCESS) {
                globus_rls_client_error_info(err, &errcode, errmsg,
                                             MAXERRMSG + 32, GLOBUS_FALSE);
                if((errcode != GLOBUS_RLS_MAPPING_NEXIST) &&
                   (errcode != GLOBUS_RLS_LFN_NEXIST) &&
                   (errcode != GLOBUS_RLS_PFN_NEXIST)) {
                  logger.msg(INFO,
                             "Warning: Failed to delete LFN/PFN from %s: %s",
                             lrc ? lrc : url.ConnectionURL().c_str(), errmsg);
                  if(lrc) globus_rls_client_close(h_);
                  failure = true;
                  continue;
                }
              }
            }
          }
          if(pfns_list) globus_rls_client_free_list(pfns_list);
        }
        else { // ! all
          err = globus_rls_client_lrc_delete
            (h_, const_cast<char*>(url.Path().c_str()),
             const_cast<char*>(location->str().c_str()));
          if(err != GLOBUS_SUCCESS) {
            globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                         GLOBUS_FALSE);
            if((errcode != GLOBUS_RLS_MAPPING_NEXIST) &&
               (errcode != GLOBUS_RLS_LFN_NEXIST) &&
               (errcode != GLOBUS_RLS_PFN_NEXIST)) {
              logger.msg(INFO, "Warning: Failed to delete LFN/PFN from %s: %s",
                         lrc, errmsg);
              if(lrc) globus_rls_client_close(h_);
              failure = true;
              continue;
            }
          }
        }
        if(lrc) globus_rls_client_close(h_);
      }
      globus_rls_client_close(h);
      if(free_lrcs) {
        globus_rls_client_free_list(lrcs);
      }
      else {
        globus_list_free(lrcs);
      }
      if(!failure) {
        is_metaexisting = false;
        locations.clear();
        location = locations.end();
      }
      return !failure;
    }
    else { // guid_enabled
      std::list<URL> rlis;
      std::list<URL> lrcs;
      rlis.push_back(url.ConnectionURL());
      lrcs.push_back(url.ConnectionURL());
      meta_unregister_rls_t arg(*this, all);
      rls_find_lrcs(rlis, lrcs, true, false,
                    &meta_unregister_callback, (void*)&arg);
      if(!arg.failure) {
        is_metaexisting = false;
        locations.clear();
        location = locations.end();
      }
      return !arg.failure;
    }
  }

  static bool get_attributes(globus_rls_handle_t *h, const std::string& lfn,
                             FileInfo& f) {
    globus_list_t *attr_list;
    char errmsg[MAXERRMSG + 32];
    globus_result_t err;
    int errcode;
    err = globus_rls_client_lrc_attr_value_get
      (h, const_cast<char*>(lfn.c_str()),
       NULL, globus_rls_obj_lrc_lfn, &attr_list);
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      if(errcode != GLOBUS_RLS_ATTR_NEXIST) {
        // logger.msg(INFO, "Warning: Failed to retrieve attributes: %s",
        // errmsg);
        return false;
      }
      return true;
    }
    for(globus_list_t *pa = attr_list; pa; pa = globus_list_rest(pa)) {
      globus_rls_attribute_t *attr =
        (globus_rls_attribute_t*)globus_list_first(pa);
      if(attr->type != globus_rls_attr_type_str) continue;
      // logger.msg(DEBUG, "Attribute: %s - %s", attr->name, attr->val.s);
      if(strcmp(attr->name, "filechecksum") == 0) {
        f.SetCheckSum(attr->val.s);
      }
      else if(strcmp(attr->name, "size") == 0) {
        f.SetSize(stringtoull(attr->val.s));
      }
      else if(strcmp(attr->name, "modifytime") == 0) {
        Time created(attr->val.s);
        if(created == -1)
          created.SetTime(stringtoull(attr->val.s));
        f.SetCreated(created);
      }
      else if(strcmp(attr->name, "created") == 0) {
        Time created(attr->val.s);
        if(created == -1)
          created.SetTime(stringtoull(attr->val.s));
        f.SetCreated(created);
      }
    }
    globus_rls_client_free_list(attr_list);
    return true;
  }

  typedef class list_files_rls_t {
   public:
    DataPointRLS& dprls;
    std::list<FileInfo>& files;
    bool success;
    bool resolve;
    std::string guid;
    list_files_rls_t(DataPointRLS& d, std::list<FileInfo>& f,
                     bool r) : dprls(d), files(f),
                               success(false), resolve(r) {};
  };

  static bool list_files_callback(globus_rls_handle_t *h,
                                  const URL& rlsurl, void *arg) {
    ((list_files_rls_t*)arg)->dprls.ListFilesCallback(h, rlsurl, arg);
  }

  bool DataPointRLS::ListFilesCallback(globus_rls_handle_t *h,
                                       const URL& rlsurl, void *arg) {
    std::list<FileInfo>& files(((list_files_rls_t*)arg)->files);
    bool& success(((list_files_rls_t*)arg)->success);
    bool& resolve(((list_files_rls_t*)arg)->resolve);
    std::string& guid(((list_files_rls_t*)arg)->guid);

    int lrc_offset = 0;
    globus_result_t err;
    int errcode;
    char errmsg[MAXERRMSG + 32];
    globus_list_t *pfns = NULL;
    if(guid_enabled && !url.Path().empty() && guid.empty()) {
      // looking gor guid only once
      // looking for guid only if lfn specified
      globus_rls_attribute_t opr;
      opr.type = globus_rls_attr_type_str;
      opr.val.s = const_cast<char*>(url.Path().c_str());
      int off = 0;
      globus_list_t *guids = NULL;
      err = globus_rls_client_lrc_attr_search(h, "lfn", globus_rls_obj_lrc_lfn,
                                              globus_rls_attr_op_eq,
                                              &opr, NULL, &off, 1, &guids);
      if(err != GLOBUS_SUCCESS) {
        globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                     GLOBUS_FALSE);
        logger.msg(INFO, "Failed to find GUID for specified LFN in %s: %s",
                   rlsurl.str().c_str(), errmsg);
        return true;
      }
      if(!guids) {
        logger.msg(INFO, "There is no GUID for specified LFN in %s",
                   rlsurl.str().c_str());
        return true;
      }
      globus_rls_attribute_object_t *obattr =
        (globus_rls_attribute_object_t*)globus_list_first(guids);
      guid = obattr->key;
      globus_rls_client_free_list(guids);
    }
    if(!guid.empty()) {
      err = globus_rls_client_lrc_get_pfn
        (h, const_cast<char*>(guid.c_str()), &lrc_offset, 1000, &pfns);
    }
    else if(!url.Path().empty()) {
      err = globus_rls_client_lrc_get_pfn
        (h, const_cast<char*>(url.Path().c_str()), &lrc_offset, 1000, &pfns);
    }
    else {
      err = globus_rls_client_lrc_get_pfn_wc(h, "*", rls_pattern_unix,
                                             &lrc_offset, 1000, &pfns);
    }
    if(err != GLOBUS_SUCCESS) {
      globus_rls_client_error_info(err, &errcode, errmsg, MAXERRMSG + 32,
                                   GLOBUS_FALSE);
      if(errcode == GLOBUS_RLS_LFN_NEXIST) {
        logger.msg(DEBUG, "No LFNs found in %s", rlsurl.str().c_str());
        success = true;
        return true;
      }
      logger.msg(INFO, "Failed to retrieve list of LFNs/PFNs from %s",
                 rlsurl.str().c_str());
      return true;
    }
    success = true;
    std::string last_lfn = "";
    std::string last_guid = "";
    for(globus_list_t *p = pfns; p; p = globus_list_rest(p)) {
      globus_rls_string2_t *str2 =
        (globus_rls_string2_t*)globus_list_first(p);
      std::string lfn(str2->s1);
      URL pfn(str2->s2);
      if(guid_enabled) {
        if(lfn != last_guid) {
          last_guid = lfn;
          last_lfn = "";
          // get real lfn
          globus_list_t *lfn_list = NULL;
          err = globus_rls_client_lrc_attr_value_get
            (h, const_cast<char*>(lfn.c_str()),
             "lfn", globus_rls_obj_lrc_lfn, &lfn_list);
          if(err != GLOBUS_SUCCESS) {
            globus_rls_client_error_info(err, &errcode, errmsg,
                                         MAXERRMSG + 32, GLOBUS_FALSE);
            continue;
          }
          if(lfn_list == NULL) continue;
          globus_rls_attribute_t *attr =
            (globus_rls_attribute_t*)globus_list_first(lfn_list);
          if(attr->type != globus_rls_attr_type_str) {
            globus_rls_client_free_list(lfn_list);
            continue;
          }
          // use only first lfn (TODO: all lfns)
          last_lfn = attr->val.s;
          globus_rls_client_free_list(lfn_list);
        }
        if(!last_lfn.empty()) {
          logger.msg(DEBUG, "lfn: %s(%s) - %s",
                     last_lfn.c_str(), last_guid.c_str(), pfn.str().c_str());
          std::list<FileInfo>::iterator f;
          for(f = files.begin(); f != files.end(); ++f)
            if(f->GetName() == last_lfn)
              break;
          if(f == files.end()) {
            f = files.insert(files.end(), FileInfo(last_lfn.c_str()));
            if(resolve)
              get_attributes(h, last_guid, *f);
          }
          f->AddURL(pfn);
        }
      }
      else { // !guid_enabled
        logger.msg(DEBUG, "lfn: %s - pfn: %s", lfn.c_str(), pfn.str().c_str());
        std::list<FileInfo>::iterator f;
        for(f = files.begin(); f != files.end(); ++f)
          if(f->GetName() == lfn)
            break;
        if(f == files.end()) {
          f = files.insert(files.end(), FileInfo(lfn));
          if(resolve)
            get_attributes(h, lfn, *f);
        }
        f->AddURL(pfn);
      }
    }
    globus_rls_client_free_list(pfns);
    return true;
  }

  bool DataPointRLS::list_files(std::list<FileInfo>& files, bool resolve) {
    std::list<URL> rlis;
    std::list<URL> lrcs;
    rlis.push_back(url.ConnectionURL());
    lrcs.push_back(url.ConnectionURL());

    list_files_rls_t arg(*this, files, resolve);
    rls_find_lrcs(rlis, lrcs, true, false,
                  &list_files_callback, (void*)&arg);
    return arg.success;
  }

} // namespace Arc
