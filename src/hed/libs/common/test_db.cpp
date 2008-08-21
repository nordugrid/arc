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
  int port = 3306;
  std::string dbname = "voms_myvo";
  std::string user = "root";
  std::string password = "aa1122";
  Arc::MySQLDatabase mydb(server, 3306);

  bool res = false;
  res = mydb.connect(dbname,user,password);
  if(res == false) {std::cerr<<"Can't establish connection to mysql database"<<std::endl; return 0;}

  Arc::MySQLQuery myquery(&mydb);
  std::cout<<"Is connected? "<<mydb.isconnected()<<std::endl;
  std::string querystr = "select * from roles";
  myquery.execute(querystr);

  int num_rows, num_colums;
  num_rows = myquery.get_num_rows();
  num_colums = myquery.get_num_colums();

  std::vector<std::string> strlist;
  strlist = myquery.get_row();

  for(int i = 0; i<strlist.size(); i++) {
    std::cout<<"The value of "<<i<<"th field :"<<strlist[i]<<std::endl;
  }

  std::string str1, str2;
  std::string fieldname = "role";
  str1 = myquery.get_row_field(0, fieldname);  
  fieldname = "rid";
  str2 = myquery.get_row_field(0, fieldname);
  std::cout<<"Number of rows: "<<num_rows<<" Number of colums: "<<num_colums<<std::endl;
  std::cout<<str1<<"  "<<str2<<std::endl; 
 
  return 0;
}

