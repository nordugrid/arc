#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>

#include <arc/client/RandomBroker.h>
	
namespace Arc {

  //RandomBroker::~RandomBroker() {}

  void RandomBroker::sort_Targets() {
            std::srand(time(NULL));
            int i, j;    
            for( unsigned int k=1; k<2*(std::rand()%found_Targets.size())+1; k++){
	  i=rand()%found_Targets.size();
	  j=rand()%found_Targets.size();
	  std::iter_swap(found_Targets.begin()+i,found_Targets.begin()+j);
            }      
  }
  

} // namespace Arc
