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
#include <cstdio>

#ifdef WIN32
#include <arc/win32.h>
#endif

namespace Arc {

static Logger logger(Logger::getRootLogger(), "InfoCache");

static RegularExpression id_regex("@id=\"([a-zA-Z0-9_\\\\-]*)\"");

static void merge_xml(std::string& path_base, XMLNode &node)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        std::string path_fl1 = Glib::build_filename(path_base, d);
        //std::cout << "merge_xml f1: " << path_fl1 << std::endl;
        if (Glib::file_test(path_fl1, Glib::FILE_TEST_IS_REGULAR)) {
            std::string xml_str = Glib::file_get_contents(path_fl1);
            XMLNode n(xml_str);
            XMLNode c;
            for (int i = 0; (bool)(c = n.Child(i)); i++) {
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

InfoCache::InfoCache(const Config &cfg, const std::string &service_id)
{
    std::string cfg_s;
    cfg.GetXML(cfg_s);
    logger.msg(DEBUG,"Cache configuration: %s",cfg_s);
    std::string root = std::string(const_cast<Config&>(cfg)["CacheRoot"]);
    if(root.empty()) {
        logger.msg(ERROR,"Missing cache root in configuration");
        return;
    }
    if(service_id.empty()) {
        logger.msg(ERROR,"Missing service id");
        return;
    }
    logger.msg(DEBUG,"Cache root: %s",root);
    if (!create_directory(root)) return;
    std::string id(service_id);
    std::string sdir = Glib::build_filename(root, id);
    if (!create_directory(sdir)) return;
    path_base=sdir;
    logger.msg(DEBUG,"Cache directory: %s",path_base);
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

static bool set_path(const std::string &path_base,const std::list<std::string> &tokens,const XMLNode &node)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    const std::list<std::string>::const_iterator itLastElement = --tokens.end();
    for (std::list<std::string>::const_iterator it = tokens.begin();
         it != itLastElement; it++) {
        dir = Glib::build_filename(dir, *it);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens.back() + ".xml");
    // Workaround needed to save namespaces properly.
    // TODO: solve it in some better way.
    XMLNode doc; node.New(doc);
    return doc.SaveToFile(file);
}

static bool unset_path(const std::string &path_base,const std::list<std::string> &tokens)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    const std::list<std::string>::const_iterator itLastElement = --tokens.end();
    for (std::list<std::string>::const_iterator it = tokens.begin();
         it != itLastElement; it++) {
        dir = Glib::build_filename(dir, *it);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens.back() + ".xml");
    return (::remove(file.c_str()) == 0);
}

static bool get_path(const std::string &path_base,const std::list<std::string> &tokens,XMLNode &node)
{
    if(tokens.size() < 1) return false;
    std::string dir = path_base;
    const std::list<std::string>::const_iterator itLastElement = --tokens.end();
    for (std::list<std::string>::const_iterator it = tokens.begin();
         it != itLastElement; it++) {
        dir = Glib::build_filename(dir, *it);
        if (!create_directory(dir)) return false;
    };
    std::string file = Glib::build_filename(dir, tokens.back() + ".xml");
    return node.ReadFromFile(file);
}

bool InfoCache::Set(const char *xml_path, XMLNode &value)
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
    std::list<std::string> tokens;
    tokenize(p, tokens, "/");
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
    std::list<std::string> tokens;
    tokenize(p, tokens, "/");
    bool ret;
    ret = unset_path(path_base, tokens);
    return ret;
}

bool InfoCache::Get(const char *xml_path, XMLNodeContainer &result)
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
    std::list<std::string> tokens;
    tokenize(p, tokens, "/");
    if (tokens.size() <= 0) {
        NS ns;
        XMLNode node(ns, "InfoDoc");
        merge_xml(path_base, node);
        result.AddNew(node);
        return true;
    }
    XMLNode node;
    return get_path(path_base,tokens,node);
}

bool InfoCache::Query(const char *path, const char *query, XMLNodeContainer &result)
{
    if (path_base.empty()) {
        logger.msg(ERROR,"InfoCache object is not set up");
        return false;
    }
    XMLNodeContainer gc;
    Get(path, gc);
    NS ns;
    
    for (int i = 0; i < gc.Size(); i++) {
        XMLNode node = gc[i];
        XMLNodeList xresult = node.XPathLookup(query,ns);
        result.AddNew(xresult);
    }
    return true;
}

// --------------------------------------------------------------------------------

InfoCacheInterface::InfoCacheInterface(Config &cfg, std::string &service_id):
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

