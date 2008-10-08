#ifndef __ARC_RANDOMBROKER_H__
#define __ARC_RANDOMBROKER_H__
#include "Broker.h"

namespace Arc {

    class RandomBroker: public Broker {
       public:
	   RandomBroker(Arc::JobDescription jd);
                    //virtual ~RandomBroker();
       protected:	   
	   void sort_Targets();
  };

} // namespace Arc

#endif // __ARC_RANDOMBROKER_H__
