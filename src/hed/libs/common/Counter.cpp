#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Counter.cpp

#include <cstdlib>

#include "Counter.h"

namespace Arc {

  const Glib::TimeVal ETERNAL(G_MAXLONG,0);
  const Glib::TimeVal HISTORIC(G_MINLONG,0);

  Counter::Counter(){
    // Nothing needs to be done.
  }

  Counter::Counter(const Counter&){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  Counter::~Counter(){
    // Nothing needs to be done.
  }

  void Counter::operator=(const Counter&){
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  Glib::TimeVal Counter::getCurrentTime(){
    Glib::TimeVal currentTime;
    currentTime.assign_current_time();
    return currentTime;
  }
  
  Glib::TimeVal Counter::getExpiryTime(Glib::TimeVal duration){
    if (duration<ETERNAL)
      return getCurrentTime()+duration;
    else
      return ETERNAL;
  }

  CounterTicket Counter::getCounterTicket(Counter::IDType reservationID,
				 Glib::TimeVal expiryTime,
				 Counter* counter)
  {
    return CounterTicket(reservationID, expiryTime, counter);
  }

  ExpirationReminder Counter::getExpirationReminder(Glib::TimeVal expTime,
					   Counter::IDType resID)
  {
    return ExpirationReminder(expTime, resID);
  }

  CounterTicket::CounterTicket() :
    reservationID(0),
    expiryTime(HISTORIC),
    counter(0)
  {
    // Nothing else needs to be done.
  }

  CounterTicket::CounterTicket(Counter::IDType reservationID,
			       Glib::TimeVal expiryTime,
			       Counter* counter) :
    reservationID(reservationID),
    expiryTime(expiryTime),
    counter(counter)
  {
    // Nothing else needs to be done.
  }

  bool CounterTicket::isValid(){
    return expiryTime>counter->getCurrentTime();
  }

  void CounterTicket::extend(Glib::TimeVal duration){
    counter->extend(reservationID, expiryTime, duration);
  }

  void CounterTicket::cancel(){
    counter->cancel(reservationID);
    reservationID=0;
    expiryTime=HISTORIC;
    counter=0;
  }

  ExpirationReminder::ExpirationReminder(Glib::TimeVal expiryTime,
					 Counter::IDType reservationID) :
    expiryTime(expiryTime), reservationID(reservationID)
  {
    // Nothing else needs to be done.
  }

  bool ExpirationReminder::operator<
    (const ExpirationReminder& other) const
  {
    // Smaller time has higher priority!
    return expiryTime>other.expiryTime;
  }

  Glib::TimeVal ExpirationReminder::getExpiryTime() const {
    return expiryTime;
  }

  Counter::IDType ExpirationReminder::getReservationID() const {
    return reservationID;
  }

}
