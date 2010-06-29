#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

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
  acl_ = GACLloadAcl((char*)filename);
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
    for(cred=entry->firstcred;cred;cred=cred->next) {
      GACLnamevalue *namevalue;
      for(namevalue=cred->firstname;namevalue;namevalue=(GACLnamevalue*)(namevalue->next)) {
        if((namevalue->value) && ((namevalue->value)[0] == '$')) {
          GACLnamevalue* sub;
          for(sub=subst;sub;sub=(GACLnamevalue*)(sub->next)) {
            if(strcmp(sub->name,namevalue->value+1) == 0) {
              char* tmp = strdup(sub->value);
              if(tmp) {
                free(namevalue->value);
                namevalue->value=tmp;
              };
              break;
            };
          };
          if(!sub) { free(namevalue->value); namevalue->value=strdup(""); };
        };
      };
    };
  };
  return 1;
}

extern GACLentry *GACLparseEntry(xmlNodePtr cur);

GACLacl *GACLacquireAcl(const char *str) {
  xmlDocPtr   doc;
  xmlNodePtr  cur;
  GACLacl    *acl;
  GACLentry  *entry;

  doc = xmlParseMemory(str,strlen(str));
  if (doc == NULL) return NULL;

  cur = xmlDocGetRootElement(doc);

  if (xmlStrcmp(cur->name, (const xmlChar *) "gacl"))
    {
      free(doc);
      free(cur);
      return NULL;
    }

  cur = cur->xmlChildrenNode;

  acl = GACLnewAcl();

  while (cur != NULL)
       {
         if(xmlNodeIsText(cur)) { cur=cur->next; continue; };
         entry = GACLparseEntry(cur);
         if (entry == NULL)
           {
             GACLfreeAcl(acl);
             xmlFreeDoc(doc);
             return NULL;
           }

         GACLaddEntry(acl, entry);

         cur=cur->next;
       }

  xmlFreeDoc(doc);
  return acl;
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

/*
GACLperm GACLtestFileAclForSubject(const char *filename,const char* subject) {
  const char* name;
  int path_l;
  char* gname;
  GACLacl* acl;
  struct stat st;
  GACLuser* user;
  GACLperm perm;
  GACLcred* cred;
  const char* gacl_name = ".gacl-";

  if(subject[0] == 0) return GACL_PERM_NONE;
  gname=GACLmakeName(filename);
  if(gname == NULL) return GACL_PERM_NONE;
  if(stat(gname,&st) == 0) {
    if(!S_ISREG(st.st_mode)) { free(gname); return GACL_PERM_NONE; };
    acl=GACLloadAcl(gname);
  } else {
    acl=GACLloadAclForFile((char*)filename);
  };
  free(gname);
  if(acl == NULL) return GACL_PERM_NONE;
  cred=GACLnewCred("person");
  if(cred==NULL) { GACLfreeAcl(acl); return GACL_PERM_NONE; };
  if(!GACLaddToCred(cred,"dn",(char*)subject)) {
    GACLfreeCred(cred); GACLfreeAcl(acl); return GACL_PERM_NONE;
  };
  * GACLnewUser does not create copy of cred. But GACLfreeUser destoyrs cred *
  user=GACLnewUser(cred);
  if(user == NULL) {
    GACLfreeAcl(acl); GACLfreeCred(cred); return GACL_PERM_NONE;
  };
  perm=GACLtestUserAcl(acl,user);
  GACLfreeAcl(acl);
  GACLfreeUser(user);
  return perm;
}
*/
