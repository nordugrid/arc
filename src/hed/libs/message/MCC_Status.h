// MCC_Status.h

#ifndef __MCC_Status__
#define __MCC_Status__

#include <string>

namespace Arc {
  
  //! Status kinds (types)
  /*! This enum defines a set of possible status kinds.
   */
  enum StatusKind {
    STATUS_UNDEFINED = 0,
    STATUS_OK = 1,
    GENERIC_ERROR = 2,
    PARSING_ERROR = 4,
    PROTOCOL_RECOGNIZED_ERROR = 8,
    UNKNOWN_SERVICE_ERROR = 16,
    BUSY_ERROR = 32
  };
  
  //! Conversion to string.
  /*! Conversion from StatusKind to string.
    @param kind The StatusKind to convert.
  */
  std::string string(StatusKind kind);



  //! A class for communication of MCC statuses
  /*! This class is used to communicate status between MCCs. It
    contains a status kind, a string specifying the origin (MCC) of
    the status object and an explanation.
  */
  class MCC_Status {
  public:

    //! The constructor
    /*! Creates a MCC_Status object.
      @param kind The StatusKind (default: STATUS_UNDEFINED)
      @param origin The origin MCC (default: "???")
      @param explanation An explanation (default: "No explanation.")
    */
    MCC_Status(StatusKind kind = STATUS_UNDEFINED,
	       const std::string& origin = "???",
	       const std::string& explanation = "No explanation.");

    //! Is the status kind ok?
    /*! This method returns true iff the status kind of this object is
      STATUS_OK
      @return true iff kind==STATUS_OK
    */
    bool isOk() const;

    //! Returns the status kind.
    /*! Returns the status kind of this object.
      @return The status kind of this object.
    */
    StatusKind getKind() const;

    //! Returns the origin.
    /*! This method returns a string specifying the origin MCC of this
      object.
      @return A string specifying the origin MCC of this object.
    */
    const std::string& getOrigin() const;

    //! Returns an explanation.
    /*! This method returns an explanation of this object.
      @return An explanation of this object.
    */
    const std::string& getExplanation() const;

    //! Conversion to string.
    /*! This operator converts a MCC_Status object to a string.
     */
    operator std::string() const;

  private:

    //! The kind (type) of status.
    StatusKind kind;

    //! A string describing the origin MCC of this object.
    std::string origin;

    //! An explanation of this object.
    std::string explanation;

  };

}

#endif
