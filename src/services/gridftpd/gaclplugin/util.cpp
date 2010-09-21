#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include "util.h"

int GACLsaveSubstituted(GACLacl* acl,GACLnamevalue *subst,const char* filename) {
  int h;
  GACLacl* acl_;
  h = open(filename,O_CREAT | O_EXCL | O_WRONLY,S_IRUSR | S_IWUSR);
  if(h == -1) {
    if(errno == EEXIST) return 0;
    return 1;
  };
  close(h);
  if(!GACLsaveAcl((char*)filename,acl)) { remove(filename); return 0; };
  acl_ = NGACLloadAcl((char*)filename);
  if(acl_ == NULL) { remove(filename); GACLfreeAcl(acl_); return 1; };
  if(!GACLsubstitute(acl_,subst)) { remove(filename); GACLfreeAcl(acl_); return 1; };
  if(!GACLsaveAcl((char*)filename,acl_)) { remove(filename); GACLfreeAcl(acl_); return 1; };
  GACLfreeAcl(acl_);
  return 0; 
}

int GACLsubstitute(GACLacl* acl,GACLnamevalue *subst) {
  GACLentry* entry;
  for(entry=(GACLentry*)acl->firstentry;entry;entry=(GACLentry*)entry->next) {
    GACLcred *cred;
    for(cred=entry->firstcred;cred;cred=(GACLcred*)cred->next) {
      std::string auri = cred->auri;
      bool replace = false;
      std::string::size_type pos;
      while((pos = auri.find("%24")) != std::string::npos) {
        replace = true;
        std::string::size_type pos2 = pos + 3;
        while((pos2 < auri.size()) && isalnum(auri[pos2])) pos2++;
        GACLnamevalue* sub;
        for(sub=subst;sub;sub=(GACLnamevalue*)(sub->next))
          if(auri.substr(pos + 3, pos2 - pos - 3) == sub->name) {
            auri.replace(pos, pos2 - pos, GACLmildUrlEncode(sub->value));
            break;
          }
        if (!sub)
          auri.erase(pos, pos2 - pos);
      }
      if (replace) {
        free(cred->auri);
        cred->auri = strdup(auri.c_str());
      }
    }
  }
  return 1;
}

char* GACLmakeName(const char* filename) {
  const char* name;
  int path_l;
  char* gname;
  const char* gacl_name = ".gacl-";

  name=strrchr(filename,'/');
  if(name == NULL) { name=filename; } else { name++; };
  if((*name) == 0) gacl_name = ".gacl";
  path_l=name-filename;
  gname=(char*)malloc(path_l+6+strlen(name)+1);
  if(gname == NULL) return NULL;
  memcpy(gname,filename,path_l); gname[path_l]=0;
  strcat(gname,gacl_name);
  strcat(gname,name);
  return gname;
}

int GACLdeleteFileAcl(const char *filename) {
  char* gname;
  struct stat st;

  gname=GACLmakeName(filename);
  if(gname == NULL) return 0;
  if(stat(gname,&st) == 0) {
    if(S_ISREG(st.st_mode)) { 
      unlink(gname);
      free(gname);
      return 1;
    }; 
  };
  free(gname);
  return 0;
}
