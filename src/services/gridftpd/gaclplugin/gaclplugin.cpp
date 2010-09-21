#define GRIDFTP_PLUGIN

#include <sys/vfs.h>

#include <arc/StringConv.h>

#include "../conf/conf.h"
#include "../userspec.h"
#include "../names.h"
#include "../misc.h"
#include "../misc/canonical_dir.h"
#include "../misc/escaped.h"

#include "gaclplugin.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"GACLPlugin");

#ifndef HAVE_STRERROR_R
static char * strerror_r (int errnum, char * buf, size_t buflen) {
  char * estring = strerror (errnum);
  strncpy (buf, estring, buflen);
  return buf;
}
#endif

#define  ADD_SUBSTITUTE_VALUE(nam,val) \
  if(val) { \
    GACLnamevalue *s = \
       (GACLnamevalue *)malloc(sizeof(GACLnamevalue)); \
    if(s) { \
      s->next=subst; subst=s; \
      subst->name=strdup(nam); \
      subst->value=strdup(val); \
    }; \
  }

#define DEFAULT_TOP_GACL "<?xml version=\"1.0\"?><gacl version=\"0.0.1\"><entry><any-user/><deny><admin/><write/><read/><list/></deny></entry></gacl>"
static const char* default_top_gacl = DEFAULT_TOP_GACL;

/* name must be absolute path */
/* make directories out of scope of mount dir */
static int makedirs(std::string &name) {
  struct stat st;
  if(stat(name.c_str(),&st) == 0) {
    if(S_ISDIR(st.st_mode)) return 0;
    return 1;
  };
  std::string::size_type n=1;
  for(;;) {
    if(n >= name.length()) break;
    n=name.find('/',n); if(n==std::string::npos) n=name.length();
    std::string dname=name.substr(0,n); n++;
    if(stat(dname.c_str(),&st) == 0) {
      if(S_ISDIR(st.st_mode)) continue;
      return 1;
    };
    if(mkdir(dname.c_str(),S_IRWXU) == 0) continue;
    char* errmsg;
    char errmgsbuf[256];
#ifndef _AIX
    errmsg=strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#else
    errmgsbuf[0]=0; errmsg=errmgsbuf;
    strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#endif
    logger.msg(Arc::ERROR, "mkdir failed: %s", errmsg);
    return 1;
  };
  return 0;
}

