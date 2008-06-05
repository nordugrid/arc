#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <globus_common.h>
#include <globus_rsl.h>
#include <arc/Logger.h>
#include "subst_rsl.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

#define GLOBUS_ERROR ((globus_result_t)(GLOBUS_FAILURE))

void subst_structure(globus_rsl_t *cur,rsl_subst_table_t* symbol_table);
char* subst_value(globus_rsl_value_t** cur_p,rsl_subst_table_t* symbol_table,int subst_flag);

//static char* rsl_operators [11] = {
//  "?0?","=","!=",">",">=","<","<=","?7?","&","|","multi"
//};

globus_result_t rsl_subst_table_init(rsl_subst_table_t* symbol_table) {
  symbol_table->keys=NULL;
  symbol_table->values=NULL;
  if((symbol_table->symbols=
        (globus_symboltable_t*)globus_malloc(sizeof(rsl_subst_table_t))) 
                          == GLOBUS_NULL) {
     logger.msg(Arc::ERROR,"Memory allocation error");
     return GLOBUS_ERROR;
  };  
  globus_symboltable_init(symbol_table->symbols,
                          globus_hashtable_string_hash,
                          globus_hashtable_string_keyeq);
  globus_symboltable_create_scope(symbol_table->symbols);
  return GLOBUS_SUCCESS;
}

globus_result_t rsl_subst_table_destroy(rsl_subst_table_t* symbol_table) {
  globus_symboltable_remove_scope(symbol_table->symbols);
  globus_symboltable_destroy(symbol_table->symbols);
  globus_list_t* cur;
  for(cur=symbol_table->keys;
      !globus_list_empty(cur);cur=globus_list_rest(cur)){
    globus_free(globus_list_first(cur));
  };
  globus_list_free(symbol_table->keys);
  for(cur=symbol_table->values;
      !globus_list_empty(cur);cur=globus_list_rest(cur)){
    globus_free(globus_list_first(cur));
  };
  globus_list_free(symbol_table->values);
  return GLOBUS_SUCCESS;
}

globus_result_t rsl_subst_table_insert(rsl_subst_table_t* symbol_table,const char* name,const char* value) {
  globus_symboltable_insert(symbol_table->symbols,(void*)name,(void*)value);
  globus_list_insert(&(symbol_table->keys),(void*)name);
  globus_list_insert(&(symbol_table->values),(void*)value);
  return GLOBUS_SUCCESS;
}

globus_result_t rsl_subst(globus_rsl_t* rsl,rsl_subst_table_t* symbol_table) {
  char** tmp_param;
  if (globus_rsl_param_get(rsl,GLOBUS_RSL_PARAM_SEQUENCE,
                           const_cast<char*>("rsl_substitution"),&tmp_param) == 0) {
    for(int i=0;tmp_param[i];i++) {
      char* name = strdup(tmp_param[i]);
      i++; if(tmp_param[i] == NULL) { free(name); break; };
      char* value = strdup(tmp_param[i]);
      rsl_subst_table_insert(symbol_table,name,value);
    };
  };

  subst_structure(rsl,symbol_table);
//  if(globus_rsl_print_recursive(rsl) != GLOBUS_SUCCESS) {
//    olog << "Failed printing RSL" << std::endl;
//  };
  
  return GLOBUS_SUCCESS;
}

void subst_structure(globus_rsl_t *cur,rsl_subst_table_t* symbol_table) {
  if(globus_rsl_is_boolean(cur)) {
    //olog<<"BOOLEAN: "<<cur->req.boolean.my_operator<<std::endl;
    globus_rsl_t* cur_;
    globus_list_t *list = cur->req.boolean.operand_list;
    for(;;) {
      if(globus_list_empty(list)) break;
      cur_=(globus_rsl_t*)globus_list_first(list);
      subst_structure(cur_,symbol_table);
      list=globus_list_rest(list);
    };
  }
  else if(globus_rsl_is_relation(cur)) {
    char* res;
    //olog<<"RELATION: "<<cur->req.relation.attribute_name<<" ";
    //olog_<<rsl_operators[cur->req.relation.my_operator]<<" ";
    res=subst_value(&(cur->req.relation.value_sequence),symbol_table,0);
    if(res) free(res);
    //olog_<<std::endl;
  }
  else {
    logger.msg(Arc::ERROR,"UNKNOWN RSL STRUCTURE");
  };
}

/* returned result must be freed */
char* subst_value(globus_rsl_value_t** cur_p,rsl_subst_table_t* symbol_table,int subst_flag) {
  char* res = NULL;
  char* res_;
  char* res__;
  globus_rsl_value_t* cur = (*cur_p);
  if(globus_rsl_value_is_literal(cur)) {
    //olog_<<"LITERAL( ";
    //olog_<<cur->value.literal.string<<" ";
    res=strdup(cur->value.literal.string);
  }
  else if(globus_rsl_value_is_concatenation(cur)) {
    //olog_<<"CONCATENATION( ";
    res=subst_value(&(cur->value.concatenation.left_value),symbol_table,0);
    res_=subst_value(&(cur->value.concatenation.right_value),symbol_table,0);
    if((res!=NULL) && (res_!=NULL)) {
      res__=(char*)malloc(strlen(res)+strlen(res_)+1);
      if(res__) { strcpy(res__,res);strcat(res__,res_); };
      free(res); free(res_); res=res__;
    }
    else {
      if(res) free(res); if(res_) free(res_); res=NULL;
    };
  }
  else if(globus_rsl_value_is_sequence(cur)) {
    //olog_<<"SEQUENCE( ";
    globus_rsl_value_t* cur_;
    globus_list_t *list = cur->value.sequence.value_list;
    if(subst_flag) {
      /* first component should be name */
      if(!globus_list_empty(list)) {
        cur_=(globus_rsl_value_t*)globus_list_first(list);
        res=subst_value(&cur_,symbol_table,0);
        /* if it was variable - cur_ can be changed */
        globus_list_replace_first(list,cur_);
        list=globus_list_rest(list);
      };
    };
    for(;;) {
      if(globus_list_empty(list)) break;
      cur_=(globus_rsl_value_t*)globus_list_first(list);
      res_=subst_value(&cur_,symbol_table,0);
      if(res_) free(res_);
      /* if it was variable - cur_ can be changed */
      globus_list_replace_first(list,cur_);
      list=globus_list_rest(list);
    };
  }
  else if(globus_rsl_value_is_variable(cur)) {
    //olog_<<"VARIABLE( ";
    res=subst_value(&(cur->value.variable.sequence),symbol_table,1);
    /* here we have name of the variable */
    if(res) {
      char* value = (char*)globus_symboltable_lookup(symbol_table->symbols,res);
      if(value) {
        //olog_<<" changing "<<res<<" to "<<value<<" , ";
        globus_rsl_value_t* tmp = globus_rsl_value_make_literal(strdup(value));
        if(tmp) { 
          globus_rsl_value_free_recursive(cur);
          (*cur_p)=tmp;
        };
      };
      free(res); res=NULL; 
      if(value) res=strdup(value); /* returning value */
    };
  }
  else {
    logger.msg(Arc::ERROR,"UNKNOWN RLS ELEMENT");
    //olog_<<"UNKNOWN( ";
  };
  //olog_<<") ";
  return res;
}

