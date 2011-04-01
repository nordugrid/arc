namespace Arc {

// Workaround for Globus adding own proxy object. It is not needed anymore.
bool GlobusRecoverProxyOpenSSL(void);

// Initializes OpenSSL indices used in Globus. This solves problem
// of race condition in Globus and bug which is triggered by that 
// race condition.
bool GlobusPrepareGSSAPI(void);

} // namespace Arc

