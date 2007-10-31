// IntraProcessCounter.h

#ifndef __IntraProcessCounter__
#define __IntraProcessCounter__

#include <map>
#include <queue>
#include <glibmm/thread.h>

#include "Counter.h"

namespace Arc {

  //! A class for counters used by threads within a single process.
  /*! This is a class for shared among different threads within a
    single process. See the Counter class for further information
    about counters and examples of usage.
   */
  class IntraProcessCounter : public Counter {

  public:

    //! Creates an IntraProcessCounter with specified limit and excess.
    /*! This constructor creates a counter with the specified limit
      (amount of resources available for reservation) and excess limit
      (an extra amount of resources that may be used for prioritized
      reservations).
      @param limit The limit of the counter.
      @param excess The excess limit of the counter.
     */
    IntraProcessCounter(int limit, int excess);

    //! Destructor.
    /*! This is the destructor of the IntraProcessCounter class. Does
      not need to do anything.
     */
    virtual ~IntraProcessCounter();

    //! Returns the current limit of the counter.
    /*! This method returns the current limit of the counter, i.e. how
      many units can be reserved simultaneously by different threads
      without claiming high priority.
      @return The current limit of the counter.
     */
    virtual int getLimit();

    //! Sets the limit of the counter.
    /*! This method sets a new limit for the counter.
      @param newLimit The new limit, an absolute number.
      @return The new limit.
     */
    virtual int setLimit(int newLimit);

    //! Changes the limit of the counter.
    /*! Changes the limit of the counter by adding a certain amount to
      the current limit.
      @param amount The amount by which to change the limit.
      @return The new limit.
     */
    virtual int changeLimit(int amount);


    //! Returns the excess limit of the counter.
    /*! Returns the excess limit of the counter, i.e. by how much the
      usual limit may be exceeded by prioritized reservations.
      @return The excess limit.
     */
    virtual int getExcess();

    //! Sets the excess limit of the counter.
    /*! This method sets a new excess limit for the counter.
      @param newExcess The new excess limit, an absolute number.
      @return The new excess limit.
     */
    virtual int setExcess(int newExcess);

    //! Changes the excess limit of the counter.
    /*! Changes the excess limit of the counter by adding a certain
      amount to the current excess limit.
      @param amount The amount by which to change the excess limit.
      @return The new excess limit.
     */
    virtual int changeExcess(int amount);

    //! Returns the current value of the counter.
    /*! Returns the current value of the counter, i.e. the number of
      unreserved units. Initially, the value is equal to the limit of
      the counter. When a reservation is made, the the value is
      decreased. Normally, the value should never be negative, but
      this may happen if there are prioritized reservations. It can
      also happen if the limit is decreased after some reservations
      have been made, since reservations are never revoked.
      @return The current value of the counter.
     */
    virtual int getValue();

    //! Makes a reservation from the counter.
    /*! This method makes a reservation from the counter. If the
      current value of the counter is too low to allow for the
      reservation, the method blocks until the reservation is
      possible or times out.
      @param amount The amount to reserve, default value is 1.
      @param duration The duration of a self expiring reservation,
      default is that it lasts forever.
      @param prioritized Whether this reservation is prioritized and
      thus allowed to use the excess limit.
      @param timeOut The maximum time to block if the value of the
      counter is too low, default is to allow "eternal" blocking.
      @return A CounterTicket that can be queried about the status of
      the reservation as well as for cancellations and extensions.
     */
    virtual CounterTicket reserve(int amount=1,
				  Glib::TimeVal duration=ETERNAL,
				  bool prioritized=false,
				  Glib::TimeVal timeOut=ETERNAL);

  protected:


    //! Cancellation of a reservation.
    /*! This method cancels a reservation. It is called by the
      CounterTicket that corresponds to the reservation.
      @param reservationID The identity number (key) of the
      reservation to cancel.
     */
    virtual void cancel(IDType reservationID);

