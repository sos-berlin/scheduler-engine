package com.sos.scheduler.engine.common.xml

import com.google.common.base.Objects.firstNonNull
import com.google.common.collect.ImmutableList
import com.sos.scheduler.engine.common.scalautil.ModifiedBy.modifiedBy
import com.sos.scheduler.engine.common.scalautil.StringWriters.writingString
import com.sos.scheduler.engine.common.scalautil.{ModifiedBy, Logger}
import com.sos.scheduler.engine.common.scalautil.ScalaThreadLocal._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import java.io._
import java.nio.charset.Charset
import javax.annotation.Nullable
import javax.xml.parsers.DocumentBuilderFactory
import javax.xml.transform.OutputKeys._
import javax.xml.transform.dom.DOMSource
import javax.xml.transform.stream.StreamResult
import javax.xml.transform.{Result, TransformerFactory}
import javax.xml.xpath.{XPathConstants, XPathFactory}
import org.w3c.dom.Node.DOCUMENT_NODE
import org.w3c.dom.{Document, Element, Node, NodeList}
import org.xml.sax.InputSource
import scala.sys.error

@ForCpp object XmlUtils {
  private val logger = Logger(getClass)
  private lazy val xPathFactory = newXPathFactory()
  private lazy val xPath = threadLocal { xPathFactory.newXPath() }
  private val documentBuilder = threadLocal { (DocumentBuilderFactory.newInstance() modifiedBy { _.setNamespaceAware(true) }).newDocumentBuilder() }
  private val transformerFactory = threadLocal { TransformerFactory.newInstance() }
  private var static_xPathNullPointerLogged = false

  @ForCpp def newDocument(): Document = {
    val result = documentBuilder.newDocument()
    postInitializeDocument(result)
    result
  }

  def prettyXml(xml: String): String =
    writingString { w => writeXmlTo(loadXml(xml), new StreamResult(w), None, indent=true) }

  @ForCpp def loadXml(xml: Array[Byte], encoding: String): Document =
    loadXml(new InputStreamReader(new ByteArrayInputStream(xml), Charset.forName(encoding)))

  def loadXml(xml: String): Document =
    loadXml(new StringReader(xml))

  def loadXml(in: Reader): Document =
    documentBuilder.parse(new InputSource(in)) modifiedBy postInitializeDocument

  def loadXml(in: InputStream): Document = {
    val result = documentBuilder.parse(in)
    postInitializeDocument(result)
    result
  }

  private def postInitializeDocument(doc: Document) {
    doc.setXmlStandalone(true)
  }

  @ForCpp def toXmlBytes(n: Node, encoding: String, indent: Boolean): Array[Byte] = {
    val o = new ByteArrayOutputStream
    writeXmlTo(n, o, Charset.forName(encoding), indent)
    o.toByteArray
  }

  def toXml(n: Node): String = {
    val result = writingString { w => writeXmlTo(n, w) }
    if (n.getNodeType == DOCUMENT_NODE) result
    else removeXmlProlog(result)
  }

  private def removeXmlProlog(xml: String) =
    if (xml startsWith "<?") xml.replaceFirst("^<[?][xX][mM][lL].+[?][>]\\w*", "") else xml

  def writeXmlTo(n: Node, o: OutputStream, encoding: Charset, indent: Boolean) {
    writeXmlTo(n, new StreamResult(o), Some(encoding), indent)
  }

  def writeXmlTo(n: Node, w: Writer) {
    writeXmlTo(n, new StreamResult(w), None)
  }

