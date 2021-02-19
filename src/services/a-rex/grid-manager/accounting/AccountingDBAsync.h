#ifndef ARC_ACCOUNTING_DB_ASYNC_H
#define ARC_ACCOUNTING_DB_ASYNC_H

#include <string>

#include "AAR.h"
#include "AccountingDB.h"

namespace ARex {
    class AccountingDBAsync: public AccountingDB {
    public:
        AccountingDBAsync(const std::string& name, AccountingDB* (*ctr)(const std::string&));

        virtual ~AccountingDBAsync();

        virtual bool createAAR(AAR& aar);

        virtual bool updateAAR(AAR& aar);

        virtual bool addJobEvent(aar_jobevent_t& events, const std::string& jobid);

        class Event {
         public:
          Event(std::string const& name);
          virtual ~Event();
          std::string name;
        };
        class EventCreateAAR: public Event {
         public:
          EventCreateAAR(std::string const& name, AAR const& aar);
          AAR aar;
        };
        class EventUpdateAAR: public Event {
         public:
          EventUpdateAAR(std::string const& name, AAR const& aar);
          AAR aar;
        };
        class EventAddJobEvent: public Event {
         public:
          EventAddJobEvent(std::string const& name, aar_jobevent_t const& events, std::string const& jobid);
          aar_jobevent_t events;
          std::string jobid;
        };
        class EventQuit: public Event {
         public:
          EventQuit();
        };
    };
}

#endif
