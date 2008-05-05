#include "lasso_attributevalue.h"

/*
 * Schema fragment (saml-core-2.0-os.pdf):
 *
 * <xs:element name="AttributeValue" type="xs:anyType" nillable="true"/>
 */

/*****************************************************************************/
/* private methods                                                           */
/*****************************************************************************/


static struct XmlSnippet schema_snippets[] = {
	{ "content", SNIPPET_TEXT_CHILD,
		G_STRUCT_OFFSET(LassoSaml2AttributeValue, content) },
	{NULL, 0, 0}
};

static LassoNodeClass *parent_class = NULL;


/*****************************************************************************/
/* instance and class init functions                                         */
/*****************************************************************************/

static void
instance_init(LassoSaml2AttributeValue *node)
{
	node->content = NULL;
}

static void
class_init(LassoSaml2AttributeValueClass *klass)
{
	LassoNodeClass *nclass = LASSO_NODE_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	nclass->node_data = g_new0(LassoNodeClassData, 1);
	lasso_node_class_set_nodename(nclass, "AttributeValue");
	lasso_node_class_add_snippets(nclass, schema_snippets);
}

GType
lasso_saml2_attributevalue_get_type()
{
	static GType this_type = 0;

	if (!this_type) {
		static const GTypeInfo this_info = {
			sizeof (LassoSaml2AttributeValueClass),
			NULL,
			NULL,
			(GClassInitFunc) class_init,
			NULL,
			NULL,
			sizeof(LassoSaml2AttributeValue),
			0,
			(GInstanceInitFunc) instance_init,
		};

		this_type = g_type_register_static(LASSO_TYPE_NODE,
				"LassoSaml2AttributeValue", &this_info, 0);
	}
	return this_type;
}

/**
 * lasso_saml2_attributevalue_new:
 *
 * Creates a new #LassoSaml2AttributeValue object.
 *
 * Return value: a newly created #LassoSaml2AttributeValue object
 **/
LassoSaml2AttributeValue*
lasso_saml2_attributevalue_new()
{
	return g_object_new(LASSO_TYPE_SAML2_ATTRIBUTEVALUE, NULL);
}


/**
 * lasso_saml2_attributevalue_new_with_string:
 * @content: 
 *
 * Creates a new #LassoSaml2AttributeValue object and initializes it
 * with @content.
 *
 * Return value: a newly created #LassoSaml2AttributeValue object
 **/
LassoSaml2AttributeValue*
lasso_saml2_attributevalue_new_with_string(const char *content)
{
	LassoSaml2AttributeValue *object;
	object = g_object_new(LASSO_TYPE_SAML2_ATTRIBUTEVALUE, NULL);
	object->content = g_strdup(content);
	return object;
}
