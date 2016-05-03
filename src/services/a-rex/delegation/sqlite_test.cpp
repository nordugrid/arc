#include <iostream>
#include <sqlite3.h>

  struct FindCallbackCountArg {
    int count;
    FindCallbackCountArg():count(0) {};
  };

  static int FindCallbackCount(void* arg, int colnum, char** texts, char** names) {
    ((FindCallbackCountArg*)arg)->count += 1;
    for(int n = 0; n < colnum; ++n) {
      std::cout<< (names[n]?names[n]:"NULL");
      std::cout<< " : ";
      std::cout<< (texts[n]?texts[n]:"NULL");
      std::cout<< std::endl;
    }
    return 0;
  }

int main(int argc, char* argv[]) {
  char* errstr = NULL;
  sqlite3* db_ = NULL;
  if(sqlite3_open_v2("testdb", &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
    std::cerr<<"open"<<std::endl;
    return -1;
  };
  if(sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS rec(id, owner, uid, meta, UNIQUE(id, owner), UNIQUE(uid))", NULL, NULL, &errstr) != SQLITE_OK) {
    std::cerr<<"CREATE: "<<errstr<<std::endl;
    return -1;
  };
  std::string id("123456789");
  std::string owner("owner123");
  std::string uid("abcdefghij");
  std::string metas("metasmanymetas");
  {
    std::string sqlcmd = "INSERT INTO rec(id, owner, uid, meta) "
                         "VALUES ('"+id+"', '"+owner+"', '"+uid+"', '"+metas+"')";
    std::cout<<"-> "<<sqlcmd<<std::endl;
    FindCallbackCountArg arg;
    errstr = NULL;
    if(sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackCount, &arg, &errstr) != SQLITE_OK) {
      std::cerr<<"INSERT: "<<errstr<<std::endl;
      return -1;
    };
  };
  {
    std::string sqlcmd = "DELETE FROM rec WHERE (uid = '"+uid+"')";
    std::cout<<"-> "<<sqlcmd<<std::endl;
    FindCallbackCountArg arg;
    errstr = NULL;
    if(sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackCount, &arg, &errstr) != SQLITE_OK) {
      std::cerr<<"DELETE: "<<errstr<<std::endl;
      return -1;
    };
  };
  (void)sqlite3_close(db_); // todo: handle error
  return 0;
}

