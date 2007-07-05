#include "Request.h"

static void make_request(XMLNode* node){
  if(!MatchXMLName(node, "Request"))
    logger.msg(Error, "The root node of request is not <Request>");

  for(int n=0;;++n){
    XMLNode nd = node->Child(n);
    if(!cn) break;
    if(MatchXMLName(nd, "RequestItem")){
      RequestItem* reqitem;
      reqitem->get_instance();//
      for(int m=0;;++m){
      	XMLNode rnd=nd.Child(m);
      	if(MatchXMLName(rnd, "Subject")){
       	  for(int k=0;k<rnd.AttributeSize();++k){
            std::string attr=rnd.Attribute(k);
            Attribute* attribute;
            attribute->get_instance(attr.c_str());  //TODO
            subjects.insert(attribute);
	  }
        continue;       
        }
      	XMLNode snd=nd->Child(m);
      	if(MatchXMLName(snd, "Subject")){
       	  XMLNode attr=snd["Attribute"];
          if(attr.empty()){
	    logger.msg(Error, "Subject has no Attribute defined");
	    continue;
	  }
	  logger.msg(DEBUG, attr);

        if(MatchXMLName(rnd, "Resource")){
          for(int k=0;k<rnd.AttributeSize();++k){
            std::string attr=rnd.Attribute(k);
            Attribute* attribute;
            attribute->get_instance(attr.c_str());  //TODO
            resources.insert(attribute);
          }
        continue; 
        }
	  Attribute* attribute;
	  attribute->get_instance(attr.c_str());  //TODO
	  subjects.insert(attribute);
	  continue;
	}
      }
    }
    if(MatchXMLName(nd, "Object")){
       std::string attr["Attribute"];
       if(attr.empty()){
          logger.msg(Error, "Object has no Attribute defined");
          continue;
       }
       logger.msg(DEBUG, attr);

        if(MatchXMLName(rnd, "Action")){
          for(int k=0;k<rnd.AttributeSize();++k){
            std::string attr=rnd.Attribute(k);
            Attribute* attribute;
            attribute->get_instance(attr.c_str());  //TODO
            actions.insert(attribute);
          }
        continue;
        }
       Attribute* attribute;
       attribute->get_instance(attr.c_str());  //TODO
       subjects.insert(attribute);
       continue;
    }
    if(MatchXMLName(nd, "Action")){
       std::string attr["Attribute"];
       if(attr.empty()){
          logger.msg(Error, "Action has no Attribute defined");
          continue;
       }
       logger.msg(DEBUG, attr);

        if(MatchXMLName(rnd, "Context")){
          for(int k=0;k<rnd.AttributeSize();++k){
            std::string attr=rnd.Attribute(k);
            Attribute* attribute;
            attribute->get_instance(attr.c_str());  //TODO
            contexts.insert(attribute);
          }
        continue;
        }
      }
      logger.msg(LogMessage(WARNING, "Unknown element \""+
                              nd.Name()+"\" - ignoring."));
       Attribute* attribute;
       attribute->get_instance(attr.c_str());  //TODO
       subjects.insert(attribute);
       continue;
    }
    if(MatchXMLName(nd, "Context")){
       std::string attr["Attribute"];
       if(attr.empty()){
          logger.msg(Error, "Context has no Attribute defined");
          continue;
       }
       logger.msg(DEBUG, attr);

       Attribute* attribute;
       attribute->get_instance(attr.c_str());  //TODO
       subjects.insert(attribute);
       continue;
    }
    logger.msg(WARNING, "Unknown element \"%s\" - ignoring",
	       nd.Name().c_str()));
    }
  }
}

Request::get_instance(const char* filename){
  std::string xml_str = "";
  std::string str;
  std::ifstream f(filename);
  // load content of file
  while (f >> str) {
      xml_str.append(str);
      xml_str.append(" ");
  }
  f.close();
  // create XMLNode
  Arc::XMLNode node(xml_str);
  get_instance(&node);
}

Request::get_instance(Arc::XMLNode* root){
  make_request(root);
}
