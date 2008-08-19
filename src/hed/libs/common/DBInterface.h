#ifndef __ARC_DBINTERFACE_H__
#define __ARC_DBINTERFACE_H__

namespace Arc {
  ///Interface for calling database client library.
  ///For different database client library, different
  ///class should be implemented by implementing this interface.
  class Database {
  public:
    Database();
    Database(const Database& other);
    virtual ~Database();

    virtual bool connect(const char* host, const char* socket_name,
                        unsigned int port, const char* db, const char* user,
                        const char* password);

    bool isconnected() const { return is_connected_; }

    virtual void disconnect();
 
    virtual bool  enable_ssl(const std::string keyfile = "", const std::string certfile = "",
                        const std::string cafile = "", const std::string capath = "",
                        const char* cipher = 0);
  };

  class Query {
  public:
    Query(Database& db);
    Query(Database& db, const std::string& sqlstr);
    ~Query();
    
    virtual bool execute(const std::string& sqlstr);

    virtual std::string fetch_row()const;

    virtual unsigned long* fetch_length()const;

  private:
    Database* db_;
  };

} // namespace Arc

#endif /* __ARC_DBINTERFACE_H__ */
