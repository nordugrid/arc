#ifndef __ARC_INFO_CACHE_H__
#define __ARC_INFO_CACHE_H__

#include <string>
#include <list>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#ifdef WIN32
#include <io.h>
#endif

namespace Arc {

class InfoCache {
    protected:
        std::string path_base;
        bool query_one(const char *key, const char *q, Arc::XMLNodeContainer &result);
        bool query_any(const char *q, Arc::XMLNodeContainer &result);
        /** Generate file and relative xml path from original xml path */
        bool get_file_and_relative_paths(std::string &xml_path, std::string &file_path, std::string &rel_path);

    public:
        bool Query(const char *xml_path, const char *q, Arc::XMLNodeContainer &result);
        bool Query(std::string &xml_path, std::string &q, Arc::XMLNodeContainer &result) { return Query(xml_path.c_str(), q.c_str(), result); };
        bool Set(const char *xml_path, Arc::XMLNode &value);
        bool Set(std::string &xml_path, Arc::XMLNode &value) { return Set(xml_path.c_str(), value) ; };
        bool Get(const char *xml_path, Arc::XMLNodeContainer &result);
        bool Get(std::string &xml_path, Arc::XMLNodeContainer &result) { return Get(xml_path.c_str(), result); };
        InfoCache(Arc::Config &cfg, std::string &service_id);
        InfoCache(Arc::Config &cfg, const char *service_id);
        ~InfoCache();
};

}

#endif // __ARC_INFO_CACHE_H__
