#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <InfoCache.h>
#include <arc/Logger.h>
#include <arc/ArcRegex.h>
#include <arc/StringConv.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>

#ifdef WIN32
#include <arc/win32.h>
#endif

namespace Arc {

static Logger logger(Logger::getRootLogger(), "InfoCache");

static RegularExpression id_regex("@id=\"([a-zA-Z0-9_\\\\-]*)\"");

static void merge_xml(std::string& path_base, Arc::XMLNode &node)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        std::string path_fl1 = Glib::build_filename(path_base, d);
        //std::cout << "merge_xml f1: " << path_fl1 << std::endl;
        if (Glib::file_test(path_fl1, Glib::FILE_TEST_IS_REGULAR)) {
            std::string xml_str = Glib::file_get_contents(path_fl1);
            Arc::XMLNode n(xml_str);
            Arc::XMLNode c;
            for (int i = 0; (c = n.Child(i)) != false; i++) {
                    node.NewChild(c);
            }
        }
    }
}

static bool create_directory(const std::string dir) {
    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
        // create directory
        if (mkdir(dir.c_str(), 0700) != 0) {
            logger.msg(ERROR,"cannot create directory: %s",dir);
            return false;
        }
    }
    return true;
}

// --------------------------------------------------------------------------------

InfoCache::InfoCache(Arc::Config &cfg, const std::string &service_id)
{
    std::string cfg_s;
    cfg.GetXML(cfg_s);
    logger.msg(VERBOSE,"Cache configuration: %s",cfg_s);
    std::string root = std::string(cfg["CacheRoot"]);
    if(root.empty()) {
        logger.msg(ERROR,"Missing cache root in configuration");
        return;
    }
    if(service_id.empty()) {
        logger.msg(ERROR,"Missing service id");
        return;
    }
    logger.msg(VERBOSE,"Cache root: %s",root);
    if (!create_directory(root)) return;
    std::string id(service_id);
    std::string sdir = Glib::build_filename(root, id);
    if (!create_directory(sdir)) return;
    path_base=sdir;
    logger.msg(VERBOSE,"Cache directory: %s",path_base);
}

InfoCache::~InfoCache()
{
    // NOP
}

static void clean_path(std::string s)
{
    size_t idx;
    do {
        idx = s.find("//", 0);
        if (idx != std::string::npos) {
            s.replace(idx, 2, "/", 0, 1);
        }
    } while (idx != std::string::npos);
}

static bool set_path(const std::string &path_base,const std::vector<std::string> &tokens,const XMLNode &node)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    int n = 0;
    for(; n < (tokens.size()-1);++n) {
        dir = Glib::build_filename(dir,tokens[n]);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens[n] + ".xml");
    // Workaround needed to save namespacesc properly.
    // TODO: solve it in some better way.
    XMLNode doc; node.New(doc);
    return doc.SaveToFile(file);
}

static bool unset_path(const std::string &path_base,const std::vector<std::string> &tokens)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    int n = 0;
    for(; n < (tokens.size()-1);++n) {
        dir = Glib::build_filename(dir,tokens[n]);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens[n] + ".xml");
    return (::remove(file.c_str()) == 0);
}

static bool get_path(const std::string &path_base,const std::vector<std::string> &tokens,Arc::XMLNode &node)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    int n = 0;
    for(; n < (tokens.size()-1);++n) {
        dir = Glib::build_filename(dir,tokens[n]);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens[n] + ".xml");
    return node.ReadFromFile(file);
}

bool InfoCache::Set(const char *xml_path, Arc::XMLNode &value)
{
    if (path_base.empty()) {
        logger.msg(ERROR,"InfoCache object is not set up");
        return false;
    }
    if (xml_path[0] != '/') {
        logger.msg(ERROR,"Invalid path in Set(): %s",xml_path);
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::vector<std::string> tokens;
    Arc::tokenize(p, tokens, "/");
    bool ret;
    ret = set_path(path_base, tokens, value);
    return ret;
}

bool InfoCache::Unset(const char *xml_path)
{
    if (path_base.empty()) {
        logger.msg(ERROR,"InfoCache object is not set up");
        return false;
    }
    if (xml_path[0] != '/') {
        logger.msg(ERROR,"Invalid path in Set(): %s",xml_path);
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::vector<std::string> tokens;
    Arc::tokenize(p, tokens, "/");
    bool ret;
    ret = unset_path(path_base, tokens);
    return ret;
}

bool InfoCache::Get(const char *xml_path, Arc::XMLNodeContainer &result)
{
    if (path_base.empty()) {
        logger.msg(ERROR,"InfoCache object is not set up");
        return false;
    }
    if (xml_path[0] != '/') {
        logger.msg(ERROR,"Invalid path in Get(): %s",xml_path);
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::vector<std::string> tokens;
    Arc::tokenize(p, tokens, "/");
    if (tokens.size() <= 0) {
        Arc::NS ns;
        Arc::XMLNode node(ns, "InfoDoc");
        merge_xml(path_base, node);
        result.AddNew(node);
        return true;
    }
    Arc::XMLNode node;
    return get_path(path_base,tokens,node);
}

bool InfoCache::Query(const char *path, const char *query, Arc::XMLNodeContainer &result)
{
    if (path_base.empty()) {
        logger.msg(ERROR,"InfoCache object is not set up");
        return false;
    }
    Arc::XMLNodeContainer gc;
    Get(path, gc);
    Arc::NS ns;
    
    for (int i = 0; i < gc.Size(); i++) {
        Arc::XMLNode node = gc[i];
        Arc::XMLNodeList xresult = node.XPathLookup(query,ns);
        result.AddNew(xresult);
    }
    return true;
}

// --------------------------------------------------------------------------------

InfoCacheInterface::InfoCacheInterface(Arc::Config &cfg, std::string &service_id):
                                       cache(cfg,service_id)
{
}

InfoCacheInterface::~InfoCacheInterface(void)
{
}

void InfoCacheInterface::Get(const std::list<std::string>& path,XMLNodeContainer& result)
{
    std::string xml_path;
    for(std::list<std::string>::const_iterator cur_name = path.begin();
        cur_name != path.end(); ++cur_name) {
        xml_path+="/"+(*cur_name);
    };
    if(xml_path.empty()) xml_path="/";
    cache.Get(xml_path,result);
}

void InfoCacheInterface::Get(XMLNode xpath,XMLNodeContainer& result)
{
    std::string query = xpath;
    if(!cache.Query("/",query.c_str(),result)) return;
    return;
}

} // namespace Arc

