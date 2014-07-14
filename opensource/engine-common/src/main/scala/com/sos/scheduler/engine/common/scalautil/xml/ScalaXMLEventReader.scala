package com.sos.scheduler.engine.common.scalautil.xml

import ScalaStax.RichStartElement
import ScalaXMLEventReader._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import java.io.StringReader
import javax.xml.stream.events._
import javax.xml.stream.{Location, XMLEventReader, XMLInputFactory}
import javax.xml.transform.Source
import javax.xml.transform.stream.StreamSource
import scala.annotation.tailrec
import scala.reflect.ClassTag

final class ScalaXMLEventReader(val delegate: XMLEventReader) {

  def parseAttributelessElement[A](name: String)(body: ⇒ A): A = {
    requireStartElement(name)
    parseAttributelessElement[A](body)
  }

  def parseAttributelessElement[A](body: ⇒ A): A = {
    require(!peek.asStartElement.getAttributes.hasNext, s"No attributes expected in element <${peek.asStartElement.getName}>")
    next()
    val result = body
    eat[EndElement]
    result
  }

  def parseDocument[A](body: ⇒ A): A = {
    eat[StartDocument]
    val result = body
    eat[EndDocument]
    result
  }

  def parseElement[A](name: String)(body: ⇒ A): A = {
    requireStartElement(name)
    parseElement()(body)
  }

  def parseElement[A]()(body: ⇒ A): A = {
    val result = body
    eat[EndElement]
    result
  }

  def ignoreAttributes() {
    next()
  }

  def noAttributes() {
    forEachAttribute(PartialFunction.empty)
  }

  def forEachAttribute(f: PartialFunction[(String, String), Unit]) {
    val element = peek.asStartElement

    def callF(nameValue: (String, String)) =
      try f.applyOrElse(nameValue, { o: (String, String) ⇒ sys.error(s"Unexpected XML attribute ${o._1}") })
      catch { case x: Exception ⇒
        val (name, value) = nameValue
        throw new RuntimeException(s"Error in XML attribute <${element.getName} $name='$value'>: $x - at ${locationStringOf(element.getLocation)}", x)
      }

    for (a <- element.attributes)
      callF(a.getName.toString -> a.getValue)
    next()
  }

  @tailrec def forEachStartElement(f: PartialFunction[String, Unit]) {
    def callF(element: StartElement) = {
      def errorSuffix = s"at ${locationStringOf(element.getLocation)}"
      val name = element.getName.toString
      try f.applyOrElse(name, { name: String => sys.error(s"Unexpected XML element <$name>, $errorSuffix") })
      catch {
        case x: Exception ⇒ throw new WrappedException(s"Error in XML element <$name>: $x - $errorSuffix", x)
      }
    }

    peek match {
      case e: StartElement ⇒
        callF(e)
        forEachStartElement(f)
      case e: Characters ⇒
        next()
        require(e.getData.trim.isEmpty)
        forEachStartElement(f)
      case e: EndElement ⇒
    }
  }

  def eatStartElement(name: String) = {
    val e = eat[StartElement]
    require(e.getName.getLocalPart == name, s"XML element <$name> expected instead of <${e.getName}>")
    e
  }

  def requireStartElement(name: String) = {
    require(peek.asStartElement.getName.getLocalPart == name)
  }

  def eatText() = {
    val result = new StringBuilder
    while (peek.isCharacters)
      result append eat[Characters].getData
    result.toString()
  }

  def eat[E <: XMLEvent : ClassTag]: E = cast[E](next())

  def hasNext = delegate.hasNext

  def next() = delegate.nextEvent()

  def locationString = locationStringOf(peek.getLocation)

  def peek = delegate.peek
}


object ScalaXMLEventReader {

  def parseString[A](xml: String, inputFactory: XMLInputFactory = XMLInputFactory.newInstance())(parse: ScalaXMLEventReader ⇒ A): A =
    parseDocument(StringSource(xml), inputFactory)(parse)

  def parseDocument[A](source: Source, inputFactory: XMLInputFactory)(parse: ScalaXMLEventReader ⇒ A): A = {
    val reader = new ScalaXMLEventReader(inputFactory.createXMLEventReader(source))
    reader.eat[StartDocument]
    val result = parse(reader)
    reader.eat[EndDocument]
    result
  }

  def parse[A](source: Source, inputFactory: XMLInputFactory = XMLInputFactory.newInstance())(parseEvents: ScalaXMLEventReader ⇒ A): A = {
    val reader = new ScalaXMLEventReader(inputFactory.createXMLEventReader(source))
    val result = parseEvents(reader)
    result
  }

  private def locationStringOf(o: Location) =
    (Option(o.getSystemId) ++ Option(o.getPublicId)).flatten.mkString(":") + ":" + o.getLineNumber +":" + o.getColumnNumber

  final class WrappedException(override val getMessage: String, override val getCause: Exception) extends RuntimeException
  
  object WrappedException {
    def unapply(o: WrappedException) = Some(o.getCause)
  }
}