GACLPlugin::GACLPlugin(std::istream &cfile,userspec_t &u) {
  data_file=-1;
  subst=NULL;
  /* read configuration */
  std::string xml;
  acl=NULL;
  subject=u.user.DN();
  user=&(u.user);
  subst=(GACLnamevalue *)malloc(sizeof(GACLnamevalue));
  if(subst) {
    subst->next=NULL;
    subst->name=strdup("subject");
    subst->value=strdup(subject.c_str());
  };
  ADD_SUBSTITUTE_VALUE("vo",u.user.default_vo());
  ADD_SUBSTITUTE_VALUE("role",u.user.default_role());
  ADD_SUBSTITUTE_VALUE("capability",u.user.default_capability());
  ADD_SUBSTITUTE_VALUE("group",u.user.default_vgroup());
  ADD_SUBSTITUTE_VALUE("voms",u.user.default_voms());
  if(!cfile.eof()) {
    std::string line = gridftpd::config_read_line(cfile);
    // autodetect central configuration
    const char* p = line.c_str();
    for(;*p;) if(!isspace(*p)) break;
    if(!*p) {
      logger.msg(Arc::ERROR, "Empty root directory for GACL plugin");
      return;
    };
    if((strncmp("gacl ",p,5) != 0) &&
       (strncmp("mount ",p,6) != 0)) {
      gridftpd::make_unescaped_string(line);
      if((line.length() == 0) || (line == "end")) {
        logger.msg(Arc::ERROR, "Empty root directory for GACL plugin");
        return;
      };
      basepath=line;
      for(;!cfile.eof();) {
        line = gridftpd::config_read_line(cfile);
        const char* p = line.c_str();
        const char* command;
        for(;*p;p++) if(!isspace(*p)) break;
        command=p;
        for(;*p;p++) if(isspace(*p)) break;
        if(((p-command)==3) && (strncmp(command,"end",3)==0)) {
          break;
        };
        xml+=line;
      };
    } else {
      for(;!cfile.eof();) {
        const char* p = line.c_str();
        const char* command;
        for(;*p;p++) if(!isspace(*p)) break;
        command=p;
        for(;*p;p++) if(isspace(*p)) break;
        if(((p-command)==3) && (strncmp(command,"end",3)==0)) {
          break;
        } else if(((p-command)==5) && (strncmp(command,"mount",5)==0)) {
          gridftpd::input_escaped_string(p,basepath);
        } else if(((p-command)==4) && (strncmp(command,"gacl",4)==0)) {
          for(;*p;p++) if(!isspace(*p)) break;
          xml=p;
        };
        line = gridftpd::config_read_line(cfile);
      };
      if(basepath.length() == 0) {
        logger.msg(Arc::ERROR, "Empty root directory for GACL plugin");
        return;
      };
    };
  };
  if(basepath.length() == 0) {
    logger.msg(Arc::ERROR, "Empty root directory for GACL plugin");
    return;
  };
  if(xml.length()) {
    acl=NGACLacquireAcl(xml.c_str());
    if(acl == NULL) {
      logger.msg(Arc::ERROR, "Failed to parse default GACL document");
    };
  };
  {
    std::string fname = basepath+"/.gacl";
    struct stat st;
    if((stat(fname.c_str(),&st) != 0) || (!S_ISREG(st.st_mode))) {
      /* There is no top-level .gacl file - create one */
      if(makedirs(basepath) != 0) {
        logger.msg(Arc::WARNING, "Mount point %s creation failed.", basepath);
      } else {
        unlink(fname.c_str());
        int h = ::open(fname.c_str(),O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
        if(h == -1) {
          logger.msg(Arc::WARNING, "Creation of top level ACL %s failed.", fname);
        } else {
          ::write(h,default_top_gacl,strlen(default_top_gacl));
          ::close(h);
        };
      };
    };
  };
}

GACLPlugin::~GACLPlugin(void) {
  for(;subst;) {
    GACLnamevalue *s = (GACLnamevalue*)subst->next;
    if(subst->name) free(subst->name);
    if(subst->value) free(subst->value);
    free(subst);
    subst=s;
  };
  if(acl) GACLfreeAcl(acl);
}

#define ReportGACLerror(filename,gacl_itself,action) {            \
 error_description="Client has no GACL:";                         \
 error_description+=(action);                                     \
 error_description+=" access to object.";                         \
 std::list<std::string> identities;                                         \
 GACLextractAdmin((filename),identities,gacl_itself);             \
 if(identities.size() == 0) {                                     \
   error_description+=" This object has no administrator.";       \
   error_description+=" Contact administrator of the service.";   \
 } else {                                                         \
   error_description+=" Contact administrator of this object: ";  \
   error_description+=*(identities.begin());                      \
 };                                                               \
 return 1;                                                        \
}

int GACLPlugin::removedir(std::string &name) {
  const char* basename = get_last_name(name.c_str());
  if(strncmp(basename,".gacl-",6) == 0) return 1;
  std::string dirname = basepath+"/"+name;
  GACLperm perm=GACLtestFileAclForVOMS(dirname.c_str(),*user);
  if(!GACLhasWrite(perm)) ReportGACLerror(dirname.c_str(),false,"write");
  struct stat st;
  if(stat(dirname.c_str(),&st) != 0) return 1;
  if(!S_ISDIR(st.st_mode)) return 1;
  DIR* d=::opendir(dirname.c_str());
  if(d == NULL) { return 1; }; /* maybe return ? */
  struct dirent *de;
  for(;;) {
    de=::readdir(d);
    if(de == NULL) break;
    if((!strcmp(de->d_name,".")) ||
       (!strcmp(de->d_name,"..")) ||
       (!strcmp(de->d_name,".gacl"))
    ) continue;
    ::closedir(d);
    return 1;
  };
  ::closedir(d);
  if(::remove((dirname+"/.gacl").c_str()) != 0) return 1;
  if(::remove(dirname.c_str()) != 0) return 1;
  GACLdeleteFileAcl(dirname.c_str());
  return 0;
}

