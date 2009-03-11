#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <globus_openssl.h>

#include <arc/globusutils/GSSCredential.h>
#include <arc/globusutils/GlobusWorkarounds.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/MCCLoader.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>

#include "MCCGSI.h"
#include "PayloadGSIStream.h"


namespace Arc {

  Logger MCC_GSI_Service::logger(MCC::logger, "GSI Service");

  Logger MCC_GSI_Client::logger(MCC::logger, "GSI Client");

  static bool proxy_initialized = false;

  // This function tries to activate Globus OpenSSL module
  // and keep it active forever because on deacivation Globus
  // destroys global structures of OpenSSL.
  static void globus_openldap_lock(ModuleManager& mm) {
    static bool done = false;
    if(done) return;
    // Increasing globus module counter so it is never deactivated
    globus_module_activate(GLOBUS_OPENSSL_MODULE);
    // Making sure this plugin is never unloaded
    // TODO: This is hack - probably proper solution would be 
    // to decouple Globus libraries from plugin.
    std::string path = mm.findLocation("mccgsi");
    // Let's hope nothing bad will happen. We can't
    // influence that anyway.
    if(!path.empty()) new Glib::Module(path,Glib::ModuleFlags(0));
  }

  static Plugin* get_mcc_service(PluginArgument* arg) {
    MCCPluginArgument* mccarg =
            arg?dynamic_cast<MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new MCC_GSI_Service(*(Arc::Config*)(*mccarg),
               *(PluginsFactory*)(*(Arc::ChainContext*)(*mccarg)));
  }

  static Plugin* get_mcc_client(PluginArgument* arg) {
    MCCPluginArgument* mccarg =
            arg?dynamic_cast<MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new MCC_GSI_Client(*(Arc::Config*)(*mccarg),
               *(PluginsFactory*)(*(Arc::ChainContext*)(*mccarg)));
  }

  class MCC_GSI_Context
    : public MessageContextElement {
  public:
    MCC_GSI_Context(const std::string& proxyPath,
		    const std::string& certificatePath,
		    const std::string& keyPath,
		    Logger& logger);
    ~MCC_GSI_Context();
    MCC_Status process(MCCInterface* next, Message &inmsg, Message &outmsg);
    operator bool() {
      return (ctx != GSS_C_NO_CONTEXT);
    }
  private:
    gss_ctx_id_t ctx;
    GSSCredential cred;
    gss_name_t client;
    OM_uint32 ret_flags;
    gss_OID oid;
    OM_uint32 time_req;
    gss_cred_id_t delegated_cred;
    bool completed;
    Logger& logger;
  };

  MCC_GSI_Context::MCC_GSI_Context(const std::string& proxyPath,
				   const std::string& certificatePath,
				   const std::string& keyPath,
				   Logger& logger)
    : ctx(GSS_C_NO_CONTEXT),
      cred(proxyPath, certificatePath, keyPath),
      client(GSS_C_NO_NAME),
      oid(GSS_C_NO_OID),
      delegated_cred(GSS_C_NO_CREDENTIAL),
      completed(false),
      logger(logger) {}

  MCC_GSI_Context::~MCC_GSI_Context() {
    if (ctx != GSS_C_NO_CONTEXT) {
      OM_uint32 majstat, minstat;
      majstat = gss_delete_sec_context(&minstat, &ctx, GSS_C_NO_BUFFER);
      ctx = GSS_C_NO_CONTEXT;
    }
  }

  MCC_Status MCC_GSI_Context::process(MCCInterface* next,
				      Message& inmsg, Message& outmsg) {

    if (!inmsg.Payload())
      return MCC_Status();

    PayloadStreamInterface *inpayload =
      dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());

    int pos = 0;
    char readbuf[5];
    while (5 > pos) {
      int len = 5 - pos;
      inpayload->Get(&readbuf[pos], len);
      pos += len;
    }
    //TODO: for different types (GSI, Globus SSL, TLS/SSL3, SSL2) of communication
    //request from client side, differently process the header of received data
    //and the sent data correspondingly.

    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;

    recv_tok.length = (unsigned char)readbuf[3] * 256 +
		      (unsigned char)readbuf[4] + 5;
    recv_tok.value = malloc(recv_tok.length);
    memcpy(recv_tok.value, readbuf, 5);

    logger.msg(DEBUG, "Recieved token length: %i", recv_tok.length);

