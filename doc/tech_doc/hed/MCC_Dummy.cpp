// MCC_Dummy.h

#include "MCC_Dummy.h"

namespace Arc {

  MCC_Dummy::MCC_Dummy(Config *cfg){
    // Due to lack of better ideas:
    next_=0;
  }
  
  MCC_Dummy::~MCC_Dummy(){
    // To delete or not delete - that is the question...
    delete next_;    
  }
  
  MCC_Status MCC_Dummy::process(Message& request, Message& response){
    MCC_Status status;
    // Do some processing of request.
    status = next_->process(request, response);
    // Check the outcome.
    if (status==0){
      // The request was successfully processed by the succeeding MCC.
      // Add some header fields to response if applicable.
      return 0;
    }
    else if (status<100){
      // This component can handle the error that occurred.
      // (The condition should be replaced by something that makes sense.)
      // Possibly this is done by adding some content to response.
      // After the error has been handled, everything is ok again!
      return 0;
    }
    else {
      // The error that occurred could not be handled by this MCC.
      // Send the status back to the previous MCC in the chain and
      // hope that it can handle it!
      return status;
    }
    // There may be many else if clauses defining different behaviour
    // for different kind of errors.
  }
  
}
