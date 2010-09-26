#include <cppunit/extensions/HelperMacros.h>

#include <arc/IniConfig.h>
#include <arc/Profile.h>
#include <arc/ArcConfig.h>

class ProfileTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ProfileTest);
  CPPUNIT_TEST(TestSingle);
  CPPUNIT_TEST(TestAttribute);
  CPPUNIT_TEST(TestMulti);
  CPPUNIT_TEST(TestMultiElement);
  CPPUNIT_TEST(TestMultiSection);
  CPPUNIT_TEST(TestSectionEnables);
  CPPUNIT_TEST(TestOthers);
  CPPUNIT_TEST_SUITE_END();

public:
  ProfileTest() : p("") {}

  void setUp() {}
  void tearDown() {}

  void TestSingle();
  void TestAttribute();
  void TestMulti();
  void TestMultiElement();
  void TestMultiSection();
  void TestSectionEnables();
  void TestOthers();

private:
  Arc::Profile p;
  Arc::IniConfig i;

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

void ProfileTest::TestSingle()
{
  /*
   * Profile:
     <ArcConfig>
       <foo>
         <bar initype="single" inisections="first second" initag="bar"/>
         <dummy initype="single"/>
       </foo>
     </ArcConfig>
   * Ini:
     [ first ]
     bar = first
     [ second ]
     bar = second
   * Config:
     <ArcConfig>
       <foo>
         <bar>first</bar>
       </foo>
     </ArcConfig>
   */

  p.NewChild("foo").NewChild("bar");
  p["foo"]["bar"].NewAttribute("initype") = "single";
  p["foo"]["bar"].NewAttribute("inisections") = "first second";
  p["foo"]["bar"].NewAttribute("initag") = "bar";
  p["foo"].NewChild("dummy");
  p["foo"]["dummy"].NewAttribute("initype") = "single";

  const std::string first = "first", second = "second";

  i.NewChild(first);
  i[first].NewChild("bar") = first;

  i.NewChild(second);
  i[second].NewChild("bar") = second;

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(1, c.Size());
  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Child(0).Child(0).AttributesSize());

  CPPUNIT_ASSERT_EQUAL(first, (std::string)c.Child(0).Child(0));

  ClearNodes();
}

void ProfileTest::TestAttribute()
{
  /*
   * Profile:
     <ArcConfig>
       <foo initype="attribute" inisections="first second" initag="foo"/>
     </ArcConfig>
   * Ini:
     [ first ]
     foo = first
     [ second ]
     foo = second
   * Config:
     <ArcConfig foo="first"/>
   */

  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "attribute";
  p["foo"].NewAttribute("inisections") = "first second";
  p["foo"].NewAttribute("initag") = "foo";

  const std::string first = "first", second = "second";

  i.NewChild(first);
  i[first].NewChild("foo") = first;

  i.NewChild(second);
  i[second].NewChild("foo") = second;

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(first, (std::string)c.Attribute("foo"));
  CPPUNIT_ASSERT_EQUAL(1, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Size());

  ClearNodes();
}

