#include <cppunit/extensions/HelperMacros.h>

#include <arc/MysqlWrapper.h>

class MysqlWrapperTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(MysqlWrapperTest);
  CPPUNIT_TEST(TestDB);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestDB();
};

void MysqlWrapperTest::TestDB() {
  /*******************************************************
   * this test only runs if you have a database set up   *
   * with the parameters specified here                  *
   *******************************************************/
  std::string server = "127.0.0.1";
  int port = 3306;
  std::string dbname = "voms_myvo";
  std::string user = "root";
  std::string password = "aa1122";
  Arc::MySQLDatabase mydb(server, port);

  bool res = false;
  res = mydb.connect(dbname, user, password);
  if (res == false)
    return;

  Arc::MySQLQuery myquery(&mydb);
  CPPUNIT_ASSERT(mydb.isconnected());
  std::string querystr = "select * from roles";
  CPPUNIT_ASSERT(myquery.execute(querystr));

  int num_rows, num_columns;
  num_rows = myquery.get_num_rows();
  num_columns = myquery.get_num_colums();

  CPPUNIT_ASSERT(num_rows > 0);
  CPPUNIT_ASSERT(num_columns > 0);
  Arc::QueryRowResult strlist;
  strlist = myquery.get_row();

  CPPUNIT_ASSERT(strlist.size() > 0);
  //for (int i = 0; i < strlist.size(); i++)
    //std::cout << "The value of " << i << "th field :" << strlist[i] << std::endl;

  std::string str1, str2;
  std::string fieldname = "role";
  str1 = myquery.get_row_field(0, fieldname);
  CPPUNIT_ASSERT(str1.length() > 0);
  fieldname = "rid";
  str2 = myquery.get_row_field(0, fieldname);
  CPPUNIT_ASSERT(str2.length() > 0);
  //std::cout << "Number of rows: " << num_rows << " Number of colums: " << num_colums << std::endl;
  //std::cout << str1 << "  " << str2 << std::endl;

  //Get role, the sql sentence can be put in some independent place, and then we
  //can adapt to different database schema without changing the code itself;
  std::string role = "'VO-Admin'";
  std::string userid = "1";
  querystr = "SELECT groups.dn, role FROM groups, m  LEFT JOIN roles ON roles.rid = m.rid WHERE groups.gid = m.gid AND roles.role =" + role + "AND m.userid =" + userid;
  CPPUNIT_ASSERT(myquery.execute(querystr));
  Arc::QueryArrayResult strarray;
  num_rows = myquery.get_num_rows();
  CPPUNIT_ASSERT(num_rows > 0);
  //std::cout << "Get " << num_rows << " rows" << std::endl;
  for (int i = 0; i < num_rows; i++) {
    strlist = myquery.get_row();
    strarray.push_back(strlist);
  }

  querystr = "SELECT groups.dn, role FROM groups, m  LEFT JOIN roles ON roles.rid = m.rid WHERE groups.gid = m.gid AND roles.role = ? AND m.userid = ?";
  Arc::QueryArrayResult strarray1;
  Arc::MySQLQuery myquery1(&mydb);
  std::vector<std::string> args;
  args.push_back(role);
  args.push_back(userid);
  CPPUNIT_ASSERT(myquery1.get_array(querystr, strarray1, args));
  //std::cout << "Get an result array with " << strarray1.size() << " rows" << std::endl;
}

CPPUNIT_TEST_SUITE_REGISTRATION(MysqlWrapperTest);
