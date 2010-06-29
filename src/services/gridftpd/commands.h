#include <string>
#include <list>

class GridFTP_Commands_timeout;


class GridFTP_Commands {
  typedef enum data_connect_type_e {
    GRIDFTP_CONNECT_NONE,
    GRIDFTP_CONNECT_PORT,
    GRIDFTP_CONNECT_PASV
  } data_connect_type_t;

  friend class GridFTP_Commands_timeout;

  private:
#ifndef __DONT_USE_FORK__
    class close_semaphor_t {
      public:
        close_semaphor_t(void);
        ~close_semaphor_t(void);
    };
    close_semaphor_t close_semaphor;
#endif
    int log_id;
    unsigned int firewall[4];
    time_t last_action_time;
    globus_ftp_control_handle_t handle;
    globus_mutex_t response_lock;
    globus_cond_t  response_cond;
    int response_done;
    globus_mutex_t abort_lock;
    globus_cond_t  abort_cond;
    int data_done;
    data_connect_type_t data_conn_type;
    globus_ftp_control_dcau_t data_dcau;
    globus_ftp_control_tcpbuffer_t tcp_buffer;
    gss_cred_id_t delegated_cred;
    unsigned long long int file_size;
    FileRoot froot;
    /* flag to mark server is transfering data right now */
    bool transfer_mode;
    /* flag to mark transfer abort was requested by client side */
    bool transfer_abort;
    /* lock used in data transfer callbacks */
    globus_mutex_t data_lock;
    /* flag to mark eof was set during data transfer(receive) or
       any other reason to stop registering new buffers */
    bool data_eof;
    /* number of buffers registered so far for data transfer */
    unsigned int data_buf_count;
    /* store array of data buffers here */
    typedef struct {
      unsigned char* data;
      unsigned long long int used;
      struct timeval time_last;
    } data_buffer_t;
    data_buffer_t* data_buffer; 
    /* size of every buffer - should it be equal to PBSZ ? */
    unsigned long long int data_buffer_size;
    unsigned int data_buffer_num;
    unsigned int data_callbacks;
    /* keeps offset in file for reading */
    unsigned long long data_offset;
    unsigned long long virt_offset;
    unsigned long long virt_size;
    bool virt_restrict;

    /* statistics */
    unsigned long long int time_spent_disc;
    unsigned long long int time_spent_network;

    void compute_data_buffer(void);
    bool allocate_data_buffer(void);
    void free_data_buffer(void);
    int send_response(const std::string& response) { return send_response(response.c_str()); };
    int send_response(const char* response);
    int wait_response(void);
    static void close_callback(void *arg,globus_ftp_control_handle_t *handle,globus_object_t *error, globus_ftp_control_response_t *ftp_response);
    static void response_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error);
    static void abort_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error);
    bool check_abort(globus_object_t *error);
    void make_abort(bool already_locked = false,bool wait_abort = true);
    void force_abort(void);
    static void accepted_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error);
    static void commands_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,union globus_ftp_control_command_u *command);
    static void authenticate_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_ftp_control_auth_info_t *result);
    time_t last_action(void) const { return last_action_time; };

  public:
    GridFTP_Commands(int n = 0,unsigned int* firewall = NULL);

    ~GridFTP_Commands(void);



#ifndef __DONT_USE_FORK__
    static int new_connection_callback(void* arg,int server_handle);
#else
    static void new_connection_callback(void* arg,globus_ftp_control_server_t *server_handle,globus_object_t *error);
#endif
    static void data_connect_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,unsigned int stripendx,globus_bool_t reused,globus_object_t *error); 
    static void data_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof);
    static void data_connect_store_callback(void* arg,globus_ftp_control_handle_t *handle,unsigned int stripendx,globus_bool_t reused,globus_object_t *error); 
    static void data_store_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof);
    static void list_connect_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,unsigned int stripendx,globus_bool_t reused,globus_object_t *error); 
    static void list_retrieve_callback(void* arg,globus_ftp_control_handle_t *handle,globus_object_t *error,globus_byte_t *buffer,globus_size_t length,globus_off_t offset,globus_bool_t eof);

    std::list<DirEntry> dir_list;
    std::list<DirEntry>::iterator dir_list_pointer;
    std::string list_name_prefix;
    globus_off_t list_offset;
    typedef enum {
      list_list_mode,
      list_nlst_mode,
      list_mlsd_mode
    } list_mode_t;
    list_mode_t list_mode;
};

class GridFTP_Commands_timeout {
  private:
    globus_thread_t timer_thread;
    std::list<GridFTP_Commands*> cmds;
    globus_mutex_t lock;
    globus_cond_t  cond;
    globus_cond_t  exit_cond;
    bool           cond_flag;
    bool           exit_cond_flag;
    static void* timer_func(void* arg);
  public:
    GridFTP_Commands_timeout(void);
    ~GridFTP_Commands_timeout(void);
    void add(GridFTP_Commands& cmd);
    void remove(const GridFTP_Commands& cmd);
};

