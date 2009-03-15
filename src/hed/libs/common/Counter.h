// -*- indent-tabs-mode: nil -*-

// Counter.h

#ifndef __Counter__
#define __Counter__

#include <glibmm/thread.h>

namespace Arc {

  //! A time very far in the future.
  extern const Glib::TimeVal ETERNAL;

  //! A time very far in the past.
  extern const Glib::TimeVal HISTORIC;



  // Some forward declarations.
  class Counter;
  class CounterTicket;
  class ExpirationReminder;



  //! A class defining a common interface for counters.
  /*! This class defines a common interface for counters as well as
     some common functionality.

     The purpose of a counter is to provide housekeeping some resource
     such as e.g. disk space, memory or network bandwidth. The counter
     itself will not be aware of what kind of resource it limits the
     use of. Neither will it be aware of what unit is being used to
     measure that resource. Counters are thus very similar to
     semaphores. Furthermore, counters are designed to handle
     concurrent operations from multiple threads/processes in a
     consistent manner.

     Every counter has a limit, an excess limit and a value. The limit
     is a number that specify how many units are available for
     reservation. The value is the number of units that are currently
     available for reservation, i.e. has not allready been reserved.
     The excess limit specify how many extra units can be reserved for
     high priority needs even if there are no normal units available
     for reservation. The excess limit is similar to the credit limit
     of e.g. a VISA card.

     The users of the resource must thus first call the counter in
     order to make a reservation of an appropriate amount of the
     resource, then allocate and use the resource and finally call the
     counter again to cancel the reservation.

     Typical usage is:
     \code
     // Declare a counter. Replace XYZ by some appropriate kind of
     // counter and provide required parameters. Unit is MB.
     Arc::XYZCounter memory(...);
     ...
     // Make a reservation of memory for 2000000 doubles.
     Arc::CounterTicket tick = memory.reserve(2*sizeof(double));
     // Use the memory.
     double* A=new double[2000000];
     doSomething(A);
     delete[] A;
     // Cancel the reservation.
     tick.cancel();
     \endcode

     There are also alternative ways to make reservations, including
     self-expiring reservations, prioritized reservations and
     reservations that fail if they cannot be made fast enough.

     For self expiring reservations, a duration is provided in the
     reserve call:
     \code
     tick = memory.reserve(2*sizeof(double), Glib::TimeVal(1,0));
     \endcode
     A self-expiring reservation can be cancelled explicitly before it
     expires, but if it is not cancelled it will expire automatically
     when the duration has passed. The default value for the duration
     is Arc::ETERNAL, which means that the reservation will not be
     cancelled automatically.

     Prioritized reservations may use the excess limit and succeed
     immediately even if there are no normal units available for
     reservation. The value of the counter will in this case become
     negative. A prioritized reservation looks like this:
     \code
     tick = memory.reserve(2*sizeof(double), Glib::TimeVal(1,0), true);
     \endcode

     Finally, a time out option can be provided for a reservation. If
     some task should be performed within two seconds or not at all,
     the reservation can look like this:
     \code
     tick = memory.reserve(2*sizeof(double), Glib::TimeVal(1,0),
                          true, Glib::TimeVal(2,0));
     if (tick.isValid())
      doSomething(...);
     \endcode
   */
  class Counter {

  protected:

    //! A typedef of identification numbers for reservation.
    /*! This is a type that is used as identification numbesrs (keys)
       for referencing of reservations. It is used internally in
       counters for book keeping of reservations as well as in the
       CounterTicket class in order to be able to cancel and extend
       reservations.
     */
    typedef unsigned long long int IDType;

    //! Default constructor.
    /*! This is the default constructor. Since Counter is an abstract
       class, it should only be used by subclasses. Therefore it is
       protected. Furthermore, since the Counter class has no
       attributes, nothing needs to be initialized and thus this
       constructor is empty.
     */
    Counter();

  public:

    //! The destructor.
    /*! This is the destructor of the Counter class. Since the Counter
       class has no attributes, nothing needs to be cleaned up and thus
       the destructor is empty.
     */
    virtual ~Counter();

    //! Returns the current limit of the counter.
    /*! This method returns the current limit of the counter, i.e. how
       many units can be reserved simultaneously by different threads
       without claiming high priority.
       @return The current limit of the counter.
     */
    virtual int getLimit() = 0;

    //! Sets the limit of the counter.
    /*! This method sets a new limit for the counter.
       @param newLimit The new limit, an absolute number.
       @return The new limit.
     */
    virtual int setLimit(int newLimit) = 0;

