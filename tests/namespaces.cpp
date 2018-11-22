/*
* Copyright (C) 2016 German Aerospace Center (DLR/SC)
*
* Created: 2016-08-22 Martin Siggel <martin.siggel@dlr.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "test.h" // Brings in the GTest framework
#include "tixi.h"
#include "namespaceFunctions.h"

class Namespaces : public ::testing::Test
{
protected:
  virtual void SetUp()
  {
    ASSERT_TRUE( tixiOpenDocument  ( "TestData/namespaces_1.xml",   &inDocumentHandle)  == SUCCESS);
  }

  virtual void TearDown()
  {
    ASSERT_TRUE ( tixiCloseDocument( inDocumentHandle ) == SUCCESS );
  }

protected:
  TixiDocumentHandle inDocumentHandle;
};

TEST_F(Namespaces, register_ns)
{
  char* text = NULL;

  // xpath not registered, html prefix not known
  ASSERT_EQ(INVALID_XPATH, tixiGetTextElement(inDocumentHandle, "/root/html:table/html:tr/html:td[1]/", &text));
  ASSERT_EQ(INVALID_XPATH, tixiGetTextElement(inDocumentHandle, "/root/h:table/h:tr/h:td[1]/", &text));

  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(inDocumentHandle, "http://www.w3.org/TR/html4/", "html"));
  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(inDocumentHandle, "http://www.w3.org/TR/html4/", "h"));

  ASSERT_EQ(SUCCESS, tixiGetTextElement(inDocumentHandle, "/root/html:table/html:tr/html:td[1]/", &text));
  ASSERT_STREQ("Apples", text);

  ASSERT_EQ(SUCCESS, tixiGetTextElement(inDocumentHandle, "/root/h:table/h:tr/h:td[2]/", &text));
  ASSERT_STREQ("Bananas", text);

  // check and register default cpacs namespace
  ASSERT_EQ(ELEMENT_NOT_FOUND, tixiCheckElement(inDocumentHandle, "/root/aircraft"));
  ASSERT_EQ(INVALID_XPATH, tixiCheckElement(inDocumentHandle, "/root/cpacs:aircraft"));
  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(inDocumentHandle, "http://www.dlr.de/cpacs", "cpacs"));

  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/cpacs:aircraft"));

  ASSERT_EQ(SUCCESS, tixiGetTextElement(inDocumentHandle, "/root/cpacs:aircraft/cpacs:modelname/", &text));
  ASSERT_STREQ("D150", text);
}

TEST_F(Namespaces, register_document_ns)
{
  char* text = NULL;

  // xpath not registered, html prefix not known
  ASSERT_EQ(INVALID_XPATH, tixiGetTextElement(inDocumentHandle, "/root/html:table/html:tr/html:td[1]/", &text));
  ASSERT_EQ(INVALID_XPATH, tixiGetTextElement(inDocumentHandle, "/root/h:table/h:tr/h:td[1]/", &text));

  ASSERT_EQ(SUCCESS, tixiRegisterNamespacesFromDocument(inDocumentHandle));

  ASSERT_EQ(SUCCESS, tixiGetTextElement(inDocumentHandle, "/root/h:table/h:tr/h:td[1]/", &text));
  ASSERT_STREQ("Apples", text);

  ASSERT_EQ(SUCCESS, tixiGetTextElement(inDocumentHandle, "/root/h:table/h:tr/h:td[2]/", &text));
  ASSERT_STREQ("Bananas", text);

  // check invalid prefix
  ASSERT_EQ(INVALID_XPATH, tixiGetTextElement(inDocumentHandle, "/root/html:table/html:tr/html:td[1]/", &text));

  // check and register default cpacs namespace
  ASSERT_EQ(ELEMENT_NOT_FOUND, tixiCheckElement(inDocumentHandle, "/root/aircraft"));
  ASSERT_EQ(INVALID_XPATH, tixiCheckElement(inDocumentHandle, "/root/cpacs:aircraft"));
}

TEST_F(Namespaces, addTextElementNS)
{
  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root", "svg:image", "http://www.svg.com", NULL, -1));
  tixiRegisterNamespace(inDocumentHandle, "http://www.svg.com", "svg");

  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:image"));

  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root/svg:image", "circle", "http://www.svg.com", NULL, -1));
  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root/svg:image", "svg:cone", "http://www.svg.com", NULL, -1));
  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:image/svg:circle"));
  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:image/svg:cone"));


  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root/svg:image", "pref:cube", "http://www.svg.com", NULL, -1));
  // NOTE: the svg mainspace is still registered with the svg prefix!
  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:image/svg:cube"));

  // Add some default namespace
  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root", "mesh", "http://www.collada.com", NULL, -1));
  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(inDocumentHandle, "http://www.collada.com", "collada"));
  ASSERT_EQ(SUCCESS, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root/collada:mesh", "points", "http://www.collada.com", NULL, -1));
  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/collada:mesh/collada:points"));

  // Test invalid namespace
  ASSERT_EQ(INVALID_NAMESPACE_URI, tixiAddTextElementNSAtIndex(inDocumentHandle, "/root", "mesh", NULL, NULL, -1));

  tixiSaveDocument(inDocumentHandle, "theout.xml");
}

TEST_F(Namespaces, addDoubleElementNS)
{
  ASSERT_EQ(SUCCESS, tixiAddDoubleElementNS(inDocumentHandle, "/root", "svg:size", "http://www.svg.com", 10.0, "%f"));
  tixiRegisterNamespace(inDocumentHandle, "http://www.svg.com", "svg");

  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:size"));

  ASSERT_EQ(INVALID_NAMESPACE_URI, tixiAddDoubleElementNS(inDocumentHandle, "/root", "svg:size", NULL, 10.0, "%f"));
}

TEST_F(Namespaces, addIntegerElementNS)
{
  ASSERT_EQ(SUCCESS, tixiAddIntegerElementNS(inDocumentHandle, "/root", "svg:number", "http://www.svg.com", 10, NULL));
  tixiRegisterNamespace(inDocumentHandle, "http://www.svg.com", "svg");

  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:number"));

  ASSERT_EQ(INVALID_NAMESPACE_URI, tixiAddIntegerElementNS(inDocumentHandle, "/root", "svg:number", NULL, 10, NULL));
}

TEST_F(Namespaces, addBooleanElementNS)
{
  ASSERT_EQ(SUCCESS, tixiAddBooleanElementNS(inDocumentHandle, "/root", "svg:iscircle", "http://www.svg.com", 0));
  tixiRegisterNamespace(inDocumentHandle, "http://www.svg.com", "svg");

  ASSERT_EQ(SUCCESS, tixiCheckElement(inDocumentHandle, "/root/svg:iscircle"));

  ASSERT_EQ(INVALID_NAMESPACE_URI, tixiAddBooleanElementNS(inDocumentHandle, "/root", "svg:iscircle", NULL, 0));
}

TEST_F(Namespaces, getNamedChildrenCount)
{
    int count = 0;
    tixiRegisterNamespace(inDocumentHandle, "http://www.w3.org/TR/html4/", "html");
    ASSERT_EQ(SUCCESS, tixiGetNamedChildrenCount(inDocumentHandle, "/root/html:table/html:tr", "html:td", &count));
    ASSERT_EQ(2, count);
}

TEST_F(Namespaces, getChildNodeName)
{
    char* name = 0;
    tixiRegisterNamespace(inDocumentHandle, "http://www.w3.org/TR/html4/", "h");
    ASSERT_EQ(SUCCESS, tixiGetChildNodeName(inDocumentHandle, "/root/h:table/h:tr", 1, &name));
    ASSERT_STREQ("h:td", name);
}

/**
  This test should produce the following output

  <root>
    <h:table xmlns:h="http://www.w3.org/TR/html4/">
      <h:tr>
        <h:td>Apples</h:td>
        <h:td>Bananas</h:td>
      </h:tr>
    </h:table>
    <aircraft xmlns="http://www.dlr.de/cpacs">
      <modelname>D150</modelname>
    </aircraft>
  </root>
*/
TEST_F(Namespaces, createRefDocument)
{
  TixiDocumentHandle handle = -1;

  ASSERT_EQ(SUCCESS, tixiCreateDocument("root", &handle));

  ASSERT_EQ(SUCCESS, tixiCreateElementNS(handle, "/root", "h:table", "http://www.w3.org/TR/html4/"));
  tixiRegisterNamespace(handle, "http://www.w3.org/TR/html4/", "h");

  ASSERT_EQ(SUCCESS, tixiCreateElementNS(handle, "/root/h:table", "tr", "http://www.w3.org/TR/html4/"));
  ASSERT_EQ(SUCCESS, tixiAddTextElementNS(handle, "/root/h:table/h:tr", "td", "http://www.w3.org/TR/html4/", "Apples"));
  ASSERT_EQ(SUCCESS, tixiAddTextElementNS(handle, "/root/h:table/h:tr", "td", "http://www.w3.org/TR/html4/", "Bananas"));

  ASSERT_EQ(SUCCESS, tixiCreateElementNS(handle, "/root", "aircraft", "http://www.dlr.de/cpacs"));
  tixiRegisterNamespace(handle, "http://www.dlr.de/cpacs", "c");
  ASSERT_EQ(SUCCESS, tixiAddTextElementNS(handle, "/root/c:aircraft", "modelname", "http://www.dlr.de/cpacs", "D150"));

  tixiSaveDocument(handle, "namespace_generated.xml");
  tixiCloseDocument(handle);

#if __unix__ || unix || __APPLE__

  // check against a reference
  ASSERT_TRUE( system("diff -w namespace_generated.xml TestData/namespace_ref.xml" ) == 0 );
#endif

}

