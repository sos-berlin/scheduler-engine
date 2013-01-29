package com.sos.scheduler.engine.common.xml;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import javax.annotation.Nullable;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Result;
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
import java.nio.charset.Charset;
import java.util.Iterator;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.base.Throwables.propagate;
import static javax.xml.transform.OutputKeys.*;
import static org.w3c.dom.Node.DOCUMENT_NODE;

@ForCpp
public final class XmlUtils {
    private static final Logger logger = LoggerFactory.getLogger(XmlUtils.class);
    private static boolean static_xPathNullPointerLogged = false;

    @ForCpp public static Document newDocument() {
        Document result = newDocumentBuilder().newDocument();
        postInitializeDocument(result);
        return result;
    }

    @ForCpp public static Document loadXml(byte[] xml, String encoding) {
        String s;
        try {
            Reader in = new InputStreamReader(new ByteArrayInputStream(xml), Charset.forName(encoding));
            s = com.google.common.io.CharStreams.toString(in);
        } catch (Exception x) { throw propagate(x); }
        return loadXml(new InputStreamReader(new ByteArrayInputStream(xml), Charset.forName(encoding)));
    }

    public static Document loadXml(String xml) {
        return loadXml(new StringReader(xml));
    }

    public static Document loadXml(Reader in) {
        try {
            Document result = newDocumentBuilder().parse(new InputSource(in));
            postInitializeDocument(result);
            return result;
        }
        catch (IOException x) { throw new XmlException(x); }
        catch (SAXException x) { throw new XmlException(x); }
    }

    public static Document loadXml(InputStream in) {
        try {
            Document result = newDocumentBuilder().parse(in);
            postInitializeDocument(result);
            return result;
        }
        catch (IOException x) { throw new XmlException(x); }
        catch (SAXException x) { throw new XmlException(x); }
    }

    private static DocumentBuilder newDocumentBuilder() {
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setNamespaceAware(true);
            return factory.newDocumentBuilder();
        }
        catch (ParserConfigurationException x) { throw new XmlException(x); }
    }

    private static void postInitializeDocument(Document doc) {
        doc.setXmlStandalone(true);
    }

    @ForCpp public static byte[] toXmlBytes(Node n, String encoding, boolean indent) {
        ByteArrayOutputStream o = new ByteArrayOutputStream();
        writeXmlTo(n, o, Charset.forName(encoding), indent);
        return o.toByteArray();
    }

    public static String toXml(Node n) {
        StringWriter w = new StringWriter();
        writeXmlTo(n, w);
        String result = w.toString();
        return n.getNodeType() == DOCUMENT_NODE? result : removeXmlProlog(result);  // Manche DOM-Implementierung liefert den XML-Prolog.
    }

    private static String removeXmlProlog(String xml) {
        // Prolog muss am Anfang stehen, ohne vorangehende Blanks oder Kommentare
        return xml.startsWith("<?")? xml.replaceFirst("^<[?][xX][mM][lL].+[?][>]\\w*", "") : xml;
    }

    public static void writeXmlTo(Node n, OutputStream o, Charset encoding, boolean indent) {
        writeXmlTo(n, new StreamResult(o), encoding, indent);
    }

    public static void writeXmlTo(Node n, Writer w) {
        writeXmlTo(n, new StreamResult(w), null, false);
    }

    private static void writeXmlTo(Node n, Result r, @Nullable Charset encoding, boolean indent) {
        try {
            Transformer transformer = TransformerFactory.newInstance().newTransformer();
            if (encoding != null)
                transformer.setOutputProperty(ENCODING, encoding.name());
            transformer.setOutputProperty(OMIT_XML_DECLARATION, n.getNodeType() == DOCUMENT_NODE? "no" : "yes");   //TODO Funktioniert nur mit org.jdom?
            if (indent) {
                transformer.setOutputProperty(INDENT, "yes");
                transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "4");
            }
            transformer.transform(new DOMSource(n), r);
        } catch (TransformerException x) { throw new XmlException(x); }
    }

    @ForCpp public static boolean booleanXmlAttribute(Element xmlElement, String attributeName, boolean defaultValue) {
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

    @ForCpp public static int intXmlAttribute(Element xmlElement, String attributeName) {
        return intXmlAttribute(xmlElement, attributeName, (Integer)null);
    }

    @ForCpp public static int intXmlAttribute(Element xmlElement, String attributeName, @Nullable Integer defaultValue) {
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

    @ForCpp public static String xmlAttribute(Element xmlElement, String attributeName, @Nullable String defaultValue) {
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
            return elementListFromNodeList((NodeList)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.NODESET));
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }

    public static ImmutableList<Element> elementListFromNodeList(NodeList list) {
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
            return firstNonNull(result, deflt);
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }

    public static boolean booleanXPath(Node baseNode, String xpathExpression) {
        try {
            return (Boolean)newXPath().evaluate(xpathExpression, baseNode, XPathConstants.BOOLEAN);
        } catch (XPathExpressionException x) { throw new XmlException(x); }
    }

    public static ImmutableList<Element> childElements(Element element) {
        return ImmutableList.copyOf(new SiblingElementIterator(element.getFirstChild()));
    }

    @Nullable public static Element childElementOrNull(Element e, String name) {
        Iterator<Element> i = new NamedChildElements(name, e).iterator();
        return i.hasNext()? i.next() : null;
    }

    @ForCpp public static NodeList xpathNodeList(Node baseNode, String xpathExpression) {
        try {
            XPath xpath = newXPath();
            return (NodeList)xpath.evaluate( xpathExpression, baseNode, XPathConstants.NODESET );
        } catch (XPathExpressionException x) { throw new XmlException( x ); }   // Programmfehler
    }

    @ForCpp public static Node xpathNode(Node baseNode, String xpathExpression) {
        try {
            XPath xpath = newXPath();
            return (Node)xpath.evaluate(xpathExpression, baseNode, XPathConstants.NODE);
        } catch (XPathExpressionException x) { throw new XmlException( x ); }   // Programmfehler
    }

    public static XPath newXPath() {
        return newXPathFactory().newXPath();
    }

    private static XPathFactory newXPathFactory() {
        try {
            return XPathFactory.newInstance();
        } catch (NullPointerException e) {
            return workaroundNewXPathFactory(e);
        }
    }

    private static XPathFactory workaroundNewXPathFactory(NullPointerException e) {
        // JSSIXFOUR-8: NullPointerException in javax.xml.xpath.XPathFactoryFinder, wenn Scheduler als Dienst läuft
        String workAroundClassName = "com.sun.org.apache.xpath.internal.jaxp.XPathFactoryImpl";
        if (!static_xPathNullPointerLogged) {
            logger.debug("Trying to use "+workAroundClassName+" as a workaround after {}", e, e);
            static_xPathNullPointerLogged = true;
        }
        try {
            @SuppressWarnings("unchecked")
            XPathFactory result = ((Class<XPathFactory>)Class.forName(workAroundClassName)).newInstance();
            return result;
        } catch (Throwable ee) {
            logger.debug("Workaround failed", ee);
            // Ab Java 7: e.addSuppressed(ee);
            throw e;
        }
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

    private XmlUtils() {}
}