    while (recv_tok.length > pos) {
      int len = recv_tok.length - pos;
      inpayload->Get(&((char*)recv_tok.value)[pos], len);
      pos += len;
    }

    OM_uint32 majstat, minstat;

    if (!completed) {

      majstat = gss_accept_sec_context(&minstat,
				       &ctx,
				       cred,
				       &recv_tok,
				       GSS_C_NO_CHANNEL_BINDINGS,
				       &client,
				       &oid,
				       &send_tok,
				       &ret_flags,
				       &time_req,
				       &delegated_cred);
      if(GSS_ERROR(majstat)) {
        logger.msg(ERROR, "GSS accept security context failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
        return MCC_Status();
      }

      logger.msg(INFO, "GSS accept security context: %i/%i", majstat, minstat);

      logger.msg(DEBUG, "Returned token length: %i", send_tok.length);

      PayloadRaw *outpayload = new PayloadRaw;
      if (send_tok.length > 0)
	outpayload->Insert((const char*)send_tok.value, 0, send_tok.length);
      outmsg.Payload(outpayload);

      if ((majstat & GSS_C_SUPPLEMENTARY_MASK) != GSS_S_CONTINUE_NEEDED)
	completed = true;
    }
    else {

      majstat = gss_unwrap(&minstat,
			   ctx,
			   &recv_tok,
			   &send_tok,
			   NULL,
			   GSS_C_QOP_DEFAULT);
      if(GSS_ERROR(majstat)) {
        logger.msg(ERROR, "GSS unwrap failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
        return MCC_Status();
      }

      logger.msg(INFO, "GSS unwrap: %i/%i", majstat, minstat);

      logger.msg(DEBUG, "Sent token length: %i", send_tok.length);

      PayloadRaw payload;
      payload.Insert((const char*)send_tok.value, 0, send_tok.length);

      Message nextinmsg = inmsg;
      nextinmsg.Payload(&payload);
      Message nextoutmsg = outmsg;
      nextoutmsg.Payload(NULL);

      MCC_Status ret = next->process(nextinmsg, nextoutmsg);

      outmsg = nextoutmsg;

      PayloadStreamInterface* outpayload =
	dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());

      outmsg.Payload(new PayloadGSIStream(outpayload, ctx, logger, false));
    }

    majstat = gss_release_buffer(&minstat, &send_tok);
    majstat = gss_release_buffer(&minstat, &recv_tok);

    return MCC_Status(STATUS_OK);
  }

