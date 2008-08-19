#ifndef __ARC_MYSQLWRAPPER_H__
#define __ARC_MYSQLWRAPPER_H__

#include "DBInterface.h"
#include <mysql.h>

namespace Arc {

  class MySQLDatabase : public Database {
  public:
    MySQLDatabase();
    MySQLDatabase(const MySQLDatabase& other);
    virtual ~MySQLDatabase();

    virtual bool connect(std::string& dbname, std::string& server, std::string& user,
                        std::string& password, unsigned int port);

    bool isconnected() const { return is_connected_; }

    virtual void close();
 
    virtual bool  enable_ssl(const std::string keyfile = "", const std::string certfile = "",
                        const std::string cafile = "", const std::string capath = "",
                        const char* cipher = 0);
    virtual bool execute(const std::string sqlstr);
  
    virtual std::string fetch_row(resType* res)const;

    virtual unsigned long* fetch_length(resType* res)const;

  private:
    bool is_connected;
    std::string server_;
    int port_;
    std::string dbname_;
    std::string user_;
    std::string password_;
    bool secured; //whether ssl is used

    MYSQL *mysql;
  };

  class MySQLQuery {
  public:
    MySQLQuery(Database* db);
    MySQLQuery(Database* db, const std::string& sqlstr);
    ~MySQLQuery();
   
    bool execute(const std::string& sqlstr);

    virtual bool execute(const std::string sqlstr);

    virtual std::string fetch_row(resType* res)const;
    
    virtual unsigned long* fetch_length(resType* res)const;

  private:
    Database* db_;
  };

} // namespace Arc

#endif /* __ARC_MYSQLWRAPPER_H__ */