int GACLPlugin::removefile(std::string &name) {
  const char* basename = get_last_name(name.c_str());
  if(strncmp(basename,".gacl-",6) == 0) return 1;
  std::string filename = basepath+"/"+name;
  GACLperm perm=GACLtestFileAclForVOMS(filename.c_str(),*user);
  if(!GACLhasWrite(perm)) ReportGACLerror(filename.c_str(),false,"write");
  struct stat st;
  if(stat(filename.c_str(),&st) != 0) return 1;
  if(!S_ISREG(st.st_mode)) return 1;
  if(::remove(filename.c_str()) != 0) return 1;
  GACLdeleteFileAcl(filename.c_str());
  return 0;
}

int GACLPlugin::makedir(std::string &name) {
  std::string dname=basepath;
  if(makedirs(dname) != 0) { /* can't make mount point */
    logger.msg(Arc::WARNING, "Mount point %s creation failed.", dname);
    return 1; 
  };
  std::string dirname = basepath+"/"+name;
  // maybe it already exists
  struct stat st;
  if(stat(dirname.c_str(),&st) == 0) {
    if(!S_ISDIR(st.st_mode)) return 1;
    return 0;
  };
  // make sure directories allowed to be created
  GACLperm perm=GACLtestFileAclForVOMS(dirname.c_str(),*user);
  if(!GACLhasWrite(perm)) ReportGACLerror(dirname.c_str(),false,"write");
  /* now go through rest of directories */
  std::string::size_type n = 0;
  std::string gname("");
  for(;;) {
    /* default acl */
    if(n >= name.length()) break; // all directories are there
    std::string::size_type nn=name.find('/',n);
    if(nn==std::string::npos) nn=name.length();
    std::string nname = name.substr(n,nn-n);
    // make sure nobody creates directories with names .gacl and .gacl-
    if(strncmp(nname.c_str(),".gacl-",6) == 0) return 1;
    if(strcmp(nname.c_str(),".gacl") == 0) return 1;
    gname=dname+"/.gacl-"+nname;
    dname=dname+"/"+nname;
    n=nn+1;
    if(stat(dname.c_str(),&st) == 0) {
      if(!S_ISDIR(st.st_mode)) return 1;
    } else {
      if(mkdir(dname.c_str(),S_IRWXU) != 0) return 1;
    };
  };
  // make gacls for last directory only
  if(acl) {
    if(!GACLsaveSubstituted(acl,subst,gname.c_str())) {
      if(stat(gname.c_str(),&st) != 0) return 1;
      if(!S_ISREG(st.st_mode)) return 1;
    };
    gname=dname+"/.gacl";
    if(!GACLsaveSubstituted(acl,subst,gname.c_str())) {
      if(stat(gname.c_str(),&st) != 0) return 1;
      if(!S_ISREG(st.st_mode)) return 1;
    };
  };
  return 0;
}

