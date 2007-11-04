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
        void query_one(const char *key, const char *q, Arc::XMLNodeContainer &result);
        void query_any(const char *q, Arc::XMLNodeContainer &result);

    public:
        void Query(const char *key, const char *q, Arc::XMLNodeContainer &result);
        void Query(std::string &key, std::string &q, Arc::XMLNodeContainer &result) { Query(key.c_str(), q.c_str(), result); };
        void Set(const char *key, std::string &value);
        void Set(std::string &key, std::string &value) { Set(key.c_str(), value) ; };
        std::string Get(const char *key);
        std::string Get(std::string &key) { return Get(key.c_str()); };
        InfoCache(Arc::Config &cfg, std::string &service_name);
        InfoCache(Arc::Config &cfg, const char *service_name);
        ~InfoCache();
};

}

#endif // __ARC_INFO_CACHE_H__
