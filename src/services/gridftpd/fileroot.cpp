#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>

#include "fileroot.h"
#include "names.h"
#include "misc.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"FilePlugin");

const std::string FileNode::no_error("");
#define NO_PLUGIN(PATH) { logger.msg(Arc::ERROR, "No plugin is configured or authorised for requested path %s", PATH); }

int FilePlugin::release(void) {
  count--;
  if(count < 0) {
    logger.msg(Arc::WARNING, "FilePlugin: more unload than load");
    count=0;
  };
  return count;
}

FileNode::FileNode(char const* dirname,char const* plugin,std::istream &cfile,userspec_t &user) {
  plug=NULL;
  init=NULL;
  point=std::string(dirname);
  plugname=std::string(plugin);
//    handle=dlopen(plugin,RTLD_LAZY);
  handle=dlopen(plugin,RTLD_NOW);
  if(!handle) {
    logger.msg(Arc::ERROR, dlerror());
    logger.msg(Arc::ERROR, "Can't load plugin %s for access point %s", plugin, dirname);
    return;
  };
  init=(plugin_init_t)dlsym(handle,"init");
  if(init == NULL) {
    logger.msg(Arc::ERROR, "Plugin %s for access point %s is broken.", plugin, dirname);
    dlclose(handle); handle=NULL; return;
  };
  if((plug=init(cfile,user,*this)) == NULL) {
    logger.msg(Arc::ERROR, "Plugin %s for access point %s is broken.", plugin, dirname);
    dlclose(handle); handle=NULL; init=NULL; return;
  };
  if(plug->acquire() != 1) {
    logger.msg(Arc::ERROR, "Plugin %s for access point %s acquire failed (should never happen).", plugin, dirname);
    delete plug; dlclose(handle); handle=NULL; init=NULL; plug=NULL; return;
  };
}

FileNode::~FileNode(void) {
  if(plug) if(plug->release() == 0) {
    logger.msg(Arc::VERBOSE, "Destructor with dlclose (%s)", point);
    delete plug; dlclose(handle); handle=NULL; init=NULL; plug=NULL;
  };
}

std::string FileNode::last_name(void) {
  int pl=point.rfind('/');
  if(pl == -1) return point;
  return point.substr(pl+1);
}

bool FileNode::belongs(const char* name) {
  int pl=point.length();
  if(pl == 0) return true;
  int l=strlen(name);
  if (pl > l) return false;
  if(strncmp(point.c_str(),name,pl)) return false;
  if(pl == l) return true;
  if(name[pl] == '/') return true;
  return false;
}

FileNode& FileNode::operator= (const FileNode &node) {
  logger.msg(Arc::VERBOSE, "FileNode: operator= (%s <- %s) %lu <- %lu", point, node.point, (unsigned long int)this, (unsigned long int)(&node));
  if(this == &node) return *this;
  if(plug) if(plug->release() == 0) {
    logger.msg(Arc::VERBOSE, "Copying with dlclose");
    delete plug; dlclose(handle); handle=NULL; init=NULL; plug=NULL;
  };
  point=node.point; plugname=node.plugname;
  plug=node.plug; handle=node.handle;
  return *this;
}

/*
bool FileNode::is_in_dir(const char* name) {
  int l=strlen(name);
  if(point.length() <= l) return false;
  if(point[l] != '/') return false;
  if(strncmp(point.c_str(),name,l)) return false;
  return true;
}
*/
/* should only last directory be shown ? */
bool FileNode::is_in_dir(const char* name) {
  int l=strlen(name);
  int pl=point.rfind('/'); /* returns -1 if not found */
  if(pl == -1) {
    if(l == 0) { 
      return true;
    };
  };
  if(l != pl) return false;
  if(strncmp(point.c_str(),name,l)) return false;
  return true;
}

int FileRoot::size(const char* name,unsigned long long int *size) {
  std::string new_name;
  if(name[0] != '/') { new_name=cur_dir+'/'+name; }
  else { new_name=name; };
  error=FileNode::no_error;
  if(!Arc::CanonicalDir(new_name,false)) return 1;
  if(new_name.empty()) {
    (*size)=0;
    return 0;
  }
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->belongs(new_name.c_str())) {
      DirEntry info;
      if(i->checkfile(new_name,info,DirEntry::basic_object_info) != 0) {
        error=i->error(); return 1;
      };
      (*size)=info.size; 
      return 0;
    };
  };
  NO_PLUGIN(name);
  return 1;
}

