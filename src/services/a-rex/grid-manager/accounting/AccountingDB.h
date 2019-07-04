#ifndef ARC_ACCOUNTING_DB_H
#define ARC_ACCOUNTING_DB_H

#include <string>
#include <map>

namespace ARex {
    class GMJob;
    class GMConfig;

    typedef std::map <std::string, unsigned int> name_id_map_t;

    /// Abstract class for storing A-REX accounting records (AAR)
    /**
     * This abstract class provides an interface which can be used to store
     * AAR information in the database
     *
     * \note This class is abstract. All functionality is provided by specialised
     * child classes.
     **/
    class AccountingDB {
    public:
        AccountingDB(const std::string& name) : name(name), isValid(false) {}
        virtual ~AccountingDB() {}

        /// Method to provide database ID for the queue
        /**
         * This method fetches the database to get stored Queue 
         * primary key. If requested Queue is missing in the database
         * it will be created and inserted ID is returned.
         *
         * Updates db_queue map of the AccountingDB object.
         *
         * If db_queue map is already populated with database data and Queue name 
         * is already inside the map, cached data will be returned.
         *
         * @return database primary key for provided queue
         **/
        virtual unsigned int getDBQueueId(const std::string& queue) = 0;

        /// Check if database connection is successfull
        /**
         * @return true if database connection successfull
         **/
        bool IsValid() const { return isValid; }
    protected:
        const std::string name;
        bool isValid;

        // queues ids
        name_id_map_t db_queue;
    };
}

#endif
