package com.sos.scheduler.engine.kernel.util;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.util.xml.NamedChildElements;
import com.sos.scheduler.engine.util.xml.SiblingElementIterator;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

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
import java.io.*;
import java.util.Iterator;

import static javax.xml.transform.OutputKeys.OMIT_XML_DECLARATION;
import static org.w3c.dom.Node.DOCUMENT_NODE;

public final class XmlUtils {
    private XmlUtils() {}

    public static Document newDocument() {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
        }
        catch (ParserConfigurationException x) { throw new XmlException(x); }
    }

    public static Document loadXml(byte[] xml) {
        return loadXml(new ByteArrayInputStream(xml));
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
        String result = w.toString();
        return n.getNodeType() != DOCUMENT_NODE && result.startsWith("<?")?
                result.replaceFirst("^<[?][xX][mM][lL].+[?][>]\\w*", "")   // Manche DOM-Implementierung liefert den XML-Prolog.
                : result;
    }

    public static void writeXmlTo(Node n, Writer w) {
        try {
            Transformer transformer = TransformerFactory.newInstance().newTransformer();
            if (n.getNodeType() == DOCUMENT_NODE)
                transformer.setOutputProperty(OMIT_XML_DECLARATION, "no");   //TODO Funktioniert nur mit org.jdom?
            transformer.transform(new DOMSource(n), new StreamResult(w));
        } catch (TransformerException x) { throw new XmlException(x); }
    }

    public static boolean booleanXmlAttribute(Element xmlElement, String attributeName, boolean defaultValue) {
        String value = xmlElement.getAttribute(attributeName);
        Boolean result = booleanOrNullOf(value, defaultValue);
        if (result == null)
            throw new RuntimeException("Invalid Boolean value in <"+ xmlElement.getNodeName() +" "+ attributeName + "=" + xmlQuoted(value) + ">");
        return result;
    }

    @Nullable private static Boolean booleanOrNullOf(String s, boolean deflt) {
        return s.equals("true")? true :
               s.equals("false")? false :
               s.equals("1")? true :
               s.equals("0")? false :
               s.isEmpty()? deflt : null;
    }

    public static int intXmlAttribute(Element xmlElement, String attributeName) {
        return intXmlAttribute(xmlElement, attributeName, (Integer)null);
    }

    public static int intXmlAttribute(Element xmlElement, String attributeName, @Nullable Integer defaultValue) {
        String value = xmlAttribute(xmlElement, attributeName, "");
        if (!value.isEmpty()) {
            try {
                return Integer.parseInt(value);
            } catch (NumberFormatException x) {
                throw new RuntimeException("Invalid numeric value in <"+ xmlElement.getNodeName() +" "+ attributeName +"="+ xmlQuoted(value) +">", x);
            }
        } else {
            if (defaultValue == null)
                throw missingAttributeException(xmlElement, attributeName);
            return defaultValue;
        }
    }

    public static String xmlAttribute(Element xmlElement, String attributeName, @Nullable String defaultValue) {
        String result = xmlElement.getAttribute(attributeName);
        if (!result.isEmpty())
            return result;
        else {
            if (defaultValue == null)
                throw missingAttributeException(xmlElement, attributeName);
            return defaultValue;
        }
    }

    private static RuntimeException missingAttributeException(Element e, String attributeName) {
        return new RuntimeException("Missing attribute <"+ e.getNodeName() +" "+ attributeName +"=...>");
    }

    public static Element elementXPath(Node baseNode, String xpathExpression) {
        Element result = elementXPathOrNull(baseNode, xpathExpression);
        if (result == null)  throw new XmlException("XPath does not return an element: "+ xpathExpression);
        return result;
    }

    @Nullable public static Element elementXPathOrNull(Node baseNode, String xpathExpression) {
        try {
            return (Element)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.NODE);
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }


    public static ImmutableList<Element> elementsXPath(Node baseNode, String xpathExpression) {
        try {
            return listFromNodeList((NodeList)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.NODESET));
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }


    public static ImmutableList<Element> listFromNodeList(NodeList list) {
        ImmutableList.Builder<Element> result = ImmutableList.builder();
        for (int i = 0; i < list.getLength(); i++)  result.add((Element)list.item(i));
        return result.build();
    }


    public static String stringXPath(Node baseNode, String xpathExpression) {
        try {
            String result = (String)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.STRING);
            if (result == null)  throw new XmlException("XPath does not match: "+ xpathExpression);
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

    public static ImmutableList<Element> childElements(Element element) {
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

    @Nullable public static Element childElementOrNull(Element e, String name) {
        Iterator<Element> i = new NamedChildElements(name, e).iterator();
        return i.hasNext()? i.next() : null;
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
