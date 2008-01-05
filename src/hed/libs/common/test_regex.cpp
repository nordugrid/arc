#include <ArcRegex.h>
#include <string>
#include <list>
#include <iostream>

int main(void)
{
    std::string str="[@id=\"ahssgf0afhha0-adja-dkkda\"]";
    std::string r = "([a-zA-Z0-9_\\\\-]*)=\"([a-zA-Z0-9_\\\\-]*)\"";
    Arc::RegularExpression rx(r);
    if (rx.isOk()) {
        std::list<std::string> ump, mp;
        rx.match(str, ump, mp);
        std::list<std::string>::iterator it;
        for (it = mp.begin(); it != mp.end(); it++) {
            std::cout << (*it) << std::endl;
        }     
    } else {
        std::cout << "Invalid regexp" << std::endl;
    }
    return 0;
}
