#include <globus_ftp_control.h>

#include <glibmm/miscutils.h>

#include <arc/Thread.h>
#include <arc/data/DMC.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataCache.h>
#include <arc/data/URLMap.h>

#include "SubmitterARC0.h"

struct cbarg {
  Arc::SimpleCondition cond;
  std::string response;
  bool data;
  bool ctrl;
};


static void ControlCallback(void *arg,
			    globus_ftp_control_handle_t*,
			    globus_object_t*,
			    globus_ftp_control_response_t *response) {
  cbarg *cb = (cbarg *)arg;
  if (response && response->response_buffer)
    cb->response.assign((const char *)response->response_buffer,
			response->response_length);
  else
    cb->response.clear();
  cb->ctrl = true;
  cb->cond.signal();
}


static void ConnectCallback(void *arg,
			    globus_ftp_control_handle_t*,
			    unsigned int,
			    globus_bool_t,
			    globus_object_t*) {
  cbarg *cb = (cbarg *)arg;
  cb->data = true;
  cb->cond.signal();
}


static void ReadWriteCallback(void *arg,
			      globus_ftp_control_handle_t*,
			      globus_object_t*,
			      globus_byte_t*,
			      globus_size_t,
			      globus_off_t,
			      globus_bool_t) {
  cbarg *cb = (cbarg *)arg;
  cb->data = true;
  cb->cond.signal();
}


namespace Arc {

  SubmitterARC0::SubmitterARC0(Config *cfg)
    : Submitter(cfg) {
    globus_module_activate(GLOBUS_FTP_CONTROL_MODULE);
  }

  SubmitterARC0::~SubmitterARC0() {
    globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
  }

  ACC *SubmitterARC0::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC0(cfg);
  }

  std::pair<URL, URL> SubmitterARC0::Submit(Arc::JobDescription& jobdesc) {
    cbarg cb;

    globus_ftp_control_handle_t control_handle;
    globus_ftp_control_handle_init(&control_handle);

    cb.ctrl = false;
    globus_ftp_control_connect(&control_handle,
			       const_cast<char *>(SubmissionEndpoint.Host().c_str()),
			       SubmissionEndpoint.Port(), &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    // should do some clever thing here to integrate ARC1 security framework
    globus_ftp_control_auth_info_t auth;
    globus_ftp_control_auth_info_init(&auth, GSS_C_NO_CREDENTIAL, GLOBUS_TRUE,
				      const_cast<char*>("ftp"),
				      const_cast<char*>("user@"),
				      GLOBUS_NULL, GLOBUS_NULL);

    cb.ctrl = false;
    globus_ftp_control_authenticate(&control_handle, &auth, GLOBUS_TRUE,
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, ("CWD " +
				    SubmissionEndpoint.Path()).c_str(),
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, "CWD new",
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    std::string::size_type pos1, pos2;
    pos2 = cb.response.rfind('"');
    pos1 = cb.response.rfind('/', pos2 - 1);

    std::string jobnumber = cb.response.substr(pos1 + 1, pos2 - pos1 - 1);

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, "DCAU N",
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, "TYPE I",
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, "PASV",
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    pos1 = cb.response.find('(');
    pos2 = cb.response.find(')', pos1 + 1);

    globus_ftp_control_host_port_t passive_addr;
    passive_addr.port = 0;
    unsigned short port_low, port_high;
    if (sscanf(cb.response.substr(pos1 + 1, pos2 - pos1 - 1).c_str(),
	       "%i,%i,%i,%i,%hu,%hu",
	       &passive_addr.host[0],
	       &passive_addr.host[1],
	       &passive_addr.host[2],
	       &passive_addr.host[3],
	       &port_high,
	       &port_low) == 6)
      passive_addr.port = 256 * port_high + port_low;

    globus_ftp_control_local_port(&control_handle, &passive_addr);
    globus_ftp_control_local_type(&control_handle,
				  GLOBUS_FTP_CONTROL_TYPE_IMAGE, 0);
    globus_ftp_control_send_command(&control_handle, "STOR job",
				    &ControlCallback, &cb);

    cb.data = false;
    cb.ctrl = false;
    globus_ftp_control_data_connect_write(&control_handle,
					  &ConnectCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();
    while (!cb.data)
      cb.cond.wait();

    cb.data = false;
    cb.ctrl = false;

    std::string jobdescstring;

    //client should add some stuff to the xrsl here
    //length of files, checksum etc ...

    jobdesc.getProduct(jobdescstring, "XRSL");

    globus_ftp_control_data_write(&control_handle,
				  (globus_byte_t *)jobdescstring.c_str(),
				  jobdescstring.size(),
				  0,
				  GLOBUS_TRUE,
				  &ReadWriteCallback,
				  &cb);
    while (!cb.data)
      cb.cond.wait();
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_quit(&control_handle, &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    globus_ftp_control_handle_destroy(&control_handle);

    URL jobid(SubmissionEndpoint);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    //prepare contact url for information about this job
    //ldap://grid.tsl.uu.se:2135/nordugrid-cluster-name=grid.tsl.uu.se,Mds-Vo-name=local,o=grid
    InfoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" + jobid.str() + ")");
    InfoEndpoint.ChangeLDAPScope(URL::subtree);

    //Upload local input files.
    


    return std::make_pair(jobid, InfoEndpoint);

  }
  
  static void progress(FILE *o, const char *, unsigned int,
		       unsigned long long int all, unsigned long long int max,
		       double, double) {
    static int rs = 0;
    const char rs_[4] = {'|', '/', '-', '\\'};
    if (max) {
      fprintf(o, "\r|");
      unsigned int l = (74 * all + 37) / max;
      if (l > 74) l = 74;
      unsigned int i = 0;
      for (; i < l; i++) fprintf(o, "=");
      fprintf(o, "%c", rs_[rs++]);
      if (rs > 3) rs = 0;
      for (; i < 74; i++) fprintf(o, " ");
      fprintf(o, "|\r");
      fflush(o);
      return;
    }
    fprintf(o, "\r%llu kB                    \r", all / 1024);
  }
  
  void SubmitterARC0::putFiles(const std::vector< std::pair< std::string, std::string> >& fileList, std::string jobid){
    
    // Create mover
    Arc::DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(false);
    mover.verbose(true);
    mover.set_progress_indicator(&progress);
    
    // Create cache
    Arc::User cache_user;
    std::string cache_path2;
    std::string cache_data_path;
    std::string id = "<ngcp>";
    Arc::DataCache cache(cache_path2, cache_data_path, "", id, cache_user);
    std::vector< std::pair< std::string, std::string > >::const_iterator file;
    
    // Loop over files and upload
    for (file = fileList.begin(); file != fileList.end(); file++) {
      std::string src = Glib::build_filename(Glib::get_current_dir(), (*file).first);
      std::string dst = Glib::build_filename(jobid, (*file).second);
      std::cout << src << " -> " << dst << std::endl;
      Arc::DataHandle source(src);
      Arc::DataHandle destination(dst);
      
      std::string failure;
      int timeout = 300;
      if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(), 0, 0, 0, timeout, failure)) {
	if (!failure.empty()) std::cerr << "File moving was not succeeded: " << failure << std::endl;
	else std::cerr << "File moving was not succeeded." << std::endl;
      } 
    }
  } // putFiles()

} // namespace Arc
