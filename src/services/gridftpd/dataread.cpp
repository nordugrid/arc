#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <globus_common.h>
#include <globus_io.h>
#include <globus_ftp_control.h>

#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/Logger.h>

#include "fileroot.h"
#include "names.h"
#include "commands.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"GridFTP_Commands");

/* 
  file retrieve callbacks 
*/

void GridFTP_Commands::data_connect_retrieve_callback(void* arg,globus_ftp_control_handle_t*,unsigned int /* stripendx */,globus_bool_t /* reused */,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  logger.msg(Arc::VERBOSE, "data_connect_retrieve_callback");
  globus_thread_blocking_will_block();
  globus_mutex_lock(&(it->data_lock));
  it->time_spent_disc=0;
  it->time_spent_network=0;
  it->last_action_time=time(NULL);
  logger.msg(Arc::VERBOSE, "Data channel connected (retrieve)");
  if(it->check_abort(error)) {
    it->froot.close(false);
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_eof = false;
  /* make buffers */
  logger.msg(Arc::VERBOSE, "data_connect_retrieve_callback: allocate_data_buffer");
  it->compute_data_buffer();
  if(!(it->allocate_data_buffer())) {
    logger.msg(Arc::ERROR, "data_connect_retrieve_callback: allocate_data_buffer failed");
    it->froot.close(false);
    it->force_abort(); globus_mutex_unlock(&(it->data_lock)); return;
  };
  /* fill and register all available buffers */
  it->data_callbacks=0;
  it->data_offset=0;
  for(unsigned int i = 0;i<it->data_buffer_num;i++) {
    logger.msg(Arc::VERBOSE, "data_connect_retrieve_callback: check for buffer %u", i);
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
      logger.msg(Arc::ERROR, "Closing channel (retrieve) due to local read error: %s", it->froot.error);
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
      logger.msg(Arc::ERROR, "Buffer registration failed");
      logger.msg(Arc::ERROR, "Globus error: %s", Arc::GlobusResult(res).str());
      it->force_abort();
      if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
      globus_mutex_unlock(&(it->data_lock)); return;
    };
    it->data_callbacks++;
    if(it->data_eof == GLOBUS_TRUE) break;
  };
  globus_mutex_unlock(&(it->data_lock)); return;
}

void GridFTP_Commands::data_retrieve_callback(void* arg,globus_ftp_control_handle_t*,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof) {
  logger.msg(Arc::VERBOSE, "data_retrieve_callback");
  globus_thread_blocking_will_block();
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv,&tz);
  globus_mutex_lock(&(it->data_lock));
  it->last_action_time=time(NULL);
  logger.msg(Arc::VERBOSE, "Data channel (retrieve) %i %i %i", (int)offset, (int)length, (int)eof);
  it->data_callbacks--;
  if(it->check_abort(error)) {
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  if(it->data_eof) {
    if(it->data_callbacks==0) {
      logger.msg(Arc::VERBOSE, "Closing channel (retrieve)");
      it->free_data_buffer();
      it->virt_offset=0;
      it->virt_restrict=false;
      it->transfer_mode=false;
      it->froot.close();
      logger.msg(Arc::VERBOSE, "Time spent waiting for network: %.3f ms", (float)(it->time_spent_network/1000.0));
      logger.msg(Arc::VERBOSE, "Time spent waiting for disc: %.3f ms", (float)(it->time_spent_disc/1000.0));
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
    logger.msg(Arc::ERROR, "data_retrieve_callback: lost buffer");
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
    logger.msg(Arc::ERROR, "Closing channel (retrieve) due to local read error: %s", it->froot.error);
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
    logger.msg(Arc::ERROR, "Buffer registration failed");
    logger.msg(Arc::ERROR, "Globus error: %s", Arc::GlobusResult(res).str());
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_callbacks++;
  globus_mutex_unlock(&(it->data_lock)); return;
}

