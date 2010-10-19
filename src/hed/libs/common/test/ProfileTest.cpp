#include <cppunit/extensions/HelperMacros.h>

#include <sstream>

#include <arc/IniConfig.h>
#include <arc/Profile.h>
#include <arc/ArcConfig.h>

#define TESTSINGLE
#define TESTATTRIBUTE
#define TESTMULTI
#define TESTMULTIELEMENT
#define TESTMULTISECTION
#define TESTTOKENENABLES
#define TESTDEFAULTVALUE
#define TESTOTHERS

class ProfileTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ProfileTest);
#ifdef TESTSINGLE
  CPPUNIT_TEST(TestSingle);
#endif
#ifdef TESTATTRIBUTE
  CPPUNIT_TEST(TestAttribute);
#endif
#ifdef TESTMULTI
  CPPUNIT_TEST(TestMulti);
#endif
#ifdef TESTMULTIELEMENT
  CPPUNIT_TEST(TestMultiElement);
#endif
#ifdef TESTMULTISECTION
  CPPUNIT_TEST(TestMultiSection);
#endif
#ifdef TESTTOKENENABLES
  CPPUNIT_TEST(TestTokenEnables);
#endif
#ifdef TESTDEFAULTVALUE
  CPPUNIT_TEST(TestDefaultValue);
#endif
#ifdef TESTOTHERS
  CPPUNIT_TEST(TestOthers);
#endif
  CPPUNIT_TEST_SUITE_END();

public:
  ProfileTest() : p(""), first("first"), second("second"), common("common"), special("special") {}

  void setUp() {}
  void tearDown() {}

#ifdef TESTSINGLE
  void TestSingle();
#endif
#ifdef TESTATTRIBUTE
  void TestAttribute();
#endif
#ifdef TESTMULTI
  void TestMulti();
#endif
#ifdef TESTMULTIELEMENT
  void TestMultiElement();
#endif
#ifdef TESTMULTISECTION
  void TestMultiSection();
#endif
#ifdef TESTTOKENENABLES
  void TestTokenEnables();
#endif
#ifdef TESTDEFAULTVALUE
  void TestDefaultValue();
#endif
#ifdef TESTOTHERS
  void TestOthers();
#endif

private:
  Arc::Profile p;
  Arc::IniConfig i;

  const std::string first, second, common, special;

  void ClearNodes();
};

void ProfileTest::ClearNodes()
{
  while (p.Size() > 0) {
    p.Child().Destroy();
  }

  while (i.Size() > 0) {
    i.Child().Destroy();
  }
}