int GACLPlugin::open(const char* name,open_modes mode,unsigned long long int size) {
  logger.msg(Arc::VERBOSE, "plugin(gacl): open: %s", name);
  file_mode=file_access_none;
  std::string filename = basepath+"/"+name;
  std::string dname=name; if(!remove_last_name(dname)) { return 1; };
  const char* basename = get_last_name(name);
  bool acl_request = false;
  if(strncmp(basename,".gacl-",6) == 0) {
    filename=basepath+"/"+dname+"/"+(basename+6);
    acl_request=true;
  } else if(strcmp(basename,".gacl") == 0) {
    filename=basepath+"/"+dname+"/";
    acl_request=true;
  };
  if( mode == GRIDFTP_OPEN_RETRIEVE ) {  /* open for reading */
    if(acl_request) {
      std::string gname = basepath+"/"+name;
      GACLperm perm=GACLtestFileAclForVOMS(gname.c_str(),*user,true);
      if(!GACLhasAdmin(perm)) ReportGACLerror(gname.c_str(),true,"admin");
      int h=::open(gname.c_str(),O_RDONLY);
      if(h==-1) { // return empty ACL (no data) in case no ACL exists
        acl_buf[0]=0; acl_length=0;
        file_mode=file_access_read_acl;
        file_name=filename;
        return 0;   
      };
      int l = 0;
      for(;;) {
        int ll = sizeof(acl_buf)-l;
        if(ll<=0) break;
        ll = ::read(h,acl_buf+l,ll);
        if(ll == -1) { close(h); return 1; };
        if(ll == 0) break;
        l+=ll; 
      };
      close(h);
      if(l>=sizeof(acl_buf)) return 1;
      acl_buf[l]=0; acl_length=l;
      file_mode=file_access_read_acl;
      file_name=filename;
      return 0;   
    };
    GACLperm perm=GACLtestFileAclForVOMS(filename.c_str(),*user);
    if(GACLhasRead(perm)) {
      data_file=::open(filename.c_str(),O_RDONLY);
      if(data_file == -1) return 1;
      file_mode=file_access_read;
      file_name=filename;
      return 0;
    };
    ReportGACLerror(filename.c_str(),false,"read");
  }
  else if( mode == GRIDFTP_OPEN_STORE ) { /* open for writing */
    if(acl_request) {
      std::string gname = basepath+"/"+name;
      GACLperm perm=GACLtestFileAclForVOMS(gname.c_str(),*user,true);
      if(!GACLhasAdmin(perm)) ReportGACLerror(gname.c_str(),true,"admin");
      struct stat st; /* make sure it is file */
      if(stat(filename.c_str(),&st) != 0) {
        if(!S_ISREG(st.st_mode)) return 1;
      };
      memset(acl_buf,0,sizeof(acl_buf));
      file_mode=file_access_write_acl;
      file_name=filename;
      return 0;
    };
    /* first check if file exists */
    struct stat st;
    if(stat(filename.c_str(),&st) == 0) {  /* overwrite */
      if(S_ISREG(st.st_mode)) {
        GACLperm perm=GACLtestFileAclForVOMS(filename.c_str(),*user);
        if(GACLhasWrite(perm)) {
          if(size > 0) {
            struct statfs dst;
            if(statfs((char*)(filename.c_str()),&dst) == 0) {
              if(size > ((dst.f_bfree*dst.f_bsize) + st.st_size)) {
                logger.msg(Arc::ERROR, "Not enough space to store file");
                return 1;
              };
            };
          };
          data_file=::open(filename.c_str(),O_WRONLY);
          if(data_file == -1) return 1;
          file_mode=file_access_overwrite;
          file_name=filename;
          truncate(file_name.c_str(),0);
          return 0;
        };
        ReportGACLerror(filename.c_str(),false,"write");
      }
      else if(S_ISDIR(st.st_mode)) { /* it's a directory */
        return 1;
      };
      return 1;
    }
    else { /* no such object in filesystem */
      GACLperm perm=GACLtestFileAclForVOMS(filename.c_str(),*user);
      if(GACLhasWrite(perm)) {
        if(makedir(dname) != 0) return 1;
        if(size > 0) {
          struct statfs dst;
          if(statfs((char*)(filename.c_str()),&dst) == 0) {
            if(size > (dst.f_bfree*dst.f_bsize)) {
              logger.msg(Arc::ERROR, "Not enough space to store file");
              return 1;
            };
          };
        };
        data_file=::open(filename.c_str(),O_WRONLY | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
        if(data_file == -1) return 1;
        file_name=filename;
        file_mode=file_access_create;
        return 0;
      };
      ReportGACLerror(filename.c_str(),false,"write");
    };
  }
  else {
    logger.msg(Arc::WARNING, "Unknown open mode %s", mode);
    return 1;
  };
  return 1;
}

int GACLPlugin::close(bool eof) {
  error_description="Intenal error on server side.";
  if((file_mode==file_access_read_acl) || (file_mode==file_access_write_acl)) { 
    if(eof) {
      if(file_mode==file_access_write_acl) {
        file_mode=file_access_none;
        std::string::size_type n=file_name.rfind('/');
        if(n==std::string::npos) n=0;
        std::string gname=file_name;
        if(gname.length() == (n+1)) {
          gname+=".gacl";
        } else {
          gname.insert(n+1,".gacl-");
        };
        if(acl_buf[0]==0) { // empty acl - want default one
          if(remove(gname.c_str()) == 0) return 0;
          if(errno == ENOENT) return 0;
          return 1;
        };
        GACLacl* acl = NGACLacquireAcl(acl_buf);
        if(acl == NULL) {
          logger.msg(Arc::ERROR, "Failed to parse GACL");
          error_description="This ACL could not be interpreted.";
          return 1;
        };
        std::list<std::string> identities;
        GACLextractAdmin(acl,identities);
        if(identities.size() == 0) {
          logger.msg(Arc::ERROR, "GACL without </admin> is not allowed");
          error_description="This ACL has no admin access defined.";
          return 1;
        };
        if(!GACLsaveAcl((char*)(gname.c_str()),acl)) {
          logger.msg(Arc::ERROR, "Failed to save GACL");
          GACLfreeAcl(acl);
          return 1;
        };
        GACLfreeAcl(acl);
        return 0;
      };
      file_mode=file_access_none;
      return 0;
    }; 
    file_mode=file_access_none;
    return 0;
  };
  if(data_file != -1) {
    if(eof) {
      ::close(data_file);
      if((file_mode==file_access_create) || 
         (file_mode==file_access_overwrite)) { 
        std::string::size_type n=file_name.rfind('/'); 
        if(n==std::string::npos) n=0;
        if(acl) { // store per-file default acl only if specified
          std::string gname=file_name;
          gname.insert(n+1,".gacl-");
          GACLsaveSubstituted(acl,subst,gname.c_str());
        };
      };
    }
    else {  /* file was not transfered properly */
      if((file_mode==file_access_create) || 
         (file_mode==file_access_overwrite)) { /* destroy file */
        ::close(data_file);
        ::unlink(file_name.c_str());
      };
    };
  };
  file_mode=file_access_none;
  return 0;
}

int GACLPlugin::read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size) {
  if(file_mode==file_access_read_acl) {
    if(offset >= acl_length) { (*size)=0; return 0; };
    int l = (acl_length - offset);
    (*size)=l;
    memcpy(buf,acl_buf+offset,l);
    return 0;
  };
  ssize_t l;
  if(data_file == -1) return 1;
  if(lseek(data_file,offset,SEEK_SET) != offset) {
    (*size)=0; return 0; /* can't read anymore */
  };
  if((l=::read(data_file,buf,(*size))) == -1) {
    logger.msg(Arc::WARNING, "Error while reading file");
    (*size)=0; return 1;
  };
  (*size)=l;
  return 0;
}