    //! Changes the limit of the counter.
    /*! Changes the limit of the counter by adding a certain amount to
       the current limit.
       @param amount The amount by which to change the limit.
       @return The new limit.
     */
    virtual int changeLimit(int amount) = 0;

    //! Returns the excess limit of the counter.
    /*! Returns the excess limit of the counter, i.e. by how much the
       usual limit may be exceeded by prioritized reservations.
       @return The excess limit.
     */
    virtual int getExcess() = 0;

    //! Sets the excess limit of the counter.
    /*! This method sets a new excess limit for the counter.
       @param newExcess The new excess limit, an absolute number.
       @return The new excess limit.
     */
    virtual int setExcess(int newExcess) = 0;

    //! Changes the excess limit of the counter.
    /*! Changes the excess limit of the counter by adding a certain
       amount to the current excess limit.
       @param amount The amount by which to change the excess limit.
       @return The new excess limit.
     */
    virtual int changeExcess(int amount) = 0;

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
    virtual int getValue() = 0;

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
    virtual CounterTicket reserve
                  (int amount = 1,
                  Glib::TimeVal duration = ETERNAL,
                  bool prioritized = false,
                  Glib::TimeVal timeOut = ETERNAL) = 0;

  protected:

    //! Cancellation of a reservation.
    /*! This method cancels a reservation. It is called by the
       CounterTicket that corresponds to the reservation.
       @param reservationID The identity number (key) of the
       reservation to cancel.
     */
    virtual void cancel(IDType reservationID) = 0;

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
                        Glib::TimeVal duration = ETERNAL) = 0;

    //! Get the current time.
    /*! Returns the current time. An "adapter method" for the
       assign_current_time() method in the Glib::TimeVal class.
       return The current time.
     */
    Glib::TimeVal getCurrentTime();

    //! Computes an expiry time.
    /*! This method computes an expiry time by adding a duration to
       the current time.
       @param duration The duration.
       @return The expiry time.
     */
    Glib::TimeVal getExpiryTime(Glib::TimeVal duration);

    //! A "relay method" for a constructor of the CounterTicket class.
    /*! This method acts as a relay for one of the constructors of the
       CounterTicket class. That constructor is private, but needs to
       be accessible from the subclasses of Counter (bot not from
       anywhere else). In order not to have to declare every possible
       subclass of Counter as a friend of CounterTicket, only the base
       class Counter is a friend and its subclasses access the
       constructor through this method. (If C++ had supported "package
       access", as Java does, this trick would not have been
       necessary.)
       @param reservationID The identity number of the reservation
       corresponding to the CounterTicket.
       @param expiryTime the expiry time of the reservation
       corresponding to the CounterTicket.
       @param counter The Counter from which the reservation has been
       made.
       @return The counter ticket that has been created.
     */
    CounterTicket getCounterTicket(Counter::IDType reservationID,
                                   Glib::TimeVal expiryTime,
                                   Counter *counter);

    //! A "relay method" for the constructor of ExpirationReminder.
    /*! This method acts as a relay for one of the constructors of the
       ExpirationReminder class. That constructor is private, but needs
       to be accessible from the subclasses of Counter (bot not from
       anywhere else). In order not to have to declare every possible
       subclass of Counter as a friend of ExpirationReminder, only the
       base class Counter is a friend and its subclasses access the
       constructor through this method. (If C++ had supported "package
       access", as Java does, this trick would not have been
       necessary.)
       @param expTime the expiry time of the reservation corresponding
       to the ExpirationReminder.
       @param resID The identity number of the reservation
       corresponding to the ExpirationReminder.
       @return The ExpirationReminder that has been created.
     */
    ExpirationReminder getExpirationReminder(Glib::TimeVal expTime,
                                             Counter::IDType resID);
  private:

    //! Copy constructor, should not be used.
    /*! A private copy constructor, since Counters should never be
       copied. It should be impossible to use, but if that would happen
       by accident the program will exit with the EXIT_FAILURE code.
     */
    Counter(const Counter& unique);

    //! Assignment operator, should not be used.
    /*! A private assignment operator, since Counters should never be
       assigned. It should be impossible to use, but if that would
       happen by accident the program will exit with the EXIT_FAILURE
       code.
     */
    void operator=(const Counter& unique);

    //! The CounterTicket class needs to be a friend.
    friend class CounterTicket;

