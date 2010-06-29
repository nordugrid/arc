#include <globus_common.h>
#include <globus_io.h>
#include <globus_ftp_control.h>

#include "fileroot.h"
#include "names.h"
#include "commands.h"
#include "misc/canonical_dir.h"
#include <arc/globusutils/GlobusErrorUtils.h>

#define oilog(int) std::cerr
/* 
  file retrieve callbacks 
*/

void GridFTP_Commands::data_connect_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,unsigned int stripendx,globus_bool_t reused,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
oilog(it->log_id)<<"data_connect_retrieve_callback"<<std::endl;
  globus_thread_blocking_will_block();
  globus_mutex_lock(&(it->data_lock));
  it->time_spent_disc=0;
  it->time_spent_network=0;
  it->last_action_time=time(NULL);
  oilog(it->log_id)<<"Data channel connected (retrieve)\n";
  if(it->check_abort(error)) {
    it->froot.close(false);
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_eof = false;
  /* make buffers */
  // oilog(it->log_id)<<"data_connect_retrieve_callback: allocate_data_buffer"<<std::endl;
  it->compute_data_buffer();
  if(!(it->allocate_data_buffer())) {
    // oilog(it->log_id)<<"data_connect_retrieve_callback: allocate_data_buffer failed"<<std::endl;
    it->froot.close(false);
    it->force_abort(); globus_mutex_unlock(&(it->data_lock)); return;
  };
  /* fill and register all available buffers */
  it->data_callbacks=0;
  it->data_offset=0;
  for(unsigned int i = 0;i<it->data_buffer_num;i++) {
oilog(it->log_id)<<"data_connect_retrieve_callback: check for buffer "<<i<<std::endl;
    if(!((it->data_buffer)[i].data)) continue;
    /* read data from file */
    unsigned long long size = it->data_buffer_size;
    if(it->virt_restrict) {
      if((it->data_offset + size) > it->virt_size)
                             size=it->virt_size-it->data_offset;
    };
    struct timezone tz;
    gettimeofday(&(it->data_buffer[i].time_last),&tz);
    int fres=it->froot.read(it->data_buffer[i].data,
                (it->virt_offset)+(it->data_offset),&size);
    if(fres != 0) {
      oilog(it->log_id)<<"Closing channel (retrieve) due to local read error: <<it->froot.error<<\n";
      it->force_abort();
      it->free_data_buffer();it->froot.close(false);
      globus_mutex_unlock(&(it->data_lock)); return;
    }; 
    if(size == 0) it->data_eof=GLOBUS_TRUE;
    /* register buffer */
    globus_result_t res;
    res=globus_ftp_control_data_write(&(it->handle),
            (globus_byte_t*)(it->data_buffer[i].data),
            size,it->data_offset,it->data_eof,
            &data_retrieve_callback,it);
    it->data_offset+=size;
    if(res != GLOBUS_SUCCESS) {
      oilog(it->log_id)<<"Buffer registration failed"<<std::endl;
      oilog(it->log_id)<<"Globus error: "<<Arc::GlobusResult(res)<<std::endl;
      it->force_abort();
      if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
      globus_mutex_unlock(&(it->data_lock)); return;
    };
    it->data_callbacks++;
    if(it->data_eof == GLOBUS_TRUE) break;
  };
  globus_mutex_unlock(&(it->data_lock)); return;
}

void GridFTP_Commands::data_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof) {
// oilog(it->log_id)<<"data_retrieve_callback"<<std::endl;
  globus_thread_blocking_will_block();
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv,&tz);
  globus_mutex_lock(&(it->data_lock));
  it->last_action_time=time(NULL);
//  oilog(it->log_id)<<"Data channel (retrieve) "<<(int)offset<<" "<<(int)length<<" "<<(int)eof<<std::endl;
  it->data_callbacks--;
  if(it->check_abort(error)) {
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  if(it->data_eof) {
    if(it->data_callbacks==0) {
      oilog(it->log_id)<<"Closing channel (retrieve)\n";
      it->free_data_buffer();
      it->virt_offset=0;
      it->virt_restrict=false;
      it->transfer_mode=false;
      it->froot.close();
      oilog(it->log_id)<<"Time spent waiting for network: "<<it->time_spent_network<<"mkS"<<std::endl;
      oilog(it->log_id)<<"Time spent waiting for disc: "<<it->time_spent_disc<<"mkS"<<std::endl;
      it->send_response("226 Requested file transfer completed\r\n");
    };
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  /* find this buffer */
  unsigned int i;
  for(i = 0;i<it->data_buffer_num;i++) {
    if((it->data_buffer)[i].data == (unsigned char*)buffer) break;
  };
  if(i >= it->data_buffer_num) { /* lost buffer - probably memory corruption */
    oilog(it->log_id)<<"ERROR: data_retrieve_callback: lost buffer"<<std::endl;
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  unsigned long long int time_diff =
     (tv.tv_sec-(it->data_buffer[i].time_last.tv_sec))*1000000+
     (tv.tv_usec-(it->data_buffer[i].time_last.tv_usec));
  it->time_spent_network+=time_diff;
  /* read data from file */
  unsigned long long size = it->data_buffer_size;
  if(it->virt_restrict) {
    if((it->data_offset + size) > it->virt_size) size=it->virt_size-it->data_offset;
  };
#ifdef __USE_PARALLEL_FILE_ACCESS__
  it->data_callbacks++;
  /*
     Unlock while reading file, so to allow others to read in parallel.
     This can speed up read if on striped device/filesystem.
  */
  globus_mutex_unlock(&(it->data_lock));
#endif
  /* NOTE: it->data_lock is not unlocked here because it->froot.write
     is not thread safe */
  struct timeval tv_last;
  gettimeofday(&tv_last,&tz);
  int fres=it->froot.read(it->data_buffer[i].data,
                (it->virt_offset)+(it->data_offset),&size);
#ifdef __USE_PARALLEL_FILE_ACCESS__
  globus_mutex_lock(&(it->data_lock));
  it->data_callbacks--;
#endif
  gettimeofday(&tv,&tz);
  time_diff=(tv.tv_sec-tv_last.tv_sec)*1000000+(tv.tv_usec-tv_last.tv_usec);
  it->time_spent_disc+=time_diff;
  if((fres != 0) || (!it->transfer_mode) || (it->transfer_abort)) {
    oilog(it->log_id)<<"Closing channel (retrieve) due to local read error:"<<it->froot.error<<"\n";
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  }; 
  if(size == 0) it->data_eof=true;
  /* register buffer */
  globus_result_t res;
  res=globus_ftp_control_data_write(&(it->handle),
            (globus_byte_t*)(it->data_buffer[i].data),
            size,it->data_offset,it->data_eof,
            &data_retrieve_callback,it);
  it->data_offset+=size;
  if(res != GLOBUS_SUCCESS) {
    oilog(it->log_id)<<"Buffer registration failed"<<std::endl;
    oilog(it->log_id)<<"Globus error: "<<Arc::GlobusResult(res)<<std::endl;
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_callbacks++;
  globus_mutex_unlock(&(it->data_lock)); return;
}