void ProfileTest::TestMulti()
{
  /*
   * Profile:
     <ArcConfig>
       <before initype="single" inisections="first second" initag="before"/>
       <foo initype="multi" inisections="first second" initag="foo"/>
       <dummy initype="multi" inisections="first second" initag="dummy"/>
       <after initype="single" inisections="first second" initag="after"/>
     </ArcConfig>
   * Ini:
     [ first ]
     foo = first1
     foo = first2
     foo = first3
     before = ***
     after  = ---
     [ second ]
     foo = second1
     foo = second2
     foo = second3
   * Config:
     <ArcConfig>
       <before>***</before>
       <foo>first1</foo>
       <foo>first2</foo>
       <foo>first3</foo>
       <after>---</after>
     </ArcConfig>
   */

  // Ordering test.
  p.NewChild("before");
  p["before"].NewAttribute("initype") = "single";
  p["before"].NewAttribute("inisections") = "first second";
  p["before"].NewAttribute("initag") = "before";

  // Test multi type.
  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "multi";
  p["foo"].NewAttribute("inisections") = "first second";
  p["foo"].NewAttribute("initag") = "foo";

  // Included for coverage.
  p.NewChild("dummy");
  p["dummy"].NewAttribute("initype") = "multi";
  p["dummy"].NewAttribute("inisections") = "first second";
  p["dummy"].NewAttribute("initag") = "dummy";

  // Included for testing ordering.
  p.NewChild("after");
  p["after"].NewAttribute("initype") = "single";
  p["after"].NewAttribute("inisections") = "first second";
  p["after"].NewAttribute("initag") = "after";

  const std::string first = "first", second = "second";

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

void ProfileTest::TestMultiElement()
{
  /*
   * Profile:
     <ArcConfig>
       <foo initype="multielement" inisections="first second" initag="baz">
         <fox initype="single" inisections="first second" initag="fox"/>
         <bar>
           <gee initype="attribute" inisections="first second" initag="gee"/>
           <baz initype="#this"/>
         </bar>
         <dummy><dummy/></dummy>
       </foo>
       <dummy initype="multielement"/>
       <empty initype="multielement" inisections="non-existent" initag="empty"/>
     </ArcConfig>
   * Ini:
     [ first ]
     baz = first-multielement1
     baz = first-multielement2
     baz = first-multielement3
     fox = first-fox
     [ second ]
     baz = second-multielement1
     baz = second-multielement2
     baz = second-multielement3
     fox = second-fox
     gee = attr
   * Config:
     <ArcConfig>
       <foo>
         <fox>first-fox</fox>
         <bar gee="attr">
           <baz>first-multielement1</baz>
         </bar>
       </foo>
       <foo>
         <fox>first-fox</fox>
         <bar gee="attr">
           <baz>first-multielement2</baz>
         </bar>
       </foo>
       <foo>
         <fox>first-fox</fox>
         <bar gee="attr">
           <baz>first-multielement3</baz>
         </bar>
       </foo>
     </ArcConfig>
   */

  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "multielement";
  p["foo"].NewAttribute("inisections") = "first second";
  p["foo"].NewAttribute("initag") = "baz";
  p["foo"].NewChild("bar");
  p["foo"]["bar"].NewChild("baz");
  p["foo"]["bar"]["baz"].NewAttribute("initype") = "#this";
  p["foo"]["bar"].NewChild("gee");
  p["foo"]["bar"]["gee"].NewAttribute("initype") = "attribute";
  p["foo"]["bar"]["gee"].NewAttribute("inisections") = "first second";
  p["foo"]["bar"]["gee"].NewAttribute("initag") = "gee";
  p["foo"].NewChild("fox");
  p["foo"]["fox"].NewAttribute("initype") = "single";
  p["foo"]["fox"].NewAttribute("inisections") = "first second";
  p["foo"]["fox"].NewAttribute("initag") = "fox";

  // Included for coverage.
  p["foo"].NewChild("dummy").NewChild("dummy");
  p.NewChild("dummy");
  p["dummy"].NewAttribute("initype") = "multielement";
  p.NewChild("empty");
  p["empty"].NewAttribute("initype") = "multielement";
  p["empty"].NewAttribute("inisections") = "non-existent";
  p["empty"].NewAttribute("initag") = "empty";

  const std::string first = "first", second = "second";

  i.NewChild(first);
  i[first].NewChild("baz") = first + "-multielement1";
  i[first].NewChild("baz") = first + "-multielement2";
  i[first].NewChild("baz") = first + "-multielement3";
  i[first].NewChild("fox") = first + "-fox";

  i.NewChild(second);
  i[second].NewChild("baz") = second + "-multielement1";
  i[second].NewChild("baz") = second + "-multielement2";
  i[second].NewChild("baz") = second + "-multielement3";
  i[second].NewChild("fox") = second + "-fox";
  i[second].NewChild("gee") = "attr";

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(first + "-multielement1", (std::string)c["foo"][0]["bar"]["baz"]);
  CPPUNIT_ASSERT_EQUAL(first + "-multielement2", (std::string)c["foo"][1]["bar"]["baz"]);
  CPPUNIT_ASSERT_EQUAL(first + "-multielement3", (std::string)c["foo"][2]["bar"]["baz"]);

  CPPUNIT_ASSERT_EQUAL(first + "-fox", (std::string)c["foo"][0]["fox"]);
  CPPUNIT_ASSERT_EQUAL(first + "-fox", (std::string)c["foo"][1]["fox"]);
  CPPUNIT_ASSERT_EQUAL(first + "-fox", (std::string)c["foo"][2]["fox"]);

  CPPUNIT_ASSERT_EQUAL((std::string)"attr", (std::string)c["foo"][0]["bar"].Attribute("gee"));
  CPPUNIT_ASSERT_EQUAL((std::string)"attr", (std::string)c["foo"][1]["bar"].Attribute("gee"));
  CPPUNIT_ASSERT_EQUAL((std::string)"attr", (std::string)c["foo"][2]["bar"].Attribute("gee"));

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].AttributesSize());
  CPPUNIT_ASSERT_EQUAL(1, c["foo"][1]["bar"].AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1]["bar"]["foo"].AttributesSize());
  CPPUNIT_ASSERT_EQUAL(3, c.Size());
  CPPUNIT_ASSERT_EQUAL(2, c["foo"][1].Size());
  CPPUNIT_ASSERT_EQUAL(1, c["foo"][1]["bar"].Size());

  ClearNodes();
}