  MCC_GSI_Service::MCC_GSI_Service(Config& cfg,ModuleManager& mm)
    : MCC(&cfg) {
    globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE);
    globus_openldap_lock(mm);
    if(!proxy_initialized)
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    proxyPath = (std::string)cfg["ProxyPath"];
    certificatePath = (std::string)cfg["CertificatePath"];
    keyPath = (std::string)cfg["KeyPath"];
  }

  MCC_GSI_Service::~MCC_GSI_Service() {
    globus_module_deactivate(GLOBUS_GSI_GSSAPI_MODULE);
  }

  MCC_Status MCC_GSI_Service::process(Message& inmsg, Message& outmsg) {

    MessageContextElement *msgctx = (*inmsg.Context())["gsi.service"];
    MCC_GSI_Context *gsictx = NULL;
    if (msgctx)
      gsictx = dynamic_cast<MCC_GSI_Context*>(msgctx);
    if (!gsictx) {
      gsictx = new MCC_GSI_Context(proxyPath, certificatePath, keyPath, logger);
      inmsg.Context()->Add("gsi.service", gsictx);
    }

    if (*gsictx)
      if (!ProcessSecHandlers(inmsg, "incoming")) {
	logger.msg(ERROR,
		   "Security check failed in GSI MCC for incoming message");
	return MCC_Status();
      }

    return gsictx->process(MCC::Next(), inmsg, outmsg);

    if (!ProcessSecHandlers(outmsg, "outgoinging")) {
      logger.msg(ERROR,
		 "Security check failed in GSI MCC for outgoing message");
      return MCC_Status();
    }
  }

  MCC_GSI_Client::MCC_GSI_Client(Config& cfg,ModuleManager& mm)
    : MCC(&cfg),
      ctx(GSS_C_NO_CONTEXT) {
    globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE);
    globus_openldap_lock(mm);
    if(!proxy_initialized)
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    proxyPath = (std::string)cfg["ProxyPath"];
    certificatePath = (std::string)cfg["CertificatePath"];
    keyPath = (std::string)cfg["KeyPath"];
  }

  MCC_GSI_Client::~MCC_GSI_Client() {
    if (ctx != GSS_C_NO_CONTEXT) {
      OM_uint32 majstat, minstat;
      majstat = gss_delete_sec_context(&minstat, &ctx, GSS_C_NO_BUFFER);
      ctx = GSS_C_NO_CONTEXT;
    }
    globus_module_deactivate(GLOBUS_GSI_GSSAPI_MODULE);
  }

  MCC_Status MCC_GSI_Client::process(Message& inmsg, Message& outmsg) {

    if (ctx == GSS_C_NO_CONTEXT) {
      MCC_Status status = InitContext();
      if (!status)
	return status;
    }

    if (!inmsg.Payload())
      return MCC_Status();

    PayloadRawInterface *inpayload =
      dynamic_cast<PayloadRawInterface*>(inmsg.Payload());

    if (!ProcessSecHandlers(inmsg, "outgoing")) {
      logger.msg(ERROR,
		 "Security check failed in GSI MCC for outgoing message");
      return MCC_Status();
    }

    PayloadRaw gsipayload;
    int size = 0;

    for (int n = 0; inpayload->Buffer(n); ++n) {

      gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;
      gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;

      recv_tok.value = inpayload->Buffer(n);
      recv_tok.length = inpayload->BufferSize(n);

      logger.msg(DEBUG, "Recieved token length: %i", recv_tok.length);

      OM_uint32 majstat, minstat;

      majstat = gss_wrap(&minstat,
			 ctx,
			 0,
			 GSS_C_QOP_DEFAULT,
			 &recv_tok,
			 NULL,
			 &send_tok);
      if(GSS_ERROR(majstat)) {
        logger.msg(ERROR, "GSS wrap failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
        return MCC_Status();
      }

      logger.msg(INFO, "GSS wrap: %i/%i", majstat, minstat);

      logger.msg(DEBUG, "Sent token length: %i", send_tok.length);

      gsipayload.Insert((const char*)send_tok.value, size, send_tok.length);
      size += send_tok.length;
    }

    Message nextinmsg = inmsg;
    nextinmsg.Payload(&gsipayload);
    Message nextoutmsg = outmsg;
    nextoutmsg.Payload(NULL);

    MCCInterface *next = MCC::Next();
    if (!next)
      return MCC_Status();
    MCC_Status ret = next->process(nextinmsg, nextoutmsg);

    if (!ProcessSecHandlers(outmsg, "incoming")) {
      logger.msg(ERROR,
		 "Security check failed in GSI MCC for incoming message");
      return MCC_Status();
    }

    PayloadStreamInterface* payload =
      dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());

    outmsg.Payload(new PayloadGSIStream(payload, ctx, logger, true));

    return MCC_Status(STATUS_OK);
  }

  void MCC_GSI_Client::Next(MCCInterface *next, const std::string& label) {
    if (label.empty())
      if (ctx != GSS_C_NO_CONTEXT) {
	OM_uint32 majstat, minstat;
	majstat = gss_delete_sec_context(&minstat, &ctx, GSS_C_NO_BUFFER);
	ctx = GSS_C_NO_CONTEXT;
      }
    MCC::Next(next, label);
  }

  MCC_Status MCC_GSI_Client::InitContext() {

    // Send empty payload in order to get access to message attributes
    MessageAttributes reqattr;
    MessageAttributes repattr;
    Message reqmsg;
    Message repmsg;
    MessageContext context;

    reqmsg.Attributes(&reqattr);
    reqmsg.Context(&context);
    repmsg.Attributes(&repattr);
    repmsg.Context(&context);

    PayloadRaw request;
    reqmsg.Payload(&request);

    MCC_Status status = MCC::Next()->process(reqmsg, repmsg);

    std::string remoteip = repmsg.Attributes()->get("TCP:REMOTEHOST");

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, remoteip.c_str(), &sa.sin_addr);

    char host[NI_MAXHOST];
    getnameinfo((sockaddr*)&sa, sizeof(sa), host, NI_MAXHOST, NULL, 0,
		NI_NAMEREQD);

    OM_uint32 majstat, minstat;

    GSSCredential cred(proxyPath, certificatePath, keyPath);

    gss_name_t target_name = GSS_C_NO_NAME;

    gss_buffer_desc send_tok = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc recv_tok = GSS_C_EMPTY_BUFFER;

    OM_uint32 ret_flags;

    std::string hostname = "host@";
    hostname += host;

    gss_buffer_desc namebuf = GSS_C_EMPTY_BUFFER;
    namebuf.value = (void*)hostname.c_str();
    namebuf.length = hostname.size();

    majstat = gss_import_name(&minstat, &namebuf, GSS_C_NT_HOSTBASED_SERVICE,
			      &target_name);
    if(GSS_ERROR(majstat)) {
      logger.msg(ERROR, "GSS import name failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
      return MCC_Status();
    }

    do {
      majstat = gss_init_sec_context(&minstat,
				     cred,
				     &ctx,
				     target_name,
				     GSS_C_NO_OID,
				     GSS_C_CONF_FLAG | GSS_C_MUTUAL_FLAG |
				     GSS_C_INTEG_FLAG, // | GSS_C_DELEG_FLAG,
				     0,
				     NULL,
				     &recv_tok,
				     NULL,
				     &send_tok,
				     &ret_flags,
				     NULL);
      if(GSS_ERROR(majstat)) {
        logger.msg(ERROR, "GSS init security context failed: %i/%i%s", majstat, minstat, GSSCredential::ErrorStr(majstat, minstat));
        return MCC_Status();
      }

      logger.msg(INFO, "GSS init security context: %i/%i", majstat, minstat);

      logger.msg(DEBUG, "Sent token length: %i", send_tok.length);

      MessageAttributes reqattr;
      MessageAttributes repattr;
      Message reqmsg;
      Message repmsg;
      MessageContext context;

      reqmsg.Attributes(&reqattr);
      reqmsg.Context(&context);
      repmsg.Attributes(&repattr);
      repmsg.Context(&context);

      PayloadRaw request;
      if (send_tok.length > 0)
        request.Insert((const char*)send_tok.value, 0, send_tok.length);
      reqmsg.Payload(&request);

      MCC_Status status = MCC::Next()->process(reqmsg, repmsg);

      if ((majstat & GSS_C_SUPPLEMENTARY_MASK) == GSS_S_CONTINUE_NEEDED) {

	if (!repmsg.Payload()) {
	  logger.msg(ERROR, "No payload during GSI context initialisation");
	  return MCC_Status();
	}

	Arc::PayloadStreamInterface *response =
          dynamic_cast<Arc::PayloadStreamInterface*>(repmsg.Payload());

	int pos = 0;
        char readbuf[5];
	while (5 > pos) {
	  int len = 5 - pos;
	  response->Get(&readbuf[pos], len);
	  pos += len;
	}

        if (readbuf[0] >= 20 && readbuf[0] <= 23)  logger.msg(DEBUG,"Transfer protocol is TLS or SSL3");
        else if (readbuf[0] ==26)  logger.msg(DEBUG,"Transfer protocol is GLOBUS SSL");
        else if (readbuf[0] & 0x80 && readbuf[0] <= 23) logger.msg(DEBUG,"Transfer protocol is SSL2");
        else logger.msg(DEBUG,"Transfer protocol is GSI");

	recv_tok.length = (unsigned char)readbuf[3] * 256 +
			  (unsigned char)readbuf[4] + 5;
        recv_tok.value = malloc(recv_tok.length);
	memcpy(recv_tok.value, readbuf, 5);

	logger.msg(DEBUG, "Recieved token length: %i", recv_tok.length);

        while (recv_tok.length > pos) {
          int len = recv_tok.length - pos;
          response->Get(&((char*)recv_tok.value)[pos], len);
          pos += len;
        }
      }
    }
    while ((majstat & GSS_C_SUPPLEMENTARY_MASK) == GSS_S_CONTINUE_NEEDED);

    majstat = gss_release_buffer(&minstat, &send_tok);
    majstat = gss_release_buffer(&minstat, &recv_tok);

    return MCC_Status(STATUS_OK);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "gsi.service", "HED:MCC", 0, &Arc::get_mcc_service },
  { "gsi.client",  "HED:MCC", 0, &Arc::get_mcc_client },
  { NULL, NULL, 0, NULL }
};
