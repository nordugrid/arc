#ifndef ARC_ACCOUNTING_DB_H
#define ARC_ACCOUNTING_DB_H

#include <string>

#include "AARContent.h"

namespace ARex {
    class GMJob;
    class GMConfig;

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

        /// Check if database connection is successfull
        /**
         * @return true if database connection successfull
         **/
        bool IsValid() const { return isValid; }
        /// Create new AAR in the database
        /** 
         * write basic info available in ACCEPTED state to the 
         * accounting database.
         * This methid registers a new job that is just accepted
         * and write down jobID and ownership information
         **/
        virtual bool createAAR(AAR& aar) = 0;
        /// Update AAR in the database
        /**
         * write all accounting info when job reaches FINISHED state
         * this updates all dynamic and resource consumtion info
         * collected during the job execution. Extra information about the job
         * is also recorded here
         **/
        virtual bool updateAAR(AAR& aar) = 0;
        /// Add job even record to AAR
        /**
         * write record about job state change to accounting log 
         **/
        virtual bool addJobEvent(aar_jobevent_t& events, const std::string& jobid) = 0;
    protected:
        const std::string name;
        bool isValid;
    };
}

#endif
