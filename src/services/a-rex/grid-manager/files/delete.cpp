#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
 
#include <string>

#include <glibmm.h>

#include "../files/info_types.h"
#include "delete.h"

struct FL_p {
  const char* s;
  FL_p* next;
  FL_p* prev;
};

/* return values: 0 - empty, 1 - has files, 2 - failed */
static int delete_all_recur(const std::string &dir_base,
                            const std::string &dir_cur,
                            FL_p **fl_list,bool excl) {
  /* take corresponding members of fl_list */
  FL_p* fl_new = NULL;  /* new list */
  FL_p* fl_cur = (*fl_list); /* pointer in old list */
  int l = dir_cur.length();
  /* extract suitable files from list */
  for(;;) {
    if(fl_cur == NULL) break;
    FL_p* tmp = fl_cur->next;
    if(!strncmp(fl_cur->s,dir_cur.c_str(),l)) {
      if(fl_cur->s[l] == '/') {
        /* remove from list */
        if(fl_cur->prev == NULL) { (*fl_list)=fl_cur->next; }
        else { fl_cur->prev->next=fl_cur->next; };
        if(fl_cur->next == NULL) { }
        else { fl_cur->next->prev=fl_cur->prev; };
        /* add to list */
        fl_cur->prev=NULL; fl_cur->next=fl_new;
        if(fl_new == NULL) { fl_new=fl_cur; }
        else { fl_new->prev = fl_cur; fl_new=fl_cur; };
      };
    };
    fl_cur=tmp;
  };
  /* go through directory and remove files */
  std::string file;
  std::string dir_s = dir_base+dir_cur;
  int files = 0;
  try {
    Glib::Dir dir(dir_s);
    for(;;) {
      file=dir.read_name();
      if(file.empty()) break;
      if(file == ".") continue;
      if(file == "..") continue;
      fl_cur = fl_new;
      for(;;) {
        if(fl_cur == NULL) break;
        if(!strcmp(file.c_str(),(fl_cur->s)+(l+1))) {
          /* do not delete or delete */
          break;
        };
        fl_cur=fl_cur->next;
      };
      if(excl) {
        if(fl_cur == NULL) {
          /* delete */
          struct stat f_st;
          std::string fname=dir_s+'/'+file;
          if(lstat(fname.c_str(),&f_st) != 0) { files++; }
          else if(S_ISDIR(f_st.st_mode)) {
            if(delete_all_recur(dir_base,
                          dir_cur+'/'+file,&fl_new,excl) != 0) {
              files++;
            }
            else {
              if(remove(fname.c_str()) != 0) { files++; };
            };
          }
          else {
            if(remove(fname.c_str()) != 0) { files++; };
          };
        }
        else { files++; };
      }
      else {
        struct stat f_st;
        std::string fname=dir_s+'/'+file;
        if(lstat(fname.c_str(),&f_st) != 0) { files++; }
        else if(S_ISDIR(f_st.st_mode)) {
          if(fl_cur != NULL) { /* MUST delete it */
            FL_p* e = NULL;
            if(delete_all_recur(dir_base,
                          dir_cur+'/'+file,&e,true) != 0) {
              files++;
            }
            else { 
              if(remove(fname.c_str()) != 0) { files++; }; 
            };
          }
          else { /* CAN delete if empty, and maybe files inside */
            if(delete_all_recur(dir_base,
                          dir_cur+'/'+file,&fl_new,excl) != 0) {
              files++;
            }
            else {
              if(remove(fname.c_str()) != 0) { files++; };
            };
          };
        }
        else {
          if(fl_cur != NULL) { /* MUST delete this file */
            if(remove(fname.c_str()) != 0) { files++; };
          }
          else { files++; };
        };
      };
    };
  } catch(Glib::FileError& e) { return 2; };
  if(files) return 1;
  return 0;
}


static int delete_links_recur(const std::string &dir_base,const std::string &dir_cur) {
  std::string file;
  std::string dir_s = dir_base+dir_cur;
  int res = 0;
  try {
    Glib::Dir dir(dir_s);
    for(;;) {
      file=dir.read_name();
      if(file.empty()) break;
      if(file == ".") continue;
      if(file == "..") continue;
      struct stat f_st;
      std::string fname=dir_s+'/'+file;
      if(lstat(fname.c_str(),&f_st) != 0) { res|=1; }
      else if(S_ISDIR(f_st.st_mode)) {
        res|=delete_links_recur(dir_base,dir_cur+'/'+file);
      }
      else {
        if(remove(fname.c_str()) != 0) { res|=1; };
      };
    };
  } catch(Glib::FileError& e) { return 2; };
  return res;
}

int delete_all_links(const std::string &dir_base,const std::list<FileData>&) {
  std::string dir_cur("");
  return delete_links_recur(dir_base,dir_cur);
}

/* filenames should start from / and not to have / at end */
int delete_all_files(const std::string &dir_base,const std::list<FileData> &files,
             bool excl,bool lfn_exs,bool lfn_mis) {
  int n = files.size();
  FL_p* fl_list = NULL;
  if(n != 0) { 
    if((fl_list=(FL_p*)malloc(sizeof(FL_p)*n)) == NULL) { return 2; };
    std::list<FileData>::const_iterator file=files.begin();
//    fl_list[0].s=file->pfn.c_str();
    int i;
    for(i=0;i<n;) {
      if(((lfn_exs) && (file->lfn.find(':') != std::string::npos)) ||
         ((lfn_mis) && (file->lfn.find(':') == std::string::npos))) {
        if(excl) {
          if(file->pfn == "/") { /* keep all requested */
            free(fl_list); return 0;
          };
        };
        fl_list[i].s=file->pfn.c_str();
        if(i) { fl_list[i].prev=fl_list+(i-1); fl_list[i-1].next=fl_list+i; }
        else { fl_list[i].prev=NULL; };
        fl_list[i].next=NULL;
        i++;
      };
      ++file; if(file == files.end()) break;
    };
    if(i==0) { free(fl_list); fl_list=NULL; };
  };
  std::string dir_cur("");
  FL_p* fl_list_tmp = fl_list;
  int res=delete_all_recur(dir_base,dir_cur,&fl_list_tmp,excl);
  if(fl_list) free(fl_list);
  return res;
}