#ifdef TESTSINGLE
void ProfileTest::TestSingle()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<foo>"
            "<bara inisections=\"first second\" initag=\"bara\"/>"
            "<barb initype=\"single\" inisections=\"first second\" initag=\"barb\"/>"
            "<barc initype=\"single\" inisections=\"first second\" initag=\"barc\" inidefaultvalue=\"default-barc\"/>"
            "<bard testa=\"testa\" testb=\"testb\" initype=\"single\" inisections=\"first second\" initag=\"bard\" inidefaultvalue=\"default-bard\"/>"
            "<dummy initype=\"single\"/>"
          "</foo>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("bara") = first;
  i[first].NewChild("barc") = first;

  i.NewChild(second);
  i[second].NewChild("bara") = second;
  i[second].NewChild("barb") = second;
  i[second].NewChild("barc") = second;

  /*
     Config:
     <ArcConfig>
       <foo>
         <bara>first</bara>
         <barb>second</barb>
         <barc>first</barc>
         <bard testa="testa" testb="testb">default-bard</bard>
       </foo>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(1, c.Size());
  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(4, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(first, (std::string)c.Child(0).Child(0));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(second, (std::string)c.Child(0).Child(1));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(2).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(2).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(first, (std::string)c.Child(0).Child(2));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(3).Size());
  CPPUNIT_ASSERT_EQUAL(2, c.Child(0).Child(3).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bard", (std::string)c.Child(0).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"testa", (std::string)c.Child(0).Child(3).Attribute("testa"));
  CPPUNIT_ASSERT_EQUAL((std::string)"testb", (std::string)c.Child(0).Child(3).Attribute("testb"));

  ClearNodes();
}
#endif

#ifdef TESTATTRIBUTE
void ProfileTest::TestAttribute()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<foo initype=\"attribute\" inisections=\"first second\" initag=\"foo\"/>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("foo") = first;

  i.NewChild(second);
  i[second].NewChild("foo") = second;

  /*
     Config:
     <ArcConfig foo="first"/>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(first, (std::string)c.Attribute("foo"));
  CPPUNIT_ASSERT_EQUAL(1, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Size());

  ClearNodes();
}
#endif

#ifdef TESTMULTI
void ProfileTest::TestMulti()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<before initype=\"single\" inisections=\"first second\" initag=\"before\"/>"
          "<foo initype=\"multi\" inisections=\"first second\" initag=\"foo\"/>"
          "<dummy initype=\"multi\" inisections=\"first second\" initag=\"dummy\"/>"
          "<after initype=\"single\" inisections=\"first second\" initag=\"after\"/>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("foo") = first + "1";
  i[first].NewChild("foo") = first + "2";
  i[first].NewChild("foo") = first + "3";
  i[first].NewChild("before") = "***";
  i[first].NewChild("after")  = "---";

  i.NewChild(second);
  i[second].NewChild("foo") = second + "1";
  i[second].NewChild("foo") = second + "2";
  i[second].NewChild("foo") = second + "3";

  /*
     Config:
     <ArcConfig>
       <before>***</before>
       <foo>first1</foo>
       <foo>first2</foo>
       <foo>first3</foo>
       <after>---</after>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(first + "1", (std::string)c["foo"][0]);
  CPPUNIT_ASSERT_EQUAL(first + "2", (std::string)c["foo"][1]);
  CPPUNIT_ASSERT_EQUAL(first + "3", (std::string)c["foo"][2]);

  // Test ordering.
  CPPUNIT_ASSERT_MESSAGE("Ordering of nodes incorrect.", c.Child(0) == c["before"][0]);
  CPPUNIT_ASSERT_MESSAGE("Ordering of nodes incorrect.", c.Child(1) == c["foo"][0]);
  CPPUNIT_ASSERT_MESSAGE("Ordering of nodes incorrect.", c.Child(2) == c["foo"][1]);
  CPPUNIT_ASSERT_MESSAGE("Ordering of nodes incorrect.", c.Child(3) == c["foo"][2]);
  CPPUNIT_ASSERT_MESSAGE("Ordering of nodes incorrect.", c.Child(4) == c["after"][0]);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].AttributesSize());
  CPPUNIT_ASSERT_EQUAL(5, c.Size());

  ClearNodes();
}
#endif

#ifdef TESTMULTIELEMENT
void ProfileTest::TestMultiElement()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<foo initype=\"multielement\" inisections=\"first second\" initag=\"baz\">"
            "<fox initype=\"single\" inisections=\"first second\" initag=\"fox\"/>"
            "<bar>"
              "<geea inisections=\"first second\" initag=\"geea\"/>"
              "<geeb initype=\"single\" inisections=\"first second\" initag=\"geeb\"/>"
              "<geec initype=\"attribute\" inisections=\"first second\" initag=\"geec\"/>"
              "<baz initype=\"#this\"/>"
            "</bar>"
            "<dummy><dummy/></dummy>"
          "</foo>"
          "<dummy initype=\"multielement\"/>"
          "<empty initype=\"multielement\" inisections=\"non-existent\" initag=\"empty\"/>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("baz") = first + "-multielement1";
  i[first].NewChild("baz") = first + "-multielement2";
  i[first].NewChild("baz") = first + "-multielement3";
  i[first].NewChild("fox") = first + "-fox";
  i[first].NewChild("geea") = first + "-geea";
  i[first].NewChild("geeb") = first + "-geeb";

  i.NewChild(second);
  i[second].NewChild("baz") = second + "-multielement1";
  i[second].NewChild("baz") = second + "-multielement2";
  i[second].NewChild("baz") = second + "-multielement3";
  i[second].NewChild("fox") = second + "-fox";
  i[second].NewChild("geec") = second + "-geec-attr";

  /*
   * Config:
     <ArcConfig>
       <foo>
         <fox>first-fox</fox>
         <bar geec="second-geec-attr">
           <geea>first-geea</geea>
           <geeb>first-geeb</geeb>
           <baz>first-multielement1</baz>
         </bar>
       </foo>
       <foo>
         <fox>first-fox</fox>
         <bar geec="second-geec-attr">
           <geea>first-geea</geea>
           <geeb>first-geeb</geeb>
           <baz>first-multielement2</baz>
         </bar>
       </foo>
       <foo>
         <fox>first-fox</fox>
         <bar geec="second-geec-attr">
           <geea>first-geea</geea>
           <geeb>first-geeb</geeb>
           <baz>first-multielement3</baz>
         </bar>
       </foo>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(3, c.Size());
  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());

  CPPUNIT_ASSERT_EQUAL(2, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"foo", (std::string)c.Child(0).Name());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"fox", (std::string)c.Child(0).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-fox", (std::string)c.Child(0).Child(0));
  CPPUNIT_ASSERT_EQUAL(3, c.Child(0).Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(0).Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"bar", (std::string)c.Child(0).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"geea", (std::string)c.Child(0).Child(1).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geea", (std::string)c.Child(0).Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"geeb", (std::string)c.Child(0).Child(1).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geeb", (std::string)c.Child(0).Child(1).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"baz", (std::string)c.Child(0).Child(1).Child(2).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-multielement1", (std::string)c.Child(0).Child(1).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"geec", (std::string)c.Child(0).Child(1).Attribute(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"second-geec-attr", (std::string)c.Child(0).Child(1).Attribute(0));

  CPPUNIT_ASSERT_EQUAL(2, c.Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"foo", (std::string)c.Child(1).Name());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(1).Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(1).Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"fox", (std::string)c.Child(1).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-fox", (std::string)c.Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL(3, c.Child(1).Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(1).Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"bar", (std::string)c.Child(1).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"geea", (std::string)c.Child(1).Child(1).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geea", (std::string)c.Child(1).Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"geeb", (std::string)c.Child(1).Child(1).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geeb", (std::string)c.Child(1).Child(1).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"baz", (std::string)c.Child(1).Child(1).Child(2).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-multielement2", (std::string)c.Child(1).Child(1).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"geec", (std::string)c.Child(1).Child(1).Attribute(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"second-geec-attr", (std::string)c.Child(1).Child(1).Attribute(0));

  CPPUNIT_ASSERT_EQUAL(2, c.Child(2).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(2).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"foo", (std::string)c.Child(2).Name());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(2).Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(2).Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"fox", (std::string)c.Child(2).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-fox", (std::string)c.Child(2).Child(0));
  CPPUNIT_ASSERT_EQUAL(3, c.Child(2).Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(2).Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"bar", (std::string)c.Child(2).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"geea", (std::string)c.Child(2).Child(1).Child(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geea", (std::string)c.Child(2).Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"geeb", (std::string)c.Child(2).Child(1).Child(1).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-geeb", (std::string)c.Child(2).Child(1).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"baz", (std::string)c.Child(2).Child(1).Child(2).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"first-multielement3", (std::string)c.Child(2).Child(1).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"geec", (std::string)c.Child(2).Child(1).Attribute(0).Name());
  CPPUNIT_ASSERT_EQUAL((std::string)"second-geec-attr", (std::string)c.Child(2).Child(1).Attribute(0));

  ClearNodes();
}
#endif

#ifdef TESTMULTISECTION
void ProfileTest::TestMultiSection()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<foo initype=\"multisection\" inisections=\"multi-first multi-second\" initag=\"foo\"/>"
          "<empty initype=\"multisection\" inisections=\"non-existent\" initag=\"empty\"/>"
          "<bar initype=\"multisection\" inisections=\"multi-first multi-second\">"
            "<baza initype=\"multi\" inisections=\"#this default\" initag=\"baza\"/>"
            "<bazb initype=\"multi\" inisections=\"#this default\" initag=\"bazb\"/>"
            "<bazc initype=\"multi\" inisections=\"default\" initag=\"bazc\"/>"
            "<wija inisections=\"#this default\" initag=\"wija\"/>"
            "<wijb initype=\"single\" inisections=\"#this default\" initag=\"wijb\"/>"
            "<wijc inisections=\"#this default\" initag=\"wijc\"/>"
            "<wijd initype=\"single\" inisections=\"#this default\" initag=\"wijd\"/>"
            "<geea initype=\"attribute\" inisections=\"#this default\" initag=\"geea\"/>"
            "<geeb initype=\"attribute\" inisections=\"#this default\" initag=\"geeb\"/>"
            "<dummy><dummy/></dummy>"
          "</bar>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild("default");
  i["default"].NewChild("baza") = "default-baza-1";
  i["default"].NewChild("baza") = "default-baza-2";
  i["default"].NewChild("baza") = "default-baza-3";
  i["default"].NewChild("bazb") = "default-bazb";
  i["default"].NewChild("wija") = "default-wija";
  i["default"].NewChild("wijb") = "default-wijb";
  i["default"].NewChild("wijc") = "default-wijc";
  i["default"].NewChild("wijd") = "default-wijd";
  i["default"].NewChild("bazc") = "default-bazc";
  i["default"].NewChild("geea") = "default-geea";

  i.NewChild("multi-first");
  i["multi-first"][0].NewChild("foo") = "1";
  i["multi-first"][0].NewChild("baza") = "multi-first-1.1";
  i["multi-first"][0].NewChild("baza") = "multi-first-1.2";
  i["multi-first"][0].NewChild("baza") = "multi-first-1.3";
  i["multi-first"][0].NewChild("geea") = "multi-first-geea-1";
  i["multi-first"][0].NewChild("geeb") = "multi-first-geeb-1";
  i["multi-first"][0].NewChild("wijc") = "multi-first-wijc-1";

  i.NewChild("multi-first");
  i["multi-first"][1].NewChild("foo"); // Included for coverage.
  i["multi-first"][1].NewChild("baza") = "multi-first-2.1";
  i["multi-first"][1].NewChild("baza") = "multi-first-2.2";
  i["multi-first"][1].NewChild("geea") = "multi-first-geea-2";

  i.NewChild("multi-first");
  i["multi-first"][2].NewChild("foo") = "3";
  i["multi-first"][2].NewChild("foo") = "3-a";
  i["multi-first"][2].NewChild("geea") = "multi-first-geea-3";
  i["multi-first"][2].NewChild("wijc") = "multi-first-wijc-2";

  i.NewChild("multi-first");
  i["multi-first"][3].NewChild("foo") = "4";
  i["multi-first"][3].NewChild("baza") = "multi-first-3.1";
  i["multi-first"][3].NewChild("baza") = "multi-first-3.2";
  i["multi-first"][3].NewChild("baza") = "multi-first-3.3";
  i["multi-first"][3].NewChild("baza") = "multi-first-3.4";
  i["multi-first"][3].NewChild("geeb") = "multi-first-geeb-2";
  i["multi-first"][3].NewChild("wijc") = "multi-first-wijc-3";

  i.NewChild("multi-second");
  i["multi-second"].NewChild("foo") = "1-second";
  i["multi-second"].NewChild("baza") = "multi-second-1.1";
  i["multi-second"].NewChild("geea") = "multi-second-geea";
  i["multi-second"].NewChild("wijd") = "multi-second-wijd-1";

  /*
   * Config:
     <ArcConfig>
       <foo>1</foo>
       <foo>2</foo>
       <foo>3</foo>
       <bar geea="multi-first-geea-1" geeb="multi-first-geeb-1">
         <baza>multi-first-1.1</baza>
         <baza>multi-first-1.2</baza>
         <baza>multi-first-1.3</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
         <wija>default-wija</wija>
         <wijb>default-wijb</wijb>
         <wijc>multi-first-wijc-1</wijc>
         <wijd>multi-second-wijd-1</wijd>
       </bar>
       <bar geea="multi-first-geea-2">
         <baza>multi-first-2.1</baza>
         <baza>multi-first-2.2</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
         <wija>default-wija</wija>
         <wijb>default-wijb</wijb>
         <wijc>default-wijc</wijc>
         <wijd>default-wijd</wijd>
       </bar>
       <bar geea="multi-first-geea-3">
         <baza>default-baza-1</baza>
         <baza>default-baza-2</baza>
         <baza>default-baza-3</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
         <wija>default-wija</wija>
         <wijb>default-wijb</wijb>
         <wijc>multi-first-wijc-2</wijc>
         <wijd>default-wijb</wijd>
       </bar>
       <bar geea="default-geea" geeb="multi-first-geeb-2">
         <baza>multi-first-3.1</baza>
         <baza>multi-first-3.2</baza>
         <baza>multi-first-3.3</baza>
         <baza>multi-first-3.4</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
         <wija>default-wija</wija>
         <wijb>default-wijb</wijb>
         <wijc>multi-first-wijc-3</wijc>
         <wijd>default-wijd</wijd>
       </bar>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(7, c.Size());
  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());

  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].Size());
  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"1", (std::string)c.Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"3", (std::string)c.Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"4", (std::string)c.Child(2));

  CPPUNIT_ASSERT_EQUAL(9, c.Child(3).Size());
  CPPUNIT_ASSERT_EQUAL(2, c.Child(3).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.1", (std::string)c.Child(3).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.2", (std::string)c.Child(3).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.3", (std::string)c.Child(3).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(3).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(3).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wija", (std::string)c.Child(3).Child(5));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijb", (std::string)c.Child(3).Child(6));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-wijc-1", (std::string)c.Child(3).Child(7));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijd", (std::string)c.Child(3).Child(8));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-1", (std::string)c.Child(3).Attribute("geea"));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geeb-1", (std::string)c.Child(3).Attribute("geeb"));

  CPPUNIT_ASSERT_EQUAL(8, c.Child(4).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(4).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-2.1", (std::string)c.Child(4).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-2.2", (std::string)c.Child(4).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(4).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(4).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wija", (std::string)c.Child(4).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijb", (std::string)c.Child(4).Child(5));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijc", (std::string)c.Child(4).Child(6));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijd", (std::string)c.Child(4).Child(7));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-2", (std::string)c.Child(4).Attribute("geea"));

  CPPUNIT_ASSERT_EQUAL(9, c.Child(5).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(5).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-1", (std::string)c.Child(5).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-2", (std::string)c.Child(5).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-3", (std::string)c.Child(5).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(5).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(5).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wija", (std::string)c.Child(5).Child(5));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijb", (std::string)c.Child(5).Child(6));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-wijc-2", (std::string)c.Child(5).Child(7));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijd", (std::string)c.Child(5).Child(8));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-3", (std::string)c.Child(5).Attribute("geea"));

  CPPUNIT_ASSERT_EQUAL(10, c.Child(6).Size());
  CPPUNIT_ASSERT_EQUAL(2, c.Child(6).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.1", (std::string)c.Child(6).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.2", (std::string)c.Child(6).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.3", (std::string)c.Child(6).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.4", (std::string)c.Child(6).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(6).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(6).Child(5));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wija", (std::string)c.Child(6).Child(6));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijb", (std::string)c.Child(6).Child(7));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-wijc-3", (std::string)c.Child(6).Child(8));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-wijd", (std::string)c.Child(6).Child(9));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-geea", (std::string)c.Child(6).Attribute("geea"));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geeb-2", (std::string)c.Child(6).Attribute("geeb"));

  ClearNodes();
}
#endif

#ifdef TESTTOKENENABLES
void ProfileTest::TestTokenEnables()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<bara initokenenables=\"first\">"
            "<baza initype=\"single\" inisections=\"non-existent first second\" initag=\"baza\"/>"
          "</bara>"
          "<barb initokenenables=\"non-existent\">"
            "<bazb initype=\"single\" inisections=\"non-existent first second\" initag=\"bazb\"/>"
          "</barb>"
          "<foo initype=\"multisection\" inisections=\"first second\">"
            "<geea initokenenables=\"first\" initype=\"single\" inisections=\"first common\" initag=\"geea\"/>"
            "<geeb initokenenables=\"non-existent\" initype=\"single\" inisections=\"non-existent common\" initag=\"geeb\"/>"
          "</foo>"
          "<bax initype=\"multielement\" inisections=\"first second\" initag=\"bax\">"
            "<hufa initokenenables=\"first\" initype=\"single\" inisections=\"first common\" initag=\"hufa\"/>"
            "<hufb initokenenables=\"non-existent\" initype=\"single\" inisections=\"non-existent common\" initag=\"hufb\"/>"
          "</bax>"
          "<xbara initokenenables=\"first#xbaza=enabled-xbaza\">"
            "<xbaza initype=\"single\" inisections=\"first second\" initag=\"xbaza\"/>"
          "</xbara>"
          "<xbarb initokenenables=\"first#non-existent=disabled-xbazb\">"
            "<xbazb initype=\"single\" inisections=\"non-existent first second\" initag=\"xbazb\"/>"
          "</xbarb>"
          "<xbarc initokenenables=\"first#xbazc=disabled-xbazc\">"
            "<xbazc initype=\"single\" inisections=\"first second\" initag=\"xbazc\"/>"
          "</xbarc>"
          "<xfoo initype=\"multisection\" inisections=\"first second\">"
            "<xgeea initokenenables=\"common#xgeea=enabled-xgeea\" initype=\"single\" inisections=\"first common\" initag=\"xgeea\"/>"
            "<xgeeb initokenenables=\"common#non-existent=disabled-xgeeb\" initype=\"single\" inisections=\"non-existent common\" initag=\"xgeeb\"/>"
            "<xgeec initokenenables=\"common#xgeec=disabled-xgeec\" initype=\"single\" inisections=\"non-existent common\" initag=\"xgeec\"/>"
          "</xfoo>"
          "<xbax initype=\"multielement\" inisections=\"first second\" initag=\"xbax\">"
            "<xhufa initokenenables=\"common#xhufa=enabled-xhufa\" initype=\"single\" inisections=\"first common\" initag=\"xhufa\"/>"
            "<xhufb initokenenables=\"common#non-existent=disabled-xhufb\" initype=\"single\" inisections=\"non-existent common\" initag=\"xhufb\"/>"
            "<xhufc initokenenables=\"common#xhufc=disabled-xhufc\" initype=\"single\" inisections=\"non-existent common\" initag=\"xhufc\"/>"
          "</xbax>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("baza") = first + "-baza";
  i[first].NewChild("xbaza") = "enabled-xbaza";
  i[first].NewChild("bazb") = first + "-bazb";
  i[first].NewChild("xbazb") = "enabled-xbazb";
  i[first].NewChild("xbazc") = "enabled-xbazc";
  i[first].NewChild("bax") = "empty";
  i[first].NewChild("xbax") = "empty";

  i.NewChild(common);
  i[common].NewChild("geea") = common + "-geea";
  i[common].NewChild("xgeea") = "enabled-xgeea";
  i[common].NewChild("geeb") = "-geeb";
  i[common].NewChild("xgeeb") = "enabled-xgeeb";
  i[common].NewChild("xgeec") = "enabled-xgeec";
  i[common].NewChild("hufa") = common + "-hufa";
  i[common].NewChild("xhufa") = "enabled-xhufa";
  i[common].NewChild("hufb") = common + "-hufb";
  i[common].NewChild("xhufb") = "enabled-xhufb";
  i[common].NewChild("xhufc") = "enabled-xhufc";

  /*
   * Config:
     <ArcConfig>
       <bara>
         <baza>first-bara</baza>
       </bara>
       <foo>
         <geea>common-geea</geea>
       </foo>
       <bax>
         <hufa>common-hufa</hufa>
       </bax>
       <xbara>
         <xbaza>enabled</xbaza>
       </bara>
       <xfoo>
         <xgeea>enabled</xgeea>
       </xfoo>
       <xbax>
         <xhufa>enabled</xhufa>
       </xbax>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(6, c.Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(first + "-baza", (std::string)c.Child(0).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(common + "-geea", (std::string)c.Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(2).Size());
  CPPUNIT_ASSERT_EQUAL(common + "-hufa", (std::string)c.Child(2).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(3).Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"enabled-xbaza", (std::string)c.Child(3).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(4).Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"enabled-xgeea", (std::string)c.Child(4).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(5).Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"enabled-xhufa", (std::string)c.Child(5).Child(0));

  ClearNodes();
}
#endif

#ifdef TESTDEFAULTVALUE
void ProfileTest::TestDefaultValue()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<bara>"
            "<baza initype=\"single\" inisections=\"common\" initag=\"baza\" inidefaultvalue=\"baza-default\"/>"
          "</bara>"
          "<barb>"
            "<bazb initype=\"single\" inisections=\"common\" initag=\"bazb\" inidefaultvalue=\"bazb-default\"/>"
          "</barb>"
          "<barc>"
            "<bazc>bazc-constant</bazc>"
          "</barc>"
          "<bard>"
            "<bazd initype=\"single\" inisections=\"common\" initag=\"bazd\">no-effect</bazd>"
          "</bard>"
          "<fooa>"
            "<foza initype=\"attribute\" inisections=\"common\" initag=\"foza\" inidefaultvalue=\"foza-default\"/>"
          "</fooa>"
          "<foob>"
            "<fozb initype=\"attribute\" inisections=\"common\" initag=\"fozb\" inidefaultvalue=\"fozb-default\"/>"
          "</foob>"
          "<fooc fozc=\"fozc-constant\"/>"
          "<food>"
            "<fozd initype=\"attribute\" inisections=\"common\" initag=\"fozd\">no-effect</fozd>"
          "</food>"
          "<mfooa>"
            "<mfoza initype=\"multi\" inisections=\"common\" initag=\"mfoza\" inidefaultvalue=\"mfoza-default\"/>"
          "</mfooa>"
          "<mfoob>"
            "<mfozb initype=\"multi\" inisections=\"common\" initag=\"mfozb\" inidefaultvalue=\"mfozb-default\"/>"
          "</mfoob>"
          "<mfood>"
            "<mfozd initype=\"multi\" inisections=\"common\" initag=\"mfozd\">no-effect</mfozd>"
          "</mfood>"
          "<msfooa initype=\"multisection\" inisections=\"special\">"
            "<msfoza initype=\"single\" inisections=\"#this\" initag=\"msfoza\" inidefaultvalue=\"msfoza-default\"/>"
            "<msbara initype=\"attribute\" inisections=\"#this\" initag=\"msbara\" inidefaultvalue=\"msbara-default\"/>"
            "<msmfoza initype=\"multi\" inisections=\"#this\" initag=\"msmfoza\" inidefaultvalue=\"msmfoza-default\"/>"
           "</msfooa>"
           "<msfoob initype=\"multisection\" inisections=\"special\">"
            "<msfozb initype=\"single\" inisections=\"#this\" initag=\"msfozb\" inidefaultvalue=\"msfozb-default\"/>"
            "<msbarb initype=\"attribute\" inisections=\"#this\" initag=\"msbarb\" inidefaultvalue=\"msbarb-default\"/>"
            "<msmfozb initype=\"multi\" inisections=\"#this\" initag=\"msmfozb\" inidefaultvalue=\"msmfozb-default\"/>"
          "</msfoob>"
          "<msfood initype=\"multisection\" inisections=\"special\">"
            "<msfozd initype=\"single\" inisections=\"#this\" initag=\"msfozd\">no-effect</msfozd>"
            "<msbard initype=\"attribute\" inisections=\"#this\" initag=\"msbard\">no-effect</msbard>"
            "<msmfozd initype=\"multi\" inisections=\"#this\" initag=\"msmfozd\">no-effect</msmfozd>"
          "</msfood>"
          "<mefooa initype=\"multielement\" inisections=\"special\" initag=\"mefoza\">"
            "<mefoza initype=\"#this\" inidefaultvalue=\"mefoza-default\"/>"
            "<mesfoza initype=\"single\" inisections=\"special\" initag=\"mesfoza\" inidefaultvalue=\"mesfoza-default\"/>"
            "<meafoza initype=\"attribute\" inisections=\"special\" initag=\"meafoza\" inidefaultvalue=\"meafoza-default\"/>"
          "</mefooa>"
          "<mefoob initype=\"multielement\" inisections=\"special\" initag=\"mefozb\">"
            "<mefozb initype=\"#this\" inidefaultvalue=\"mefozb-default\"/>"
            "<mesfozb initype=\"single\" inisections=\"special\" initag=\"mesfozb\" inidefaultvalue=\"mesfozb-default\"/>"
            "<meafozb initype=\"attribute\" inisections=\"special\" initag=\"meafozb\" inidefaultvalue=\"meafozb-default\"/>"
          "</mefoob>"
          "<mefood initype=\"multielement\" inisections=\"special\" initag=\"mefozd\">"
            "<mefozd initype=\"#this\">no-effect</mefozd>"
            "<mesfozd initype=\"single\" inisections=\"special\" initag=\"mesfozd\">no-effect</mesfozd>"
            "<meafozd initype=\"attribute\" inisections=\"special\" initag=\"meafozd\">no-effect</meafozd>"
          "</mefood>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(common);
  i[common].NewChild("baza") = common + "-baza";
  i[common].NewChild("foza") = common + "-foza";
  i[common].NewChild("mfoza") = common + "-mfoza";

  i.NewChild(special);
  i[special].NewChild("msfoza") = special + "-msfoza";
  i[special].NewChild("msbara") = special + "-msbara";
  i[special].NewChild("msmfoza") = special + "-msmfoza";
  i[special].NewChild("mefoza") = special + "-mefoza";
  i[special].NewChild("mesfoza") = special + "-mesfoza";
  i[special].NewChild("meafoza") = special + "-meafoza";
  i[special].NewChild("mefozb") = special + "-mefozb";

  /*
   * Config:
     <ArcConfig>
       <bara>
         <baza>common-baza</baza>
       </bara>
       <barb>
         <bazb>bazb-default</bazb>
       </barb>
       <barc>
         <bazc>bazc-constant</bazc>
       </barc>
       <bard/>
       <fooa foza="common-foza"/>
       <foob fozb="fozb-default"/>
       <fooc fozc="fozc-constant"/>
       <food/>
       <mfooa>
         <mfoza>common-mfoza</mfoza>
       </mfooa>
       <mfoob>
         <mfozb>mfozb-default</mfozb>
       </mfoob>
       <mfood/>
       <msfooa msbara="special-msbara">
         <msfoza>special-msfoza</msfoza>
         <msmfoza>special-msmfoza</msmfoza>
       </msfooa>
       <msfoob msfozb="msbarb-default">
         <msfozb>msfozb-default</msfozb>
         <msmfozb>msmfozb-default</msmfozb>
       </msfoob>
       <mefooa meafoza="special-meafoza">
         <mefoza>special-mefoza</mefoza>
         <mesfoza>special-mesfoza</mesfoza>
       </mefooa>
       <mefoob meafozb="meafozb-default">
         <mefozb>mefozb-default</mefozb>
         <mesfozb>mesfozb-default</mesfozb>
       </mefoob>
     </ArcConfig>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(15, c.Size());

  CPPUNIT_ASSERT_EQUAL(1, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(common + "-baza", (std::string)c.Child(0).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(1).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"bazb-default", (std::string)c.Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(2).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(2).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"bazc-constant", (std::string)c.Child(2).Child(0));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(3).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(3).AttributesSize());

  CPPUNIT_ASSERT_EQUAL(0, c.Child(4).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(4).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(common + "-foza", (std::string)c.Child(4).Attribute("foza"));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(5).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(5).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"fozb-default", (std::string)c.Child(5).Attribute("fozb"));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(6).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(6).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"fozc-constant", (std::string)c.Child(6).Attribute("fozc"));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(7).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(7).AttributesSize());

  CPPUNIT_ASSERT_EQUAL(1, c.Child(8).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(8).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(common + "-mfoza", (std::string)c.Child(8).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(9).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(9).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"mfozb-default", (std::string)c.Child(9).Child(0));
  CPPUNIT_ASSERT_EQUAL(0, c.Child(10).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(10).AttributesSize());

  CPPUNIT_ASSERT_EQUAL(2, c.Child(11).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(11).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(special + "-msfoza", (std::string)c.Child(11).Child(0));
  CPPUNIT_ASSERT_EQUAL(special + "-msbara", (std::string)c.Child(11).Attribute("msbara"));
  CPPUNIT_ASSERT_EQUAL(special + "-msmfoza", (std::string)c.Child(11).Child(1));
  CPPUNIT_ASSERT_EQUAL(2, c.Child(12).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(12).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"msfozb-default", (std::string)c.Child(12).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"msbarb-default", (std::string)c.Child(12).Attribute("msbarb"));
  CPPUNIT_ASSERT_EQUAL((std::string)"msmfozb-default", (std::string)c.Child(12).Child(1));

  CPPUNIT_ASSERT_EQUAL(2, c.Child(13).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(13).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(special + "-mefoza", (std::string)c.Child(13).Child(0));
  CPPUNIT_ASSERT_EQUAL(special + "-mesfoza", (std::string)c.Child(13).Child(1));
  CPPUNIT_ASSERT_EQUAL(special + "-meafoza", (std::string)c.Child(13).Attribute("meafoza"));
  CPPUNIT_ASSERT_EQUAL(2, c.Child(14).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(14).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(special + "-mefozb", (std::string)c.Child(14).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"mesfozb-default", (std::string)c.Child(14).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"meafozb-default", (std::string)c.Child(14).Attribute("meafozb"));

  ClearNodes();
}
#endif

#ifdef TESTOTHERS
void ProfileTest::TestOthers()
{
  std::stringstream ps;
  ps << "<ArcConfig>"
          "<foo initype=\"non-existent\" inisections=\"first second\" initag=\"foo\"/>"
        "</ArcConfig>";
  p.ReadFromStream(ps);

  i.NewChild(first);
  i[first].NewChild("foo") = first;

  i.NewChild(second);
  i[second].NewChild("foo") = second;

  /*
   * Config:
     <ArcConfig/>
   */

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Size());

  ClearNodes();
}
#endif

CPPUNIT_TEST_SUITE_REGISTRATION(ProfileTest);
