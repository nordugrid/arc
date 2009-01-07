#ifndef __ARC_INFO_CACHE_H__
#define __ARC_INFO_CACHE_H__

#include <string>
#include <list>
#include <vector>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/ArcRegex.h>
#include <arc/infosys/InformationInterface.h>
#ifdef WIN32
#include <io.h>
#endif

namespace Arc {

/// Stores XML document in filesystem split into parts
/** */
class InfoCache {
    protected:
        std::string path_base;
    public:
        bool Query(const char *xml_path, const char *q, Arc::XMLNodeContainer &result);
        bool Query(const std::string &xml_path, std::string &q, Arc::XMLNodeContainer &result) { return Query(xml_path.c_str(), q.c_str(), result); };
        bool Set(const char *xml_path, Arc::XMLNode &value);
        bool Set(const std::string &xml_path, Arc::XMLNode &value) { return Set(xml_path.c_str(), value) ; };
        bool Get(const char *xml_path, Arc::XMLNodeContainer &result);
        bool Get(const std::string &xml_path, Arc::XMLNodeContainer &result) { return Get(xml_path.c_str(), result); };
        bool Unset(const char *xml_path);
        bool Unset(const std::string &xml_path) { return Unset(xml_path.c_str()); };
        /// Creates object according to configuration (see InfoCacheConfig.xsd)
        /** XML configuration is passed in cfg. Argument service_id is used
           to distiguish between various documents stored under same 
           path - corresponding files will be stored in subdirectory with
           service_id name. */
        InfoCache(const Arc::Config &cfg, const std::string &service_id);
        ~InfoCache();
};

class InfoCacheInterface: public InformationInterface {
 protected:
  InfoCache cache;
  virtual void Get(const std::list<std::string>& path,XMLNodeContainer& result);
  virtual void Get(XMLNode xpath,XMLNodeContainer& result);
 public:
  InfoCacheInterface(Arc::Config &cfg, std::string &service_id);
  virtual ~InfoCacheInterface(void);
  InfoCache& Cache(void) { return cache; };
};

} // namespace Arc

#endif // __ARC_INFO_CACHE_H__
