// MCC_Dummy.h

#ifndef __ARC_MCC_DUMMY__
#define __ARC_MCC_DUMMY__

namespace Arc {
  
  //! The Message class
  /*! Only a dummy Message class instead of the real thing.
   */
  class Message {};

  //! The Config class
  /*! Only a dummy Config class instead of the real thing.
   */
  class Config {};

  //! A status type
  /*! A type to be used by MCCs to indicate the status after
    processing a request. For the moment only an ordinary int, where
    the value 0 indicates that the request was successfully
    processed. A more detailed analysis of possible errors is needed
    in order to define a more apropriate type.
  */
  typedef int MCC_Status;


  //! The base class for all Message Chain Components.
  /*! All Message Chain Component (MCC) classes will extend this
    class. It's main purpose is to define an interface for
    communication between MCCs, i.e. the process() method.
  */
  class MCC {
  public:

    //! A constructor
    /*! This is the constructor. It does nothing since there are no
      attributes to initialize.
     */
    MCC() {}

    //! The destructor
    /*! This is the destructor. It does nothing since there is nothing
      that needs to be cleaned up.
     */
    virtual ~MCC() {}

    //! The method that processes an incoming request
    /*! This method is called by the preceeding MCC in a chain when an
      incoming request needs to be processed.
      The advantage of sending out the response through a reference
      parameter is that no new Message object is created, as would be
      the case if the response was sent as a return value. The problem
      with creating new message objects is that it either involves a
      shallow copy of the payload (which may result in memory leaks or
      "dangling" pointers when the copy is deallocated) or a deep copy
      of the payload (which may be an expensive operation or even
      impossible in case of streams).
      \param request The incoming request that needs to be processed.
      \param response A message object that will contain the response
      of the request when the method returns.
      \return An object (integer) representing the status of the call,
      zero if everything was ok and non-zero if an error occurred. The
      precise meaning of non-zero values have to be decided.
     */
    virtual MCC_Status process(Message& request, Message& response) = 0;
  };


  //! A dummy class illustrating how to extend the MCC class
  /*! This is just a dummy MCC class that does nothing. The sole
    purpose of it is to illustrate how to extend the MCC class and
    coarsely what an implementation of the process() method could look
    like.
  */
  class MCC_Dummy {
  public:

    //! A constructor
    /*! This is the constructor. It should initialize the next_
      attribute to point to the MCC that is the successor of the
      present MCC.
      \param cfg A configuration object. No details are known yet.
     */
    MCC_Dummy(Config *cfg);

    //! The destructor
    /*! This is the destructor. Should it delete (deallocate) the MCC
      pointed to by next_? If not, there may be a memory leak. If it
      does, there may be "dangling pointers" left in case several
      chains are merged to that MCC.
     */
    ~MCC_Dummy();

    //! The method that processes an incoming request
    /*! See the corresponding method in the MCC class for a thorough
      description.
     */    
    virtual MCC_Status process(Message& request, Message& response);
  private:

    //! The succeeding MCC
    /*! A pointer to the MCC that is the successor of the present MCC,
      i.e. to which MCC the process() method shall pass incoming
      requests.
     */
    MCC *next_;
  };

}

#endif
