#include "DataPointRLS.h"
#include "DMCRLS.h"
#include "common/StringConv.h"

extern "C" {
#include "globus_rls_client.h"
}

namespace Arc {

  DataPointRLS::DataPointRLS(DMCRLS* dmc, const URL& url) :
    DataPoint(url), dmc(dmc) {}

  unsigned long long int DataPointRLS::GetSize() {
    if (size == -1) {
      globus_rls_handle_t* h = NULL;
      globus_rls_client_connect(const_cast<char *>
				(url.ConnectionURL().c_str()), &h);
      globus_list_t* attr_list;
      globus_rls_client_lrc_attr_value_get(h, const_cast<char *>
					   (url.Path().c_str()), NULL,
					   globus_rls_obj_lrc_lfn, &attr_list);
      for(globus_list_t* pa = attr_list; pa; pa = globus_list_rest(pa)) {
	globus_rls_attribute_t* attr =
	  (globus_rls_attribute_t*)globus_list_first(pa);
	if(attr->type != globus_rls_attr_type_str)
	  continue;
	if(strcmp(attr->name, "size") == 0) {
	  size = stringtoull(attr->val.s);
	}
      }      
      globus_rls_client_free_list(attr_list);
      globus_rls_client_close(h);
    }
    return size;
  }

} // namespace Arc
