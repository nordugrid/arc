#define GRIDFTP_PLUGIN

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <grp.h>
#include <sys/vfs.h>

#include "fileplugin.h"
#include "../conf/conf.h"
#include "../userspec.h"
#include "../names.h"
#include "../misc.h"
#include "../misc/canonical_dir.h"

#ifndef HAVE_STRERROR_R
static char * strerror_r (int errnum, char * buf, size_t buflen) {
  char * estring = strerror (errnum);
  strncpy (buf, estring, buflen);
  return buf;
}
#endif


static bool parse_id(std::string s,int &id,int base = 10) {
  if((s.length()==1) && (s[0] == '*')) { id=-1; return true; }
  else {
    char* end; id=strtoul(s.c_str(),&end,base);
    if(*end) { return false; };
  };
  return true;
}

static bool parse_owner_rights(std::string &rest,int &uid,int &gid,int &orbits,int &andbits) {
  struct passwd pw_;
  struct group gr_;
  struct passwd *pw;
  struct group *gr;
  char buf[BUFSIZ];
  std::string owner = gridftpd::config_next_arg(rest);
  std::string acc_rights = gridftpd::config_next_arg(rest);
  if(acc_rights.length() == 0) {
    olog << "Warning: can't parse access rights in configuration line" << std::endl;
    return false;
  };
  std::string::size_type n;
  n=owner.find(':');
  if(n == std::string::npos) {
    olog << "Warning: can't parse user:group in configuration line" << std::endl;
    return false;
  };
  if(!parse_id(owner.substr(0,n),uid)) {
    /* not number, must be name */
    getpwnam_r(owner.substr(0,n).c_str(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog << "Warning: can't recognize user in configuration line" << std::endl;
      return false;
    };
    uid=pw->pw_uid;
  };
  if(!parse_id(owner.substr(n+1),gid)) {
    /* not number, must be name */
    getgrnam_r(owner.substr(n+1).c_str(),&gr_,buf,BUFSIZ,&gr);
    if(gr == NULL) {
      olog << "Warning: can't recognize group in configuration line" << std::endl;
      return false;
    };
    gid=gr->gr_gid;
  };
  n=acc_rights.find(':');
  if(n == std::string::npos) {
    olog << "Warning: can't parse or:and in configuration line" << std::endl;
    return false;
  };
  if((!parse_id(acc_rights.substr(0,n),orbits,8)) ||
     (!parse_id(acc_rights.substr(0,n),andbits,8))) {
    olog << "Warning: can't parse or:and in configuration line" << std::endl;
    return false;
  };
  return true;
}

DirectFilePlugin::DirectFilePlugin(std::istream &cfile,userspec_t &user) {
//  olog << "DirectFilePlugin: constructor" << std::endl;
  data_file=-1;
  uid=user.get_uid();
  gid=user.get_gid();
  /* read configuration */
  for(;;) {
    std::string rest;
    std::string command=gridftpd::config_read_line(cfile,rest);
    if(command.length() == 0) break; /* end of file - should not be here */
    if(command == "dir") {
      DirectAccess::diraccess_t laccess; 
      /* filling default access */
      laccess.read=false; laccess.dirlist=false;
      laccess.cd=false; laccess.creat=false; 
      laccess.overwrite=false; laccess.append=false;
      laccess.del=false; laccess.mkdir=false;
      laccess.access=DirectAccess::local_unix_access;
      bool parsed_line = false;
      rest=subst_user_spec(rest,&user);
      std::string dir = gridftpd::config_next_arg(rest);
      if(dir.length() == 0) {
        olog << "Warning: can't parse configuration line" << std::endl;
        continue;
      };
      if(gridftpd::canonical_dir(dir,false) != 0) {
        olog << "Warning: bad directory name: " << dir << std::endl;
        continue;
      };
      for(;;) {
        std::string subcommand = gridftpd::config_next_arg(rest);
        if(subcommand.length() == 0) { parsed_line=true; break; };
        if(subcommand == "read") { laccess.read=true; }
        else if(subcommand == "delete") { laccess.del=true; }
        else if(subcommand == "append") { laccess.append=true; }
        else if(subcommand == "overwrite") { laccess.overwrite=true; }
        else if(subcommand == "cd") { laccess.cd=true; }
        else if(subcommand == "dirlist") { laccess.dirlist=true; }
        else if(subcommand == "create") {
          laccess.creat=true;
          if(!parse_owner_rights(rest,
                 laccess.creat_uid,laccess.creat_gid,
                 laccess.creat_perm_or,laccess.creat_perm_and)) {
             olog << "Warning: can't parse create arguments in configuration line" << std::endl;
             break;
          };
        }
        else if(subcommand == "mkdir") { 
          laccess.mkdir=true;
          if(!parse_owner_rights(rest,
                 laccess.mkdir_uid,laccess.mkdir_gid,
                 laccess.mkdir_perm_or,laccess.mkdir_perm_and)) {
             olog << "Warning: can't parse mkdir arguments in configuration line" << std::endl;
             break;
          }; 
        }
        else if(subcommand == "owner") {
          laccess.access=DirectAccess::local_user_access;
        }
        else if(subcommand == "group") {
          laccess.access=DirectAccess::local_group_access;
        }
        else if(subcommand == "other") {
          laccess.access=DirectAccess::local_other_access;
        }
        else if(subcommand == "nouser") {
          laccess.access=DirectAccess::local_none_access;
        }
        else {
          olog << "Warning: bad subcommand in configuration line: " << subcommand << std::endl;
          continue;
        };
      };
      if(parsed_line) {
        //olog << "DirectFileAccess: adding directory " << dir << std::endl;
        access.push_back(DirectAccess(dir,laccess));
      };
    }
    else if(command == "mount") {
      rest=subst_user_spec(rest,&user);
      mount=gridftpd::config_next_arg(rest);
      if((mount.length() == 0) || (gridftpd::canonical_dir(mount,false) != 0)) {
        olog << "Warning: bad mount directory specified" << std::endl;
      };
      olog << "Mount point " << mount << std::endl;
    }
    else if(command == "end") {
      break; /* end of section */
    }
    else {
      olog << "Warning: unsupported configuration command: " << command << std::endl;
    };
  };
  access.sort(DirectAccess::comp);
//  for(std::list<DirectAccess>::iterator i=access.begin();i!=access.end();++i) {
//    olog << "Access: " << (*i).name << " - " << (*i).access.dirlist << std::endl;
//  };
  file_mode=file_access_none;
}

/* name must be absolute path */
/* make directories out of scope of mount dir */
int makedirs(std::string &name) {
  /* to make it faster - just check if it exists */
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
    if(stat(dname.c_str(),&st) == 0) { /* have intermediate object */
      if(S_ISDIR(st.st_mode)) continue; /* already have - ok */
      return 1; /* can't make directory based on file - not in unix */
    };
    /* no such object - create */
    if(mkdir(dname.c_str(),S_IRWXU | S_IRWXG | S_IRWXO) == 0) continue;
    char* errmsg;
    char errmgsbuf[256];
#ifndef _AIX
    errmsg=strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#else
    errmgsbuf[0]=0; errmsg=errmgsbuf;
    strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#endif
    olog<<"mkdir failed: "<<errmsg<<std::endl;
    return 1; /* directory creation failed */
  };
  return 0;
}

