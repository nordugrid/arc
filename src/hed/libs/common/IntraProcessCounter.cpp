// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Counter.cpp

#include "IntraProcessCounter.h"

namespace Arc {

  IntraProcessCounter::IntraProcessCounter(int limit, int excess)
    : limit(limit),
      excess(excess),
      value(limit),
      nextReservationID(1) {
    // Nothing else needs to be done.
  }

  IntraProcessCounter::~IntraProcessCounter() {
    // Nothing needs to be done.
  }

  int IntraProcessCounter::getLimit() {
    return limit;
  }

  int IntraProcessCounter::setLimit(int newLimit) {
    synchMutex.lock();
    value += newLimit - limit;
    limit = newLimit;
    synchMutex.unlock();
    synchCond.notify_one();
    return newLimit;
  }

  int IntraProcessCounter::changeLimit(int amount) {
    int newLimit;
    synchMutex.lock();
    newLimit = limit + amount;
    value += amount;
    limit = newLimit;
    synchMutex.unlock();
    synchCond.notify_one();
    return newLimit;
  }

  int IntraProcessCounter::getExcess() {
    return excess;
  }

  int IntraProcessCounter::setExcess(int newExcess) {
    synchMutex.lock();
    excess = newExcess;
    synchMutex.unlock();
    synchCond.notify_one();
    return newExcess;
  }

  int IntraProcessCounter::changeExcess(int amount) {
    int newExcess;
    synchMutex.lock();
    newExcess = excess + amount;
    excess += amount;
    synchMutex.unlock();
    synchCond.notify_one();
    return newExcess;
  }

  int IntraProcessCounter::getValue() {
    int result;
    synchMutex.lock();
    result = unsafeGetValue();
    synchMutex.unlock();
    return result;
  }

  CounterTicket
  IntraProcessCounter::reserve(int amount,
                               std::chrono::system_clock::duration duration,
                               bool prioritized,
                               std::chrono::system_clock::duration timeOut) {
    std::chrono::system_clock::time_point deadline = getExpiryTime(timeOut);
    std::chrono::system_clock::time_point expiryTime;
    IDType reservationID;
    std::unique_lock<std::mutex> lock(synchMutex);
    while (amount > unsafeGetValue() + (prioritized ? excess : 0) and
           getCurrentTime() < deadline)
      synchCond.wait_until(lock,
                           std::min(deadline, unsafeGetNextExpiration()));
    if (amount <= unsafeGetValue() + (prioritized ? excess : 0)) {
      expiryTime = getExpiryTime(duration);
      reservationID = unsafeReserve(amount, expiryTime);
    }
    else {
      expiryTime = std::chrono::system_clock::time_point(HISTORIC);
      reservationID = 0;
    }
    return getCounterTicket(reservationID, expiryTime, this);
  }

  void IntraProcessCounter::cancel(unsigned long long int reservationID) {
    synchMutex.lock();
    unsafeCancel(reservationID);
    synchMutex.unlock();
    synchCond.notify_one();
  }

  void IntraProcessCounter::extend(IDType& reservationID,
                                   std::chrono::system_clock::time_point& expiryTime,
                                   std::chrono::system_clock::duration duration) {
    int amount;
    synchMutex.lock();
    amount = unsafeCancel(reservationID);
    if (amount > 0) {
      expiryTime = getExpiryTime(duration);
      reservationID = unsafeReserve(amount, expiryTime);
    }
    else {
      expiryTime = std::chrono::system_clock::time_point(HISTORIC);
      reservationID = 0;
    }
    synchMutex.unlock();
    synchCond.notify_one();
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

  Counter::IDType
  IntraProcessCounter::unsafeReserve(int amount,
                                     std::chrono::system_clock::time_point expiryTime) {
    IDType reservationID = nextReservationID++;
    value -= amount;
    reservations[reservationID] = amount;
    if (expiryTime < std::chrono::system_clock::time_point(ETERNAL))
      selfExpiringReservations.push(getExpirationReminder(expiryTime,
                                                          reservationID));
    return reservationID;
  }

  std::chrono::system_clock::time_point IntraProcessCounter::unsafeGetNextExpiration() {
    if (selfExpiringReservations.empty())
      return std::chrono::system_clock::time_point(ETERNAL);
    else
      return selfExpiringReservations.top().getExpiryTime();
  }

}
