#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MysqlWrapper.h"
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>

int main(void)
{
  std::string server = "127.0.0.1";
  int port = 3305;
  std::string dbname = "testdb";
  std::string user = "test";
  std::string password = "1234";
  Arc::MySQLDatabase mydb(server, port);

  mydb.connect(dbname,user,password);
  Arc::MySQLQuery myquery(&mydb);

  std::string querystr = "";
  myquery.execute(querystr);
  int num_rows;
  num_rows = myquery.get_num_rows();
  
  return 0;
}

