#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ClassLoader.h>

#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/alg/DenyOverridesAlg.h>
#include <arc/security/ArcPDP/alg/PermitOverridesAlg.h>
#include <arc/security/ArcPDP/alg/OrderedAlg.h>

#include "XACMLAlgFactory.h"

using namespace Arc;

namespace ArcSec {

Arc::Plugin* get_xacmlpdp_alg_factory (Arc::PluginArgument*) {
    return new ArcSec::XACMLAlgFactory();
}


void XACMLAlgFactory::initCombiningAlg(CombiningAlg* alg) {
  if(alg) algmap[alg->getalgId()]=alg;
}

void XACMLAlgFactory::initCombiningAlgs(){
  //Some XACML specified algorithm types
  CombiningAlg* alg = NULL;
  initCombiningAlg(new DenyOverridesCombiningAlg);
  initCombiningAlg(new PermitOverridesCombiningAlg);
  initCombiningAlg(new PermitDenyIndeterminateNotApplicableCombiningAlg);
  initCombiningAlg(new PermitDenyNotApplicableIndeterminateCombiningAlg);
  initCombiningAlg(new PermitIndeterminateDenyNotApplicableCombiningAlg);
  initCombiningAlg(new PermitIndeterminateNotApplicableDenyCombiningAlg);
  initCombiningAlg(new PermitNotApplicableDenyIndeterminateCombiningAlg);
  initCombiningAlg(new PermitNotApplicableIndeterminateDenyCombiningAlg);
  initCombiningAlg(new DenyPermitIndeterminateNotApplicableCombiningAlg);
  initCombiningAlg(new DenyPermitNotApplicableIndeterminateCombiningAlg);
  initCombiningAlg(new DenyIndeterminatePermitNotApplicableCombiningAlg);
  initCombiningAlg(new DenyIndeterminateNotApplicablePermitCombiningAlg);
  initCombiningAlg(new DenyNotApplicablePermitIndeterminateCombiningAlg);
  initCombiningAlg(new DenyNotApplicableIndeterminatePermitCombiningAlg);
  initCombiningAlg(new IndeterminatePermitDenyNotApplicableCombiningAlg);
  initCombiningAlg(new IndeterminatePermitNotApplicableDenyCombiningAlg);
  initCombiningAlg(new IndeterminateDenyPermitNotApplicableCombiningAlg);
  initCombiningAlg(new IndeterminateDenyNotApplicablePermitCombiningAlg);
  initCombiningAlg(new IndeterminateNotApplicablePermitDenyCombiningAlg);
  initCombiningAlg(new IndeterminateNotApplicableDenyPermitCombiningAlg);
  initCombiningAlg(new NotApplicablePermitDenyIndeterminateCombiningAlg);
  initCombiningAlg(new NotApplicablePermitIndeterminateDenyCombiningAlg);
  initCombiningAlg(new NotApplicableDenyPermitIndeterminateCombiningAlg);
  initCombiningAlg(new NotApplicableDenyIndeterminatePermitCombiningAlg);
  initCombiningAlg(new NotApplicableIndeterminatePermitDenyCombiningAlg);
  initCombiningAlg(new NotApplicableIndeterminateDenyPermitCombiningAlg);
  /** TODO:  other algorithm type............. */

}

XACMLAlgFactory::XACMLAlgFactory(){
  initCombiningAlgs();
}

CombiningAlg* XACMLAlgFactory::createAlg(const std::string& type){
  AlgMap::iterator it; 
  if((it=algmap.find(type)) != algmap.end()){
    return (*it).second;
  }
  else return NULL;
}

XACMLAlgFactory::~XACMLAlgFactory(){
  AlgMap::iterator it;
  for(it = algmap.begin(); it != algmap.end(); it = algmap.begin()){
    CombiningAlg * alg = (*it).second;
    algmap.erase(it);
    if(alg) delete alg;
  }
}

} // namespace ArcSec

