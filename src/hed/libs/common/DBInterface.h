#ifndef __ARC_DBINTERFACE_H__
#define __ARC_DBINTERFACE_H__

#include <vector>
#include <string>

namespace Arc {
  ///Interface for calling database client library.
  ///For different database client library, different
  ///class should be implemented by implementing this interface.
  class Database {
  public:
    Database(){};
    Database(std::string& server, int port) {};
    Database(const Database& other) {};
    virtual ~Database() {};

    virtual bool connect(std::string& dbname, std::string& user,
                        std::string& password) = 0;

    virtual bool isconnected() const = 0;

    virtual void close() = 0;
 
    virtual bool  enable_ssl(const std::string keyfile = "", const std::string certfile = "",
                        const std::string cafile = "", const std::string capath = "") = 0;
    
    virtual bool shutdown() = 0;
  };

  class Query {
  public:
    Query(){};
    Query(Database* db){};
    //Query(Database* db, const std::string& sqlstr);
    ~Query(){};

    virtual int get_num_colums() = 0;
    virtual int get_num_rows() = 0;

    virtual bool execute(const std::string& sqlstr) = 0;
    virtual std::vector<std::string> get_row(int row_number)const = 0;
    virtual std::vector<std::string> get_row() const = 0;
    virtual std::string get_row_field(int row_number, std::string& field_name) = 0;
  };

} // namespace Arc

#endif /* __ARC_DBINTERFACE_H__ */
