#include <string>
#include <arc/ArcConfig.h>
#include <arc/infosys/InfoCache.h>

std::string doc1 = "\
<?xml version=\"1.0\"?>\
<InfoDoc>\
    <Resource>\
        <Memory>A lot</Memory>\
        <Performance>Turltle-like</Performance>\
    </Resource>\
    <Owner>\
        <Name>Unknown</Name>\
    </Owner>\
    <Resource>\
        <Memory>640kb enough for everyone</Memory>\
        <Performance>Quantum computer</Performance>\
    </Resource>\
</InfoDoc>\
";

std::string doc2 = "\
<?xml version=\"1.0\"?>\
<InfoDoc>\
    <Resource>\
        <Memory>2000</Memory>\
        <Performance>Turltle-like</Performance>\
    </Resource>\
    <Owner>\
        <Name>Unknown</Name>\
    </Owner>\
    <Resource>\
        <Memory>640kb enough for everyone</Memory>\
        <Performance>Quantum computer</Performance>\
    </Resource>\
</InfoDoc>\
";

int main(void)
{
    Arc::Config cfg("./service.xml");
    /* cfg.print();
    std::string root = std::string(cfg["InformationSystem"]["CacheRoot"]);
    std::cout << root << std::endl;
*/
    Arc::InfoCache cache(cfg, "test_service");
    cache.Set("1", doc1);
    std::cout << "Set 1" << std::endl;
    cache.Set("2", doc2);
    std::cout << "Set 2" << std::endl;
    std::string r = cache.Get("1");
    std::cout << "Get 1" << std::endl;
    std::cout << r << std::endl;
    std::list<Arc::XMLNode> result;
    cache.Query("1", "//Memory", result);
    std::list<Arc::XMLNode>::iterator it;
    std::cout << "test_cache" << std::endl;
    for (it = result.begin(); it != result.end(); it++) {
        std::cout << (*it).Name() << ":" << std::string(*it) << std::endl;
    } 
    return 0;
}