/* make all directories */
int DirectFilePlugin::makedir(std::string &dname) {
  /* first check for mount point */
  std::string mname='/'+mount;
  if(makedirs(mname) != 0) { /* can't make mount point */
    olog << "Warning: mount point " << mname << " creation failed." << std::endl;
    return 1; 
  };
  /* now go through rest of directories */
  std::string::size_type n = 0;
  std::string pdname("");
  std::list<DirectAccess>::iterator i=control_dir(pdname,false);
  if(i==access.end()) return 1; /* no root ? - strange */
  pdname=real_name(pdname);
  int ur=i->unix_rights(pdname,uid,gid);
  if(ur & S_IFREG) return 1; 
  if(!(ur & S_IFDIR)) return 1; 
  for(;;) {
    if(n >= dname.length()) break;
    n=dname.find('/',n); if(n==std::string::npos) n=dname.length();
    std::string fdname=dname.substr(0,n); n++;
    /* remember if parrent directory allows mkdir */
    bool allow_mkdir = i->access.mkdir;
    i=control_dir(fdname,false);
    if(i==access.end()) return 1;
    /* first check maybe it already exists */
    fdname=real_name(fdname);
    int pur = ur;
    ur=i->unix_rights(fdname,uid,gid);
    if(ur & S_IFDIR) continue; /* already exists */
    if(ur & S_IFREG) return 1; /* can't make directory with same name as file */
    /* check if parrent directory allows mkdir */
    if(!allow_mkdir) return -1;
    if(!(pur & S_IWUSR)) return 1;
    /* create directory with proper rights */
    if(i->unix_set(uid,gid) == 0) {
      if(::mkdir(fdname.c_str(),
              i->access.mkdir_perm_or & i->access.mkdir_perm_and) == 0) {
        (void)::chmod(fdname.c_str(),
              i->access.mkdir_perm_or & i->access.mkdir_perm_and);
        i->unix_reset();
        uid_t u = i->access.mkdir_uid;
        gid_t g = i->access.mkdir_gid;
        if(u == ((uid_t)(-1))) u=uid; if(g == ((gid_t)(-1))) g=gid;
        (void)::chown(fdname.c_str(),u,g);
        continue;
      } else {
        i->unix_reset();
      };
    };
    char* errmsg;
    char errmgsbuf[256];
#ifndef _AIX
    errmsg=strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#else
    errmgsbuf[0]=0; errmsg=errmgsbuf;
    strerror_r(errno,errmgsbuf,sizeof(errmgsbuf));
#endif
    olog<<"mkdir failed: "<<errmsg<<std::endl;
    return 1; /* directory creation failed */
  };
  return 0;
}

