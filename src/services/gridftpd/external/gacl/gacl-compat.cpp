// -*- indent-tabs-mode: nil -*-

// This file contains functions for backward compatibility with the
// old NorduGrid gacl extensions

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <string>
#include <cstring>

#include <gacl.h>

// These are not in the gridsite header file
extern "C" {
GRSTgaclAcl *GRSTgaclAclParse(xmlDocPtr, xmlNodePtr, GRSTgaclAcl *);
GRSTgaclAcl *GRSTxacmlAclParse(xmlDocPtr, xmlNodePtr, GRSTgaclAcl *);
}

static GRSTgaclAcl *NGACLparse(xmlDocPtr doc)
{
  // convert old NorduGrid voms extension

  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  xmlXPathObjectPtr xpathObj =
    xmlXPathEvalExpression((const xmlChar*)"//entry/voms", xpathCtx);

  if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr) {
    int size = xpathObj->nodesetval->nodeNr;
    for (int i = 0; i < size; ++i)
      if (xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
        std::string vo;
        std::string group;
        std::string role;
        std::string capability;
        bool old = true;
        xmlNodePtr cur = xpathObj->nodesetval->nodeTab[i];
        xmlNodePtr child = cur->xmlChildrenNode;
        while (child) {
          if (!xmlIsBlankNode(child)) {
            if (strcmp((const char*)child->name, "vo") == 0)
              vo = (const char*)xmlNodeGetContent(child);
            else if (strcmp((const char*)child->name, "group") == 0)
              group = (const char*)xmlNodeGetContent(child);
            else if (strcmp((const char*)child->name, "role") == 0)
              role = (const char*)xmlNodeGetContent(child);
            else if (strcmp((const char*)child->name, "capability") == 0)
              capability = (const char*)xmlNodeGetContent(child);
            else if (strcmp((const char*)child->name, "fqan") == 0)
              old = false;
          }
          child = child->next;
        }
        if (old) {
          std::string fqan;
          if (!vo.empty())
            fqan += '/' + vo;
          if (!group.empty())
            fqan += '/' + group;
          if (!role.empty())
            fqan += "/Role=" + role;
          if (!capability.empty())
            fqan += "/Capability=" + capability;
          xmlNodePtr child = cur->xmlChildrenNode;
          while (child) {
            xmlNodePtr next = child->next;
            xmlUnlinkNode(child);
            xmlFreeNode(child);
            child = next;
          }
          xmlNewTextChild(cur, NULL, (const xmlChar*)"fqan",
                          (const xmlChar*)fqan.c_str());
        }
      }
  }

  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);

  // parse converted tree

  xmlNodePtr  cur;
  GRSTgaclAcl    *acl;

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) 
    {
      xmlFreeDoc(doc);
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLparse failed to parse root of ACL");
      return NULL;
    }

  if (!xmlStrcmp(cur->name, (const xmlChar *) "Policy")) 
    { 
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLparse parsing XACML");
      acl=GRSTxacmlAclParse(doc, cur, acl);
    }
  else if (!xmlStrcmp(cur->name, (const xmlChar *) "gacl")) 
    {
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLparse parsing GACL");
      acl=GRSTgaclAclParse(doc, cur, acl);
    }
  else // ACL format not recognised
    {
      xmlFreeDoc(doc);
      return NULL;
    }
    
  xmlFreeDoc(doc);
  return acl;
}


GRSTgaclAcl *NGACLloadAcl(char *filename)
{
  xmlDocPtr   doc;

  GRSTerrorLog(GRST_LOG_DEBUG, "NGACLloadAcl() starting");

  if (filename == NULL) 
    {
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLloadAcl() cannot open a NULL filename");
      return NULL;
    }

  doc = xmlParseFile(filename);
  if (doc == NULL) 
    {
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLloadAcl failed to open ACL file %s", filename);
      return NULL;
    }

  return NGACLparse(doc);
}


// Return ACL that governs the given file or directory (for directories,
// the ACL file is in the directory itself.)
GRSTgaclAcl *NGACLloadAclForFile(char *pathandfile)
{
  char        *path;
  GRSTgaclAcl     *acl;

  path = GRSTgaclFileFindAclname(pathandfile);
  
  if (path != NULL)
    {
      acl = NGACLloadAcl(path);
      free(path);
      return acl;
    }
    
  return NULL;
}


GRSTgaclAcl *NGACLacquireAcl(const char *str)
{
  xmlDocPtr   doc;

  GRSTerrorLog(GRST_LOG_DEBUG, "NGACLacquireAcl() starting");

  doc = xmlParseMemory(str,strlen(str));
  if (doc == NULL) 
    {
      GRSTerrorLog(GRST_LOG_DEBUG, "NGACLacquireAcl failed to parse ACL string");
      return NULL;
    }

  return NGACLparse(doc);
}