    //! Extension of a reservation.
    /*! This method extends a reservation. It is called by the
      CounterTicket that corresponds to the reservation.
      @param reservationID Used for input as well as output. Contains
      the identification number of the original reservation on entry
      and the new identification number of the extended reservation on
      exit.
      @param expiryTime Used for input as well as output. Contains the
      expiry time of the original reservation on entry and the new
      expiry time of the extended reservation on exit.
      @param duration The time by which to extend the reservation. The
      new expiration time is computed based on the current time, NOT
      the previous expiration time.
     */
    virtual void extend(IDType& reservationID,
			Glib::TimeVal& expiryTime,
			Glib::TimeVal duration=ETERNAL);

  private:

    //! Copy constructor, should not be used.
    /*! A private copy constructor, since Counters should never be
      copied. It should be impossible to use, but if that would happen
      by accident the program will exit with the EXIT_FAILURE code.
     */
    IntraProcessCounter(const IntraProcessCounter& unique);

    //! Assignment operator, should not be used.
    /*! A private assignment operator, since Counters should never be
      assigned. It should be impossible to use, but if that would
      happen by accident the program will exit with the EXIT_FAILURE
      code.
     */
    void operator=(const IntraProcessCounter& unique);

    //! Computes and returns the value of the counter.
    /*! Cancels any pending reservations that have expired and returns
      the value of the counter. This method is not thread-safe by
      itself and should only be called from other methods that have
      allready locked synchMutex.
      @return The value of the counter.
     */
    int unsafeGetValue();

    //! Cancels a reservation.
    /*! Cancels a reservation with the specified identification
      number, i.e. removes that entry from the reservations map and
      increases the value by the corresponding amount. This method is
      not thread-safe by itself and should only be called from other
      methods that have allready locked synchMutex.
      @param reservationID The identification number of the
      reservation to cancel.
      @return The amount that was reseved, or zero if there was no
      reservation with the specified identification number.
     */
    int unsafeCancel(IDType reservationID);

    //! Makes a reservation.
    /*! Makes a reservation of the specified amount for the specified
      duration and returns the identification number of the
      reservation. This method is not thread-safe by itself and should
      only be called from other methods that have allready locked
      synchMutex. Furthermore, it assumes that the calling method has
      allready asserted that the specified amount is available for
      reservation.
      @param amount The amount to reserve.
      @duration The duration of the reservation.
      @return The identification number of the reservation.
     */
    IDType unsafeReserve(int amount, Glib::TimeVal duration);

    //! Returns the expiry time for the next expiring reservation.
    /*! Returns the expiry time for the next expiring reservation,
      i.e. the expiry time of the top entry of the
      selfExpiringReservations priority queue.
      @return The expiry time for the next expiring reservation.
     */
    Glib::TimeVal unsafeGetNextExpiration();

    //! The limit of the counter.
    /*! The current limit of the counter. Should not be altered unless
      synchMutex is locked.
     */
    int limit;

    //! The excess limit of the counter.
    /*! The current excess limit of the counter. Should not be altered
      unless synchMutex is locked.
     */
    int excess;

    //! The value of the counter.
    /*! The current value of the counter. Should not be altered unless
      synchMutex is locked.
     */
    int value;

    //! The identification number of the next reservation.
    /*! The attribute holds the identification number of the next
      reservation. When a new identification number is needed, this
      number is used and the attribute is incremented in order to hold
      a number that is available for the next reservation. Should not
      be altered unless synchMutex is locked.
     */
    IDType nextReservationID;

    //! Maps identification numbers of reservations to amounts.
    /*! This is a map that uses identification numbers of reservations
      as keys and maps them to the corresponding amounts amounts.
     */
    std::map<IDType, int> reservations;

    //! Contains expiration reminders of self-expiring reservations.
    /*! This priority queue contains expiration reminders of
      self-expiring reservations. The next reservation to expire is
      allways at the top.
     */
    std::priority_queue<ExpirationReminder> selfExpiringReservations;

    //! A mutex that protects the attributes.
    /*! This mutex is used for protection of attributes from
      concurrent access from several threads. Any method that alter an
      attribute should lock this mutex.
     */
    Glib::Mutex synchMutex;

    //! A condition used for waiting for waiting for a higher value.
    /*! This condition is used when a reservation cannot be made
      immediately because the ammount that shall be reserved is larger
      than what is currently available.
     */
    Glib::Cond synchCond;

  };

}

#endif