    //! The ExpirationReminder class needs to be a friend.
    friend class ExpirationReminder;

  };



  //! A class for "tickets" that correspond to counter reservations.
  /*! This is a class for reservation tickets. When a reservation is
     made from a Counter, a ReservationTicket is returned. This ticket
     can then be queried about the validity of a reservation. It can
     also be used for cancelation and extension of reservations.

     Typical usage is:
     \code
     // Declare a counter. Replace XYZ by some appropriate kind of
     // counter and provide required parameters. Unit is MB.
     Arc::XYZCounter memory(...);
     ...
     // Make a reservation of memory for 2000000 doubles.
     Arc::CounterTicket tick = memory.reserve(2*sizeof(double));
     // Use the memory.
     double* A=new double[2000000];
     doSomething(A);
     delete[] A;
     // Cancel the reservation.
     tick.cancel();
     \endcode
   */
  class CounterTicket {

  public:

    //! The default constructor.
    /*! This is the default constructor. It creates a CounterTicket
       that is not valid. The ticket object that is created can later
       be assigned a ticket that is returned by the reserve() method of
       a Counter.
     */
    CounterTicket();

    //! Returns the validity of a CounterTicket.
    /*! This method checks whether a CounterTicket is valid. The
       ticket was probably returned earlier by the reserve() method of
       a Counter but the corresponding reservation may have expired.
       @return The validity of the ticket.
     */
    bool isValid();

    //! Extends a reservation.
    /*! Extends a self-expiring reservation. In order to succeed the
       extension should be made before the previous reservation
       expires.
       @param duration The time by which to extend the reservation. The
       new expiration time is computed based on the current time, NOT
       the previous expiration time.
     */
    void extend(Glib::TimeVal duration);

    //! Cancels a resrvation.
    /*! This method is called to cancel a reservation. It may be
       called also for self-expiring reservations, which will then be
       cancelled before they were originally planned to expire.
     */
    void cancel();

  private:

    //! A private constructor.
    /*! This constructor creates an CounterTicket containing the
       specified expiry time and identity number of a reservation
       besides a pointer to the counter from which the reservation was
       made. In order to prevent unintended use, it is private. Because
       the Counter class must be able to use this constructor, it is
       declared to be a friend of this class.
       @param reservationID The identification number of the reservation.
       @param expiryTime The expiry time of the reservation.
       @param counter A pointer to the counter from which the
       reservation was made.
     */
    CounterTicket(Counter::IDType reservationID,
                  Glib::TimeVal expiryTime,
                  Counter *counter);

    //! The identification number of the corresponding reservation.
    Counter::IDType reservationID;

    //! The expiry time of the corresponding reservation.
    Glib::TimeVal expiryTime;

    //! A pointer to the Counter from which the reservation was made.
    Counter *counter;

    //! The Counter class needs to be a friend.
    friend class Counter;

  };



  //! A class intended for internal use within counters.
  /*! This class is used for "reminder objects" that are used for
     automatic deallocation of self-expiring reservations.
   */
  class ExpirationReminder {

  public:

    //! Less than operator, compares "soonness".
    /*! This is the less than operator for the ExpirationReminder
       class. It compares the priority of such objects with respect to
       which reservation expires first. It is used when reminder
       objects are inserted in a priority queue in order to allways
       place the next reservation to expire at the top.
     */
    bool operator<(const ExpirationReminder& other) const;

    //! Returns the expiry time.
    /*! This method returns the expiry time of the reservation that
       this ExpirationReminder is associated with.
       @return The expiry time.
     */
    Glib::TimeVal getExpiryTime() const;

    //! Returns the identification number of the reservation.
    /*! This method returns the identification number of the
       self-expiring reservation that this ExpirationReminder is
       associated with.
       @return The identification number.
     */
    Counter::IDType getReservationID() const;

  private:

    //! The constructor.
    /*! This constructor creates an ExpirationReminder containing the
       specified expiry time and identity number of a reservation. In
       order to prevent unintended use, it is private. Because the
       Counter class must be able to use this constructor, it is
       declared to be a friend of this class.
       @param expiryTime The expiry time of the reservation.
       @param reservationID The identification number of the reservation.
     */
    ExpirationReminder(Glib::TimeVal expiryTime,
                       Counter::IDType reservationID);

    //! The expiry time of the corresponding reservation.
    Glib::TimeVal expiryTime;

    //! The identification number of t he corresponding reservation.
    Counter::IDType reservationID;

    //! The Counter class needs to be a friend.
    friend class Counter;

  };

}

#endif
