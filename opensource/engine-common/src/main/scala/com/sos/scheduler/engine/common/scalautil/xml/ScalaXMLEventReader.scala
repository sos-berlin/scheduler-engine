package com.sos.scheduler.engine.common.scalautil.xml

import ScalaStax.RichStartElement
import ScalaXMLEventReader._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import java.io.StringReader
import javax.xml.stream.events._
import javax.xml.stream.{XMLInputFactory, Location, XMLEventReader}
import javax.xml.transform.Source
import javax.xml.transform.stream.StreamSource
import scala.annotation.tailrec
import scala.reflect.ClassTag

final class ScalaXMLEventReader(eventReader: XMLEventReader) {
  def currentEvent = eventReader.peek

  def parseAttributelessElement[A](name: String)(f: ⇒ A): A = {
    requireStartElement(name)
    parseAttributelessElement[A](f)
  }

  def parseAttributelessElement[A](f: ⇒ A): A = {
    require(!currentEvent.asStartElement.getAttributes.hasNext, s"No attributes expected in element <${currentEvent.asStartElement.getName}>")
    next()
    val result = f
    eat[EndElement]
    result
  }

  def parseElement[A](name: String)(f: ⇒ A): A = {
    requireStartElement(name)
    parseElement(f)
  }

  def parseElement[A](f: ⇒ A): A = {
    val result = f
    eat[EndElement]
    result
  }

  def forEachAttribute(f: PartialFunction[(String, String), Unit]) {
    val element = currentEvent.asStartElement

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
        case x: Exception ⇒ throw new RuntimeException(s"Error in XML element <$name>: $x - $errorSuffix", x)
      }
    }

    currentEvent match {
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
    require(currentEvent.asStartElement.getName.getLocalPart == name)
  }

  def eatText() = {
    val result = new StringBuilder
    while (currentEvent.isCharacters)
      result append eat[Characters].getData
    result.toString()
  }

  def eat[E <: XMLEvent : ClassTag]: E = cast[E](next())

  def locationString = locationStringOf(currentEvent.getLocation)

  def hasNext = eventReader.hasNext

  def next() = eventReader.nextEvent()
}


object ScalaXMLEventReader {

  def parseString[A](xml: String, inputFactory: XMLInputFactory = XMLInputFactory.newInstance())(parse: XMLEventReader ⇒ A): A =
    parseDocument(new StreamSource(new StringReader(xml)), inputFactory)(parse)

  def parseDocument[A](source: Source, inputFactory: XMLInputFactory)(parse: XMLEventReader ⇒ A): A = {
    val reader = inputFactory.createXMLEventReader(source)
    reader.nextEvent().asInstanceOf[StartDocument]
    val result = parse(reader)
    reader.nextEvent().asInstanceOf[EndDocument]
    result
  }

  private def locationStringOf(o: Location) =
    (Option(o.getSystemId) ++ Option(o.getPublicId)).flatten.mkString(":") + ":" + o.getLineNumber +":" + o.getColumnNumber
}
