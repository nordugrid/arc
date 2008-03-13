#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <InfoCache.h>
#include <arc/Logger.h>
#include <arc/ArcRegex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <vector>

#ifndef WIN32
#define MKDIR(x) (mkdir(x, 0700))
#else
#define MKDIR(x) (mkdir(x))
#endif

namespace Arc {

static Arc::RegularExpression id_regex("@id=\"([a-zA-Z0-9_\\\\-]*)\"");

InfoCache::InfoCache(Arc::Config &cfg, std::string &service_id)
{
    InfoCache(cfg, service_id.c_str());
}

InfoCache::InfoCache(Arc::Config &cfg, const char *service_id)
{
    std::string root = std::string(cfg["InformationSystem"]["CacheRoot"]);
    std::cout << "Cache root: " << root << std::endl;
    if (!Glib::file_test(root, Glib::FILE_TEST_IS_DIR)) {
        // create root directory
        if (MKDIR(root.c_str()) != 0)
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
        if (MKDIR(path_base.c_str()) != 0) 
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

static Arc::XMLNode get_node_by_path(Arc::XMLNode &xml, std::vector<std::string> &path_elements)
{
    std::vector<std::string>::iterator pe;
    Arc::XMLNode ret = xml;
    for (pe = path_elements.begin(); pe != path_elements.end(); pe++) {
        std::cout << "node: " << *pe << std::endl;
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

bool set_sub_path(std::string path_base, std::vector<std::string> &tokens, Arc::XMLNode &node)
{
    std::string data_str = "";
    
    // determine file path and relative tokens
    std::string dir = Glib::build_filename(path_base, tokens[1]);
    std::string name = tokens[2];
    std::vector<std::string> rel_tokens(tokens.begin()+2, tokens.end());
    
    std::string file_path;
    std::list<std::string> match, unmatch;
    bool ret = id_regex.match(name, unmatch, match);
    std::string id;

    if (ret == true) {
        std::list<std::string>::iterator it;
        int j;
        for (it = match.begin(), j = 0; it != match.end(); it++, j++) {
            // std::cout << (*it) << ":" << j << std::endl;
            if (j == 1) {
                id = *it;
                break;
            }
        }
        name.resize(name.find("["));
        rel_tokens[0].resize(rel_tokens[0].find("["));
        std::string n = name + "-" + id + ".xml";
        file_path = Glib::build_filename(dir, n);
    } else {
        // here assume non id 
        file_path = Glib::build_filename(dir, "xml"); 
    }
    
    std::cout << file_path << std::endl;

    if (Glib::file_test(file_path, Glib::FILE_TEST_EXISTS) == true) {
        std::string xml_str = Glib::file_get_contents(file_path);
        Arc::XMLNode xml(xml_str);
        Arc::XMLNode child = get_node_by_path(xml, rel_tokens);
        if (child == true) {
            child.Replace(node);
        } else {
            xml.NewChild(node);
        }
        xml.GetXML(data_str);
    } else {
        std::string s;
        node.GetXML(s);
        std::string root_name = *(tokens.end()-1);
        data_str = "<" + root_name + ">" + s + "</" + root_name + ">";
    }
    
    std::ofstream out(file_path.c_str(), std::ios::out); 
    if (!out) return false;
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    out << data_str;
    out.close();
    
    return true;
}

static bool set_secund_level(std::string &dir, std::vector<std::string> &tokens, Arc::XMLNode &node)
{
    Arc::XMLNode id = node.Attribute("id");
    Arc::XMLNode n;
    std::string root_name = tokens[1];
    std::string name = tokens[2];
    std::string file_path;

    if (id == true) {
        file_path = Glib::build_filename(dir, node.Name() + "-" + (std::string)id + ".xml");
    } else {
        file_path = Glib::build_filename(dir, "xml");
    }
    
    std::string data_str;
    if (Glib::file_test(file_path, Glib::FILE_TEST_EXISTS) == true) {
        std::string xml_str = Glib::file_get_contents(file_path);
        Arc::XMLNode xml(xml_str);
        Arc::XMLNode child = xml[name];
        if (child == true) {
            child.Replace(node);
        } else {
            xml.NewChild(node);
        }
        xml.GetXML(data_str);
    } else {
        std::string s;
        node.GetXML(s);
        data_str = "<" + root_name + ">" + s + "</" + root_name + ">";
    }
    
    std::ofstream out(file_path.c_str(), std::ios::out); 
    if (!out) return false;
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    out << data_str;
    out.close();

    return true;
}

static bool set_first_level(std::string &dir, std::vector<std::string>&, Arc::XMLNode &node)
{
    std::string data_str = "";
    Arc::XMLNode nn;

    for (int j = 0; (nn = node.Child(j)) != false; j++) {
        Arc::XMLNode id = nn.Attribute("id");
        if (id == true) {
            // set elements with id attribute to separate file
            std::string s;
            nn.GetXML(s);
            std::string file_path = Glib::build_filename(dir, nn.Name() + "-" + (std::string)id + ".xml");
            std::ofstream out(file_path.c_str(), std::ios::out); 
            if (!out) return false;
            out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
            std::string root_name = node.Name();
            out << "<" + root_name +">" + s + "</" + root_name + ">";
            out.close();
        } else {
            // collect element content which has no id
            std::string s;
            nn.GetXML(s);
            data_str += s;
        }
    }

    std::string file_path = Glib::build_filename(dir, "xml");
    std::ofstream out(file_path.c_str(), std::ios::out); 
    if (!out) return false;
    out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    std::string root_name = node.Name();
    out << "<" + root_name +">" + data_str + "</" + root_name + ">";
    out.close();
    return true;
}

static bool set_full_path(std::string &path_base, std::vector<std::string> &tokens, Arc::XMLNode &value)
{
    Arc::XMLNode n;
    std::string dir;
    
    for (int i = 0; (n = value.Child(i)) != false; i++)  {
        dir = Glib::build_filename(path_base, n.Name());
        if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
            // create directory
            if (MKDIR(dir.c_str()) != 0) 
            {
                std::cerr << "Cannot create directory: " << dir << std::endl;
                return false;
            }
        }
        if (set_first_level(dir, tokens, n) == false) {
            return false;
        }
    }
    return true;
}

bool InfoCache::Set(const char *xml_path, Arc::XMLNode &value)
{
    if (xml_path[0] != '/') {
        std::cerr << "invalid xml_path" << std::endl;
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::vector<std::string> tokens;
    tokenize(p, tokens, "/");
    bool ret;
    std::cout << "path: " << p << "(" << tokens.size() << ")" << std::endl;
    if (tokens.size() < 2) {
        ret = set_full_path(path_base, tokens, value);
    } else {
        if (tokens.size() == 2) {
            std::string dir = Glib::build_filename(path_base, tokens[1]);
            ret = set_first_level(dir, tokens, value);
        } else if (tokens.size() == 3) {
            std::string dir = Glib::build_filename(path_base, tokens[1]);
            ret = set_secund_level(dir, tokens, value);
        } else {
            ret = set_sub_path(path_base, tokens, value);
        }
    }
    return ret;
}

static void merge_xml(std::string& path_base, Arc::XMLNode &node)
{
    Glib::Dir dir(path_base);
    std::string d;
    
    std::cout << "merge_xml dir: " << path_base << std::endl;
    while ((d = dir.read_name()) != "") {
        std::string path_fl1 = Glib::build_filename(path_base, d);
        std::cout << "merge_xml f1: " << path_fl1 << std::endl;
        if (Glib::file_test(path_fl1, Glib::FILE_TEST_IS_DIR)) {
            Glib::Dir dir_fl1(path_fl1);
            std::string dd;
            Arc::XMLNode nnode = node.NewChild(d);
            while ((dd = dir_fl1.read_name()) != "") {
                std::string path = Glib::build_filename(path_fl1, dd);
                std::string xml_str = Glib::file_get_contents(path);
                Arc::XMLNode n(xml_str);
                Arc::XMLNode c;
                for (int i = 0; (c = n.Child(i)) != false; i++) {
                    nnode.NewChild(c);
                }
            }
        } else {
            std::string xml_str = Glib::file_get_contents(path_fl1);
            Arc::XMLNode n(xml_str);
            Arc::XMLNode c;
            for (int i = 0; (c = n.Child(i)) != false; i++) {
                    node.NewChild(c);
            }
        }
    }
}

bool InfoCache::Get(const char *xml_path, Arc::XMLNodeContainer &result)
{
    if (xml_path[0] != '/') {
        std::cerr << "invalid xml_path" << std::endl;
        return false;
    }
    std::string p(xml_path);
    clean_path(p);
    std::vector<std::string> tokens;
    tokenize(p, tokens, "/");
    std::cout << "get path: " << p << "(" << tokens.size() << ")" << std::endl;
    if (tokens.size() < 2) {
        Arc::NS ns;
        Arc::XMLNode node(ns, "InfoDoc");
        merge_xml(path_base, node);
        result.AddNew(node);
        return true;
    } else {
        if (tokens.size() == 2) {
            std::string dir = Glib::build_filename(path_base, tokens[1]);
            Arc::NS ns;
            Arc::XMLNode node(ns, "InfoDoc");
            Arc::XMLNode nnode = node.NewChild(tokens[1]);
            merge_xml(dir, nnode);
            result.AddNew(node);
            return true;
        } else {
            std::string dir = Glib::build_filename(path_base, tokens[1]);
            std::string name = tokens[2]; 
            std::list<std::string> match, unmatch;
            bool ret = id_regex.match(name, unmatch, match);
            std::string file_path;
            if (ret == true) {
                std::list<std::string>::iterator it;
                int j;
                std::string id;
                for (it = match.begin(), j = 0; it != match.end(); it++, j++) {
                    // std::cout << (*it) << ":" << j << std::endl;
                    if (j == 1) {
                        id = *it;
                        break;
                    }
                }
                name.resize(name.find("["));
                tokens[2].resize(tokens[2].find("["));
                std::string n = name + "-" + id + ".xml";
                file_path = Glib::build_filename(dir, n);
            } else {
                file_path = Glib::build_filename(dir, "xml"); 
            }
            std::string str = Glib::file_get_contents(file_path);
            Arc::XMLNode node(str);
            result.AddNew(node);
        }
    }
    return false;
}

bool InfoCache::Query(const char *path, const char *query, Arc::XMLNodeContainer &result)
{
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

}
