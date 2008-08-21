#ifndef __ARC_MYSQLWRAPPER_H__
#define __ARC_MYSQLWRAPPER_H__

#include <string>
#include <map>
#include <vector>
#include <mysql.h>
#include "DBInterface.h"

namespace Arc {

  class MySQLDatabase : public Database {
  friend class MySQLQuery;
  public:
    MySQLDatabase(std::string& server, int port);
    MySQLDatabase(const MySQLDatabase& other);
    virtual ~MySQLDatabase();

    virtual bool connect(std::string& dbname, std::string& user,
                        std::string& password);

    virtual bool isconnected() const { return is_connected; }

    virtual void close();
 
    virtual bool  enable_ssl(const std::string keyfile = "", const std::string certfile = "",
                        const std::string cafile = "", const std::string capath = "");
  
    virtual bool shutdown();

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

  class MySQLQuery : public Query {
  public:
    MySQLQuery(Database* db);
    //MySQLQuery(Database* db, const std::string& sqlstr);
    ~MySQLQuery();

    virtual int get_num_colums();
    virtual int get_num_rows();   
    virtual bool execute(const std::string& sqlstr);
    virtual std::vector<std::string> get_row(int row_number)const;
    virtual std::vector<std::string> get_row() const;
    virtual std::string get_row_field(int row_number, std::string& field_name);

  private:
    MySQLDatabase* db_;
    MYSQL_RES *res;
    int num_rows;
    int num_colums;
    std::map<std::string, int> field_names; 
  };

} // namespace Arc

#endif /* __ARC_MYSQLWRAPPER_H__ */
