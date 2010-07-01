#include <globus_common.h>
#include <globus_io.h>
#include <globus_ftp_control.h>

#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/Logger.h>

#include "fileroot.h"
#include "names.h"
#include "commands.h"
#include "misc/canonical_dir.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"GridFTP_Commands");

/* 
  file store callbacks 
*/

void GridFTP_Commands::data_connect_store_callback(void* arg,globus_ftp_control_handle_t *handle,unsigned int stripendx,globus_bool_t reused,globus_object_t *error) {
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  logger.msg(Arc::VERBOSE, "data_connect_store_callback");
  globus_thread_blocking_will_block();
  globus_mutex_lock(&(it->data_lock));
  it->time_spent_disc=0;
  it->time_spent_network=0;
  it->last_action_time=time(NULL);
  logger.msg(Arc::INFO, "Data channel connected (store)");
  if(it->check_abort(error)) {
    it->froot.close(false);
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_eof = false;
  /* make buffers */
  it->compute_data_buffer();
  if(!(it->allocate_data_buffer())) {
    it->froot.close(false);
    it->force_abort(); globus_mutex_unlock(&(it->data_lock)); return;
  };
  /* register all available buffers */
  it->data_callbacks=0;
  globus_result_t res = GLOBUS_SUCCESS;
  for(unsigned int i = 0;i<it->data_buffer_num;i++) {
    if(!((it->data_buffer)[i].data)) continue;
    struct timezone tz;
    gettimeofday(&(it->data_buffer[i].time_last),&tz);
    res=globus_ftp_control_data_read(&(it->handle),
            (globus_byte_t*)(it->data_buffer[i].data),
            it->data_buffer_size,
            &data_store_callback,it);
    if(res==GLOBUS_SUCCESS) { it->data_callbacks++; }
    else { break; };
  };
  if(it->data_callbacks==0) {
    logger.msg(Arc::ERROR, "Failed to register any buffer");
    if(res != GLOBUS_SUCCESS) {
      logger.msg(Arc::ERROR, "Globus error: %s", Arc::GlobusResult(res).str());
    };
    it->froot.close(false);
    it->free_data_buffer();
    it->force_abort(); globus_mutex_unlock(&(it->data_lock)); return;
  };
  globus_mutex_unlock(&(it->data_lock)); return;
}

void GridFTP_Commands::data_store_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof) {
  globus_thread_blocking_will_block();
  GridFTP_Commands *it = (GridFTP_Commands*)arg;
  struct timezone tz;
  struct timeval tv;
  gettimeofday(&tv,&tz);
  globus_mutex_lock(&(it->data_lock));
  it->last_action_time=time(NULL);
  logger.msg(Arc::VERBOSE, "Data channel (store) %i %i %i", (int)offset, (int)length, (int)eof);
  it->data_callbacks--;
  if(it->check_abort(error)) {
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  if(eof) it->data_eof=true;
  /* find this buffer */
  unsigned int i;
  for(i = 0;i<it->data_buffer_num;i++) {
    if((it->data_buffer)[i].data == (unsigned char*)buffer) break;
  };
  if(i >= it->data_buffer_num) { /* lost buffer - probably memory corruption */
    logger.msg(Arc::ERROR, "data_store_callback: lost buffer");
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  unsigned long long int time_diff = 
     (tv.tv_sec-(it->data_buffer[i].time_last.tv_sec))*1000000+
     (tv.tv_usec-(it->data_buffer[i].time_last.tv_usec));
  it->time_spent_network+=time_diff;
  /* write data to file 
     NOTE: it->data_lock is not unlocked here because it->froot.write 
     is not thread safe */
  struct timeval tv_last;
  gettimeofday(&tv_last,&tz);
  if(it->froot.write(it->data_buffer[i].data,
                (it->virt_offset)+offset,length) != 0) {
    logger.msg(Arc::ERROR, "Closing channel (store) due to error: %s", it->froot.error);
    it->force_abort();
    if(it->data_callbacks==0){it->free_data_buffer();it->froot.close(false);};
    globus_mutex_unlock(&(it->data_lock)); return;
  }; 
  gettimeofday(&tv,&tz);
  time_diff=(tv.tv_sec-tv_last.tv_sec)*1000000+(tv.tv_usec-tv_last.tv_usec);
  it->time_spent_disc+=time_diff;
  if(it->data_eof) {
    if(it->data_callbacks==0) {
      logger.msg(Arc::INFO, "Closing channel (store)");
      it->free_data_buffer();
      it->virt_offset=0;
      it->virt_restrict=false;
      it->transfer_mode=false;
      if(it->froot.close() != 0) {
        if(it->froot.error.length()) {
          it->send_response("451 "+it->froot.error+"\r\n");
        } else {
          it->send_response("451 Local error\r\n");
        };
      }
      else {
        logger.msg(Arc::VERBOSE, "Time spent waiting for network: %llu mkS", it->time_spent_network);
        logger.msg(Arc::VERBOSE, "Time spent waiting for disc: %llu mkS", it->time_spent_disc);
        it->send_response("226 Requested file transfer completed\r\n");
      };
    };
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  /* register buffer */
  globus_result_t res;
  gettimeofday(&(it->data_buffer[i].time_last),&tz);
  res=globus_ftp_control_data_read(&(it->handle),
            (globus_byte_t*)(it->data_buffer[i].data),
            it->data_buffer_size,
            &data_store_callback,it);
  if(res != GLOBUS_SUCCESS) {
    /* Because this error can be caused by EOF, abort should not be
       called unless this is last buffer */
    if(it->data_callbacks==0) {
      logger.msg(Arc::ERROR, "Globus error: %s", Arc::GlobusResult(res).str());
      it->force_abort();
      it->free_data_buffer();it->froot.close(false);
    };
    globus_mutex_unlock(&(it->data_lock)); return;
  };
  it->data_callbacks++;
  globus_mutex_unlock(&(it->data_lock)); return;
}

