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
#ifndef WIN32
        if (mkdir(root.c_str(), 0700) != 0) {
#else
	if (mkdir(root.c_str()) != 0) {
#endif
            // rootLogger.msg(Arc::ERROR, "cannot create directory:" + root);
            std::cerr << "cannot create directory: " << root << std::endl;
            return;
        }        
    }
    std::string id(service_id);
    path_base = Glib::build_filename(root, id);
    if (!Glib::file_test(path_base, Glib::FILE_TEST_IS_DIR)) {
        // create path_base directory
#ifndef WIN32
        if (mkdir(path_base.c_str(), 0700) != 0) {
#else
	if (mkdir(path_base.c_str()) != 0) {
#endif
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

void InfoCache::query_one(const char *key, const char *q, Arc::XMLNodeContainer &result)
{
    std::string id(key);
    std::string query(q);
    std::string path = Glib::build_filename(path_base, id);
    std::string xml = Glib::file_get_contents(path);
    Arc::XMLNode doc(xml);
    Arc::NS ns;
    std::list<Arc::XMLNode> xresult = doc.XPathLookup(query, ns);
    result.AddNew(xresult);
}

void InfoCache::query_any(const char *q, Arc::XMLNodeContainer &result)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        query_one(d.c_str(), q, result);
    }
}

void InfoCache::Query(const char *key, const char *q, Arc::XMLNodeContainer &result)
{
    if (strcmp(key, "any") == 0) {
        return query_any(q, result);
    }
    return query_one(key, q, result);
}

}
