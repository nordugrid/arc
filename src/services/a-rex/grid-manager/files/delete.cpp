//@ #include "../std.h"
//@ 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
//@ 
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
  struct dirent file_;
  struct dirent *file;
  std::string dir_s = dir_base+dir_cur;
  DIR *dir=opendir(dir_s.c_str());
  if(dir == NULL) { return 2; };
  int files = 0;
  for(;;) {
    readdir_r(dir,&file_,&file);
    if(file == NULL) break;
    if(!strcmp(file->d_name,".")) continue;
    if(!strcmp(file->d_name,"..")) continue;
    fl_cur = fl_new;
    for(;;) {
      if(fl_cur == NULL) break;
      if(!strcmp(file->d_name,(fl_cur->s)+(l+1))) {
        /* do not delete or delete */
        break;
      };
      fl_cur=fl_cur->next;
    };
    if(excl) {
      if(fl_cur == NULL) {
        /* delete */
        struct stat f_st;
        std::string fname=dir_s+'/'+file->d_name;
        if(lstat(fname.c_str(),&f_st) != 0) { files++; }
        else if(S_ISDIR(f_st.st_mode)) {
          if(delete_all_recur(dir_base,
                        dir_cur+'/'+file->d_name,&fl_new,excl) != 0) {
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
      std::string fname=dir_s+'/'+file->d_name;
      if(lstat(fname.c_str(),&f_st) != 0) { files++; }
      else if(S_ISDIR(f_st.st_mode)) {
        if(fl_cur != NULL) { /* MUST delete it */
          FL_p* e = NULL;
          if(delete_all_recur(dir_base,
                        dir_cur+'/'+file->d_name,&e,true) != 0) {
            files++;
          }
          else { 
            if(remove(fname.c_str()) != 0) { files++; }; 
          };
        }
        else { /* CAN delete if empty, and maybe files inside */
          if(delete_all_recur(dir_base,
                        dir_cur+'/'+file->d_name,&fl_new,excl) != 0) {
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
  closedir(dir);
  if(files) return 1;
  return 0;
}


static int delete_links_recur(const std::string &dir_base,const std::string &dir_cur) {
  struct dirent file_;
  struct dirent *file;
  std::string dir_s = dir_base+dir_cur;
  DIR *dir=opendir(dir_s.c_str());
  if(dir == NULL) { return 2; };
  int res = 0;
  for(;;) {
    readdir_r(dir,&file_,&file);
    if(file == NULL) break;
    if(!strcmp(file->d_name,".")) continue;
    if(!strcmp(file->d_name,"..")) continue;
    struct stat f_st;
    std::string fname=dir_s+'/'+file->d_name;
    if(lstat(fname.c_str(),&f_st) != 0) { res|=1; }
    else if(S_ISDIR(f_st.st_mode)) {
      res|=delete_links_recur(dir_base,dir_cur+'/'+file->d_name);
    }
    else {
      if(remove(fname.c_str()) != 0) { res|=1; };
    };
  };
  closedir(dir);
  return res;
}

int delete_all_links(const std::string &dir_base,std::list<FileData> &files) {
  std::string dir_cur("");
  return delete_links_recur(dir_base,dir_cur);
};

/* filenames should start from / and not to have / at end */
int delete_all_files(const std::string &dir_base,std::list<FileData> &files,
             bool excl,bool lfn_exs,bool lfn_mis) {
  int n = files.size();
  FL_p* fl_list = NULL;
  if(n != 0) { 
    if((fl_list=(FL_p*)malloc(sizeof(FL_p)*n)) == NULL) { return 2; };
    std::list<FileData>::iterator file=files.begin();
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

