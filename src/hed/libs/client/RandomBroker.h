#ifndef __ARC_RANDOMBROKER_H__
#define __ARC_RANDOMBROKER_H__
#include <arc/client/Broker.h>

namespace Arc {

    class RandomBroker: public Broker {
       public:
	   RandomBroker(Arc::TargetGenerator& targen,  Arc::JobDescription jobd): Broker( targen, jobd ) {}
                    //virtual ~RandomBroker();
       protected:	   
	   virtual void sort_Targets();
  };

} // namespace Arc

#endif // __ARC_RANDOMBROKER_H__
