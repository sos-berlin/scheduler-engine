package com.sos.scheduler.engine.kernel.util;

import com.sos.scheduler.engine.kernel.SchedulerException;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.List;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
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


public class XmlUtils {
    private XmlUtils() {}


    public static Document loadXml(InputStream in) {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(in);
        }
        catch (IOException x) { throw new RuntimeException(x); }
        catch (ParserConfigurationException x) { throw new RuntimeException(x); }
        catch (SAXException x) { throw new RuntimeException(x); }
    }


    public static Document loadXml(String xml) {
        try {
            return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(new InputSource(new StringReader(xml)));
        }
        catch (IOException x) { throw new RuntimeException(x); }
        catch (ParserConfigurationException x) { throw new RuntimeException(x); }
        catch (SAXException x) { throw new RuntimeException(x); }
    }


    public static boolean booleanXmlAttribute(Element xmlElement, String attributeName, boolean defaultValue) {
        String value = xmlElement.getAttribute(attributeName);
        if (value.equals("true" ))  return true;
        if (value.equals("false"))  return false;
        if (value.equals("1"    ))  return true;
        if (value.equals("0"    ))  return false;
        if (value.isEmpty() )  return defaultValue;
        throw new SchedulerException("Ungültiger Boolescher Wert in <" + xmlElement.getNodeName() + " " + attributeName + "=" + xmlQuoted(value) + ">");
    }


    public static int intXmlAttribute(Element xmlElement, String attributeName, int defaultValue) {
        String value = xmlElement.getAttribute(attributeName);
        if (value.isEmpty())  return defaultValue;

        try {
            return Integer.parseInt( value );
        } catch (NumberFormatException x) {
            throw new SchedulerException("Ungültiger numerischer Wert in <" + xmlElement.getNodeName() + " " + attributeName + "=" + xmlQuoted(value) + ">", x);
        }
    }



    public static Element elementXPath(Node baseNode, String xpathExpression) {
        Element result = elementXPathOrNull(baseNode, xpathExpression);
        if (result == null)  throw new RuntimeException("XPath liefert kein Element: " + xpathExpression);
        return result;
    }


    public static Element elementXPathOrNull(Node baseNode, String xpathExpression) {
        try {
            XPath xpath  = XPathFactory.newInstance().newXPath();
            return (Element)xpath.evaluate(xpathExpression, baseNode, XPathConstants.NODE);
        } catch (XPathExpressionException x) { throw new RuntimeException(x); }
    }


    public static List<Element> elementsXPath(Node baseNode, String xpathExpression) {
        try {
            XPath xpath  = XPathFactory.newInstance().newXPath();
            return listFromNodeList((NodeList)xpath.evaluate(xpathExpression, baseNode, XPathConstants.NODESET));
        } catch (XPathExpressionException x) { throw new RuntimeException(x); }
    }


    public static List<Element> listFromNodeList(NodeList list) {
        ArrayList<Element> result = new ArrayList<Element>(list.getLength());
        for (int i = 0; i < list.getLength(); i++)  result.add((Element)list.item(i));
        return result;
    }


    public static String stringXPath(Node baseNode, String xpathExpression, String deflt) {
        try {
            XPath xpath  = XPathFactory.newInstance().newXPath();
            String result = (String)xpath.evaluate(xpathExpression, baseNode, XPathConstants.STRING);
            if (result == null)  result = deflt;
            return result;
        } catch (XPathExpressionException x) { throw new RuntimeException(x); }
    }


    public static ArrayList<Element> childElements(Element element) {
        NodeList children = element.getChildNodes();
        ArrayList<Element> result = new ArrayList<Element>(children.getLength());
        for (int i = 0; i < children.getLength(); i++) {
            Node child = children.item(i);
            if (child.getNodeType() == Node.ELEMENT_NODE)
                result.add((Element)child);
        }
        return result;
    }


    public static NodeList nodeListXpath(Node baseNode, String xpathExpression) {
        try {
            XPath xpath = XPathFactory.newInstance().newXPath();
            return (NodeList)xpath.evaluate( xpathExpression, baseNode, XPathConstants.NODESET );
        } catch (XPathExpressionException x) { throw new RuntimeException( x ); }   // Programmfehler
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
}
