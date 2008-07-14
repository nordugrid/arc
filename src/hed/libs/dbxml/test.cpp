#include "XmlDatabase.h"

int main(void)
{
    Arc::XmlDatabase db("db", "test");
    std::string content = "<?xml version=\"1.0\"?><foo>bar</foo>";
    db.put("1", content);
    Arc::XMLNode n;
    db.get("1", n);
    std::string s;
    n.GetDoc(s);
    std::cout << s << std::endl;
    db.del("1");
    return 0;
}

