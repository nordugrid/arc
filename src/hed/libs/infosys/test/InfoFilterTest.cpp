#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/Utils.h>

#include <arc/infosys/InfoFilter.h>

using namespace Arc;

class InfoFilterTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(InfoFilterTest);
  CPPUNIT_TEST(TestInfoFilter);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestInfoFilter();
};

static void add_libs(std::string& paths,const std::string& curpath) {
  std::string fname = Glib::build_filename(curpath,".libs");
  if(file_test(fname, Glib::FILE_TEST_IS_DIR)) {
    paths+=":"+fname;
  };
  Glib::Dir dir(curpath);
  for(;;) {
    fname = dir.read_name();
    if(fname.empty()) break;
    fname=Glib::build_filename(curpath,fname);
    if(fname == ".libs") {
    } else if(file_test(fname, Glib::FILE_TEST_IS_DIR)) {
      add_libs(paths,fname);
    };
  };
}

void InfoFilterTest::setUp() {
  std::string paths;
  std::string toppath = Glib::get_current_dir();
  // Find top source directory by looking for configure file
  for(int n=0;n<15;++n) {
    std::string fname = Glib::build_filename(toppath, "configure");
    if(file_test(fname, Glib::FILE_TEST_IS_REGULAR)) {
      // Go to all .libs directories
      add_libs(paths,toppath);
      break;
    };
    toppath = Glib::path_get_dirname(toppath);
  };
  Arc::SetEnv("ARC_PLUGIN_PATH",paths);
}


void InfoFilterTest::tearDown() {
}


class TestSecAttr: public Arc::SecAttr {
 public:
  TestSecAttr(const char* id):id_(id) { };
  virtual ~TestSecAttr(void) { };
  virtual operator bool(void) const { return true; };
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
 protected:
  virtual bool equal(const SecAttr &b) const { return false; };
 private:
  std::string id_;
};


static void add_subject_attribute(XMLNode item,const std::string& subject,const
char* id) {
   XMLNode attr = item.NewChild("ra:SubjectAttribute");
   attr=subject; attr.NewAttribute("Type")="string";
   attr.NewAttribute("AttributeId")=id;
}

bool TestSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format != ARCAuth) return false;
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  val.Namespaces(ns); val.Name("ra:Request");
  XMLNode item = val.NewChild("ra:RequestItem");
  XMLNode subj = item.NewChild("ra:Subject");
  XMLNode attr = subj.NewChild("ra:SubjectAttribute");
  attr=id_;
  attr.NewAttribute("Type")="string";
  attr.NewAttribute("AttributeId")="urn:testID";
  return true;
}


void InfoFilterTest::TestInfoFilter() {

  // Service description document
  Arc::XMLNode infodoc("\
<?xml version=\"1.0\"?>\n\
<InfoDoc xmlns=\"urn:info\">\n\
  <Resource>\n\
    <Memory>A lot</Memory>\n\
    <Performance>Turltle-like</Performance>\n\
  </Resource>\n\
  <Owner>\n\
      <Name>Unknown</Name>\n\
  </Owner>\n\
  <Resource>\n\
      <Memory>640kb enough for everyone</Memory>\n\
      <Performance>Quantum computer</Performance>\n\
  </Resource>\n\
</InfoDoc>\n");

  // Policies
  Arc::XMLNode policy1("\
<?xml version=\"1.0\"?>\n\
<InfoFilterDefinition xmlns=\"http://www.nordugrid.org/schemas/InfoFilter/2008\" id=\"policy1\">\n\
  <Policy xmlns=\"http://www.nordugrid.org/schemas/policy-arc\" PolicyId=\"policy1\" CombiningAlg=\"Deny-Overrides\">\n\
    <Rule RuleId=\"rule1\" Effect=\"Permit\">\n\
      <Subjects>\n\
        <Subject Type=\"string\" AttributeId=\"urn:testID\">USER1</Subject>\n\
      </Subjects>\n\
    </Rule>\n\
  </Policy>\n\
</InfoFilterDefinition>");

  Arc::XMLNode policy2("\
<?xml version=\"1.0\"?>\n\
<InfoFilterDefinition xmlns=\"http://www.nordugrid.org/schemas/InfoFilter/2008\" id=\"policy2\">\n\
  <Policy xmlns=\"http://www.nordugrid.org/schemas/policy-arc\" PolicyId=\"policy1\" CombiningAlg=\"Deny-Overrides\">\n\
    <Rule RuleId=\"rule1\" Effect=\"Permit\">\n\
      <Subjects>\n\
        <Subject Type=\"string\" AttributeId=\"urn:testID\">USER2</Subject>\n\
      </Subjects>\n\
    </Rule>\n\
  </Policy>\n\
</InfoFilterDefinition>");

  // Service description with policies
  XMLNode infodoc_sec;
  infodoc.New(infodoc_sec);
  infodoc_sec["Resource"][0].NewChild(policy1);
  infodoc_sec["Resource"][0].NewAttribute("InfoFilterTag")="policy1";
  infodoc_sec["Resource"][1].NewChild(policy2);
  infodoc_sec["Resource"][1].NewAttribute("InfoFilterTag")="policy2";

  // Requestor's identifier
  MessageAuth user_id;
  TestSecAttr* user_attr = new TestSecAttr("USER1");
  user_id.set("TEST",user_attr);

  // Filter
  InfoFilter filter(user_id);

  // External policies
  //Arc::NS ns;
  //XMLNode policy1();
  std::list< std::pair<std::string,XMLNode> > policies;

  // Service description for filtering
  XMLNode infodoc_filtered;

  // Applying filter
  infodoc_sec.New(infodoc_filtered);
  CPPUNIT_ASSERT(filter.Filter(infodoc_filtered));

}

CPPUNIT_TEST_SUITE_REGISTRATION(InfoFilterTest);