int DirectFilePlugin::removefile(std::string &name) {
  std::list<DirectAccess>::iterator i=control_dir(name,true);
  if(i==access.end()) return 1;
  if(!(i->access.del)) return 1;
  std::string fname=real_name(name);
  int ur=i->unix_rights(fname,uid,gid);
  if(!(ur & S_IFREG)) return 1;
  if(i->unix_set(uid,gid) != 0) return 1;
  if(::remove(fname.c_str()) != 0) {
    i->unix_reset();
    return 1;
  };
  i->unix_reset();
  return 0;
}

int DirectFilePlugin::removedir(std::string &dname) {
  std::list<DirectAccess>::iterator i=control_dir(dname,true);
  if(i==access.end()) return 1;
  if(!(i->access.del)) return 1;
  std::string fdname=real_name(dname);
  int ur=i->unix_rights(fdname,uid,gid);
  if(!(ur & S_IFDIR)) return 1;
  if(i->unix_set(uid,gid) != 0) return 1;
  if(::remove(fdname.c_str()) != 0) {
    i->unix_reset();
    return 1;
  };
  i->unix_reset();
  return 0;
}

int DirectFilePlugin::open(const char* name,open_modes mode,unsigned long long int size) {
  olog << "plugin: open: " << name << std::endl;
  std::string fname = real_name(name);
  if( mode == GRIDFTP_OPEN_RETRIEVE ) {  /* open for reading */
    std::list<DirectAccess>::iterator i=control_dir(name,true);
    if(i==access.end()) return 1; /* error ? */
    if(i->access.read) {
      int ur=(*i).unix_rights(fname,uid,gid);
      if((ur & S_IFREG) && (ur & S_IRUSR)) {
        /* so open it */
        if(i->unix_set(uid,gid) != 0) return 1;
        data_file=::open(fname.c_str(),O_RDONLY);
        i->unix_reset();
        if(data_file == -1) return 1;
        file_mode=file_access_read;
        file_name=fname;
        return 0;
      };
    };
    return 1;
  }
  else if( mode == GRIDFTP_OPEN_STORE ) { /* open for writing - overwrite */
    std::string dname=name; if(!remove_last_name(dname)) { return 1; };
    std::list<DirectAccess>::iterator i=control_dir(name,true);
    if(i==access.end()) return 1;
    /* first check if file exists */
    int ur=i->unix_rights(fname,uid,gid);
    if(ur & S_IFREG) {
      if(i->access.overwrite) { /* can overwrite */
        if(ur & S_IWUSR) {  /* really can ? */
          if(size > 0) {
            struct statfs dst;
            if(statfs((char*)(fname.c_str()),&dst) == 0) {
              uid_t uid_;
              gid_t gid_;
              unsigned long long size_ = 0;
              time_t changed_,modified_;
              bool is_file_;
              i->unix_info(fname,uid_,gid_,size_,changed_,modified_,is_file_);
              if(size > ((dst.f_bfree*dst.f_bsize) + size_)) {
                olog<<"Not enough space to store file"<<std::endl;
                return 1;
              };
            };
          };
          if(i->unix_set(uid,gid) != 0) return 1;
          data_file=::open(fname.c_str(),O_WRONLY);
          i->unix_reset();
          if(data_file == -1) return 1;
          file_mode=file_access_overwrite;
          file_name=fname;
          truncate(file_name.c_str(),0);
          return 0;
        };
      };
      return 1;
    }
    else if(ur & S_IFDIR) { /* it's a directory */
      return 1;
    }
    else { /* no such object in filesystem */
      if(i->access.creat) { /* allowed to create new file */
        std::string fdname = real_name(dname);
        /* make sure we have directory to store file */
        if(makedir(dname) != 0) return 1; /* problems with underlaying dir */
        int ur=i->unix_rights(fdname,uid,gid);
        if((ur & S_IWUSR) && (ur & S_IFDIR)) {
          if(size > 0) {
            struct statfs dst;
            if(statfs((char*)(fname.c_str()),&dst) == 0) {
              if(size > (dst.f_bfree*dst.f_bsize)) {
                olog<<"Not enough space to store file"<<std::endl;
                return 1;
              };
            };
          };
          if(i->unix_set(uid,gid) != 0) return 1;
          data_file=::open(fname.c_str(),O_WRONLY | O_CREAT | O_EXCL,
                 i->access.creat_perm_or & i->access.creat_perm_and);
          i->unix_reset();
          if(data_file == -1) return 1;
          uid_t u = i->access.creat_uid;
          gid_t g = i->access.creat_gid;
          if(u == ((uid_t)(-1))) u=uid; if(g == ((gid_t)(-1))) g=gid;
// olog<<"open: changing owner for "<<fname<<" "<<u<<" "<<gid<<std::endl;
          (void)chown(fname.c_str(),u,g);
          /* adjust permissions because open uses umask */
          (void)chmod(fname.c_str(),
                i->access.creat_perm_or & i->access.creat_perm_and);
          struct stat st;
          stat(fname.c_str(),&st);
// olog<<"open: owner: "<<st.st_uid<<" "<<st.st_gid<<std::endl;
          file_mode=file_access_create;
          file_name=fname;
          return 0;
        };
      };
    };
    return 1;
  }
  else {
    olog << "Warning: unknown open mode " << mode << std::endl;
    return 1;
  };
}

