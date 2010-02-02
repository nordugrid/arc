#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <glibmm/module.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/loader/Plugin.h>
#include <arc/StringConv.h>

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
          std::cerr << "Empty descriptor file will be created to avoid loading this module at all" << std::endl;
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
            apd << "name=" << encode_for_var(desc->name) << std::endl;
            apd << "kind=" << encode_for_var(desc->kind) << std::endl;
            apd << "version=" << encode_for_var(desc->version) << std::endl;
            apd << std::endl; // end of description mark
        } else {
            std::cout << "name: " << desc->name << std::endl;
            std::cout << "kind: " << desc->kind << std::endl;
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

    if (argc > 1) {
        if (strcmp(argv[1],"-c") == 0) {
            create_apd = true;
            --argc; ++argv;
        };
    };
   
    if (argc < 2) {
        std::cerr << "Invalid arguments" << std::endl;        
        return -1;
    };

    std::list<std::string> paths;
    for(int n = 1; n < argc; ++n) paths.push_back(argv[n]);

    for(std::list<std::string>::iterator path = paths.begin();
                  path != paths.end(); ++path) {
        try {
          Glib::Dir dir(*path);
          for (Glib::DirIterator file = dir.begin();
                                file != dir.end(); file++) {
            std::string name = *file;
            if(name == ".") continue;
            if(name == "..") continue;
            paths.push_back(Glib::build_filename(*path, name));
          }
        } catch (Glib::FileError) {
          if(path->length() <= modsuffix.length()) continue;
          if(path->substr(path->length()-modsuffix.length()) != modsuffix) continue;
          process_module(*path, create_apd);
        }
    }

    //return 0;
    // Do quick exit to avoid possible problems with module unloading
    _exit(0);
}

