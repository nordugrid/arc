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
#include <vector>

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
        if (mkdir(root.c_str(), 0700) != 0)
#else
	    if (mkdir(root.c_str()) != 0)
#endif
        {
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
        if (mkdir(path_base.c_str(), 0700) != 0) 
#else
	    if (mkdir(path_base.c_str()) != 0) 
#endif
        {
            std::cerr << "cannot create directory: " << path_base << std::endl;
            // rootLogger.msg(Arc::ERROR, "cannot create directory:" + path_base);
            path_base = "";
            return;
        }        
    }
}

InfoCache::~InfoCache()
{
    // NOP
}

bool InfoCache::get_file_and_relative_paths(std::string& xml_path, 
                                            std::string& file_path, 
                                            std::string& rel_path)
{
    size_t secund_per = xml_path.find("/", xml_path.find("/")+1) + 1;
    if (secund_per == std::string::npos || secund_per == 0) {
        return false;
    }
    size_t thrid_per = xml_path.find("/", secund_per);
    if (thrid_per == std::string::npos || thrid_per == 0) {
        return false;
    }
    std::string first_level = xml_path.substr(secund_per, thrid_per - secund_per);
    if (first_level.empty() == true) {
        return false;
    }
    std::string dir = Glib::build_filename(path_base, first_level);
    file_path = Glib::build_filename(dir, "xml");
    rel_path = xml_path.substr(thrid_per);

    return true;
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

static void tokenize(const std::string& str, std::vector<std::string>& tokens,
                     const std::string& delimiters = " ")
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

static Arc::XMLNode get_node_by_path(Arc::XMLNode &xml, std::string &path)
{
    std::vector<std::string> path_elements;
    tokenize(path, path_elements, "/");
    std::vector<std::string>::iterator pe;
    Arc::XMLNode ret = xml;
    for (pe = path_elements.begin(); pe != path_elements.end(); pe++) {
        std::cout << *pe << std::endl;
        Arc::XMLNode n = ret[(*pe)];
        if (n == false) {
            std::cout << "invalid path" << std::endl;
            return n;
        } else {
            ret = n; 
        }
    }   
    return ret;
}

bool InfoCache::Set(const char *xml_path, Arc::XMLNode &value)
{
    if (xml_path[0] != '/') {
        std::cerr << "invalid xml_path" << std::endl;
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::string file_path;
    std::string rel_path;
    std::string data_str;
    if (get_file_and_relative_paths(p, file_path, rel_path) == true) {
        std::cout << file_path << "," << rel_path << std::endl;
        if (Glib::file_test(file_path, Glib::FILE_TEST_EXISTS) == true) {
            // update XML
            std::string xml_str = Glib::file_get_contents(file_path);
            Arc::XMLNode xml(xml_str);
            Arc::XMLNode child = get_node_by_path(xml, rel_path);
            if (child == true) {
                std::string s;
                child.GetXML(s);
                std::cout << s << std::endl;
                child.Replace(value);
                xml.GetXML(data_str);
            } else {
                return false;
            }
        } else {
            // create new XML
            value.GetXML(data_str);
        }
        std::ofstream out(file_path.c_str(), std::ios::out); 
        if (!out) return false;
        out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
        out << data_str;
        out.close();
    } else {
        // xml should fragmented
        // std::cout << "XML split" << std::endl;
        std::vector<std::string> tokens;
        tokenize(p, tokens, "/");
        /* for (std::vector<std::string>::iterator it=tokens.begin(); it != tokens.end(); it++) {
            std::cout << (*it) << std::endl;
        } */
        if (tokens.size() == 0 || tokens.size() == 1) {
            // xml should fragmented
            Arc::XMLNode n;
            for (int i = 0; (n = value.Child(i)) != false; i++)  {
                std::string dir = Glib::build_filename(path_base, n.Name());
                std::cout << dir << std::endl;
                if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
                    // create directory
#ifndef WIN32
                    if (mkdir(dir.c_str(), 0700) != 0) 
#else
	                if (mkdir(dir.c_str()) != 0)
#endif
                    {
                        std::cerr << "Cannot create directory: " << dir << std::endl;
                        return false;
                    }
                }
                std::string fn = Glib::build_filename(dir, "xml");
                std::ofstream out(fn.c_str(), std::ios::out); 
                if (!out) return false;
                std::string data_str;
                n.GetXML(data_str);
                out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
                out << data_str;
                out.close();
            }
        } else {
            std::string dir = Glib::build_filename(path_base, value.Name());
            if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
                // create directory
#ifndef WIN32
                if (mkdir(dir.c_str(), 0700) != 0) 
#else
	            if (mkdir(dir.c_str()) != 0)
#endif
                {
                    std::cerr << "Cannot create directory: " << dir << std::endl;
                    return false;
                }
            }
            std::string fn = Glib::build_filename(dir, "xml");
            std::ofstream out(fn.c_str(), std::ios::out); 
            if (!out) return false;
            std::string data_str;
            value.GetXML(data_str);
            out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
            out << data_str;
            out.close();
        }
    }  
    return true;
}

static void merge_xml(std::string& path_base, Arc::XMLNode &node)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        std::string path = Glib::build_filename(Glib::build_filename(path_base, d), "xml");
        std::string xml_str = Glib::file_get_contents(path);
        Arc::XMLNode n(xml_str);
        node.NewChild(n);
    }
}

bool InfoCache::Get(const char *xml_path, Arc::XMLNodeContainer &result)
{
    std::string p(xml_path);
    clean_path(p);
    std::string file_path;
    std::string rel_path;
    if (get_file_and_relative_paths(p, file_path, rel_path) == true) {
        if (Glib::file_test(file_path, Glib::FILE_TEST_EXISTS) == true) {
            std::string str = Glib::file_get_contents(file_path);
            Arc::XMLNode node(str);
            result.AddNew(node);
            return true;
        }
    } else {
        std::vector<std::string> tokens;
        tokenize(p, tokens, "/");
        if (tokens.size() == 0 || tokens.size() == 1) {
            Arc::NS ns;
            Arc::XMLNode node(ns, "InfoDoc");
            merge_xml(path_base, node);
            result.AddNew(node);
            return true;
        } else {
            std::string t = *tokens.end();
            std::string dir = Glib::build_filename(path_base, t);
            if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
                return false;
            } else {
                std::string fn = Glib::build_filename(dir, "xml");
                std::string str = Glib::file_get_contents(fn);
                Arc::XMLNode node(str);
                result.AddNew(node);
            }
        }
    }
    return false;
}

bool InfoCache::query_one(const char *key, const char *q, Arc::XMLNodeContainer &result)
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

bool InfoCache::query_any(const char *q, Arc::XMLNodeContainer &result)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    while ((d = dir.read_name()) != "") {
        query_one(d.c_str(), q, result);
    }
}

bool InfoCache::Query(const char *key, const char *q, Arc::XMLNodeContainer &result)
{
    if (strcmp(key, "any") == 0) {
        return query_any(q, result);
    }
    return query_one(key, q, result);
}

}
