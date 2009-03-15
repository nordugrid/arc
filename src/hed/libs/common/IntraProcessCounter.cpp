// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Counter.cpp

#include <cstdlib>

#include "IntraProcessCounter.h"

namespace Arc {

  IntraProcessCounter::IntraProcessCounter(int limit, int excess)
    : limit(limit),
      excess(excess),
      value(limit),
      nextReservationID(1) {
    // Nothing else needs to be done.
  }

  IntraProcessCounter::IntraProcessCounter(const IntraProcessCounter&)
    : Counter() {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  IntraProcessCounter::~IntraProcessCounter() {
    // Nothing needs to be done.
  }

  void IntraProcessCounter::operator=(const IntraProcessCounter&) {
    // Executing this code should be impossible!
    exit(EXIT_FAILURE);
  }

  int IntraProcessCounter::getLimit() {
    return limit;
  }

  int IntraProcessCounter::setLimit(int newLimit) {
    synchMutex.lock();
    value += newLimit - limit;
    limit = newLimit;
    synchMutex.unlock();
    synchCond.signal();
    return newLimit;
  }

  int IntraProcessCounter::changeLimit(int amount) {
    int newLimit;
    synchMutex.lock();
    newLimit = limit + amount;
    value += amount;
    limit = newLimit;
    synchMutex.unlock();
    synchCond.signal();
    return newLimit;
  }

  int IntraProcessCounter::getExcess() {
    return excess;
  }

  int IntraProcessCounter::setExcess(int newExcess) {
    synchMutex.lock();
    excess = newExcess;
    synchMutex.unlock();
    synchCond.signal();
    return newExcess;
  }

  int IntraProcessCounter::changeExcess(int amount) {
    int newExcess;
    synchMutex.lock();
    newExcess = excess + amount;
    excess += amount;
    synchMutex.unlock();
    synchCond.signal();
    return newExcess;
  }

  int IntraProcessCounter::getValue() {
    int result;
    synchMutex.lock();
    result = unsafeGetValue();
    synchMutex.unlock();
    return result;
  }

  CounterTicket IntraProcessCounter::reserve
              (int amount,
              Glib::TimeVal duration,
              bool prioritized,
              Glib::TimeVal timeOut) {
    Glib::TimeVal deadline = getExpiryTime(timeOut);
    Glib::TimeVal expiryTime;
    IDType reservationID;
    synchMutex.lock();
    while (amount > unsafeGetValue() + (prioritized ? excess : 0) and
           getCurrentTime() < deadline)
      synchCond.timed_wait(synchMutex,
                           std::min(deadline, unsafeGetNextExpiration()));
    if (amount <= unsafeGetValue() + (prioritized ? excess : 0)) {
      expiryTime = getExpiryTime(duration);
      reservationID = unsafeReserve(amount, expiryTime);
    }
    else {
      expiryTime = HISTORIC;
      reservationID = 0;
    }
    synchMutex.unlock();
    return getCounterTicket(reservationID, expiryTime, this);
  }

  void IntraProcessCounter::cancel(unsigned long long int reservationID) {
    synchMutex.lock();
    unsafeCancel(reservationID);
    synchMutex.unlock();
    synchCond.signal();
  }

  void IntraProcessCounter::extend
              (IDType& reservationID,
              Glib::TimeVal& expiryTime,
              Glib::TimeVal duration) {
    int amount;
    synchMutex.lock();
    amount = unsafeCancel(reservationID);
    if (amount > 0) {
      expiryTime = getExpiryTime(duration);
      reservationID = unsafeReserve(amount, expiryTime);
    }
    else {
      expiryTime = HISTORIC;
      reservationID = 0;
    }
    synchMutex.unlock();
    synchCond.signal();
  }

  int IntraProcessCounter::unsafeGetValue() {
    while (unsafeGetNextExpiration() < getCurrentTime()) {
      unsafeCancel(selfExpiringReservations.top().getReservationID());
      selfExpiringReservations.pop();
    }
    return value;
  }

  int IntraProcessCounter::unsafeCancel(IDType reservationID) {
    std::map<IDType, int>::iterator resIter = reservations.find(reservationID);
    int amount = 0;
    if (resIter != reservations.end()) {
      amount = resIter->second;
      value += amount;
      reservations.erase(resIter);
    }
    return amount;
  }

  Counter::IDType IntraProcessCounter::unsafeReserve(int amount,
                                                     Glib::TimeVal expiryTime) {
    IDType reservationID = nextReservationID++;
    value -= amount;
    reservations[reservationID] = amount;
    if (expiryTime < ETERNAL)
      selfExpiringReservations.push
                    (getExpirationReminder(expiryTime, reservationID));
    return reservationID;
  }

  Glib::TimeVal IntraProcessCounter::unsafeGetNextExpiration() {
    if (selfExpiringReservations.empty())
      return ETERNAL;
    else
      return selfExpiringReservations.top().getExpiryTime();
  }

}
