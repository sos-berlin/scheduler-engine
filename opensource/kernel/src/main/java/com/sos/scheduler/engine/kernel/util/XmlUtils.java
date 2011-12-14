package com.sos.scheduler.engine.kernel.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import javax.annotation.Nullable;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.util.xml.NamedChildElements;
import com.sos.scheduler.engine.util.xml.SiblingElementIterator;

public final class XmlUtils {
    private XmlUtils() {}

    public static Document newDocument() {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
        }
        catch (ParserConfigurationException x) { throw new XmlException(x); }
    }

    public static Document loadXml(InputStream in) {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(in);
        }
        catch (IOException x) { throw new XmlException(x); }
        catch (ParserConfigurationException x) { throw new XmlException(x); }
        catch (SAXException x) { throw new XmlException(x); }
    }

    public static Document loadXml(String xml) {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(new InputSource(new StringReader(xml)));
        }
        catch (IOException x) { throw new XmlException(x); }
        catch (ParserConfigurationException x) { throw new XmlException(x); }
        catch (SAXException x) { throw new XmlException(x); }
    }

    public static String toXml(Node n) {
        StringWriter w = new StringWriter();
        writeXmlTo(n, w);
        return w.toString();
    }

    public static void writeXmlTo(Node n, Writer w) {
        try {
            Transformer transformer = TransformerFactory.newInstance().newTransformer();
            transformer.setOutputProperty("omit-xml-declaration", "false");   //TODO Funktioniert nur mit org.jdom?
            transformer.transform(new DOMSource(n), new StreamResult(w));
        } catch (TransformerException x) { throw new XmlException(x); }
    }

    public static boolean booleanXmlAttribute(Element xmlElement, String attributeName, boolean defaultValue) {
        String value = xmlElement.getAttribute(attributeName);
        Boolean result = booleanOrNullOf(value, defaultValue);
        if (result == null)
            throw new RuntimeException("Ungültiger Boolescher Wert in <" + xmlElement.getNodeName() + " " + attributeName + "=" + xmlQuoted(value) + ">");
        return result;
    }

    @Nullable private static Boolean booleanOrNullOf(String s, boolean deflt) {
        return s.equals("true")? true :
               s.equals("false")? false :
               s.equals("1")? true :
               s.equals("0")? false :
               s.isEmpty()? deflt : null;
    }

    public static int intXmlAttribute(Element xmlElement, String attributeName, int defaultValue) {
        String value = xmlElement.getAttribute(attributeName);
        if (value.isEmpty())  return defaultValue;

        try {
            return Integer.parseInt( value );
        } catch (NumberFormatException x) {
            throw new RuntimeException("Ungültiger numerischer Wert in <" + xmlElement.getNodeName() + " " + attributeName + "=" + xmlQuoted(value) + ">", x);
        }
    }

    public static Element elementXPath(Node baseNode, String xpathExpression) {
        Element result = elementXPathOrNull(baseNode, xpathExpression);
        if (result == null)  throw new XmlException("XPath liefert kein Element: " + xpathExpression);
        return result;
    }

    public static Element elementXPathOrNull(Node baseNode, String xpathExpression) {
        try {
            return (Element)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.NODE);
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }


    public static Collection<Element> elementsXPath(Node baseNode, String xpathExpression) {
        try {
            return listFromNodeList((NodeList)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.NODESET));
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }


    public static List<Element> listFromNodeList(NodeList list) {
        ArrayList<Element> result = new ArrayList<Element>(list.getLength());
        for (int i = 0; i < list.getLength(); i++)  result.add((Element)list.item(i));
        return result;
    }


    public static String stringXPath(Node baseNode, String xpathExpression) {
        try {
            String result = (String)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.STRING);
            if (result == null)  throw new XmlException("XPath passt nicht: " + xpathExpression);
            return result;
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }


    public static String stringXPath(Node baseNode, String xpathExpression, String deflt) {
        try {
            String result = (String)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.STRING);
            if (result == null)  result = deflt;
            return result;
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }

    public static boolean booleanXPath(Node baseNode, String xpathExpression) {
        try {
            return (Boolean)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.BOOLEAN);
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }

    public static XPath newXPath() {
        return XPathFactory.newInstance().newXPath();
    }

    public static List<Element> childElements(Element element) {
        return ImmutableList.copyOf(new SiblingElementIterator(element.getFirstChild()));
//        NodeList children = element.getChildNodes();
//        ArrayList<Element> result = new ArrayList<Element>(children.getLength());
//        for (int i = 0; i < children.getLength(); i++) {
//            Node child = children.item(i);
//            if (child.getNodeType() == Node.ELEMENT_NODE)
//                result.add((Element)child);
//        }
//        return result;
    }

    public static List<Element> namedChildElements(String name, Element element) {
        return ImmutableList.copyOf(new NamedChildElements(name, element));
    }

    public static NodeList nodeListXpath(Node baseNode, String xpathExpression) {
        try {
            XPath xpath = XPathFactory.newInstance().newXPath();
            return (NodeList)xpath.evaluate( xpathExpression, baseNode, XPathConstants.NODESET );
        } catch (XPathExpressionException x) { throw new XmlException( x ); }   // Programmfehler
    }

    public static String xmlQuoted(String value) {
        StringBuilder result = new StringBuilder(value.length() + 20);

        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt( i );
            switch (c) {
                case '"': result.append("&quot;");  break;
                case '&': result.append("&amp;" );  break;
                case '<': result.append("&lt;"  );  break;
                default:  result.append(c);
            }
        }

        return "\"" + result + "\"";
    }

    private static final class XmlException extends RuntimeException {
        private XmlException(Exception x) {
            super(x);
        }

        private XmlException(String s) {
            super(s);
        }
    }
}
