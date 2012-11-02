#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <unistd.h>

#include <glibmm/module.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/loader/Plugin.h>
#include <arc/StringConv.h>
#include <arc/ArcRegex.h>


std::list< std::pair<Arc::RegularExpression,uint32_t> > priorities_map;

static uint32_t map_priority(const std::string& str1, const std::string& str2) {
  for(std::list< std::pair<Arc::RegularExpression,uint32_t> >::iterator p = priorities_map.begin();
                         p != priorities_map.end(); ++p) {
    if(p->first.match(str1) || p->first.match(str2)) return p->second;
  }
  return ARC_PLUGIN_DEFAULT_PRIORITY;
}


static std::string encode_for_var(uint32_t v) {
    return "\"" + Arc::tostring(v) + "\"";
}

static std::string encode_for_var(const char* str) {
    std::string stro = "\"";
    stro += str;
    std::string::size_type p = 1;
    for(;;++p) {
        p = stro.find_first_of("\"\\",p);
        if(p == std::string::npos) break;
        stro.insert(p, "\\");
        ++p;
    }
    stro += "\"";
    return stro;
}

static std::string replace_file_suffix(const std::string& path,const std::string& newsuffix) {
    std::string newpath = path;
    std::string::size_type name_p = newpath.rfind(G_DIR_SEPARATOR_S);
    if(name_p == std::string::npos) {
        name_p = 0;
    } else {
        ++name_p;
    }
    std::string::size_type suffix_p = newpath.find('.',name_p);
    if(suffix_p != std::string::npos) {
        newpath.resize(suffix_p);
    }
    newpath += "." + newsuffix;
    return newpath;
}

static bool process_module(const std::string& plugin_filename, bool create_apd) {
    Arc::PluginDescriptor dummy_desc[2];
    memset(dummy_desc,0,sizeof(dummy_desc));
    dummy_desc[0].name = "";
    dummy_desc[0].kind = "";
    dummy_desc[0].description = "";
    dummy_desc[0].version = 0;
    dummy_desc[0].instance = (Arc::get_plugin_instance)dummy_desc;
    std::string descriptor_filename = replace_file_suffix(plugin_filename,"apd");
    Glib::ModuleFlags flags = Glib::ModuleFlags(0);
    flags|=Glib::MODULE_BIND_LAZY;
    Glib::Module *module = new Glib::Module(plugin_filename,flags);
    if ((!module) || (!(*module))) {
        std::cerr << "Failed to load module " << plugin_filename << ": " << Glib::Module::get_last_error() << std::endl;
        return false;
    }

    std::cout << "Loaded module " << plugin_filename << std::endl;
    std::cout << std::endl;

    void *ptr = NULL;
    if(!module->get_symbol(PLUGINS_TABLE_SYMB,ptr)) {
        std::cerr << "Module " << plugin_filename << " is not an ARC plugin: " << Glib::Module::get_last_error() << std::endl;
        if(create_apd) {
          std::cerr << "Dummy descriptor file will be created to avoid loading this module at all" << std::endl;
          // This is needed to make rpmlint happy.
          ptr = dummy_desc;
        }
        //delete module;
        //return -1;
    };

    Arc::PluginDescriptor* desc = (Arc::PluginDescriptor*)ptr;

    std::ofstream apd;
    if(create_apd) {
        apd.open(descriptor_filename.c_str());
        if(!apd) {
            std::cerr << "Failed to create descriptor file " << descriptor_filename << std::endl;
            return false;
        };
    };

    for(;desc;++desc) {
        if(desc->name == NULL) break;
        if(desc->kind == NULL) break;
        if(desc->instance == NULL) break;
        if(create_apd) {
            uint32_t priority = map_priority(desc->name, desc->kind);
            apd << "name=" << encode_for_var(desc->name) << std::endl;
            apd << "kind=" << encode_for_var(desc->kind) << std::endl;
            if (desc->description != NULL) {
              apd << "description=" << encode_for_var(desc->description) << std::endl;
            }
            apd << "version=" << encode_for_var(desc->version) << std::endl;
            apd << "priority=" << encode_for_var(priority) <<  std::endl;
            apd << std::endl; // end of description mark
        } else {
            std::cout << "name: " << desc->name << std::endl;
            std::cout << "kind: " << desc->kind << std::endl;
            if (desc->description != NULL) {
              std::cout << "description: " << desc->description << std::endl;
            }
            std::cout << "version: " << desc->version << std::endl;
            std::cout << std::endl;
        };
    };

    if(create_apd) {
        apd.close();
        std::cout << "Created descriptor " << descriptor_filename << std::endl;
    };

    // We are not unloading module because it may be not suitable
    // for unloading or it may be library which may fail unloading
    // after it was loaded with dlopen().
    //delete module;

    return true;
}

