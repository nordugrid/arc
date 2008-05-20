#include <globus_ftp_control.h>

#include <arc/Thread.h>

#include "SubmitterARC0.h"

struct cbarg {
  Arc::SimpleCondition cond;
  std::string response;
  bool data;
  bool ctrl;
};


static void ControlCallback(void *arg,
			    globus_ftp_control_handle_t *handle,
			    globus_object_t *error,
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
			    globus_ftp_control_handle_t *handle,
			    unsigned int stripe_ndx,
			    globus_bool_t reused,
			    globus_object_t *error) {
  cbarg *cb = (cbarg *)arg;
  cb->data = true;
  cb->cond.signal();
}


static void ReadWriteCallback(void *arg,
			      globus_ftp_control_handle_t *handle,
			      globus_object_t *error,
			      globus_byte_t *buffer,
			      globus_size_t length,
			      globus_off_t offset,
			      globus_bool_t eof) {
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

  std::pair<URL, URL> SubmitterARC0::Submit(const std::string& jobdesc) {
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
				      "ftp", "user@", GLOBUS_NULL, GLOBUS_NULL);

    cb.ctrl = false;
    globus_ftp_control_authenticate(&control_handle, &auth, GLOBUS_TRUE,
				    &ControlCallback, &cb);
    while (!cb.ctrl)
      cb.cond.wait();

    cb.ctrl = false;
    globus_ftp_control_send_command(&control_handle, "CWD jobs",
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
    globus_ftp_control_data_write(&control_handle,
				  (globus_byte_t *)jobdesc.c_str(),
				  jobdesc.size(),
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

    return std::make_pair(jobid, InfoEndpoint);

  }

} // namespace Arc
