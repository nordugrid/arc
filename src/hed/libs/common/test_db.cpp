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

  Arc::QueryRowResult strlist;
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

  //Get role, the sql sentence can be put in some independent place, and then we
  //can adapt to different database schema without changing the code itself;
  std::string role = "'VO-Admin'";
  std::string userid = "1";
  querystr = "SELECT groups.dn, role FROM groups, m  LEFT JOIN roles ON roles.rid = m.rid WHERE groups.gid = m.gid AND roles.role =" + role + "AND m.userid =" + userid;
  myquery.execute(querystr);
  Arc::QueryArrayResult strarray;
  num_rows = myquery.get_num_rows();
  std::cout<<"Get "<<num_rows<<" rows"<<std::endl;
  for(int i = 0; i<num_rows; i++) {
    strlist = myquery.get_row();
    strarray.push_back(strlist);
  }


  querystr = "SELECT groups.dn, role FROM groups, m  LEFT JOIN roles ON roles.rid = m.rid WHERE groups.gid = m.gid AND roles.role = ? AND m.userid = ?";
  Arc::QueryArrayResult strarray1;
  Arc::MySQLQuery myquery1(&mydb);
  std::vector<std::string> args;
  args.push_back(role);
  args.push_back(userid);
  myquery1.get_array(querystr, strarray1, args);
  std::cout<<"Get an result array with "<<strarray1.size()<<" rows"<<std::endl;
 
  return 0;
}

