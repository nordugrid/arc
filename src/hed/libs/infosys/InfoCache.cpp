#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <InfoCache.h>
#include <arc/Logger.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>

namespace Arc {

InfoCache::InfoCache(Arc::Config &cfg, std::string &service_id)
{
    InfoCache(cfg, service_id.c_str());
}

InfoCache::InfoCache(Arc::Config &cfg, const char *service_id)
{
    std::string root = std::string(cfg["InformationSystem"]["CacheRoot"]);
    if (!Glib::file_test(root, Glib::FILE_TEST_IS_DIR)) {
        // create root directory
        if (mkdir(root.c_str(), 0700) != 0) {
            // rootLogger.msg(Arc::ERROR, "cannot create directory:" + root);
            std::cerr << "cannot create directory: " << root << std::endl;
            return;
        }        
    }
    std::string id(service_id);
    path_base = Glib::build_filename(root, id);
    if (!Glib::file_test(path_base, Glib::FILE_TEST_IS_DIR)) {
        // create path_base directory
        if (mkdir(path_base.c_str(), 0700) != 0) {
            std::cerr << "cannot create directory: " << path_base << std::endl;
            // rootLogger.msg(Arc::ERROR, "cannot create directory:" + path_base);
            return;
        }        
    }
}

InfoCache::~InfoCache()
{
    // NOP
}

void InfoCache::Set(const char *key, std::string &value)
{
    std::string id(key);
    std::string path = Glib::build_filename(path_base, id);
    std::ofstream out(path.c_str(), std::ios::out); 
    if (!out) return;
    out << value << std::endl;
    out.close();   
}

std::string InfoCache::Get(const char *key)
{
    std::string id(key);
    std::string path = Glib::build_filename(path_base, id);
    std::string out = Glib::file_get_contents(path);
    return out;
}

void InfoCache::query_one(const char *key, const char *q, std::list<Arc::XMLNode> &result)
{
    std::string id(key);
    std::string query(q);
    std::string path = Glib::build_filename(path_base, id);
    std::string xml = Glib::file_get_contents(path);
    Arc::XMLNode doc(xml);
    Arc::NS ns;
    result = doc.XPathLookup(query, ns);
    std::list<Arc::XMLNode>::iterator it;
    std::cout << "query_one" << std::endl;
    for (it = result.begin(); it != result.end(); it++) {
        std::cout << (*it).Name() << ":" << std::string(*it) << std::endl;
    }
}

/*
std::list<Arc::XMLNode> *InfoCache::query_any(const char *q)
{
    Glib::Dir dir(path_base);
    std::list<Arc::XMLNode> *result;
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        std::list<Arc::XMLNode> *r = query_one(d.c_str(), q);
        std::list<Arc::XMLNode>::iterator it;
        for (it = r->begin(); it != r->end(); it++) {
            std::cout << (*it).Name() << ":" << std::string(*it) << std::endl;
        }
        result->merge(*r);
        for (it = result->begin(); it != result->end(); it++) {
            std::cout << (*it).Name() << ":" << std::string(*it) << std::endl;
        }
    }
    
    return result; 
}
*/

void InfoCache::Query(const char *key, const char *q, std::list<Arc::XMLNode> &result)
{
   /* if (strcmp(key, "any") == 0) {
        return query_any(q);
    } */
    return query_one(key, q, result);
}

}
