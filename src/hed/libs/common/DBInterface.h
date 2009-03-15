// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DBINTERFACE_H__
#define __ARC_DBINTERFACE_H__

#include <vector>
#include <string>

namespace Arc {
  ///Interface for calling database client library.
  /**For different types of database client library, different
     classes should be implemented by implementing this interface.
   */
  class Database {
  public:
    /**Default constructor*/
    Database() {}
    /**Constructor which uses the server's name(or IP address)
       and port as parametes*/
    Database(std::string& server, int port) {}
    /**Copy constructor*/
    Database(const Database& other) {}
    /**Deconstructor*/
    virtual ~Database() {}

    /**Do connection with database server
       @param dbname   The database name which will be used.
       @param user     The username which will be used to access database.
       @param password The password which will be used to access database.
     */
    virtual bool connect(std::string& dbname, std::string& user,
                         std::string& password) = 0;
    /**Get the connection status*/
    virtual bool isconnected() const = 0;
    /**Close the connection with database server*/
    virtual void close() = 0;
    /**Enable ssl communication for the connection
       @param keyfile  The location of key file.
       @param certfile The location of certificate file.
       @param cafile   The location of ca file.
       @param capath   The location of ca directory
     */
    virtual bool enable_ssl(const std::string keyfile = "", const std::string certfile = "",
                            const std::string cafile = "", const std::string capath = "") = 0;
    /**Ask database server to shutdown*/
    virtual bool shutdown() = 0;
  };

  typedef std::vector<std::vector<std::string> > QueryArrayResult;
  typedef std::vector<std::string> QueryRowResult;

  class Query {
  public:
    /**Default constructor*/
    Query() {}
    /**Constructor
       @param db  The database object which will be used by Query class to get the database connection */
    Query(Database *db) {}
    //Query(Database* db, const std::string& sqlstr);
    /**Deconstructor*/
    virtual ~Query() {}

    /**Get the colum number in the query result*/
    virtual int get_num_colums() = 0;
    /**Get the row number in the query result*/
    virtual int get_num_rows() = 0;

    /**Execute the query
       @param sqlstr  The sql sentence used to query*/
    virtual bool execute(const std::string& sqlstr) = 0;
    /**Get the value of one row in the query result
       @param row_number  The number of the row
       @return A vector includes all the values in the row*/
    virtual QueryRowResult get_row(int row_number) const = 0;
    /**Get the value of one row in the query result, the row number will be automatically
       increased each time the method is called*/
    virtual QueryRowResult get_row() const = 0;
    /**Get the value of one specific field in one specific row
       @param row_number  The row number inside the query result
       @param field_name  The field name for the value which will be return
       @return   The value of the specified filed in the specified row
     */
    virtual std::string get_row_field(int row_number, std::string& field_name) = 0;
    /** Query the database by using some parameters into sql sentence
        e.g. "select table.value from table where table.name = ?"
       @param sqlstr    The sql sentence with some parameters marked with "?".
       @param result    The result in an array which includes all of the value in query result.
       @param arguments The argument list which should exactely correspond with the parametes in sql sentence.
     */
    virtual bool get_array(std::string& sqlstr, QueryArrayResult& result, std::vector<std::string>& arguments) = 0;
  };

} // namespace Arc

#endif /* __ARC_DBINTERFACE_H__ */
