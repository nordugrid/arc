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
/*    Arc::XMLNodeContainer c;
    Arc::XMLNode doc(doc1);
    Arc::NS ns;
    std::list<Arc::XMLNode> xresult = doc.XPathLookup("//Memory", ns);
    std::list<Arc::XMLNode>::iterator it;
    c.AddNew(xresult);
    std::cout << "xresult" << std::endl;
    for (it = xresult.begin(); it != xresult.end(); it++) {
        std::cout << it->Name() << std::endl;
    }
    std::cout << "container size:" << c.Size() << std::endl;
    for (int i = 0; i < c.Size(); i++) {
        std::cout << c[i].Name() << std::endl;
    }
*/
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
    Arc::XMLNodeContainer result;
    cache.Query("1", "//Memory", result);
    std::cout << "test_cache" << std::endl;
    for (int it = 0; it < result.Size(); it++) {
        std::cout << result[it].Name() << ":" << std::string(result[it]) << std::endl;
    }
    Arc::XMLNodeContainer result2;
    cache.Query("any", "//Memory", result2);
    for (int it = 0; it < result2.Size(); it++) {
        std::cout << result2[it].Name() << ":" << std::string(result2[it]) << std::endl;
    }

    return 0;
}
