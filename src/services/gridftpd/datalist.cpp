#include <string>

#include <globus_common.h>
#include <globus_io.h>
#include <globus_ftp_control.h>

#include <arc/StringConv.h>
#include <arc/Logger.h>

#include "fileroot.h"
#include "names.h"
#include "commands.h"
#include "misc/canonical_dir.h"
#include "misc.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"GridFTP_Commands");

static std::string timetostring_rfc3659(time_t t) {
  struct tm tt;
  struct tm *tp;
  tp=gmtime_r(&t,&tt);
  if(tp == NULL) return "";
  char buf[16]; 
  snprintf(buf,sizeof(buf),"%04u%02u%02u%02u%02u%02u",
           tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,
           tp->tm_hour,tp->tm_min,tp->tm_sec);
  buf[sizeof(buf)-1]=0;
  return std::string(buf);
}

int make_list_string(const DirEntry &entr,GridFTP_Commands::list_mode_t mode,
                     unsigned char* buf,int size,const char *prefix) {
  std::string str;
  switch(mode) {
    case GridFTP_Commands::list_mlsd_mode: {
      if(entr.is_file) { str+="type=file;"; }
      else { str+="type=dir;"; };
      str+="size="+Arc::tostring(entr.size)+";";
      str+="modify="+timetostring_rfc3659(entr.modified)+";";
      str+="perm=";
      if(entr.is_file) {
        if(entr.may_append) str+="a";
        if(entr.may_delete) str+="d";
        if(entr.may_rename) str+="f";
        if(entr.may_read)   str+="r";
        if(entr.may_write)  str+="w";
      }
      else {
        if(entr.may_create) str+="c";
        if(entr.may_delete) str+="d";
        if(entr.may_chdir)  str+="e";
        if(entr.may_rename) str+="f";
        if(entr.may_dirlist)str+="l";
        if(entr.may_purge)  str+="p";
      };
      str+="; ";
      str+=prefix+entr.name;
    }; break;
    case GridFTP_Commands::list_list_mode: {
      if(entr.is_file) {
        str="-------   1 user     group  "+timetostring(entr.modified)+" "+
            Arc::tostring(entr.size,16)+"  "+prefix+entr.name;
      }
      else {
        str="d------   1 user     group  "+timetostring(entr.modified)+" "+
            Arc::tostring(entr.size,16)+"  "+prefix+entr.name;
      };
    }; break;
    case GridFTP_Commands::list_nlst_mode: {
      str=prefix+entr.name;
    }; break;
    default: {
    };
  };
  int len = str.length();
  if(len > (size-3)) { str.resize(size-6); str+="..."; len=size-3; };
  strcpy((char*)buf,str.c_str()); 
  buf[len]='\r'; len++; buf[len]='\n'; len++; buf[len]=0;
  return len; 
}

/* *** list transfer callbacks *** */
void GridFTP_Commands::list_retrieve_callback(void* arg,globus_ftp_control_handle_t*,globus_object_t *error,globus_byte_t* /* buffer */,globus_size_t /* length */,globus_off_t /* offset */,globus_bool_t /* eof */) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  globus_mutex_lock(&(it->data_lock));
  it->last_action_time=time(NULL);
  if(it->check_abort(error)) {
    it->free_data_buffer();
    globus_mutex_unlock(&(it->data_lock));
    return;
  };
  globus_bool_t eodf;
  globus_size_t size;
  if(it->dir_list_pointer == it->dir_list.end()) {
    it->virt_offset=0;
    it->transfer_mode=false;
    it->free_data_buffer();
    logger.msg(Arc::VERBOSE, "Closing channel (list)");
    it->send_response("226 Transfer completed.\r\n");
    globus_mutex_unlock(&(it->data_lock));
    return;
  };
  globus_ftp_control_local_send_eof(&(it->handle),GLOBUS_TRUE);
  ++(it->dir_list_pointer);
  if(it->dir_list_pointer == it->dir_list.end()) {
    size=0; eodf=GLOBUS_TRUE;
  }
  else {
    size=make_list_string(*(it->dir_list_pointer),it->list_mode,
                        it->data_buffer[0].data,it->data_buffer_size,
                        it->list_name_prefix.c_str());
    eodf=GLOBUS_FALSE;
  };
  globus_result_t res;
  res=globus_ftp_control_data_write(&(it->handle),
                   (globus_byte_t*)(it->data_buffer[0].data),
                   size,it->list_offset,eodf,
                   &list_retrieve_callback,it);
  if(res != GLOBUS_SUCCESS) {
    it->free_data_buffer();
    it->force_abort();
    globus_mutex_unlock(&(it->data_lock));
    return;
  };
  globus_mutex_unlock(&(it->data_lock));
}

void GridFTP_Commands::list_connect_retrieve_callback(void* arg,globus_ftp_control_handle_t*,unsigned int /* stripendx */,globus_bool_t /* reused */,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  globus_mutex_lock(&(it->data_lock));
  it->last_action_time=time(NULL);
  if(it->check_abort(error)) {
    globus_mutex_unlock(&(it->data_lock));
    return;
  };
  it->data_buffer_size=4096;
  it->data_buffer_num=1;
  if(!it->allocate_data_buffer()) {
    it->force_abort(); globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->dir_list_pointer=it->dir_list.begin();
  globus_size_t size;
  globus_bool_t eodf;
  if(it->dir_list_pointer == it->dir_list.end()) {
    size=0; eodf=GLOBUS_TRUE;
  }
  else {
    size=make_list_string(*(it->dir_list_pointer),it->list_mode,
                         it->data_buffer[0].data,it->data_buffer_size,
                        it->list_name_prefix.c_str());
    eodf=GLOBUS_FALSE;
  };
  it->list_offset = 0;
  logger.msg(Arc::VERBOSE, "Data channel connected (list)");
  globus_ftp_control_local_send_eof(&(it->handle),GLOBUS_TRUE);
  globus_result_t res;
  res=globus_ftp_control_data_write(&(it->handle),
                   (globus_byte_t*)(it->data_buffer[0].data),
                   size,it->list_offset,eodf,
                   &list_retrieve_callback,it);
  if(res != GLOBUS_SUCCESS) {
    it->free_data_buffer();
    it->force_abort();
    globus_mutex_unlock(&(it->data_lock));
    return;
  };
  it->list_offset+=size;
  globus_mutex_unlock(&(it->data_lock));
}

