// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Counter.cpp

#include "Counter.h"

namespace Arc {

  const std::chrono::system_clock::duration ETERNAL =
    std::chrono::system_clock::duration::max();
  const std::chrono::system_clock::duration HISTORIC =
    std::chrono::system_clock::duration::min();

  Counter::Counter() {
    // Nothing needs to be done.
  }

  Counter::~Counter() {
    // Nothing needs to be done.
  }

  std::chrono::system_clock::time_point Counter::getCurrentTime() {
    return std::chrono::system_clock::now();
  }

  std::chrono::system_clock::time_point
  Counter::getExpiryTime(std::chrono::system_clock::duration duration) {
    if (duration < ETERNAL)
      return getCurrentTime() + duration;
    else
      return std::chrono::system_clock::time_point(ETERNAL);
  }

  CounterTicket
  Counter::getCounterTicket(Counter::IDType reservationID,
                            std::chrono::system_clock::time_point expiryTime,
                            Counter *counter) {
    return CounterTicket(reservationID, expiryTime, counter);
  }

  ExpirationReminder
  Counter::getExpirationReminder(std::chrono::system_clock::time_point expTime,
                                 Counter::IDType resID) {
    return ExpirationReminder(expTime, resID);
  }

  CounterTicket::CounterTicket()
    : reservationID(0),
      expiryTime(HISTORIC),
      counter(0) {
    // Nothing else needs to be done.
  }

  CounterTicket::CounterTicket(Counter::IDType reservationID,
                               std::chrono::system_clock::time_point expiryTime,
                               Counter *counter)
    : reservationID(reservationID),
      expiryTime(expiryTime),
      counter(counter) {
    // Nothing else needs to be done.
  }

  bool CounterTicket::isValid() {
    return expiryTime > counter->getCurrentTime();
  }

  void CounterTicket::extend(std::chrono::system_clock::duration duration) {
    counter->extend(reservationID, expiryTime, duration);
  }

  void CounterTicket::cancel() {
    counter->cancel(reservationID);
    reservationID = 0;
    expiryTime = std::chrono::system_clock::time_point(HISTORIC);
    counter = 0;
  }

  ExpirationReminder::ExpirationReminder(std::chrono::system_clock::time_point expiryTime,
                                         Counter::IDType reservationID)
    : expiryTime(expiryTime),
      reservationID(reservationID) {
    // Nothing else needs to be done.
  }

  bool ExpirationReminder::operator<(const ExpirationReminder& other) const {
    // Smaller time has higher priority!
    return expiryTime > other.expiryTime;
  }

  std::chrono::system_clock::time_point
  ExpirationReminder::getExpiryTime() const {
    return expiryTime;
  }

  Counter::IDType
  ExpirationReminder::getReservationID() const {
    return reservationID;
  }

}