  private def writeXmlTo(n: Node, r: Result, encoding: Option[Charset], indent: Boolean = false) {
    val transformer = transformerFactory.newTransformer()
    for (o <- encoding) transformer.setOutputProperty(ENCODING, o.name)
    transformer.setOutputProperty(OMIT_XML_DECLARATION, if (n.getNodeType == DOCUMENT_NODE) "no" else "yes")
    if (indent) {
      transformer.setOutputProperty(INDENT, "yes")
      transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "4")
    }
    transformer.transform(new DOMSource(n), r)
  }

  @ForCpp def booleanXmlAttribute(xmlElement: Element, attributeName: String, defaultValue: Boolean): Boolean = {
    val value = xmlElement.getAttribute(attributeName)
    booleanOptionOf(value, defaultValue) getOrElse error(s"Invalid Boolean value in <${xmlElement.getNodeName} $attributeName=${xmlQuoted(value)}>")
  }

  private def booleanOptionOf(s: String, deflt: Boolean): Option[Boolean] = s match {
    case "true" => Some(true)
    case "false" => Some(false)
    case "1" => Some(true)
    case "0" => Some(false)
    case _ if (s.isEmpty) => Some(deflt)
    case _ => None
  }

  @ForCpp def intXmlAttribute(xmlElement: Element, attributeName: String): Int =
    intXmlAttribute(xmlElement, attributeName, null.asInstanceOf[Integer])

  @ForCpp def intXmlAttribute(xmlElement: Element, attributeName: String, @Nullable defaultValue: Integer): Int = {
    val value = xmlAttribute(xmlElement, attributeName, "")
    if (!value.isEmpty) {
      try Integer.parseInt(value)
      catch {
        case x: NumberFormatException =>
          throw new RuntimeException(s"Invalid numeric value in <${xmlElement.getNodeName} $attributeName=${xmlQuoted(value)}>", x)
      }
    }
    else {
      if (defaultValue == null) throw missingAttributeException(xmlElement, attributeName)
      defaultValue
    }
  }

  @ForCpp def xmlAttribute(xmlElement: Element, attributeName: String, @Nullable defaultValue: String): String = {
    val result = xmlElement.getAttribute(attributeName)
    if (!result.isEmpty) result
    else {
      if (defaultValue == null) throw missingAttributeException(xmlElement, attributeName)
      defaultValue
    }
  }

  private def missingAttributeException(e: Element, attributeName: String) =
    new RuntimeException(s"Missing attribute <${e.getNodeName} $attributeName=...>")

  def elementXPath(baseNode: Node, xpathExpression: String): Element =
    elementXPathOption(baseNode, xpathExpression) getOrElse error(s"XPath does not return an element: $xpathExpression")

  @Nullable def elementXPathOrNull(baseNode: Node, xpathExpression: String): Element =
    elementXPathOption(baseNode, xpathExpression).orNull

  def elementXPathOption(baseNode: Node, xpathExpression: String): Option[Element] =
    Option(xPath.evaluate(xpathExpression, baseNode, XPathConstants.NODE).asInstanceOf[Element])

  def elementsXPath(baseNode: Node, xpathExpression: String) =
    elementListFromNodeList(xPath.evaluate(xpathExpression, baseNode, XPathConstants.NODESET).asInstanceOf[NodeList])

  def elementListFromNodeList(list: NodeList): IndexedSeq[Element] =
    0 until list.getLength map list.item map { _.asInstanceOf[Element] }

  def stringXPath(baseNode: Node, xpathExpression: String): String = {
      val result = xPath.evaluate(xpathExpression, baseNode, XPathConstants.STRING).asInstanceOf[String]
      if (result == null) error("XPath does not match: " + xpathExpression)
      result
  }

  def stringXPath(baseNode: Node, xpathExpression: String, deflt: String): String = {
    val result = xPath.evaluate(xpathExpression, baseNode, XPathConstants.STRING).asInstanceOf[String]
    firstNonNull(result, deflt)
  }

  def booleanXPath(baseNode: Node, xpathExpression: String): Boolean =
    xPath.evaluate(xpathExpression, baseNode, XPathConstants.BOOLEAN).asInstanceOf[Boolean]

  def childElements(element: Element): ImmutableList[Element] =
    ImmutableList.copyOf(new SiblingElementIterator(element.getFirstChild))

  @Nullable def childElementOrNull(e: Element, name: String): Element = {
    val i = new NamedChildElements(name, e).iterator
    if (i.hasNext) i.next else null
  }

  @ForCpp def xpathNodeList(baseNode: Node, xpathExpression: String): NodeList = {
    xPath.evaluate(xpathExpression, baseNode, XPathConstants.NODESET).asInstanceOf[NodeList]
  }

  @ForCpp def xpathNode(baseNode: Node, xpathExpression: String): Node =
    xPath.evaluate(xpathExpression, baseNode, XPathConstants.NODE).asInstanceOf[Node]

  private def newXPathFactory(): XPathFactory = {
    try XPathFactory.newInstance
    catch { case e: NullPointerException => workaroundNewXPathFactory(e) }
  }

  private def workaroundNewXPathFactory(e: NullPointerException): XPathFactory = {
    val workAroundClassName = "com.sun.org.apache.xpath.internal.jaxp.XPathFactoryImpl"
    if (!static_xPathNullPointerLogged) {
      logger.debug(s"Trying to use $workAroundClassName as a workaround after $e", e)
      static_xPathNullPointerLogged = true
    }
    try {
      @SuppressWarnings(Array("unchecked")) val result = (Class.forName(workAroundClassName).asInstanceOf[Class[XPathFactory]]).newInstance
      result
    }
    catch {
      case ee: Throwable => {
        logger.debug("Workaround failed", ee)
        throw e
      }
    }
  }

  def xmlQuoted(value: String): String = {
    val result = new StringBuilder(value.length + 20)
    value foreach {
      _ match {
        case '"' => result.append("&quot;")
        case '&' => result.append("&amp;")
        case '<' => result.append("&lt;")
        case o => result.append(o)
      }
    }
    "\""+ result +"\""
  }
}
