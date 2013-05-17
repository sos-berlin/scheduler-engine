package com.sos.scheduler.engine.common.scalautil.xml

import ScalaStax.RichStartElement
import ScalaXMLEventReader._
import javax.xml.stream.events._
import javax.xml.stream.{Location, XMLEventReader}
import scala.annotation.tailrec
import scala.reflect.ClassTag
import scala.sys.error

final class ScalaXMLEventReader(eventReader: XMLEventReader) {
  def currentEvent = eventReader.peek

  def parseAttributelessElement[A](name: String)(f: => A): A = {
    requireStartElement(name)
    parseAttributelessElement[A](f)
  }

  def parseAttributelessElement[A](f: => A): A = {
    require(!currentEvent.asStartElement.getAttributes.hasNext)
    next()
    val result = f
    eat[EndElement]
    result
  }

  def parseElement[A](name: String)(f: => A): A = {
    requireStartElement(name)
    parseElement(f)
  }

  def parseElement[A](f: => A): A = {
    val result = f
    eat[EndElement]
    result
  }

  def forEachAttribute(f: PartialFunction[(String, String), Unit]) {
    val element = currentEvent.asStartElement

    def callF(o: (String, String)) =
      try {
        if (!(f isDefinedAt o)) error("Unexpected XML attribute")
        f(o)
      }
      catch { case x: Exception =>
        throw new RuntimeException(s"Error in XML attribute <${element.getName} ${o._1}='${o._2}'>: $x - at ${locationStringOf(element.getLocation)}", x)
      }

    for (a <- element.attributes)
      callF(attributeToPair(a))
    next()
  }

  @tailrec def forEachStartElement(f: PartialFunction[String, Unit]) {
    def callF(element: StartElement) = {
      val name = element.getName.toString
      try {
        if (!(f isDefinedAt name)) error(s"Unexpected XML element")
        f(name)
      }
      catch { case x: Exception =>
        throw new RuntimeException(s"Error in XML element <$name>: $x - at ${locationStringOf(element.getLocation)}", x)
      }
    }

    currentEvent match {
      case e: StartElement =>
        callF(e)
        forEachStartElement(f)
      case e: Characters =>
        next()
        require(e.getData.trim.isEmpty)
        forEachStartElement(f)
      case e: EndElement =>
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

  def eat[E <: XMLEvent](implicit c: ClassTag[E]): E = {
    val r = next()
    require(c.runtimeClass isAssignableFrom r.getClass, s"${c.runtimeClass.getName} expected instead of ${r.getClass}")
    r.asInstanceOf[E]
  }

  def locationString =
    locationStringOf(currentEvent.getLocation)

  def hasNext = eventReader.hasNext

  def next() = eventReader.nextEvent()
}

object ScalaXMLEventReader {
  private def attributeToPair(a: Attribute) =
    a.getName.toString -> a.getValue

  private def locationStringOf(o: Location) =
    (Option(o.getSystemId) ++ Option(o.getPublicId)).flatten.mkString(":") + ":" + o.getLineNumber +":" + o.getColumnNumber
}