int DirectFilePlugin::close(bool eof) {
//  olog << "plugin: close\n";
  if(data_file != -1) {
    if(eof) {
      ::close(data_file);
    }
    else {  /* file was not transfered properly */
      if((file_mode==file_access_create) || 
         (file_mode==file_access_overwrite)) { /* destroy file */
        ::close(data_file);
        ::unlink(file_name.c_str());
      };
    };
  };
  return 0;
}

int DirectFilePlugin::open_direct(const char* name,open_modes mode) {
  std::string fname = name;
  if( mode == GRIDFTP_OPEN_RETRIEVE ) {  /* open for reading */
    data_file=::open(fname.c_str(),O_RDONLY);
    if(data_file == -1) return 1;
    file_mode=file_access_read;
    file_name=fname;
    return 0;
  }
  else if( mode == GRIDFTP_OPEN_STORE ) { /* open for writing - overwrite */
    data_file=::open(fname.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
    if(data_file == -1) return 1;
    file_mode=file_access_create;
    file_name=fname;
    truncate(file_name.c_str(),0);
    (void)chown(fname.c_str(),uid,gid);
    (void)chmod(fname.c_str(),S_IRUSR | S_IWUSR);
    return 0;
  }
  else {
    olog << "Warning: unknown open mode " << mode << std::endl;
    return 1;
  };
}

int DirectFilePlugin::read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size) {
  ssize_t l;
//  olog << "plugin: read\n";
  if(data_file == -1) return 1;
  if(lseek(data_file,offset,SEEK_SET) != offset) {
    (*size)=0; return 0; /* can't read anymore */
  };
  if((l=::read(data_file,buf,(*size))) == -1) {
    olog <<"Warning: error while reading file"<<std::endl;
    (*size)=0; return 1;
  };
  (*size)=l;
  return 0;
}

