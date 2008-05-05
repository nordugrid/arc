///This class is for the <AttributeValue> inside <Attribute> (see 2.7.3.1.1 of saml 2.0 core specification), 
///with the processing favor of lasso
#ifndef __LASSO_SAML2_ATTRIBUTEVALUE_H__
#define __LASSO_SAML2_ATTRIBUTEVALUE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <lasso/xml/xml.h>

#define LASSO_TYPE_SAML2_ATTRIBUTEVALUE (lasso_saml2_attributevalue_get_type())
#define LASSO_SAML2_ATTRIBUTEVALUE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
		LASSO_TYPE_SAML2_ATTRIBUTEVALUE, \
		LassoSaml2AttributeValue))
#define LASSO_SAML2_ATTRIBUTEVALUE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), \
		LASSO_TYPE_SAML2_ATTRIBUTEVALUE, \
		LassoSaml2AttributeValueClass))
#define LASSO_IS_SAML2_ATTRIBUTEVALUE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), \
		LASSO_TYPE_SAML2_ATTRIBUTEVALUE))
#define LASSO_IS_SAML2_ATTRIBUTEVALUE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
		LASSO_TYPE_SAML2_ATTRIBUTEVALUE))
#define LASSO_SAML2_ATTRIBUTEVALUE_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS ((o), \
		LASSO_TYPE_SAML2_ATTRIBUTEVALUE, \
		LassoSaml2AttributeValueClass)) 


typedef struct _LassoSaml2AttributeValue LassoSaml2AttributeValue;
typedef struct _LassoSaml2AttributeValueClass LassoSaml2AttributeValueClass;


struct _LassoSaml2AttributeValue {
	LassoNode parent;

	/*< public >*/
	/* elements */
        //char *type;
	char *content;
};


struct _LassoSaml2AttributeValueClass {
	LassoNodeClass parent;
};

LASSO_EXPORT GType lasso_saml2_attributevalue_get_type(void);
LASSO_EXPORT LassoSaml2AttributeValue* lasso_saml2_attributevalue_new(void);

LASSO_EXPORT LassoSaml2AttributeValue* lasso_saml2_attributevalue_new_with_string(const char *content);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LASSO_SAML2_ATTRIBUTEVALUE_H__ */