int GACLPlugin::write(unsigned char *buf,unsigned long long int offset,unsigned long long int size) {
  if(file_mode==file_access_write_acl) {
    if(offset >= (sizeof(acl_buf)-1)) { return 1; };
    if((offset+size) > (sizeof(acl_buf)-1)) { return 1; };
    memcpy(acl_buf+offset,buf,size);
    return 0;
  };
  ssize_t l;
  size_t ll;
  logger.msg(Arc::VERBOSE, "plugin: write");
  if(data_file == -1) return 1;
  if(lseek(data_file,offset,SEEK_SET) != offset) {
    perror("lseek");
    return 1; /* can't write at that position */
  };
  for(ll=0;ll<size;ll+=l) {
    if((l=::write(data_file,buf+ll,size-ll)) == -1) {
      perror("write");
      return 1;
    };
    if(l==0) logger.msg(Arc::WARNING, "Zero bytes written to file");
  };
  return 0;
}

int GACLPlugin::readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode) {
  std::string dirname = basepath+"/"+name;
  /* check if allowed to read this directory */
  GACLperm perm=GACLtestFileAclForVOMS(dirname.c_str(),*user);
  if(!GACLhasList(perm)) ReportGACLerror(dirname.c_str(),false,"list");
  struct stat st;
  if(stat(dirname.c_str(),&st) != 0) return 1;
  if(S_ISDIR(st.st_mode)) {
    DIR* d=::opendir(dirname.c_str());
    if(d == NULL) { return 1; }; /* maybe return ? */
    struct dirent *de;
    for(;;) {
      de=::readdir(d);
      if(de == NULL) break;
      if((!strcmp(de->d_name,".")) || 
         (!strcmp(de->d_name,"..")) ||
         (!strcmp(de->d_name,".gacl")) ||
         (!strncmp(de->d_name,".gacl-",6))
      ) continue;
      DirEntry dent(true,de->d_name); // treat it as file by default
      bool is_manageable = fill_object_info(dent,dirname,mode);
      if(is_manageable) {
        dir_list.push_back(dent);
      };
    };
    ::closedir(d);
    return 0;
  }
  else if(S_ISREG(st.st_mode)) {
    DirEntry dent(true,"");
    bool is_manageable = fill_object_info(dent,dirname,mode);
    if(is_manageable) {
      dir_list.push_back(dent);
      return -1;
    };
  };
  return 1;
}

