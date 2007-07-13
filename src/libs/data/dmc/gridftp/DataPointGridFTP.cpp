#include "DataPointGridFTP.h"
#include "DMCGridFTP.h"

#include "globus_ftp_client.h"

static void callback (void* arg, globus_ftp_client_handle_t* h,
		      globus_object_t* error) {
  bool* done = (bool*)arg;
  *done = true;
}

namespace Arc {

  DataPointGridFTP::DataPointGridFTP(DMCGridFTP* dmc, const URL& url) :
    DataPoint(url), dmc(dmc) {}

  unsigned long long int DataPointGridFTP::GetSize() {
    if (size == -1) {
      globus_ftp_client_handle_t h;
      globus_ftp_client_handle_init(&h, NULL);
      globus_off_t gsize = -1;
      bool done = false;
      globus_ftp_client_size(&h, url.str().c_str(), NULL, &gsize,
			     &callback, &done);
      while(!done) usleep(10000);
      size = gsize;
      globus_ftp_client_handle_destroy(&h);
    }
    return size;
  }

} // namespace Arc
