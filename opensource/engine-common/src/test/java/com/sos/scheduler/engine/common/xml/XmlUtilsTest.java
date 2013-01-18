package com.sos.scheduler.engine.common.xml;

import org.junit.Ignore;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import java.util.List;

import static com.google.common.base.Charsets.US_ASCII;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.hasSize;

public final class XmlUtilsTest {
    @Test public void testLoadXml_String() {
        String xml = "<a>A</a>";
        Document result = XmlUtils.loadXml(xml);
        assertThat(result.getDocumentElement().getNodeName(), equalTo("a"));
    }

    @Test public void testToXmlBytes() {
        String xml = "<a b=\"B\">Ä</a>";
        Document doc = XmlUtils.loadXml(xml);
        byte[] result = XmlUtils.toXmlBytes(doc, "ASCII", false);
        String resultString = new String(result, US_ASCII);
        assertThat(resultString, equalTo("<?xml version=\"1.0\" encoding=\"US-ASCII\"?>" +  // Warum steht hier standalone?
                "<a b=\"B\">&#196;</a>"));
    }

    // Test zunächst deaktiviert, da er unter Unix fehlschlägt. SS 18.01.2013
    @Ignore
    public void testToXmlBytesIndented() {
        String xml = "<a><b>B</b></a>";
        Document doc = XmlUtils.loadXml(xml);
        byte[] result = XmlUtils.toXmlBytes(doc, "ASCII", true);
        String resultString = new String(result, US_ASCII);
        String nl = System.getProperty("line.separator");
        assertThat(resultString, equalTo("<?xml version=\"1.0\" encoding=\"US-ASCII\"?>" + //nl +
                "<a>"+nl+"    <b>B</b>"+nl+"</a>"+nl));
    }

    @Test public void testToXml() {
        String xml = "<a b=\"B\">Ä</a>";
        Document doc = XmlUtils.loadXml(xml);
        String result = XmlUtils.toXml(doc.getDocumentElement());
        assertThat(result, equalTo(xml));
    }

    @Test public void testBooleanXmlAttribute_true() {
        String[] trueXmls = {"<a b='1'/>", "<a b='true'/>"};
        testBooleanXmlAttribute(trueXmls, true);
    }

    @Test public void testBooleanXmlAttribute_false() {
        String[] trueXmls = {"<a b='0'/>", "<a b='false'/>"};
        testBooleanXmlAttribute(trueXmls, false);
    }

    private static void testBooleanXmlAttribute(String[] xmls, boolean expected) {
        for (String xml: xmls) {
            Element e = XmlUtils.loadXml(xml).getDocumentElement();
            assertThat(XmlUtils.booleanXmlAttribute(e, "b", false), equalTo(expected));
            assertThat(XmlUtils.booleanXmlAttribute(e, "b", true), equalTo(expected));
        }
    }

    @Test public void testBooleanXmlAttribute_default() {
        String[] xmls = {"<a/>", "<a b=''/>"};
        testBooleanXmlAttribute_default(xmls, false);
        testBooleanXmlAttribute_default(xmls, true);
    }

    private static void testBooleanXmlAttribute_default(String[] xmls, boolean deflt) {
        for (String xml: xmls) {
            Element e = XmlUtils.loadXml(xml).getDocumentElement();
            assertThat(XmlUtils.booleanXmlAttribute(e, "b", deflt), equalTo(deflt));
            assertThat(XmlUtils.booleanXmlAttribute(e, "b", deflt), equalTo(deflt));
        }
    }

    @Test(expected=RuntimeException.class) public void testBooleanXmlAttribute_exception() {
        Element e = XmlUtils.loadXml("<a b='x'/>").getDocumentElement();
        XmlUtils.booleanXmlAttribute(e, "b", false);
    }

    @Test public void testIntXmlAttribute() {
        Element e = XmlUtils.loadXml("<a b='4711'/>").getDocumentElement();
        assertThat(XmlUtils.intXmlAttribute(e, "b", -99), equalTo(4711));
        assertThat(XmlUtils.intXmlAttribute(e, "x", -99), equalTo(-99));
    }

    @Test(expected=Exception.class) public void testMissingIntXmlAttribute() {
        Element e = XmlUtils.loadXml("<a/>").getDocumentElement();
        XmlUtils.intXmlAttribute(e, "b");
    }

    @Test(expected=Exception.class) public void testEmptyIntXmlAttribute() {
        Element e = XmlUtils.loadXml("<a b=''/>").getDocumentElement();
        XmlUtils.intXmlAttribute(e, "b");
    }

    @Test public void testChildElements() {
        testChildElements("<root><a/>x<b><bb/></b>x<c/></root>", "a", "b", "c");
        testChildElements("<root>xx<a/>x<b><bb/></b>x<c/>xx</root>", "a", "b", "c");
    }

    private static void testChildElements(String xml, String... expected) {
        Element e = XmlUtils.loadXml(xml).getDocumentElement();
        List<Element> list = XmlUtils.childElements(e);
        assertThat(list, hasSize(expected.length));
        for (int i = 0; i < list.size(); i++)
            assertThat(list.get(i).getTagName(), equalTo(expected[i]));
    }
}