/* checkdir is allowed to change dirname to show actual target of cd */
int GACLPlugin::checkdir(std::string &name) {
  std::string dirname = basepath+"/"+name;
  GACLperm perm=GACLtestFileAclForVOMS(dirname.c_str(),*user);
  if(!GACLhasList(perm)) ReportGACLerror(dirname.c_str(),false,"list");
  struct stat st;
  if(stat(dirname.c_str(),&st) != 0) return 1;
  if(!S_ISDIR(st.st_mode)) return 1;
  return 0;
}

int GACLPlugin::checkfile(std::string &name,DirEntry &info,DirEntry::object_info_level mode) {
  const char* basename = get_last_name(name.c_str());
  if(strncmp(basename,".gacl-",6) == 0) { /* hack - any acl already exists */
    DirEntry dent(true,basename);
    info=dent;
    return 0;
  };
  std::string filename = basepath+"/"+name;
  GACLperm perm=GACLtestFileAclForVOMS(filename.c_str(),*user);
  if(!GACLhasList(perm)) ReportGACLerror(filename.c_str(),false,"list");
  DirEntry dent(true,get_last_name(filename.c_str()));
  std::string dirname = filename; remove_last_name(dirname);
  bool is_manageable = fill_object_info(dent,dirname,mode);
  if(!is_manageable) {
    return 1;
  };
  info=dent;
  return 0;
}

bool GACLPlugin::fill_object_info(DirEntry &dent,std::string dirname,
                      DirEntry::object_info_level mode) {
  bool is_manageable = true;
  if(mode != DirEntry::minimal_object_info) {
    std::string ffname = dirname;
    if(dent.name.length() != 0) ffname+="/"+dent.name;
    struct stat st;
    if(stat(ffname.c_str(),&st) != 0) { is_manageable=false; }
    else {
      // TODO: treat special files (not regular) properly. (how?)
      if((!S_ISREG(st.st_mode)) && (!S_ISDIR(st.st_mode))) is_manageable=false;
    };
    if(is_manageable) {
      dent.uid=st.st_uid;
      dent.gid=st.st_gid;
      dent.changed=st.st_ctime;
      dent.modified=st.st_mtime;
      dent.is_file=S_ISREG(st.st_mode);
      dent.size=st.st_size;
      if(mode != DirEntry::basic_object_info) {
        GACLperm perm=GACLtestFileAclForVOMS(ffname.c_str(),*user);
        if(dent.is_file) {
          if(GACLhasWrite(perm)) {
            dent.may_delete=true; dent.may_write=true; dent.may_append=true;
          };
          if(GACLhasRead(perm)) {
            dent.may_read=true;
          };
        } else {
          if(GACLhasWrite(perm)) {
            dent.may_delete=true;
            dent.may_create=true;
            dent.may_mkdir=true;
            dent.may_purge=true;
          };
          if(GACLhasList(perm)) {
            dent.may_chdir=true;
            dent.may_dirlist=true;
          };
        };
      };
    };
  };
  return is_manageable;
}

