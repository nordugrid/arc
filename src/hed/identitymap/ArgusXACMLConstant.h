#ifndef __ARC_SEC_ARGUSPEP_CONSTANT_H__
#define __ARC_SEC_ARGUSPEP_CONSTANT_H__


namespace ArcSec {

/*
 * XACML Data-types identifiers (XACML 2.0, Appendix B.3) 
 */
static const char XACML_DATATYPE_X500NAME[]= "urn:oasis:names:tc:xacml:1.0:data-type:x500Name";
/**<  XACML data-type @c x500Name identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_RFC822NAME[]= "urn:oasis:names:tc:xacml:1.0:data-type:rfc822Name";
/**<  XACML data-type @c rfc822Name identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_IPADDRESS[]= "urn:oasis:names:tc:xacml:1.0:data-type:ipAddress";
/**<  XACML data-type @c ipAddress identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_DNSNAME[]= "urn:oasis:names:tc:xacml:1.0:data-type:dnsName";
/**<  XACML data-type @c dnsName identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_STRING[]= "http://www.w3.org/2001/XMLSchema#string";
/**<  XACML data-type @c string identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_BOOLEAN[]= "http://www.w3.org/2001/XMLSchema#boolean";
/**<  XACML data-type @c boolean identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_INTEGER[]= "http://www.w3.org/2001/XMLSchema#integer";
/**<  XACML data-type @c integer identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_DOUBLE[]= "http://www.w3.org/2001/XMLSchema#double";
/**<  XACML data-type @c double identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_TIME[]= "http://www.w3.org/2001/XMLSchema#time";
/**<  XACML data-type @c time identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_DATE[]= "http://www.w3.org/2001/XMLSchema#date";
/**<  XACML data-type @c date identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_DATETIME[]= "http://www.w3.org/2001/XMLSchema#dateTime";
/**<  XACML data-type @c dateTime identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_ANYURI[]= "http://www.w3.org/2001/XMLSchema#anyURI";
/**<  XACML data-type @c anyURI identifier (XACML 2.0, B.3) */

static const char XACML_DATATYPE_HEXBINARY[]= "http://www.w3.org/2001/XMLSchema#hexBinary";
/**<  XACML data-type @c hexBinary identifier (XACML 2.0, B.3) */


/*
 * PEP XACML Subject/Attribute identifiers and Subject/\@SubjectCategory values (XACML 2.0, Appendix B)
 */
static const char XACML_SUBJECT_ID[]= "urn:oasis:names:tc:xacml:1.0:subject:subject-id"; 
/**<  XACML Subject/Attribute @c subject-id identifier (XACML 2.0, B.4) */

static const char XACML_SUBJECT_ID_QUALIFIER[]= "urn:oasis:names:tc:xacml:1.0:subject:subject-id-qualifier"; 
/**<  XACML Subject/Attribute @c subject-id-qualifier identifier (XACML 2.0, B.4) */

static const char XACML_SUBJECT_KEY_INFO[]= "urn:oasis:names:tc:xacml:1.0:subject:key-info"; 
/**<  XACML Subject/Attribute @c key-info identifier (XACML 2.0, B.4) */

static const char XACML_SUBJECT_CATEGORY_ACCESS[]= "urn:oasis:names:tc:xacml:1.0:subject-category:access-subject"; 
/**<  XACML Subject/\@SubjectCategory attribute @b access-subject value (XACML 2.0, B.2) */

static const char XACML_SUBJECT_CATEGORY_INTERMEDIARY[]= "urn:oasis:names:tc:xacml:1.0:subject-category:intermediary-subject"; 
/**<  XACML Subject/\@SubjectCategory  attribute @b intermediary-subject value  (XACML 2.0, B.2) */

static const char XACML_SUBJECT_CATEGORY_RECIPIENT[]= "urn:oasis:names:tc:xacml:1.0:subject-category:recipient-subject"; 
/**<  XACML Subject/\@SubjectCategory  attribute @b recipient-subject value (XACML 2.0, B.2) */

static const char XACML_SUBJECT_CATEGORY_CODEBASE[]= "urn:oasis:names:tc:xacml:1.0:subject-category:codebase"; 
/**<  XACML Subject/\@SubjectCategory  attribute @b codebase value (XACML 2.0, B.2) */

static const char XACML_SUBJECT_CATEGORY_REQUESTING_MACHINE[]= "urn:oasis:names:tc:xacml:1.0:subject-category:requesting-machine"; 
/**<  XACML Subject/\@SubjectCategory  attribute @b requesting-machine value (XACML 2.0, B.2) */


/*
 * XACML Resource/Attribute Identifiers (XACML 2.0, Appendix B)
 */
static const char XACML_RESOURCE_ID[]= "urn:oasis:names:tc:xacml:1.0:resource:resource-id"; 
/**<  XACML Resource/Attribute @b resource-id identifier (XACML 2.0, B.6) */


/*
 * XACML Action/Attribute Identifiers (XACML 2.0, Appendix B)
 */
static const char XACML_ACTION_ID[]= "urn:oasis:names:tc:xacml:1.0:action:action-id"; 
/**<  XACML Action/Attribute @b action-id identifier (XACML 2.0, B.7) */


/*
 * PEP XACML Environment/Attribute identifiers (XACML 2.0, Appendix B)
 */
static const char XACML_ENVIRONMENT_CURRENT_TIME[]= "urn:oasis:names:tc:xacml:1.0:environment:current-time"; 
/**<  XACML Environment/Attribute @c current-time identifier (XACML 2.0, B.8) */

static const char XACML_ENVIRONMENT_CURRENT_DATE[]= "urn:oasis:names:tc:xacml:1.0:environment:current-date"; 
/**<  XACML Environment/Attribute @c current-date identifier (XACML 2.0, B.8) */

static const char XACML_ENVIRONMENT_CURRENT_DATETIME[]= "urn:oasis:names:tc:xacml:1.0:environment:current-dateTime"; 
/**<  XACML Environment/Attribute @c current-dateTime identifier (XACML 2.0, B.8) */


/*
 * PEP XACML StatusCode/\@Value values (XACML 2.0, B.9)
 */
static const char XACML_STATUSCODE_OK[]= "urn:oasis:names:tc:xacml:1.0:status:ok"; 
/**< XACML StatusCode/\@Value attribute @c ok value (XACML 2.0, B.9) */

static const char XACML_STATUSCODE_MISSINGATTRIBUTE[]= "urn:oasis:names:tc:xacml:1.0:status:missing-attribute"; 
/**< XACML StatusCode/\@Value attribute @c missing-attribute value (XACML 2.0, B.9) */

static const char XACML_STATUSCODE_SYNTAXERROR[]= "urn:oasis:names:tc:xacml:1.0:status:syntax-error"; 
/**< XACML StatusCode/\@Value attribute @c syntax-error value (XACML 2.0, B.9) */

static const char XACML_STATUSCODE_PROCESSINGERROR[]= "urn:oasis:names:tc:xacml:1.0:status:processing-error"; 
/**< XACML StatusCode/\@Value attribute @c processing-error value (XACML 2.0, B.9) */






/** @defgroup CommonAuthZ Common XACML Authorization Profile
 *  @ingroup Profiles
 *
 * EMI Common XACML Authorization Profile (Version 1.1)
 * @version 1.1
 *
 * Document: http://dci-sec.org/xacml/profile/common-authz/1.1
 *
 * Profile version, XACML attribute identifiers and obligation identifiers for the Common XACML
 * Authorization Profile.
 * @{
 */
/*
 * Common XACML Authorization Profile version
 */
static const char XACML_COMMONAUTHZ_PROFILE_1_1[]= "http://dci-sec.org/xacml/profile/common-authz/1.1"; /**< Common XACML Authorization Profile version 1.1 value. See attribute #XACML_DCISEC_ATTRIBUTE_PROFILE_ID [Common XACML Authorization Profile v.1.1, 4.1.1] */

static const char XACML_DCISEC_ATTRIBUTE_PROFILE_ID[]= "http://dci-sec.org/xacml/attribute/profile-id"; 
/**< Common XACML Authorization Profile Environment/Attribute @b profile-id identifier. Datatype: anyURI, see #XACML_DATATYPE_ANYURI [Common XACML Authorization Profile v.1.1, 4.1.1] */

static const char XACML_DCISEC_ATTRIBUTE_SUBJECT_ISSUER[]= "http://dci-sec.org/xacml/attribute/subject-issuer"; 
/**< Common XACML Authorization Profile Subject/Attribute @b subject-issuer identifier. Datatype: x500Name, see #XACML_DATATYPE_X500NAME [Common XACML Authorization Profile v.1.1, 4.2.3] */

static const char XACML_DCISEC_ATTRIBUTE_VIRTUAL_ORGANIZATION[]= "http://dci-sec.org/xacml/attribute/virtual-organization"; 
/**< Common XACML Authorization Profile Subject/Attribute @b virtual-organization  (VO) identifier. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 4.2.4] */

static const char XACML_DCISEC_ATTRIBUTE_GROUP[]= "http://dci-sec.org/xacml/attribute/group"; 
/**< Common XACML Authorization Profile Subject/Attribute @b group (VO) identifier. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 4.2.6] */

static const char XACML_DCISEC_ATTRIBUTE_GROUP_PRIMARY[]= "http://dci-sec.org/xacml/attribute/group/primary"; 
/**< Common XACML Authorization Profile Subject/Attribute @b group/primary (VO) identifier. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 4.2.5] */

static const char XACML_DCISEC_ATTRIBUTE_ROLE[]= "http://dci-sec.org/xacml/attribute/role"; 
/**< Common XACML Authorization Profile Subject/Attribute @b role (VO) identifier. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 4.2.8] */

static const char XACML_DCISEC_ATTRIBUTE_ROLE_PRIMARY[]= "http://dci-sec.org/xacml/attribute/role/primary"; 
/**< Common XACML Authorization Profile Subject/Attribute @b role/primary (VO) identifier. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 4.2.7] */

static const char XACML_DCISEC_ATTRIBUTE_RESOURCE_OWNER[]= "http://dci-sec.org/xacml/attribute/resource-owner"; 
/**< Common XACML Authorization Profile Resource/Attribute @b resource-owner identifier. Datatype: x500Name, see #XACML_DATATYPE_X500NAME [Common XACML Authorization Profile v.1.1, 4.3.2] */

static const char XACML_DCISEC_ACTION_NAMESPACE[]= "http://dci-sec.org/xacml/action"; 
/**< Namespace for the Common XACML Authorization Profile Action values. See attribute #XACML_ACTION_ID [Common XACML Authorization Profile v.1.1, 4.4.1] */

static const char XACML_DCISEC_ACTION_ANY[]= "http://dci-sec.org/xacml/action/ANY"; 
/**< Common XACML Authorization Profile Action @b ANY value. See attribute #XACML_ACTION_ID [Common XACML Authorization Profile v.1.1, 4.4.1] */

static const char XACML_DCISEC_OBLIGATION_MAP_LOCAL_USER[]= "http://dci-sec.org/xacml/obligation/map-local-user"; 
/**< Common XACML Authorization Profile Obligation @b map-local-user identifier [Common XACML Authorization Profile v.1.1, 5.1.1] */

static const char XACML_DCISEC_OBLIGATION_MAP_POSIX_USER[]= "http://dci-sec.org/xacml/obligation/map-local-user/posix"; 
/**< Common XACML Authorization Profile Obligation @b map-local-user/posix identifier. See attribute assignments #XACML_DCISEC_ATTRIBUTE_USER_ID, #XACML_DCISEC_ATTRIBUTE_GROUP_ID_PRIMARY and #XACML_DCISEC_ATTRIBUTE_GROUP_ID [Common XACML Authorization Profile v.1.1, 5.1.2] */

static const char XACML_DCISEC_ATTRIBUTE_USER_ID[]= "http://dci-sec.org/xacml/attribute/user-id"; 
/**< Common XACML Authorization Profile Obligation/AttributeAssignment @b user-id (username) identifier, see obligation #XACML_DCISEC_OBLIGATION_MAP_POSIX_USER. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 5.2.1] */

static const char XACML_DCISEC_ATTRIBUTE_GROUP_ID[]= "http://dci-sec.org/xacml/attribute/group-id"; 
/**< Common XACML Authorization Profile Obligation/AttributeAssignment @b group-id (user group name) identifier, see obligation #XACML_DCISEC_OBLIGATION_MAP_POSIX_USER. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 5.2.3] */

static const char XACML_DCISEC_ATTRIBUTE_GROUP_ID_PRIMARY[]= "http://dci-sec.org/xacml/attribute/group-id/primary"; 
/**< Common XACML Authorization Profile Obligation/AttributeAssignment @b group-id/primary (primary group name) identifier, see obligation #XACML_DCISEC_OBLIGATION_MAP_POSIX_USER. Datatype: string, see #XACML_DATATYPE_STRING [Common XACML Authorization Profile v.1.1, 5.2.2] */

/** @} */



/** @defgroup GridWNAuthZ Grid Worker Node Authorization Profile
 *  @ingroup Profiles
 *
 * XACML Grid Worker Node Authorization Profile (Version 1.0)
 * @version 1.0
 *
 * Document: https://edms.cern.ch/document/1058175/1.0
 *
 * Profile version, XACML Attribute identifiers, XACML Obligation identifiers, and datatypes for the Grid WN AuthZ Profile.
 * @{
 */
/*
 * XACML Grid WN AuthZ Profile version
 */
static const char XACML_GRIDWN_PROFILE_VERSION[]= "http://glite.org/xacml/profile/grid-wn/1.0"; /**< XACML Grid WN AuthZ Profile version value [XACML Grid WN AuthZ 1.0, 3.1.1] */

/*
 * XACML Grid WN AuthZ Attribute identifiers
 */

static const char XACML_GLITE_ATTRIBUTE_PROFILE_ID[]= "http://glite.org/xacml/attribute/profile-id"; 
/**< XACML Grid WN AuthZ Environment/Attribute @b profile-id identifier. Datatype: anyURI, see #XACML_DATATYPE_ANYURI [XACML Grid WN AuthZ 1.0, 3.1.1] */

static const char XACML_GLITE_ATTRIBUTE_SUBJECT_ISSUER[]= "http://glite.org/xacml/attribute/subject-issuer"; 
/**< XACML Grid WN AuthZ Subject/Attribute @b subject-issuer identifier. Datatype: x500Name, see #XACML_DATATYPE_X500NAME [XACML Grid WN AuthZ 1.0, 3.2.2 and 4.2] */

static const char XACML_GLITE_ATTRIBUTE_VOMS_ISSUER[]= "http://glite.org/xacml/attribute/voms-issuer"; 
/**< XACML Grid WN AuthZ Subject/Attribute @b voms-issuer identifier [XACML Grid WN AuthZ 1.0, 4.6.2] */

static const char XACML_GLITE_ATTRIBUTE_VIRTUAL_ORGANIZATION[]= "http://glite.org/xacml/attribute/virtual-organization"; 
/**< XACML Grid WN AuthZ Subject/Attribute @b virutal-organization identifier. Datatype: string, see #XACML_DATATYPE_STRING [XACML Grid WN AuthZ 1.0, 3.2.3 and 4.3] */

static const char XACML_GLITE_ATTRIBUTE_FQAN[]= "http://glite.org/xacml/attribute/fqan"; 
/**< XACML Grid WN AuthZ Subject/Attribute @b fqan identifier. Datatype: FQAN, see #XACML_GLITE_DATATYPE_FQAN [XACML Grid WN AuthZ 1.0, 3.2.4 and 4.4] */

static const char XACML_GLITE_ATTRIBUTE_FQAN_PRIMARY[]= "http://glite.org/xacml/attribute/fqan/primary"; 
/**< XACML Grid WN AuthZ Subject/Attribute @b fqan/primary identifier. Datatype: FQAN, see #XACML_GLITE_DATATYPE_FQAN [XACML Grid WN AuthZ 1.0, 3.2.5] */

static const char XACML_GLITE_ATTRIBUTE_PILOT_JOB_CLASSIFIER[]= "http://glite.org/xacml/attribute/pilot-job-classifer"; 
/**< XACML Grid WN AuthZ Action/Attribute @b pilot-job-classifer identifier. Datatype: string, see #XACML_DATATYPE_STRING [XACML Grid WN AuthZ 1.0, 3.4.2] */

static const char XACML_GLITE_ATTRIBUTE_USER_ID[]= "http://glite.org/xacml/attribute/user-id"; 
/**< XACML Grid WN AuthZ Obligation/AttributeAssignment @b user-id identifier [XACML Grid WN AuthZ 1.0, 3.6.1] */

static const char XACML_GLITE_ATTRIBUTE_GROUP_ID[]= "http://glite.org/xacml/attribute/group-id"; 
/**< XACML Grid WN AuthZ Obligation/AttributeAssignment @b group-id identifier [XACML Grid WN AuthZ 1.0, 3.6.2] */

static const char XACML_GLITE_ATTRIBUTE_GROUP_ID_PRIMARY[]= "http://glite.org/xacml/attribute/group-id/primary"; 
/**< XACML Grid WN AuthZ Obligation/AttributeAssignment @b group-id/primary identifier [XACML Grid WN AuthZ 1.0, 3.6.3] */

static const char XACML_GLITE_OBLIGATION_LOCAL_ENVIRONMENT_MAP[]= "http://glite.org/xacml/obligation/local-environment-map"; 
/**< XACML Grid WN AuthZ Obligation @b local-environment-map identifier [XACML Grid WN AuthZ 1.0, 3.5.1] */

static const char XACML_GLITE_OBLIGATION_LOCAL_ENVIRONMENT_MAP_POSIX[]= "http://glite.org/xacml/obligation/local-environment-map/posix"; 
/**< XACML Grid WN AuthZ Obligation @b local-environment-map/posix identifier [XACML Grid WN AuthZ 1.0, 3.5.2] */

static const char XACML_GLITE_DATATYPE_FQAN[]= "http://glite.org/xacml/datatype/fqan"; 
/**< XACML Grid WN AuthZ @b fqan datatype [XACML Grid WN AuthZ 1.0, 3.7.1] */



/** @defgroup AuthzInterop Authorization Interoperability Profile
 *  @ingroup Profiles *
 * XACML Attribute and Obligation Profile for Authorization Interoperability in Grids (Version 1.1) * @version 1.1
 * * Document: https://edms.cern.ch/document/929867/2
 * * XACML Subject's Attribute identifiers, XACML Obligation and Obligation's AttributeAssignment
 * identifiers for the AuthZ Interop Profile * @{
 */
/*
 * XACML Authz Interop Subject/Attribute identifiers (XACML Authz Interop Profile 1.1)
 */

static const char XACML_AUTHZINTEROP_SUBJECT_X509_ID[]= "http://authz-interop.org/xacml/subject/subject-x509-id"; 
/**<  XACML AuthZ Interop Subject/Attribute @b subject-x509-id identifier (Datatype: string, OpenSSL format) */

static const char XACML_AUTHZINTEROP_SUBJECT_X509_ISSUER[]= "http://authz-interop.org/xacml/subject/subject-x509-issuer"; 
/**<  XACML AuthZ Interop Subject/Attribute @b subject-x509-issuer identifier (Datatype: string, OpenSSL format) */

static const char XACML_AUTHZINTEROP_SUBJECT_VO[]= "http://authz-interop.org/xacml/subject/vo"; 
/**<  XACML AuthZ Interop Subject/Attribute @b vo identifier (Datatype: string) */

static const char XACML_AUTHZINTEROP_SUBJECT_CERTCHAIN[]= "http://authz-interop.org/xacml/subject/cert-chain"; 
/**<  XACML AuthZ Interop Subject/Attribute @b cert-chain identifier (Datatype: base64Binary) */

static const char XACML_AUTHZINTEROP_SUBJECT_VOMS_FQAN[]= "http://authz-interop.org/xacml/subject/voms-fqan"; 
/**<  XACML AuthZ Interop Subject/Attribute @b voms-fqan identifier (Datatype: string) */

static const char XACML_AUTHZINTEROP_SUBJECT_VOMS_PRIMARY_FQAN[]= "http://authz-interop.org/xacml/subject/voms-primary-fqan"; 
/**<  XACML AuthZ Interop Subject/Attribute @b voms-primary-fqan identifier (Datatype: string) */


/*
 * XACML Authz Interop Obligation and Obligation/AttributeAssignment identifiers (XACML Authz Interop Profile 1.1)
 */

static const char XACML_AUTHZINTEROP_OBLIGATION_UIDGID[]= "http://authz-interop.org/xacml/obligation/uidgid"; 
/**<  XACML AuthZ Interop Obligation @b uidgid identifier (XACML Authz Interop: UID GID) */

static const char XACML_AUTHZINTEROP_OBLIGATION_SECONDARY_GIDS[]= "http://authz-interop.org/xacml/obligation/secondary-gids"; 
/**<  XACML AuthZ Interop Obligation @b secondary-gids identifier (XACML Authz Interop: Multiple Secondary GIDs) */

static const char XACML_AUTHZINTEROP_OBLIGATION_USERNAME[]= "http://authz-interop.org/xacml/obligation/username"; 
/**<  XACML AuthZ Interop Obligation @b username identifier (XACML Authz Interop: Username) */

static const char XACML_AUTHZINTEROP_OBLIGATION_AFS_TOKEN[]= "http://authz-interop.org/xacml/obligation/afs-token"; 
/**<  XACML AuthZ Interop Obligation @b afs-token identifier (XACML Authz Interop: AFS Token) */

static const char XACML_AUTHZINTEROP_OBLIGATION_ATTR_POSIX_UID[]= "http://authz-interop.org/xacml/attribute/posix-uid"; 
/**<  XACML AuthZ Interop Obligation/AttributeAssignment @b posix-uid identifier (C Datatype: string, must be converted to integer) */

static const char XACML_AUTHZINTEROP_OBLIGATION_ATTR_POSIX_GID[]= "http://authz-interop.org/xacml/attribute/posix-gid"; 
/**<  XACML AuthZ Interop Obligation/AttributeAssignment @b posix-gid identifier (C Datatype: string, must be converted to integer) */

static const char XACML_AUTHZINTEROP_OBLIGATION_ATTR_USERNAME[]= "http://authz-interop.org/xacml/attribute/username"; 
/**<  XACML AuthZ Interop Obligation/AttributeAssignment @b username identifier (Datatype: string) */

static const char XACML_AUTHZINTEROP_OBLIGATION_ATTR_AFS_TOKEN[]= "http://authz-interop.org/xacml/attribute/afs-token"; 
/**<  XACML AuthZ Interop Obligation/AttributeAssignment @b afs-token identifier (Datatype: base64Binary) */

/** @} */



/*
 * PEP XACML Result/Decision element constants.
 */
typedef enum xacml_decision {
        XACML_DECISION_DENY = 0, /**< Decision is @b Deny */
        XACML_DECISION_PERMIT, /**< Decision is @b Permit */
        XACML_DECISION_INDETERMINATE, /**< Decision is @b Indeterminate, the PEP was unable to evaluate the request */
        XACML_DECISION_NOT_APPLICABLE /**< Decision is @b NotApplicable, the PEP does not have any policy that applies to the request */
} xacml_decision_t;

/*
 * PEP XACML Obligation/\@FulfillOn attribute constants.
 */
typedef enum xacml_fulfillon {
        XACML_FULFILLON_DENY = 0, /**< Fulfill the Obligation on @b Deny decision */
        XACML_FULFILLON_PERMIT /**< Fulfill the Obligation on @b Permit decision */
} xacml_fulfillon_t;


} // namespace ArcSec

#endif /* __ARC_SEC_ARGUSPEP_CONSTANT_H__ */
