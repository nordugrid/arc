#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XmlDatabase.h"

namespace Arc
{

XmlDatabase::XmlDatabase(const std::string &db_path, 
                         const std::string &db_name)
{
    container_ = new Arc::XmlContainer(db_path, db_name);
}

XmlDatabase::~XmlDatabase(void)
{
    delete container_;
}

void
XmlDatabase::put(const std::string &name, const std::string &content)
{
    container_->put(name, content);
}

void
XmlDatabase::put(const std::string &name, Arc::XMLNode &doc)
{
    std::string content;
    doc.GetDoc(content);
    container_->put(name, content);
}

void
XmlDatabase::get(const std::string &name, Arc::XMLNode &doc)
{
    std::string content = container_->get(name);
    if (!content.empty()) {
        Arc::XMLNode nn(content);
        nn.New(doc);
    }
}   

void
XmlDatabase::del(const std::string &name)
{
    container_->del(name);
}

Arc::XMLNodeList
XmlDatabase::query(const std::string &name, const std::string &q)
{
    Arc::XMLNode node;
    get(name, node);
    return node.XPathLookup(q, node.Namespaces());
}

void
XmlDatabase::queryAll(const std::string &q, 
                      std::map<std::string, Arc::XMLNodeList> &result)
{
    std::vector<std::string> doc_names = container_->get_doc_names();

    for (int i = 0; i < doc_names.size(); i++) {
        Arc::XMLNodeList r = query(doc_names[i], q);
        result[doc_names[i]] = r;
    }
}

void
XmlDatabase::update(const std::string &name, const std::string &query,
                    Arc::XMLNode &new_value)
{
    container_->start_update();
    // get content
    // parse content to XMLNode
    // run query
    // replace query result nodes with new_value
    // get the new document as string
    // put the content back to container
    container_->end_update();
}

} // namespace Arc