int FileRoot::time(const char* name,time_t *time) {
  std::string new_name;
  if(name[0] != '/') { new_name=cur_dir+'/'+name; }
  else { new_name=name; };
  error=FileNode::no_error;
  if(!Arc::CanonicalDir(new_name,false)) return 1;
  if(new_name.empty()) {
    (*time)=0;
    return 0;
  };
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->belongs(new_name.c_str())) {
      DirEntry info;
      if(i->checkfile(new_name,info,DirEntry::basic_object_info) != 0) {
        error=i->error(); return 1;
      };
      (*time)=info.modified; 
      return 0;
    };
  };
  NO_PLUGIN(name);
  return 1;
}

int FileRoot::checkfile(const char* name,DirEntry &info,DirEntry::object_info_level mode) {
  std::string new_name;
  if(name[0] != '/') { new_name=cur_dir+'/'+name; }
  else { new_name=name; };
  error=FileNode::no_error;
  if(!Arc::CanonicalDir(new_name,false)) return 1;
  if(new_name.empty()) {
    info.reset();
    info.name="/";
    info.is_file=false;
    return 0;
  };
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->belongs(new_name.c_str())) {
      if(i->checkfile(new_name,info,mode) != 0) {
        error=i->error(); return 1;
      };
      info.name="/"+new_name;
      return 0;
    };
  };
  NO_PLUGIN(name);
  return 1;
}

int FileRoot::mkd(std::string& name) {
  std::string new_dir;
  if(name[0] != '/') { new_dir=cur_dir+'/'+name; }
  else { new_dir=name; };
  error=FileNode::no_error;
  if(Arc::CanonicalDir(new_dir,false)) {
    for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
      if((*i) == new_dir) { /* already exists, at least virtually */
        name=new_dir;
        return 0;
      };
      if((*i).belongs(new_dir.c_str())) {
        if((*i).makedir(new_dir) == 0) {
          name=new_dir;
          return 0;
        };
        error=i->error(); name=cur_dir; return 1;
      };
    };
    NO_PLUGIN(name);
  };
  name=cur_dir;
  return 1;
}

int FileRoot::rmd(std::string& name) {
  std::string new_dir;
  if(name[0] != '/') { new_dir=cur_dir+'/'+name; }
  else { new_dir=name; };
  error=FileNode::no_error;
  if(Arc::CanonicalDir(new_dir,false)) {
    for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
      if((*i) == new_dir) { /* virtual - not removable */
        return 1;
      };
      if(i->belongs(new_dir.c_str())) {
        int res = i->removedir(new_dir);
        error=i->error(); return res;  
      };
    };
    NO_PLUGIN(name);
  };
  return 1;
}

int FileRoot::rm(std::string& name) {
  std::string new_dir;
  if(name[0] != '/') { new_dir=cur_dir+'/'+name; }
  else { new_dir=name; };
  error=FileNode::no_error;
  if(Arc::CanonicalDir(new_dir,false)) {
    for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
      if((*i) == new_dir) { /* virtual dir - not removable */
        return 1;
      };
      if(i->belongs(new_dir.c_str())) {
        int res = i->removefile(new_dir);
        error=i->error(); return res;  
      };
    };
    NO_PLUGIN(name);
  };
  return 1;
}

int FileRoot::cwd(std::string& name) {
  std::string new_dir;
  if(name[0] != '/') { new_dir=cur_dir+'/'+name; }
  else { new_dir=name; };
  error=FileNode::no_error;
  if(Arc::CanonicalDir(new_dir,false)) {
    if(new_dir.length() == 0) { /* always can go to root ? */
      cur_dir=new_dir;
      name=cur_dir;
      return 0;
    };
    /* check if can cd */
    for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
      if((*i) == new_dir) {
        cur_dir=new_dir;
        name=cur_dir;
        return 0;
      };
      if((*i).belongs(new_dir.c_str())) {
        if((*i).checkdir(new_dir) == 0) {
          cur_dir=new_dir;
          name=cur_dir;
          return 0;
        };
        error=i->error(); name=cur_dir; return 1;
      };
    };
    NO_PLUGIN(name);
  };
  name="/"+cur_dir;
  return 1;
}

int FileRoot::open(const char* name,open_modes mode,unsigned long long int size) {
  std::string new_name;
  if(name[0] != '/') { new_name=cur_dir+'/'+name; }
  else { new_name=name; };
  error=FileNode::no_error;
  if(!Arc::CanonicalDir(new_name,false)) { return 1; };
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->belongs(new_name.c_str())) {
      if(i->open(new_name.c_str(),mode,size) == 0) {
        opened_node=i;
        return 0;
      };
      error=i->error(); return 1;
    };
  };
  NO_PLUGIN(name);
  return 1;
}