void ProfileTest::TestMultiSection()
{
  /*
   * Profile:
     <ArcConfig>
       <foo initype="multisection" inisections="multi-first multi-second" initag="foo"/>
       <empty initype="multisection" inisections="non-existent" initag="empty"/>
       <bar initype="multisection" inisections="multi-first multi-second">
         <baza initype="multi" inisections="#this default" initag="baza"/>
         <bazb initype="multi" inisections="#this default" initag="bazb"/>
         <bazc initype="multi" inisections="default" initag="bazc"/>
         <geea initype="attribute" inisections="#this default" initag="geea"/>
         <geeb initype="attribute" inisections="#this default" initag="geeb"/>
         <dummy><dummy/></dummy>
       </bar
     </ArcConfig>
   * Ini:
     [ default ]
     baza = default-baza-1
     baza = default-baza-2
     baza = default-baza-3
     bazb = default-bazb
     bazc = default-bazc-1
     bazc = default-bazc-2
     geea = default-geea
     [ multi-first ]
     foo = 1
     baza = multi-first-1.1
     baza = multi-first-1.2
     baza = multi-first-1.3
     geea = multi-first-geea-1
     geeb = multi-first-geeb-1
     [ multi-first ]
     baza = multi-first-2.1
     baza = multi-first-2.2
     geea = multi-first-geea-2
     [ multi-first ]
     foo = 3
     foo = 3-a
     geea = multi-first-geea-3
     [ multi-first ]
     foo = 4
     baza = multi-first-3.1
     baza = multi-first-3.2
     baza = multi-first-3.3
     baza = multi-first-3.4
     geeb = multi-first-geeb-2
     [ multi-second ]
     foo = 1-second
     baza = multi-second-1.1
     geea = multi-second-geea
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
       </bar>
       <bar geea="multi-first-geea-2">
         <baza>multi-first-2.1</baza>
         <baza>multi-first-2.2</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
       </bar>
       <bar geea="multi-first-geea-3">
         <baza>default-baza-1</baza>
         <baza>default-baza-2</baza>
         <baza>default-baza-3</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
       </bar>
       <bar geea="default-geea" geeb="multi-first-geeb-2">
         <baza>multi-first-3.1</baza>
         <baza>multi-first-3.2</baza>
         <baza>multi-first-3.3</baza>
         <baza>multi-first-3.4</baza>
         <bazb>default-bazb</bazb>
         <bazc>default-bazc</bazc>
       </bar>
     </ArcConfig>
   */

  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "multisection";
  p["foo"].NewAttribute("inisections") = "multi-first multi-second";
  p["foo"].NewAttribute("initag") = "foo";

  // Included for coverage.
  p.NewChild("empty");
  p["empty"].NewAttribute("initype") = "multisection";
  p["empty"].NewAttribute("inisections") = "non-existent";
  p["empty"].NewAttribute("initag") = "empty";

  p.NewChild("bar");
  p["bar"].NewAttribute("initype") = "multisection";
  p["bar"].NewAttribute("inisections") = "multi-first multi-second";
  p["bar"].NewChild("baza");
  p["bar"]["baza"].NewAttribute("initype") = "multi";
  p["bar"]["baza"].NewAttribute("inisections") = "#this default";
  p["bar"]["baza"].NewAttribute("initag") = "baza";
  p["bar"].NewChild("bazb");
  p["bar"]["bazb"].NewAttribute("initype") = "multi";
  p["bar"]["bazb"].NewAttribute("inisections") = "#this default";
  p["bar"]["bazb"].NewAttribute("initag") = "bazb";
  p["bar"].NewChild("bazc");
  p["bar"]["bazc"].NewAttribute("initype") = "multi";
  p["bar"]["bazc"].NewAttribute("inisections") = "default";
  p["bar"]["bazc"].NewAttribute("initag") = "bazc";
  p["bar"].NewChild("geea");
  p["bar"]["geea"].NewAttribute("initype") = "attribute";
  p["bar"]["geea"].NewAttribute("inisections") = "#this default";
  p["bar"]["geea"].NewAttribute("initag") = "geea";
  p["bar"].NewChild("geeb");
  p["bar"]["geeb"].NewAttribute("initype") = "attribute";
  p["bar"]["geeb"].NewAttribute("inisections") = "#this default";
  p["bar"]["geeb"].NewAttribute("initag") = "geeb";

  // Included for coverage.
  p["bar"].NewChild("dummy").NewChild("dummy");

  const std::string mFirst = "multi-first", mSecond = "multi-second";

  i.NewChild("default");
  i["default"].NewChild("baza") = "default-baza-1";
  i["default"].NewChild("baza") = "default-baza-2";
  i["default"].NewChild("baza") = "default-baza-3";
  i["default"].NewChild("bazb") = "default-bazb";
  i["default"].NewChild("bazc") = "default-bazc";
  i["default"].NewChild("geea") = "default-geea";
  i.NewChild(mFirst);
  i.NewChild(mFirst);
  i.NewChild(mFirst);
  i.NewChild(mFirst);
  i[mFirst][0].NewChild("foo") = "1";
  i[mFirst][0].NewChild("baza") = "multi-first-1.1";
  i[mFirst][0].NewChild("baza") = "multi-first-1.2";
  i[mFirst][0].NewChild("baza") = "multi-first-1.3";
  i[mFirst][0].NewChild("geea") = "multi-first-geea-1";
  i[mFirst][0].NewChild("geeb") = "multi-first-geeb-1";

  i[mFirst][1].NewChild("foo"); // Included for coverage.
  i[mFirst][1].NewChild("baza") = "multi-first-2.1";
  i[mFirst][1].NewChild("baza") = "multi-first-2.2";
  i[mFirst][1].NewChild("geea") = "multi-first-geea-2";

  i[mFirst][2].NewChild("foo") = "3";
  i[mFirst][2].NewChild("foo") = "3-a";
  i[mFirst][2].NewChild("geea") = "multi-first-geea-3";

  i[mFirst][3].NewChild("foo") = "4";
  i[mFirst][3].NewChild("baza") = "multi-first-3.1";
  i[mFirst][3].NewChild("baza") = "multi-first-3.2";
  i[mFirst][3].NewChild("baza") = "multi-first-3.3";
  i[mFirst][3].NewChild("baza") = "multi-first-3.4";
  i[mFirst][3].NewChild("geeb") = "multi-first-geeb-2";

  i.NewChild(mSecond);
  i[mSecond].NewChild("foo") = "1-second";
  i[mSecond].NewChild("baza") = "multi-second-1.1";
  i[mSecond].NewChild("geea") = "multi-second-geea";

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(7, c.Size());
  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());

  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].Size());
  CPPUNIT_ASSERT_EQUAL(0, c["foo"][1].AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"1", (std::string)c.Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"3", (std::string)c.Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"4", (std::string)c.Child(2));

  CPPUNIT_ASSERT_EQUAL(5, c.Child(3).Size());
  CPPUNIT_ASSERT_EQUAL(2, c.Child(3).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.1", (std::string)c.Child(3).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.2", (std::string)c.Child(3).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-1.3", (std::string)c.Child(3).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(3).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(3).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-1", (std::string)c.Child(3).Attribute("geea"));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geeb-1", (std::string)c.Child(3).Attribute("geeb"));

  CPPUNIT_ASSERT_EQUAL(4, c.Child(4).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(4).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-2.1", (std::string)c.Child(4).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-2.2", (std::string)c.Child(4).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(4).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(4).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-2", (std::string)c.Child(4).Attribute("geea"));

  CPPUNIT_ASSERT_EQUAL(5, c.Child(5).Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(5).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-1", (std::string)c.Child(5).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-2", (std::string)c.Child(5).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-baza-3", (std::string)c.Child(5).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(5).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(5).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geea-3", (std::string)c.Child(5).Attribute("geea"));

  CPPUNIT_ASSERT_EQUAL(6, c.Child(6).Size());
  CPPUNIT_ASSERT_EQUAL(2, c.Child(6).AttributesSize());
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.1", (std::string)c.Child(6).Child(0));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.2", (std::string)c.Child(6).Child(1));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.3", (std::string)c.Child(6).Child(2));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-3.4", (std::string)c.Child(6).Child(3));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazb", (std::string)c.Child(6).Child(4));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-bazc", (std::string)c.Child(6).Child(5));
  CPPUNIT_ASSERT_EQUAL((std::string)"default-geea", (std::string)c.Child(6).Attribute("geea"));
  CPPUNIT_ASSERT_EQUAL((std::string)"multi-first-geeb-2", (std::string)c.Child(6).Attribute("geeb"));

  ClearNodes();
}