int DirectFilePlugin::write(unsigned char *buf,unsigned long long int offset,unsigned long long int size) {
  ssize_t l;
  size_t ll;
//  olog << "plugin: write\n";
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
    if(l==0) olog << "Warning: zero bytes written to file" << std::endl;
  };
  return 0;
}

int DirectAccess::unix_info(std::string &name,uid_t &uid,gid_t &gid,unsigned long long &size,time_t &changed,time_t &modified,bool &is_file) {
  struct stat fst;
  if(stat(name.c_str(),&fst) != 0) {
    return 1;
  };
  uid=fst.st_uid; gid=fst.st_gid;
  size=fst.st_size; modified=fst.st_mtime; changed=fst.st_ctime;
  if(S_ISREG(fst.st_mode)) { is_file=true; return 0; }
  if(S_ISDIR(fst.st_mode)) { is_file=false; return 0; }
  return 1;
}

int DirectAccess::unix_rights(std::string &name,int uid,int gid) {
  /* first get file/dir's unix rights */
  struct stat fst;
  if(stat(name.c_str(),&fst) != 0) {
    return 0;
  };
  /* file/dir exists */
  if(access.access == local_none_access) {
    /* access controlled entirely by conf file - allow everything */
    return S_IRWXU | (fst.st_mode & (S_IFDIR | S_IFREG)); 
  };
  if((!S_ISREG(fst.st_mode)) && (!S_ISDIR(fst.st_mode))) {
    /* special files are not allowed */
    return 0;
  };
  if(access.access == local_unix_access) {
    /* combine all permissions */
    if(uid == 0) { /* root */
      return S_IRWXU | (fst.st_mode & (S_IFDIR | S_IFREG)); 
    };
    int res = 0;
    if(fst.st_uid == uid) {
      res |= fst.st_mode & S_IRWXU;
    };
    if(fst.st_gid == gid) {
      res |= (fst.st_mode & S_IRWXG) << 3;
    };
    res |= (fst.st_mode & S_IRWXO) << 6;
    return res | (fst.st_mode & (S_IFDIR | S_IFREG));
  }
  else if(access.access == local_user_access) { 
    /* take only user part of files, without checking owner */
    if(fst.st_uid != uid) return 0;
    return (fst.st_mode & S_IRWXU) | (fst.st_mode & (S_IFDIR | S_IFREG));    
  }
  else if(access.access == local_group_access) {
    if(fst.st_gid != gid) return 0;
    return ((fst.st_mode & S_IRWXG) << 3) | (fst.st_mode & (S_IFDIR | S_IFREG)); 
  }
  else if(access.access == local_other_access) {
    return ((fst.st_mode & S_IRWXO) << 6) | (fst.st_mode & (S_IFDIR | S_IFREG));
  };
  /* some bug */
  return 0;
}