TEST_F(Namespaces, setNamespace)
{
  TixiDocumentHandle handle = -1;
  char* text = NULL;
  const char* refText = "<?xml version=\"1.0\"?>\n<cpacs:root xmlns:cpacs=\"http://www.dlr.de/cpacs\"/>\n";

  ASSERT_EQ(SUCCESS, tixiCreateDocument("root", &handle));
  ASSERT_EQ(SUCCESS, tixiSetElementNamespace(handle, "/root", "http://www.dlr.de/cpacs", "cpacs"));
  tixiRegisterNamespace(handle, "http://www.dlr.de/cpacs", "c");

  ASSERT_EQ(SUCCESS, tixiCheckElement(handle, "/c:root"));

  // check that we created the correct xml
  tixiUsePrettyPrint(handle, 0);
  tixiExportDocumentAsString(handle, &text);
  ASSERT_STREQ(refText, text);

  ASSERT_EQ(SUCCESS, tixiCloseDocument(handle));
}

TEST_F(Namespaces, addNamespaceAttribute)
{
  TixiDocumentHandle handle = -1;
  char* text = NULL;
  const char* refText = "<?xml version=\"1.0\"?>\n<root xmlns:cpacs=\"http://www.dlr.de/cpacs\"/>\n";

  ASSERT_EQ(SUCCESS, tixiCreateDocument("root", &handle));
  ASSERT_EQ(SUCCESS, tixiDeclareNamespace(handle, "/root", "http://www.dlr.de/cpacs", "cpacs"));
  tixiRegisterNamespace(handle, "http://www.dlr.de/cpacs", "c");

  ASSERT_EQ(SUCCESS, tixiCheckElement(handle, "/root"));
  ASSERT_EQ(ELEMENT_NOT_FOUND, tixiCheckElement(handle, "/c:root"));

  // check empty prefixes
  ASSERT_EQ(FAILED, tixiDeclareNamespace(handle, "/root", "http://www.dlr.de/cpacs", NULL));
  ASSERT_EQ(FAILED, tixiDeclareNamespace(handle, "/root", "http://www.dlr.de/cpacs", ""));
  // check empty uri
  ASSERT_EQ(INVALID_NAMESPACE_URI, tixiDeclareNamespace(handle, "/root", NULL, "cpacs2"));

  // check that we created the correct xml
  tixiUsePrettyPrint(handle, 0);
  tixiExportDocumentAsString(handle, &text);
  ASSERT_STREQ(refText, text);

  ASSERT_EQ(SUCCESS, tixiCloseDocument(handle));
}

