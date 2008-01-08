#ifndef __ARC_INFO_CACHE_H__
#define __ARC_INFO_CACHE_H__

#include <string>
#include <list>
#include <vector>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/ArcRegex.h>
#ifdef WIN32
#include <io.h>
#endif

namespace Arc {

class InfoCache {
    protected:
        std::string path_base;
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
