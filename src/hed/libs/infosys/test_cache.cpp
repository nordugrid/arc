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
    <Jobs>\
        <Job id=\"1\">\
            <Data>1</Data>\
        </Job>\
        <Job id=\"2\">\
            <Data>2</Data>\
        </Job>\
    </Jobs>\
    <Owner>\
        <Name>Unknown</Name>\
    </Owner>\
    <Level1>\
        <Level2>\
            <Level3>\
                <Level4>1</Level4>\
            </Level3>\
        </Level2>\
    </Level1>\
</InfoDoc>\
";

std::string doc2 = "\
<?xml version=\"1.0\"?>\
<Resource>\
    <Memory>2000</Memory>\
    <Performance>Turltle-like2</Performance>\
</Resource>\
";

std::string doc31 = "\
<?xml version=\"1.0\"?>\
<Memory>3000</Memory>\
";

std::string doc32 = "\
<?xml version=\"1.0\"?>\
<Job id=\"1\"><Data>2</Data><Data2>22</Data2></Job>\
";

std::string doc33 = "\
<?xml version=\"1.0\"?>\
<Foo><Bar>2</Bar></Foo>\
";

std::string doc41 = "\
<?xml version=\"1.0\"?>\
<Level3><Level4>33</Level4></Level3>\
";

std::string doc42 = "\
<?xml version=\"1.0\"?>\
<Level4>3</Level4>\
";

std::string doc5 = "\
<?xml version=\"1.0\"?>\
<Data>22</Data>\
";

int main(void)
{
    Arc::XMLNode xml1(doc1);
    Arc::XMLNode xml2(doc1);
    Arc::Config cfg("./service.xml");
    Arc::InfoCache cache(cfg, std::string("test_service"));
    bool ret;
    
    ret = cache.Set("/", xml1);
    std::cout << ret << "(" << true << ")" << std::endl;
    Arc::XMLNodeContainer c1;
    std::string s;
    cache.Get("/", c1);
    c1[0].GetXML(s);
    std::cout << s << std::endl; 
    
    cache.Set("/InfoDoc", xml2);  
    std::cout << ret << "(" << true << ")" << std::endl;
    Arc::XMLNodeContainer c2;
    cache.Get("/InfoDoc", c2);
    c2[0].GetXML(s);
    std::cout << s << std::endl; 
    
    Arc::XMLNode xml3(doc2);
    ret = cache.Set("/InfoDoc/Resource", xml3);
    std::cout << ret << "(" << true << ")" << std::endl;
    Arc::XMLNodeContainer c3;
    cache.Get("/InfoDoc/Resource", c3);
    c3[0].GetXML(s);
    std::cout << s << std::endl; 

    Arc::XMLNode xml41(doc31);
    ret = cache.Set("/InfoDoc/Resource/Memory", xml41);
    std::cout << ret << "(" << true << ")" << std::endl;
    Arc::XMLNodeContainer c4;
    cache.Get("/InfoDoc/Resource/Memory", c4);
    c4[0].GetXML(s);
    std::cout << s << std::endl; 
    
    Arc::XMLNode xml42(doc32);
    ret = cache.Set("/InfoDoc/Jobs/Job", xml42);
    std::cout << ret << "(" << true << ")" << std::endl;
    
    Arc::XMLNode xml43(doc33);
    ret = cache.Set("/InfoDoc/Resource/Foo", xml43);
    std::cout << ret << "(" << true << ")" << std::endl;
    
    Arc::XMLNode xml51(doc41);
    ret = cache.Set("/InfoDoc/Level1/Level2/Level3", xml51);
    std::cout << ret << "(" << true << ")" << std::endl;
    
    Arc::XMLNode xml52(doc42);
    ret = cache.Set("/InfoDoc/Level1/Level2/Level3/Level4", xml52);
    std::cout << ret << "(" << true << ")" << std::endl;

    Arc::XMLNode xml6(doc5);
    ret = cache.Set("/InfoDoc/Jobs/Job[@id=\"1\"]/Data", xml6);
    std::cout << ret << "(" << true << ")" << std::endl;
    
    Arc::XMLNodeContainer c5;
    cache.Get("/InfoDoc/Jobs/Job[@id=\"1\"]", c5);
    c5[0].GetXML(s);
    std::cout << s << std::endl; 
    
#if 0
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
#endif
    return 0;
}
