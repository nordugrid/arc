
// Functions in arcproxy_proxy.cpp

// Create simple temporary proxy
void create_tmp_proxy(std::string& proxy, Arc::Credential& signer);

// Create proxy with all bells and whistles as specified in arguments
void create_proxy(std::string& proxy,
    Arc::Credential& signer,
    const std::string& proxy_policy,
    const Arc::Time& proxy_start, const Arc::Period& proxy_period,
    const std::string& vomsacseq,
    int keybits,
    const std::string& signing_algorithm);

// Store content of proxy
void write_proxy_file(const std::string& path, const std::string& content);

// Delete proxy file
void remove_proxy_file(const std::string& path);

// Delete certificate file
void remove_cert_file(const std::string& path);



// Functions in arcproxy_voms.cpp

// Create simple temporary proxy
// Collect VOMS AC from configured Voms servers
bool contact_voms_servers(std::map<std::string,std::list<std::string> >& vomscmdlist,
    std::list<std::string>& orderlist, std::string& vomses_path,
    bool use_gsi_comm, bool use_http_comm, const std::string& voms_period,
    Arc::UserConfig& usercfg, Arc::Logger& logger, const std::string& tmp_proxy_path, std::string& vomsacseq);



// Functions in arcproxy_myproxy.cpp

// Communicate with MyProxy server
bool contact_myproxy_server(const std::string& myproxy_server, const std::string& myproxy_command,
    const std::string& myproxy_user_name, bool use_empty_passphrase, const std::string& myproxy_period,
    const std::string& retrievable_by_cert, Arc::Time& proxy_start, Arc::Period& proxy_period,
    std::list<std::string>& vomslist, std::string& vomses_path, const std::string& proxy_path,
    Arc::UserConfig& usercfg, Arc::Logger& logger);