int DirectAccess::unix_set(int uid,int gid) {
  if(uid == 0) return 0;
  if(access.access == local_none_access) return 0;
#ifdef HAVE_SETEUID
#ifdef HAVE_SETEGID
  if(gid != 0) {
    if(getegid() != gid) if(setegid(gid) != 0) return -1;
  };
#endif
  if(geteuid() != uid) if(seteuid(uid) != 0) {
#ifdef HAVE_SETEGID
    if(getegid() != getgid()) setegid(getgid());
#endif
    return -1;
  };
#endif
  return 0;
}

void DirectAccess::unix_reset(void) {
    if(geteuid() != getuid()) seteuid(getuid());
    if(getegid() != getgid()) setegid(getgid());
}

std::list<DirectAccess>::iterator DirectFilePlugin::control_dir(const std::string &name,bool indir) {
  return control_dir(name.c_str(),indir);
}

std::list<DirectAccess>::iterator DirectFilePlugin::control_dir(const char* name,bool indir) {
  std::list<DirectAccess>::iterator i;
  for(i=access.begin();i!=access.end();++i) {
    if(i->belongs(name,indir)) break;
  };
// if(i!=access.end()) olog << "control_dir: is in: "<<i->name<<std::endl;
  return i;
}

std::string DirectFilePlugin::real_name(char* name) {
  return real_name(std::string(name));
}

std::string DirectFilePlugin::real_name(std::string name) {
  std::string fname = "";
  if(mount.length() != 0) { fname+='/'+mount; };
  if(name.length() != 0) { fname+='/'+name; };
  return fname;
}

bool DirectFilePlugin::fill_object_info(DirEntry &dent,std::string dirname,int ur,
                      std::list<DirectAccess>::iterator i,
                      DirEntry::object_info_level mode) {
   
  bool is_manageable = true;
  if(mode != DirEntry::minimal_object_info) {
    std::string ffname = dirname;
    if(dent.name.length() != 0) ffname+="/"+dent.name;
    if(i->unix_set(uid,gid) != 0) {
      is_manageable=false;
    } else {
      if(i->unix_info(ffname,
                      dent.uid,dent.gid,dent.size,
                      dent.changed,dent.modified,dent.is_file) != 0) {
        is_manageable=false;
      };
      i->unix_reset();
    };
    if(is_manageable) {
      if(mode != DirEntry::basic_object_info) {
        int fur=i->unix_rights(ffname,uid,gid);
        if(S_IFDIR & fur) { dent.is_file=false; }
        else if(S_IFREG & fur) { dent.is_file=true; }
        else { is_manageable=false; };
        // TODO: treat special files (not regular) properly. (how?)
        if(is_manageable) {
          if(dent.is_file) {
            if(i->access.del && (ur & S_IWUSR)) dent.may_delete=true;
            if(i->access.overwrite &&(fur & S_IWUSR)) dent.may_write=true;
            if(i->access.append && (fur & S_IWUSR)) dent.may_append=true;
            if(i->access.read && (fur & S_IRUSR)) dent.may_read=true;
          }
          else {
            // TODO: this directory can have different rules than i !!!!!!
            if(i->access.del && (ur & S_IWUSR)) dent.may_delete=true;
            if(i->access.creat && (fur & S_IWUSR)) dent.may_create=true;
            if(i->access.mkdir && (fur & S_IWUSR)) dent.may_mkdir=true;
            if(i->access.cd && (fur & S_IXUSR)) dent.may_chdir=true;
            if(i->access.dirlist &&(fur & S_IRUSR)) dent.may_dirlist=true;
            if(i->access.del && (fur & S_IWUSR)) dent.may_purge=true;
          };
        };
      };
    };
  };
  return is_manageable;
}

