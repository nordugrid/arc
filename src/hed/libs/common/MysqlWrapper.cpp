#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MysqlWrapper.h"
namespace Arc {
MySQLDatabase::MySQLDatabase(std::string& server, int port): server_(server), port_(port), 
        mysql(NULL), isconnected(false) 
{
}

MySQLDatabase::MySQLDatabase(const MySQLDatabase& other) : is_connected(false) {
  if(other.isconnected()) {
    if(isconnected()) close();
    is_connected = mysql_real_connect(mysql_, other.server_, other.user_,
                        other.password_, other.dbname_, other.port_, NULL, 0);
  }
  else is_connected = false;
}

MySQLDatabase::~MySQLDatabase() {
  if(isconnected()) close();
}

bool MySQLDatabase::connect(std::string& dbname, std::string& user, std::string& password) {
  mysql = mysql_init(NULL);
  if (!mysql_real_connect(mysql, server.c_str(), user.c_str(), password.c_str(), dbname.c_str, port, NULL, 0)) {
    std::cerr<<"Database connection failed"<<std::endl;
    return false;
  }
  return true;
}

bool MySQLDatabase::isconnected() const { return is_connected_; }

void MySQLDatabase::close() {
  if(mysql)
    mysql_close(mysql);
  mysql = NULL;
  is_connected = false;
}

bool MySQLDatabase::enable_ssl(const std::string keyfile, const std::string certfile,
                     const std::string cafile, const std::string capath) {
  return mysql_ssl_set(&mysql_, keyfile.c_str(), certfile.c_str(), cafile.c_str(), capath.c_str(), NULL) == 0;

}

bool MySQLDatabase::shutdown() {
  return mysql_shutdown(mysql, SHUTDOWN_DEFAUL);
}


MySQLQuery::MySQLQuery(Database* db) : db_(NULL) {
  db_ = new MySQLDatabase(&db);
}

MySQLQuery::~MySQLQuery() {
  if(db_!=NULL) delete db_;
  db_ == NULL:
}  

bool MySQLQuery::execute(const std::string& sqlstr) {
  if(mysql_query(db_->mysql, sqlstr.c_str())) {
    std::cerr<<"Database query failed"<<std::endl;
    return false;
  }
  else return true;
}
    
std::string fetch_row()const {
  mysql_fetch_row(res_);
}
   
    virtual unsigned long* fetch_length(resType* res)const;


} // namespace Arc