int FileRoot::close(bool eof) {
  error=FileNode::no_error;
  if(opened_node != nodes.end()) {
    int i=(*opened_node).close(eof);
    error=opened_node->error();
    opened_node=nodes.end();
    return i;
  };
  return 1;
}

int FileRoot::read(unsigned char* buf,unsigned long long int offset,unsigned long long *size) {
  error=FileNode::no_error;
  if(opened_node != nodes.end()) {
    int res = (*opened_node).read(buf,offset,size);
    error=opened_node->error(); return res;
  };
  return 1;
}

int FileRoot::write(unsigned char *buf,unsigned long long int offset,unsigned long long size) {
  error=FileNode::no_error;
  if(opened_node != nodes.end()) {
    int res = (*opened_node).write(buf,offset,size);
    error=opened_node->error(); return res;
  };
  return 1;
}

/* 0 - ok , 1 - failure, -1 - this is a file */
int FileRoot::readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode) {
  std::string fullname;
  if(name[0] != '/') { fullname=cur_dir+'/'+name; }
  else { fullname=name; };
  error=FileNode::no_error;
  if(!Arc::CanonicalDir(fullname,false)) return 1;
  int res = 1;
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->belongs(fullname.c_str())) {
      res=i->readdir(fullname.c_str(),dir_list,mode);
      error=i->error();
      break;
    };
  };
  if(res == -1) { /* means this is a file */
    std::list<DirEntry>::iterator di = dir_list.end(); --di;
    di->name="/"+fullname;
    return -1;
  };
  for(std::list<FileNode>::iterator i=nodes.begin();i!=nodes.end();++i) {
    if(i->is_in_dir(fullname.c_str())) {
      DirEntry de;
      de.name=i->last_name();
      de.is_file=false;

//      if(i->checkfile(i->point,de,mode) == 0) {
//        if(de.is_file) {
//          de.reset(); de.name=i->last_name(); de.is_file=false;
          /* TODO: fill other attributes */
//
//        };
//      };
      dir_list.push_front(de);
      res=0;
    };
  };
  return res;
}

FileRoot::FileRoot(void):error(FileNode::no_error) {
  cur_dir="";
  opened_node=nodes.end();
  heavy_encryption=true;
  active_data=true;
  //unix_mapped=false;
}

int FileNode::readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode) {
  if(plug) {
    plug->error_description="";
    return plug->readdir(remove_head_dir_c(name,point.length()),dir_list,mode);
  };
  return 0;
}

int FileNode::checkfile(std::string &name,DirEntry &info,DirEntry::object_info_level mode) {
  if(plug) {
    plug->error_description="";
    std::string dname=remove_head_dir_s(name,point.length());
    return plug->checkfile(dname,info,mode);
  };
  return 1;
}

int FileNode::checkdir(std::string &dirname) {
  if(plug) {
    plug->error_description="";
    std::string dname=remove_head_dir_s(dirname,point.length());
    if(plug->checkdir(dname) == 0) {
      dirname=point+'/'+dname;
      return 0;
    };
  };
  return 1;
}

int FileNode::makedir(std::string &dirname) {
  if(plug) {
    plug->error_description="";
    std::string dname=remove_head_dir_s(dirname,point.length());
    return plug->makedir(dname);
  };
  return 1;
}

int FileNode::removedir(std::string &dirname) {
  if(plug) {
    plug->error_description="";
    std::string dname=remove_head_dir_s(dirname,point.length());
    return plug->removedir(dname);
  };
  return 1;
}

int FileNode::removefile(std::string &name) {
  if(plug) {
    plug->error_description="";
    std::string dname=remove_head_dir_s(name,point.length());
    return plug->removefile(dname);
  };
  return 1;
}

int FileNode::open(const char* name,open_modes mode,unsigned long long int size) {
  if(plug) {
    plug->error_description="";
    return plug->open(remove_head_dir_c(name,point.length()),mode,size);
  };
  return 1;
}

int FileNode::close(bool eof) {
  if(plug) {
    plug->error_description="";
    return plug->close(eof);
  };
  return 1;
}

int FileNode::read(unsigned char *buf,unsigned long long int offset,unsigned long long *size) {
  if(plug) {
    plug->error_description="";
    return plug->read(buf,offset,size);
  };
  return 1;
}

int FileNode::write(unsigned char *buf,unsigned long long int offset,unsigned long long size) {
  if(plug) {
    plug->error_description="";
    return plug->write(buf,offset,size);
  };
  return 1;
}