void ProfileTest::TestSectionEnables()
{
  /*
   * Profile:
     <ArcConfig>
       <bara inisectionenables="first">
         <baza initype="single" inisections="non-existent first second" initag="baza"/>
       </bara>
       <barb inisectionenables="non-existent">
         <bazb initype="single" inisections="non-existent first second" initag="bazb"/>
       </barb>
       <foo initype="multisection" inisections="first second">
         <geea inisectionenables="first" initype="single" inisections="first common" initag="geea"/>
         <geeb inisectionenables="non-existent" initype="single" inisections="non-existent common" initag="geeb"/>
       </foo>
       <bax initype="multielement" inisections="first second" initag="bax">
         <hufa inisectionenables="first" initype="single" inisections="first common" initag="hufa"/>
         <hufb inisectionenables="non-existent" initype="single" inisections="non-existent common" initag="hufb"/>
       </foo>
     </ArcConfig>
   * Ini:
     [ first ]
     baza = first-baza
     bazb = first-bazb
     bax = empty
     [ common ]
     geea = common-geea
     geeb = common-geeb
     hufa = common-hufa
     hufb = common-hufb
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
     </ArcConfig>
   */

  p.NewChild("bara");
  p["bara"].NewAttribute("inisectionenables") = "first";
  p["bara"].NewChild("baza");
  p["bara"]["baza"].NewAttribute("initype") = "single";
  p["bara"]["baza"].NewAttribute("inisections") = "non-existent first second";
  p["bara"]["baza"].NewAttribute("initag") = "baza";

  p.NewChild("barb");
  p["barb"].NewAttribute("inisectionenables") = "non-existent";
  p["barb"].NewChild("bazb");
  p["barb"]["bazb"].NewAttribute("initype") = "single";
  p["barb"]["bazb"].NewAttribute("inisections") = "non-existent first second";
  p["barb"]["bazb"].NewAttribute("initag") = "bazb";

  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "multisection";
  p["foo"].NewAttribute("inisections") = "first second";

  p["foo"].NewChild("geea");
  p["foo"]["geea"].NewAttribute("inisectionenables") = "first";
  p["foo"]["geea"].NewAttribute("initype") = "single";
  p["foo"]["geea"].NewAttribute("inisections") = "first common";
  p["foo"]["geea"].NewAttribute("initag") = "geea";

  p["foo"].NewChild("geeb");
  p["foo"]["geeb"].NewAttribute("inisectionenables") = "non-existent";
  p["foo"]["geeb"].NewAttribute("initype") = "single";
  p["foo"]["geeb"].NewAttribute("inisections") = "first common";
  p["foo"]["geeb"].NewAttribute("initag") = "geeb";

  p.NewChild("bax");
  p["bax"].NewAttribute("initype") = "multielement";
  p["bax"].NewAttribute("inisections") = "first second";
  p["bax"].NewAttribute("initag") = "bax";

  p["bax"].NewChild("hufa");
  p["bax"]["hufa"].NewAttribute("inisectionenables") = "first";
  p["bax"]["hufa"].NewAttribute("initype") = "single";
  p["bax"]["hufa"].NewAttribute("inisections") = "first common";
  p["bax"]["hufa"].NewAttribute("initag") = "hufa";

  p["bax"].NewChild("hufb");
  p["bax"]["hufb"].NewAttribute("inisectionenables") = "non-existent";
  p["bax"]["hufb"].NewAttribute("initype") = "single";
  p["bax"]["hufb"].NewAttribute("inisections") = "first common";
  p["bax"]["hufb"].NewAttribute("initag") = "hufb";

  const std::string first = "first", common = "common";

  i.NewChild(first);
  i[first].NewChild("baza") = first + "-baza";
  i[first].NewChild("bazb") = first + "-bazb";
  i[first].NewChild("bax") = "empty";

  i.NewChild(common);
  i[common].NewChild("geea") = common + "-geea";
  i[common].NewChild("geeb") = common + "-geeb";
  i[common].NewChild("hufa") = common + "-hufa";
  i[common].NewChild("hufb") = common + "-hufb";

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(3, c.Size());
  CPPUNIT_ASSERT_EQUAL(1, c.Child(0).Size());
  CPPUNIT_ASSERT_EQUAL(first + "-baza", (std::string)c.Child(0).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(1).Size());
  CPPUNIT_ASSERT_EQUAL(common + "-geea", (std::string)c.Child(1).Child(0));
  CPPUNIT_ASSERT_EQUAL(1, c.Child(2).Size());
  CPPUNIT_ASSERT_EQUAL(common + "-hufa", (std::string)c.Child(2).Child(0));

  ClearNodes();
}

void ProfileTest::TestOthers()
{
  /*
   * Profile:
     <ArcConfig>
       <foo initype="non-existent" inisections="first second" initag="foo"/>
     </ArcConfig>
   * Ini:
     [ first ]
     foo = first
     [ second ]
     foo = second
   * Config:
     <ArcConfig/>
   */

  p.NewChild("foo");
  p["foo"].NewAttribute("initype") = "non-existent";
  p["foo"].NewAttribute("inisections") = "first second";
  p["foo"].NewAttribute("initag") = "foo";

  const std::string first = "first", second = "second";

  i.NewChild(first);
  i[first].NewChild("foo") = first;

  i.NewChild(second);
  i[second].NewChild("foo") = second;

  Arc::Config c;
  p.Evaluate(c, i);

  CPPUNIT_ASSERT_EQUAL(0, c.AttributesSize());
  CPPUNIT_ASSERT_EQUAL(0, c.Size());

  ClearNodes();
}

CPPUNIT_TEST_SUITE_REGISTRATION(ProfileTest);
