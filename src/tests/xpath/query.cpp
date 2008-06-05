#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/random.h>
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <arc/XMLNode.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if(argc < 3) return -1;
    int filenum = atoi(argv[1]);
    int attrnum = atoi(argv[2]);

    for (int i = 0; i < filenum; i++) { 
	    // load content of file
        char fname[200];
        sprintf(fname, "file%d.xml", i);
        
        std::string xml_str = "";
        std::string str;
	    std::ifstream f(fname);
	    while (f >> str) {
	        xml_str.append(str);
	        xml_str.append(" ");
	    }
	    f.close();
	    Arc::XMLNode doc(xml_str);
	    Arc::NS ns;
	Glib::Rand r; 
        int n = r.get_int_range(0, attrnum);
	char query[200];
        sprintf(query, "//AttributeName%d", n);
        
        std::cout << "Query: " << query << std::endl;
        std::list<Arc::XMLNode> result = doc.XPathLookup(query, ns);
	    std::list<Arc::XMLNode>::iterator it;
	
	    for (it = result.begin(); it != result.end(); it++) {
	        std::cout << fname << ":" << (*it).Name() << ":" << std::string(*it) << std::endl;
	    }
    }
    return 0;
}
