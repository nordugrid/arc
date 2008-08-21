#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "MysqlWrapper.h"

namespace Arc {
MySQLDatabase::MySQLDatabase(std::string& server, int port): server_(server), port_(port), 
        mysql(NULL), is_connected(false) 
{
}

MySQLDatabase::MySQLDatabase(const MySQLDatabase& other) : is_connected(false) {
  if(other.isconnected()) {
    if(isconnected()) close();
    is_connected = mysql_real_connect(mysql, other.server_.c_str(), other.user_.c_str(),
                        other.password_.c_str(), other.dbname_.c_str(), other.port_, NULL, 0);
  }
  else is_connected = false;
}

MySQLDatabase::~MySQLDatabase() {
  if(isconnected()) close();
}

bool MySQLDatabase::connect(std::string& dbname, std::string& user, std::string& password) {
  mysql = mysql_init(NULL);
  if (!mysql_real_connect(mysql, server_.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port_, NULL, 0)) {
    std::cerr<<"Database connection failed"<<std::endl;
    return false;
  }
  return true;
}

void MySQLDatabase::close() {
  if(mysql)
    mysql_close(mysql);
  mysql = NULL;
  is_connected = false;
}

bool MySQLDatabase::enable_ssl(const std::string keyfile, const std::string certfile,
                     const std::string cafile, const std::string capath) {
  return mysql_ssl_set(mysql, keyfile.c_str(), certfile.c_str(), cafile.c_str(), capath.c_str(), NULL) == 0;

}

bool MySQLDatabase::shutdown() {
  return mysql_shutdown(mysql, SHUTDOWN_DEFAULT);

}


MySQLQuery::MySQLQuery(Database* db) : db_(NULL) {
  MySQLDatabase* database = NULL;
  database = dynamic_cast<MySQLDatabase*>(db);
//  if(database == NULL) std::cerr<<"The parameter of constructor should be MySQLDatabase type"<<std::endl; 
//  db_ = new MySQLDatabase(*database);
  db_ = database;
}

MySQLQuery::~MySQLQuery() {
//  if(db_!=NULL) delete db_;
  db_ == NULL;
}  

bool MySQLQuery::execute(const std::string& sqlstr) {
  std::cout<<"Query: "<<sqlstr<<std::endl;
  if(db_->mysql == NULL) std::cerr<<"mysql object is NULL"<<std::endl;
  if(mysql_query(db_->mysql, sqlstr.c_str())) {
    std::cerr<<"Database query failed"<<std::endl;
    return false;
  }
  res = mysql_store_result(db_->mysql);
  if(res) {
    num_colums = 0;
    MYSQL_FIELD* field = NULL;
    while(true) {
      field = mysql_fetch_field(res);
      if(field) {
        if(field->name) field_names[field->name] = num_colums;
        num_colums++;  
      }
      else break;
    }
    num_rows = mysql_num_rows(res);
  }
  return true;
}

int MySQLQuery::get_num_colums() {
  return num_colums;
}

int MySQLQuery::get_num_rows() {
  return num_rows;
}
    
std::vector<std::string> MySQLQuery::get_row(int row_number)const {
  mysql_data_seek(res,row_number);
  return get_row();
}

std::vector<std::string> MySQLQuery::get_row()const {
  MYSQL_ROW row = mysql_fetch_row(res);
  std::vector<std::string> row_value;
  if(row == NULL) return row_value;

  std::string field;
  for(int i=0; i<num_colums; i++) {
    field = row[i];
    row_value.push_back(field);
  }
  return row_value;
}

std::string MySQLQuery::get_row_field(int row_number, std::string& field_name) {
  int field_number = field_names[field_name];
  std::vector<std::string> row_value = get_row(row_number);
  return row_value[field_number];
}

} // namespace Arc