int main(int argc, char **argv)
{
    const std::string modsuffix("." G_MODULE_SUFFIX);
    bool create_apd = false;
    bool recursive = false;

    while (argc > 1) {
        if (strcmp(argv[1],"-c") == 0) {
            create_apd = true;
            --argc; ++argv;
        } else if(strcmp(argv[1],"-r") == 0) {
            recursive = true;
            --argc; ++argv;
        } else if(strcmp(argv[1],"-p") == 0) {
            if(argc <= 2) {
                std::cerr << "Missing option for -p" << std::endl;
                return -1;
            }
            uint32_t priority;
            std::string option = argv[2];
            std::string::size_type comma = option.find(',');
            if(comma == std::string::npos) {
                std::cerr << "Missing , in -p option" << std::endl;
                return -1;
            }
            if(!Arc::stringto(option.substr(0,comma),priority)) {
                std::cerr << "Can't parse prority number " << option.substr(0,comma) << std::endl;
                return -1;
            }
std::cerr<<"+++ "<<priority<<" - '"<<option.substr(comma+1)<<"'"<<std::endl;
            priorities_map.push_back(std::pair<Arc::RegularExpression,uint32_t>(Arc::RegularExpression(option.substr(comma+1)),priority));
            --argc; ++argv;
            --argc; ++argv;
        } else if (strcmp(argv[1],"-h") == 0) {
            std::cout << "arcplugin [-c] [-r] [-p priority,regex] [-h] plugin_path [plugin_path [...]]" << std::endl;
            std::cout << "  -c If specified then APD file is created using same name" << std::endl;
            std::cout << "     as ARC plugin with suffix replaced with .apd." << std::endl;
            std::cout << "  -r If specified operation is fully recursive." << std::endl;
            std::cout << "  -p Defines which priority to be assigned for each plugin." << std::endl;
            std::cout << "     Each plugin's kind and name attributes are matched" << std::endl;
            std::cout << "     specified regex. One which matches gets specified" << std::endl;
            std::cout << "     This option can be specified multiple times." << std::endl;
            std::cout << "  -h prints this help and exits." << std::endl;
            std::cout << "  plugin_path is full path to ARC plugin loadable module" << std::endl;
            std::cout << "     file or directory containing such modules." << std::endl;
            return 0;
        } else {
            break;
        };
    };

    if (argc < 2) {
        std::cerr << "Missing arguments" << std::endl;
        return -1;
    };

    std::list<std::string> paths;
    for(int n = 1; n < argc; ++n) paths.push_back(argv[n]);
    int user_paths = paths.size();

    int num = 0;
    for(std::list<std::string>::iterator path = paths.begin();
                  path != paths.end(); ++path) {
        try {
          Glib::Dir dir(*path);
          if((!recursive) && (num >= user_paths)) continue;
          for (Glib::DirIterator file = dir.begin();
                                file != dir.end(); file++) {
            std::string name = *file;
            if(name == ".") continue;
            if(name == "..") continue;
            paths.push_back(Glib::build_filename(*path, name));
          }
        } catch (Glib::FileError&) {
          if(path->length() <= modsuffix.length()) continue;
          if(path->substr(path->length()-modsuffix.length()) != modsuffix) continue;
          process_module(*path, create_apd);
        }
        ++num;
    }

    //return 0;
    // Do quick exit to avoid possible problems with module unloading
    _exit(0);
}