TEST(NamespaceFunctions, extractPrefixAndName)
{
  char* prefix = NULL;
  char* name = NULL;

  extractPrefixAndName("html:td", &prefix, &name);
  ASSERT_STREQ("html", prefix);
  ASSERT_STREQ("td", name);
  free(prefix);
  free(name);
  prefix = NULL;
  name = NULL;

  extractPrefixAndName("mynewtag", &prefix, &name);
  ASSERT_TRUE(prefix==NULL);
  ASSERT_STREQ("mynewtag", name);
  free(name);
  name = NULL;

  extractPrefixAndName("h:table", &prefix, &name);
  ASSERT_STREQ("h", prefix);
  ASSERT_STREQ("table", name);
  free(prefix);
  free(name);
  name = NULL;
  prefix = NULL;

}

TEST_F(Namespaces, attributesWithNS)
{
  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(inDocumentHandle, "http://www.w3.org/TR/html4/", "h"));

  ASSERT_EQ(SUCCESS, tixiAddDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:width", 6., "%f"));
  // prefix without any namespace
  ASSERT_EQ(SUCCESS, tixiAddDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "size", 2., "%f"));
  // Invalid prefix
  ASSERT_EQ(INVALID_NAMESPACE_PREFIX, tixiAddDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "g:length", 0., "%f"));

  double number;
  ASSERT_EQ(SUCCESS, tixiGetDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "size", &number));
  ASSERT_NEAR(2., number, 1e-12);
  ASSERT_EQ(SUCCESS, tixiGetDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:width", &number));
  ASSERT_NEAR(6., number, 1e-12);

  // invalid prefix
  ASSERT_EQ(INVALID_NAMESPACE_PREFIX, tixiGetDoubleAttribute(inDocumentHandle, "/root/h:table/h:tr", "g:width", &number));

  int attCount = 0;
  ASSERT_EQ(SUCCESS, tixiGetNumberOfAttributes(inDocumentHandle, "/root/h:table/h:tr", &attCount));
  ASSERT_EQ(2, attCount);

  // test attribute names
  char * name = NULL;
  ASSERT_EQ(SUCCESS, tixiGetAttributeName(inDocumentHandle, "/root/h:table/h:tr", 1, &name));
  EXPECT_STREQ("h:width", name);

  ASSERT_EQ(SUCCESS, tixiGetAttributeName(inDocumentHandle, "/root/h:table/h:tr", 2, &name));
  EXPECT_STREQ("size", name);

  ASSERT_EQ(SUCCESS, tixiCheckAttribute(inDocumentHandle, "/root/h:table/h:tr", "size"));
  ASSERT_EQ(SUCCESS, tixiCheckAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:width"));
  ASSERT_EQ(ATTRIBUTE_NOT_FOUND, tixiCheckAttribute(inDocumentHandle, "/root/h:table/h:tr", "g:width"));
  ASSERT_EQ(ATTRIBUTE_NOT_FOUND, tixiCheckAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:size"));

  // remove the size attribute
  EXPECT_EQ(SUCCESS, tixiRemoveAttribute(inDocumentHandle, "/root/h:table/h:tr", "size"));
  EXPECT_EQ(SUCCESS, tixiGetNumberOfAttributes(inDocumentHandle, "/root/h:table/h:tr", &attCount));
  EXPECT_EQ(1, attCount);

  // remove with invalid prefix
  EXPECT_EQ(INVALID_NAMESPACE_PREFIX, tixiRemoveAttribute(inDocumentHandle, "/root/h:table/h:tr", "g:size"));

  // correct prefix but invalid attribute
  EXPECT_EQ(ATTRIBUTE_NOT_FOUND, tixiRemoveAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:size"));

  // remove h:width attribute
  EXPECT_EQ(SUCCESS, tixiRemoveAttribute(inDocumentHandle, "/root/h:table/h:tr", "h:width"));
  EXPECT_EQ(SUCCESS, tixiGetNumberOfAttributes(inDocumentHandle, "/root/h:table/h:tr", &attCount));
  EXPECT_EQ(0, attCount);
}

TEST_F(Namespaces, createCpacsFile)
{
  const char * refText = "<?xml version=\"1.0\"?>\n"
                         "<cpacs xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"cpacs_schema.xsd\">\n"
                         "  <header/>\n"
                         "</cpacs>\n";

  char * text = NULL;

  TixiDocumentHandle handle;
  ASSERT_EQ(SUCCESS, tixiCreateDocument("cpacs", &handle));
  ASSERT_EQ(SUCCESS, tixiDeclareNamespace(handle, "/cpacs", "http://www.w3.org/2001/XMLSchema-instance", "xsi"));
  ASSERT_EQ(SUCCESS, tixiRegisterNamespace(handle, "http://www.w3.org/2001/XMLSchema-instance", "xsi"));
  ASSERT_EQ(SUCCESS, tixiAddTextAttribute(handle, "/cpacs", "xsi:noNamespaceSchemaLocation", "cpacs_schema.xsd"));
  ASSERT_EQ(SUCCESS, tixiCreateElement(handle, "/cpacs", "header"));

  tixiExportDocumentAsString(handle, &text);
  ASSERT_STREQ(refText, text);

  tixiCloseDocument(handle);
}

