#ifndef __ARC_GM_RSL_SUBST_H__
#define __ARC_GM_RSL_SUBST_H__
#include <globus_rsl.h>

struct rsl_subst_table_s {
  globus_symboltable_t* symbols;
  globus_list_t* keys;
  globus_list_t* values;
};
typedef struct rsl_subst_table_s rsl_subst_table_t;

globus_result_t rsl_subst_table_init(rsl_subst_table_t* symbol_table);
globus_result_t rsl_subst_table_destroy(rsl_subst_table_t* symbol_table);
globus_result_t rsl_subst_table_insert(rsl_subst_table_t* symbol_table,const char* name,const char* value);
globus_result_t rsl_subst(globus_rsl_t* rsl,rsl_subst_table_t* symbol_table);

#endif