int DirectFilePlugin::readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode) {
  /* first check if allowed to read this directory */
  std::list<DirectAccess>::iterator i=control_dir(name,false);
  if(i==access.end()) return 1; /* error ? */
  std::string fname = real_name(name);
  if(i->access.dirlist) {
    int ur=i->unix_rights(fname,uid,gid);
    if((ur & S_IFDIR) && (ur & S_IRUSR) && (ur & S_IXUSR)) {
      /* allowed to list in configuration and by unix rights */
      /* following Linux semantics - need r-x for dirlist */
      /* now get real listing */
      if(i->unix_set(uid,gid) != 0) return 1;
      DIR* d=::opendir(fname.c_str());
      i->unix_reset();
      if(d == NULL) { return 1; }; /* maybe return ? */
      struct dirent *de;
      for(;;) {
        de=::readdir(d);
        if(de == NULL) break;
        if((!strcmp(de->d_name,".")) || (!strcmp(de->d_name,".."))) continue;
        DirEntry dent(true,de->d_name); // treat it as file by default
        bool is_manageable = fill_object_info(dent,fname,ur,i,mode);
        if(is_manageable) {
          dir_list.push_back(dent);
        };
      };
      ::closedir(d);
      return 0;
    }
    else if(ur & S_IFREG) {
      DirEntry dent(true,"");
      bool is_manageable = fill_object_info(dent,fname,ur,i,mode);
      if(is_manageable) {
        dir_list.push_back(dent);
        return -1;
      };
    };
  }
  return 1;
}

/* checkdir is allowed to change dirname to show actual target of cd */
int DirectFilePlugin::checkdir(std::string &dirname) {
//  olog << "plugin: checkdir: " << dirname << std::endl;
  std::list<DirectAccess>::iterator i=control_dir(dirname,false);
  if(i==access.end()) return 0; /* error ? */
//  olog << "plugin: checkdir: access: " << (*i).name << std::endl;
  std::string fname = real_name(dirname);
  if(i->access.cd) {
    int ur=(*i).unix_rights(fname,uid,gid);
    if((ur & S_IXUSR) && (ur & S_IFDIR)) {
//  olog << "plugin: checkdir: access: allowed: " << fname << std::endl;
      return 0;
    };
  };
  return 1;
}

int DirectFilePlugin::checkfile(std::string &name,DirEntry &info,DirEntry::object_info_level mode) {
  std::list<DirectAccess>::iterator i=control_dir(name,true);
  if(i==access.end()) return 1; /* error ? */
/* TODO check permissions of higher level directory */
  std::string dname=name; 
  if(!remove_last_name(dname)) {
    /* information about top directory was requested.
       Since this directory is declared it should exist.
       At least virtually */
    info.uid=getuid(); info.gid=getgid();
    info.is_file=false; info.name="";
    return 0; 
  };
  if(!(i->access.dirlist)) { return 1; };
  std::string fdname = real_name(dname);
  int ur=i->unix_rights(fdname,uid,gid);
  if(!((ur & S_IXUSR) && (ur & S_IFDIR)))  { return 1; };
  std::string fname = real_name(name);
  DirEntry dent(true,get_last_name(fname.c_str()));
  bool is_manageable = fill_object_info(dent,fdname,ur,i,mode);
  if(!is_manageable) {
    return 1;
  };
  info=dent;
  return 0;
}

bool DirectAccess::belongs(std::string &name,bool indir) {
  return belongs(name.c_str(),indir);
}

bool DirectAccess::belongs(const char* name,bool indir) {
  int pl=this->name.length();
  if(pl == 0) return true; /* root dir */
  int l=strlen(name);
  if (pl > l) return false;
  if(strncmp(this->name.c_str(),name,pl)) return false;
  if(!indir) if(pl == l) return true;
  if(name[pl] == '/') return true;
  return false;
}
